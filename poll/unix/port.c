/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_arch_poll_private.h"
#include "apr_atomic.h"

#ifdef POLLSET_USES_PORT

static apr_int16_t get_event(apr_int16_t event)
{
    apr_int16_t rv = 0;

    if (event & APR_POLLIN)
        rv |= POLLIN;
    if (event & APR_POLLPRI)
        rv |= POLLPRI;
    if (event & APR_POLLOUT)
        rv |= POLLOUT;
    if (event & APR_POLLERR)
        rv |= POLLERR;
    if (event & APR_POLLHUP)
        rv |= POLLHUP;
    if (event & APR_POLLNVAL)
        rv |= POLLNVAL;

    return rv;
}

static apr_int16_t get_revent(apr_int16_t event)
{
    apr_int16_t rv = 0;

    if (event & POLLIN)
        rv |= APR_POLLIN;
    if (event & POLLPRI)
        rv |= APR_POLLPRI;
    if (event & POLLOUT)
        rv |= APR_POLLOUT;
    if (event & POLLERR)
        rv |= APR_POLLERR;
    if (event & POLLHUP)
        rv |= APR_POLLHUP;
    if (event & POLLNVAL)
        rv |= APR_POLLNVAL;

    return rv;
}


struct apr_pollset_t
{
    apr_pool_t *pool;
    apr_uint32_t nelts;
    apr_uint32_t nalloc;
    int port_fd;
    port_event_t *port_set;
    apr_pollfd_t *result_set;
    apr_uint32_t flags;
#if APR_HAS_THREADS
    /* A thread mutex to protect operations on the rings */
    apr_thread_mutex_t *ring_lock;
#endif
    /* A ring containing all of the pollfd_t that are active */
    APR_RING_HEAD(pfd_query_ring_t, pfd_elem_t) query_ring;
    APR_RING_HEAD(pfd_add_ring_t, pfd_elem_t) add_ring;
    /* A ring of pollfd_t that have been used, and then _remove'd */
    APR_RING_HEAD(pfd_free_ring_t, pfd_elem_t) free_ring;
    /* A ring of pollfd_t where rings that have been _remove'd but
       might still be inside a _poll */
    APR_RING_HEAD(pfd_dead_ring_t, pfd_elem_t) dead_ring;
    /* number of threads in poll */
    volatile apr_uint32_t waiting;
};

static apr_status_t backend_cleanup(void *p_)
{
    apr_pollset_t *pollset = (apr_pollset_t *) p_;
    close(pollset->port_fd);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollset_create(apr_pollset_t **pollset,
                                             apr_uint32_t size,
                                             apr_pool_t *p,
                                             apr_uint32_t flags)
{
    apr_status_t rv = APR_SUCCESS;
    *pollset = apr_palloc(p, sizeof(**pollset));
#if APR_HAS_THREADS
    if (flags & APR_POLLSET_THREADSAFE &&
        ((rv = apr_thread_mutex_create(&(*pollset)->ring_lock,
                                       APR_THREAD_MUTEX_DEFAULT,
                                       p)) != APR_SUCCESS)) {
        *pollset = NULL;
        return rv;
    }
#else
    if (flags & APR_POLLSET_THREADSAFE) {
        *pollset = NULL;
        return APR_ENOTIMPL;
    }
#endif
    (*pollset)->waiting = 0;
    (*pollset)->nelts = 0;
    (*pollset)->nalloc = size;
    (*pollset)->flags = flags;
    (*pollset)->pool = p;

    (*pollset)->port_set = apr_palloc(p, size * sizeof(port_event_t));

    (*pollset)->port_fd = port_create();

    if ((*pollset)->port_fd < 0) {
        return APR_ENOMEM;
    }

    apr_pool_cleanup_register(p, (void *) (*pollset), backend_cleanup,
                              apr_pool_cleanup_null);

    (*pollset)->result_set = apr_palloc(p, size * sizeof(apr_pollfd_t));

    APR_RING_INIT(&(*pollset)->query_ring, pfd_elem_t, link);
    APR_RING_INIT(&(*pollset)->add_ring, pfd_elem_t, link);
    APR_RING_INIT(&(*pollset)->free_ring, pfd_elem_t, link);
    APR_RING_INIT(&(*pollset)->dead_ring, pfd_elem_t, link);

    return rv;
}

APR_DECLARE(apr_status_t) apr_pollset_destroy(apr_pollset_t *pollset)
{
    return apr_pool_cleanup_run(pollset->pool, pollset, backend_cleanup);
}

APR_DECLARE(apr_status_t) apr_pollset_add(apr_pollset_t *pollset,
                                          const apr_pollfd_t *descriptor)
{
    apr_os_sock_t fd;
    pfd_elem_t *elem;
    int res;
    apr_status_t rv = APR_SUCCESS;

    pollset_lock_rings();

    if (!APR_RING_EMPTY(&(pollset->free_ring), pfd_elem_t, link)) {
        elem = APR_RING_FIRST(&(pollset->free_ring));
        APR_RING_REMOVE(elem, link);
    }
    else {
        elem = (pfd_elem_t *) apr_palloc(pollset->pool, sizeof(pfd_elem_t));
        APR_RING_ELEM_INIT(elem, link);
    }
    elem->pfd = *descriptor;

    if (descriptor->desc_type == APR_POLL_SOCKET) {
        fd = descriptor->desc.s->socketdes;
    }
    else {
        fd = descriptor->desc.f->filedes;
    }

    if (apr_atomic_read32(&pollset->waiting)) {
        res = port_associate(pollset->port_fd, PORT_SOURCE_FD, fd, 
                             get_event(descriptor->reqevents), (void *)elem);

        if (res < 0) {
            rv = APR_ENOMEM;
            APR_RING_INSERT_TAIL(&(pollset->free_ring), elem, pfd_elem_t, link);
        }
        else {
            pollset->nelts++;
            APR_RING_INSERT_TAIL(&(pollset->query_ring), elem, pfd_elem_t, link);
        }
    } 
    else {
        pollset->nelts++;
        APR_RING_INSERT_TAIL(&(pollset->add_ring), elem, pfd_elem_t, link);
    }

    pollset_unlock_rings();

    return rv;
}

APR_DECLARE(apr_status_t) apr_pollset_remove(apr_pollset_t *pollset,
                                             const apr_pollfd_t *descriptor)
{
    apr_os_sock_t fd;
    pfd_elem_t *ep;
    apr_status_t rv = APR_SUCCESS;
    int res;
    int err = 0;

    pollset_lock_rings();

    if (descriptor->desc_type == APR_POLL_SOCKET) {
        fd = descriptor->desc.s->socketdes;
    }
    else {
        fd = descriptor->desc.f->filedes;
    }

    res = port_dissociate(pollset->port_fd, PORT_SOURCE_FD, fd);

    if (res < 0) {
        err = errno;
        rv = APR_NOTFOUND;
    }

    if (!APR_RING_EMPTY(&(pollset->query_ring), pfd_elem_t, link)) {
        for (ep = APR_RING_FIRST(&(pollset->query_ring));
             ep != APR_RING_SENTINEL(&(pollset->query_ring),
                                     pfd_elem_t, link);
             ep = APR_RING_NEXT(ep, link)) {

            if (descriptor->desc.s == ep->pfd.desc.s) {
                APR_RING_REMOVE(ep, link);
                APR_RING_INSERT_TAIL(&(pollset->dead_ring),
                                     ep, pfd_elem_t, link);
                break;
            }
        }
    }

    if (!APR_RING_EMPTY(&(pollset->add_ring), pfd_elem_t, link)) {
        for (ep = APR_RING_FIRST(&(pollset->add_ring));
             ep != APR_RING_SENTINEL(&(pollset->add_ring),
                                     pfd_elem_t, link);
             ep = APR_RING_NEXT(ep, link)) {

            if (descriptor->desc.s == ep->pfd.desc.s) {
                APR_RING_REMOVE(ep, link);
                APR_RING_INSERT_TAIL(&(pollset->dead_ring),
                                     ep, pfd_elem_t, link);
                if (ENOENT == err) {
                    rv = APR_SUCCESS;
                }
                break;
            }
        }
    }

    pollset_unlock_rings();

    return rv;
}

APR_DECLARE(apr_status_t) apr_pollset_poll(apr_pollset_t *pollset,
                                           apr_interval_time_t timeout,
                                           apr_int32_t *num,
                                           const apr_pollfd_t **descriptors)
{
    apr_os_sock_t fd;
    int ret, i;
    unsigned int nget;
    pfd_elem_t *ep;
    struct timespec tv, *tvptr;
    apr_status_t rv = APR_SUCCESS;

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = (long) apr_time_sec(timeout);
        tv.tv_nsec = (long) apr_time_usec(timeout) * 1000;
        tvptr = &tv;
    }

    nget = 1;

    pollset_lock_rings();

    apr_atomic_inc32(&pollset->waiting);

    while (!APR_RING_EMPTY(&(pollset->add_ring), pfd_elem_t, link)) {
        ep = APR_RING_FIRST(&(pollset->add_ring));
        APR_RING_REMOVE(ep, link);

        if (ep->pfd.desc_type == APR_POLL_SOCKET) {
            fd = ep->pfd.desc.s->socketdes;
        }
        else {
            fd = ep->pfd.desc.f->filedes;
        }

        port_associate(pollset->port_fd, PORT_SOURCE_FD, 
                           fd, get_event(ep->pfd.reqevents), ep);

        APR_RING_INSERT_TAIL(&(pollset->query_ring), ep, pfd_elem_t, link);

    }

    pollset_unlock_rings();

    ret = port_getn(pollset->port_fd, pollset->port_set, pollset->nalloc,
                    &nget, tvptr);

    /* decrease the waiting ASAP to reduce the window for calling 
       port_associate within apr_pollset_add() */
    apr_atomic_dec32(&pollset->waiting);
    (*num) = nget;

    if (ret == -1) {
        (*num) = 0;
        if (errno == EINTR) {
            rv = APR_EINTR;
        }
        else if (errno == ETIME) {
            rv = APR_TIMEUP;
        }
        else {
            rv = APR_EGENERAL;
        }
    }
    else if (nget == 0) {
        rv = APR_TIMEUP;
    }
    else {

        pollset_lock_rings();

        for (i = 0; i < nget; i++) {
            pollset->result_set[i] =
                (((pfd_elem_t*)(pollset->port_set[i].portev_user))->pfd);
            pollset->result_set[i].rtnevents =
                get_revent(pollset->port_set[i].portev_events);

            APR_RING_REMOVE((pfd_elem_t*)pollset->port_set[i].portev_user, link);

            APR_RING_INSERT_TAIL(&(pollset->add_ring), 
                                 (pfd_elem_t*)pollset->port_set[i].portev_user,
                                 pfd_elem_t, link);
        }

        pollset_unlock_rings();

        if (descriptors) {
            *descriptors = pollset->result_set;
        }
    }


    pollset_lock_rings();

    /* Shift all PFDs in the Dead Ring to be Free Ring */
    APR_RING_CONCAT(&(pollset->free_ring), &(pollset->dead_ring), pfd_elem_t, link);

    pollset_unlock_rings();

    return rv;
}

struct apr_pollcb_t {
    apr_pool_t *pool;
    apr_uint32_t nalloc;
    port_event_t *port_set;
    int port_fd;
};

static apr_status_t cb_cleanup(void *p_)
{
    apr_pollcb_t *pollcb = (apr_pollcb_t *) p_;
    close(pollcb->port_fd);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollcb_create(apr_pollcb_t **pollcb,
                                            apr_uint32_t size,
                                            apr_pool_t *p,
                                            apr_uint32_t flags)
{
    int fd;

    fd = port_create();

    if (fd < 0) {
        *pollcb = NULL;
        return apr_get_netos_error();
    }

    *pollcb = apr_palloc(p, sizeof(**pollcb));
    (*pollcb)->nalloc = size;
    (*pollcb)->pool = p;
    (*pollcb)->port_fd = fd;
    (*pollcb)->port_set = apr_palloc(p, size * sizeof(port_event_t));
    apr_pool_cleanup_register(p, *pollcb, cb_cleanup, cb_cleanup);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollcb_add(apr_pollcb_t *pollcb,
                                         apr_pollfd_t *descriptor)
{
    int ret, fd;

    if (descriptor->desc_type == APR_POLL_SOCKET) {
        fd = descriptor->desc.s->socketdes;
    }
    else {
        fd = descriptor->desc.f->filedes;
    }

    ret = port_associate(pollcb->port_fd, PORT_SOURCE_FD, fd,
                         get_event(descriptor->reqevents), descriptor);

    if (ret == -1) {
        return apr_get_netos_error();
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollcb_remove(apr_pollcb_t *pollcb,
                                            apr_pollfd_t *descriptor)
{
    int fd, ret;

    if (descriptor->desc_type == APR_POLL_SOCKET) {
        fd = descriptor->desc.s->socketdes;
    }
    else {
        fd = descriptor->desc.f->filedes;
    }

    ret = port_dissociate(pollcb->port_fd, PORT_SOURCE_FD, fd);

    if (ret < 0) {
        return APR_NOTFOUND;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollcb_poll(apr_pollcb_t *pollcb,
                                          apr_interval_time_t timeout,
                                          apr_pollcb_cb_t func,
                                          void *baton)
{
    int ret;
    apr_pollfd_t *pollfd;
    struct timespec tv, *tvptr;
    apr_status_t rv = APR_SUCCESS;
    unsigned int i, nget = pollcb->nalloc;

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = (long) apr_time_sec(timeout);
        tv.tv_nsec = (long) apr_time_usec(timeout) * 1000;
        tvptr = &tv;
    }

    ret = port_getn(pollcb->port_fd, pollcb->port_set, pollcb->nalloc,
                    &nget, tvptr);

    if (ret == -1) {
        if (errno == ETIME || errno == EINTR) {
            rv = APR_TIMEUP;
        }
        else {
            rv = APR_EGENERAL;
        }
    }
    else if (nget == 0) {
        rv = APR_TIMEUP;
    }
    else {
        for (i = 0; i < nget; i++) {
            pollfd = (apr_pollfd_t *)(pollcb->port_set[i].portev_user);
            pollfd->rtnevents = get_revent(pollcb->port_set[i].portev_events);

            rv = func(baton, pollfd);
            if (rv) {
                return rv;
            }
            rv = apr_pollcb_add(pollcb, pollfd);
        }
    }

    return rv;
}

#endif /* POLLSET_USES_PORT */
