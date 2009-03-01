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

#include "apr.h"
#include "apr_poll.h"
#include "apr_time.h"
#include "apr_portable.h"
#include "apr_atomic.h"
#include "apr_arch_file_io.h"
#include "apr_arch_networkio.h"
#include "apr_arch_poll_private.h"
#include "apr_arch_inherit.h"

#if defined(HAVE_PORT_CREATE)

static apr_int16_t get_event(apr_int16_t event)
{
    apr_int16_t rv = 0;

    if (event & APR_POLLIN)
        rv |= POLLIN;
    if (event & APR_POLLPRI)
        rv |= POLLPRI;
    if (event & APR_POLLOUT)
        rv |= POLLOUT;
    /* TODO: Confirm that the set of return-only events is the same as with 
     * poll(), and axe these checks:
     */
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


struct apr_pollset_private_t
{
    int port_fd;
    port_event_t *port_set;
    apr_pollfd_t *result_set;
#if APR_HAS_THREADS
    /* A thread mutex to protect operations on the rings */
    apr_thread_mutex_t *ring_lock;
#endif
    /* A ring containing all of the pollfd_t that are active */
    APR_RING_HEAD(pfd_query_ring_t, pfd_elem_t) query_ring;
    /* A ring containing the pollfd_t that will be added on the
     * next call to apr_pollset_poll().
     */
    APR_RING_HEAD(pfd_add_ring_t, pfd_elem_t) add_ring;
    /* A ring of pollfd_t that have been used, and then _remove'd */
    APR_RING_HEAD(pfd_free_ring_t, pfd_elem_t) free_ring;
    /* A ring of pollfd_t where rings that have been _remove'd but
       might still be inside a _poll */
    APR_RING_HEAD(pfd_dead_ring_t, pfd_elem_t) dead_ring;
    /* number of threads in poll */
    volatile apr_uint32_t waiting;
};

static apr_status_t impl_pollset_cleanup(apr_pollset_t *pollset)
{
    close(pollset->p->port_fd);
    return APR_SUCCESS;
}

static apr_status_t impl_pollset_create(apr_pollset_t *pollset,
                                             apr_uint32_t size,
                                             apr_pool_t *p,
                                             apr_uint32_t flags)
{
    apr_status_t rv = APR_SUCCESS;
    pollset->p = apr_palloc(p, sizeof(apr_pollset_private_t));
#if APR_HAS_THREADS
    if (flags & APR_POLLSET_THREADSAFE &&
        ((rv = apr_thread_mutex_create(&pollset->p->ring_lock,
                                       APR_THREAD_MUTEX_DEFAULT,
                                       p)) != APR_SUCCESS)) {
        pollset->p = NULL;
        return rv;
    }
#else
    if (flags & APR_POLLSET_THREADSAFE) {
        pollset->p = NULL;
        return APR_ENOTIMPL;
    }
#endif
    pollset->p->waiting = 0;

    pollset->p->port_set = apr_palloc(p, size * sizeof(port_event_t));

    pollset->p->port_fd = port_create();

    if (pollset->p->port_fd < 0) {
        pollset->p = NULL;
        return apr_get_netos_error();
    }

    {
        int flags;

        if ((flags = fcntl(pollset->p->port_fd, F_GETFD)) == -1)
            return errno;

        flags |= FD_CLOEXEC;
        if (fcntl(pollset->p->port_fd, F_SETFD, flags) == -1)
            return errno;
    }

    pollset->p->result_set = apr_palloc(p, size * sizeof(apr_pollfd_t));

    APR_RING_INIT(&pollset->p->query_ring, pfd_elem_t, link);
    APR_RING_INIT(&pollset->p->add_ring, pfd_elem_t, link);
    APR_RING_INIT(&pollset->p->free_ring, pfd_elem_t, link);
    APR_RING_INIT(&pollset->p->dead_ring, pfd_elem_t, link);

    return rv;
}

static apr_status_t impl_pollset_add(apr_pollset_t *pollset,
                                     const apr_pollfd_t *descriptor)
{
    apr_os_sock_t fd;
    pfd_elem_t *elem;
    int res;
    apr_status_t rv = APR_SUCCESS;

    pollset_lock_rings();

    if (!APR_RING_EMPTY(&(pollset->p->free_ring), pfd_elem_t, link)) {
        elem = APR_RING_FIRST(&(pollset->p->free_ring));
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

    /* If another thread is polling, notify the kernel immediately; otherwise,
     * wait until the next call to apr_pollset_poll().
     */
    if (apr_atomic_read32(&pollset->p->waiting)) {
        res = port_associate(pollset->p->port_fd, PORT_SOURCE_FD, fd, 
                             get_event(descriptor->reqevents), (void *)elem);

        if (res < 0) {
            rv = apr_get_netos_error();
            APR_RING_INSERT_TAIL(&(pollset->p->free_ring), elem, pfd_elem_t, link);
        }
        else {
            pollset->nelts++;
            APR_RING_INSERT_TAIL(&(pollset->p->query_ring), elem, pfd_elem_t, link);
        }
    } 
    else {
        pollset->nelts++;
        APR_RING_INSERT_TAIL(&(pollset->p->add_ring), elem, pfd_elem_t, link);
    }

    pollset_unlock_rings();

    return rv;
}

static apr_status_t impl_pollset_remove(apr_pollset_t *pollset,
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

    res = port_dissociate(pollset->p->port_fd, PORT_SOURCE_FD, fd);

    if (res < 0) {
        err = errno;
        rv = APR_NOTFOUND;
    }

    for (ep = APR_RING_FIRST(&(pollset->p->query_ring));
         ep != APR_RING_SENTINEL(&(pollset->p->query_ring),
                                 pfd_elem_t, link);
         ep = APR_RING_NEXT(ep, link)) {

        if (descriptor->desc.s == ep->pfd.desc.s) {
            APR_RING_REMOVE(ep, link);
            APR_RING_INSERT_TAIL(&(pollset->p->dead_ring),
                                 ep, pfd_elem_t, link);
            if (ENOENT == err) {
                rv = APR_SUCCESS;
            }
            break;
        }
    }

    for (ep = APR_RING_FIRST(&(pollset->p->add_ring));
         ep != APR_RING_SENTINEL(&(pollset->p->add_ring),
                                 pfd_elem_t, link);
         ep = APR_RING_NEXT(ep, link)) {

        if (descriptor->desc.s == ep->pfd.desc.s) {
            APR_RING_REMOVE(ep, link);
            APR_RING_INSERT_TAIL(&(pollset->p->dead_ring),
                                 ep, pfd_elem_t, link);
            if (ENOENT == err) {
                rv = APR_SUCCESS;
            }
            break;
        }
    }

    pollset_unlock_rings();

    return rv;
}

static apr_status_t impl_pollset_poll(apr_pollset_t *pollset,
                                      apr_interval_time_t timeout,
                                      apr_int32_t *num,
                                      const apr_pollfd_t **descriptors)
{
    apr_os_sock_t fd;
    int ret, i, j;
    unsigned int nget;
    pfd_elem_t *ep;
    struct timespec tv, *tvptr;
    apr_status_t rv = APR_SUCCESS;
    apr_pollfd_t fp;

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

    apr_atomic_inc32(&pollset->p->waiting);

    while (!APR_RING_EMPTY(&(pollset->p->add_ring), pfd_elem_t, link)) {
        ep = APR_RING_FIRST(&(pollset->p->add_ring));
        APR_RING_REMOVE(ep, link);

        if (ep->pfd.desc_type == APR_POLL_SOCKET) {
            fd = ep->pfd.desc.s->socketdes;
        }
        else {
            fd = ep->pfd.desc.f->filedes;
        }

        ret = port_associate(pollset->p->port_fd, PORT_SOURCE_FD, 
                             fd, get_event(ep->pfd.reqevents), ep);
        if (ret < 0) {
            rv = apr_get_netos_error();
            break;
        }

        APR_RING_INSERT_TAIL(&(pollset->p->query_ring), ep, pfd_elem_t, link);
    }

    pollset_unlock_rings();

    if (rv != APR_SUCCESS) {
        apr_atomic_dec32(&pollset->p->waiting);
        return rv;
    }

    ret = port_getn(pollset->p->port_fd, pollset->p->port_set, pollset->nalloc,
                    &nget, tvptr);

    /* decrease the waiting ASAP to reduce the window for calling 
       port_associate within apr_pollset_add() */
    apr_atomic_dec32(&pollset->p->waiting);
    (*num) = nget;

    if (ret == -1) {
        (*num) = 0;
        if (errno == ETIME) {
            rv = APR_TIMEUP;
        }
        else {
            rv = apr_get_netos_error();
        }
    }
    else if (nget == 0) {
        rv = APR_TIMEUP;
    }
    else {

        pollset_lock_rings();

        for (i = 0, j = 0; i < nget; i++) {
            fp = (((pfd_elem_t*)(pollset->p->port_set[i].portev_user))->pfd);
            if ((pollset->flags & APR_POLLSET_WAKEABLE) &&
                fp.desc_type == APR_POLL_FILE &&
                fp.desc.f == pollset->wakeup_pipe[0]) {
                apr_pollset_drain_wakeup_pipe(pollset);
                rv = APR_EINTR;
            }
            else {
                pollset->p->result_set[j] = fp;            
                pollset->p->result_set[j].rtnevents =
                    get_revent(pollset->p->port_set[i].portev_events);

                APR_RING_REMOVE((pfd_elem_t*)pollset->p->port_set[i].portev_user,
                                link);
                APR_RING_INSERT_TAIL(&(pollset->p->add_ring), 
                                (pfd_elem_t*)pollset->p->port_set[i].portev_user,
                                pfd_elem_t, link);
                j++;
            }
        }
        pollset_unlock_rings();
        if ((*num = j)) { /* any event besides wakeup pipe? */
            rv = APR_SUCCESS;
            if (descriptors) {
                *descriptors = pollset->p->result_set;
            }
        }
    }

    pollset_lock_rings();

    /* Shift all PFDs in the Dead Ring to the Free Ring */
    APR_RING_CONCAT(&(pollset->p->free_ring), &(pollset->p->dead_ring), pfd_elem_t, link);

    pollset_unlock_rings();

    return rv;
}

static apr_pollset_provider_t impl = {
    impl_pollset_create,
    impl_pollset_add,
    impl_pollset_remove,
    impl_pollset_poll,
    impl_pollset_cleanup,
    "port"
};

apr_pollset_provider_t *apr_pollset_provider_port = &impl;

static apr_status_t cb_cleanup(void *p_)
{
    apr_pollcb_t *pollcb = (apr_pollcb_t *) p_;
    close(pollcb->fd);
    return APR_SUCCESS;
}

static apr_status_t impl_pollcb_create(apr_pollcb_t *pollcb,
                                       apr_uint32_t size,
                                       apr_pool_t *p,
                                       apr_uint32_t flags)
{
    pollcb->fd = port_create();

    if (pollcb->fd < 0) {
        return apr_get_netos_error();
    }

    {
        int flags;

        if ((flags = fcntl(pollcb->fd, F_GETFD)) == -1)
            return errno;

        flags |= FD_CLOEXEC;
        if (fcntl(pollcb->fd, F_SETFD, flags) == -1)
            return errno;
    }

    pollcb->pollset.port = apr_palloc(p, size * sizeof(port_event_t));
    apr_pool_cleanup_register(p, pollcb, cb_cleanup, apr_pool_cleanup_null);

    return APR_SUCCESS;
}

static apr_status_t impl_pollcb_add(apr_pollcb_t *pollcb,
                                    apr_pollfd_t *descriptor)
{
    int ret, fd;

    if (descriptor->desc_type == APR_POLL_SOCKET) {
        fd = descriptor->desc.s->socketdes;
    }
    else {
        fd = descriptor->desc.f->filedes;
    }

    ret = port_associate(pollcb->fd, PORT_SOURCE_FD, fd,
                         get_event(descriptor->reqevents), descriptor);

    if (ret == -1) {
        return apr_get_netos_error();
    }

    return APR_SUCCESS;
}

static apr_status_t impl_pollcb_remove(apr_pollcb_t *pollcb,
                                       apr_pollfd_t *descriptor)
{
    int fd, ret;

    if (descriptor->desc_type == APR_POLL_SOCKET) {
        fd = descriptor->desc.s->socketdes;
    }
    else {
        fd = descriptor->desc.f->filedes;
    }

    ret = port_dissociate(pollcb->fd, PORT_SOURCE_FD, fd);

    if (ret < 0) {
        return APR_NOTFOUND;
    }

    return APR_SUCCESS;
}

static apr_status_t impl_pollcb_poll(apr_pollcb_t *pollcb,
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

    ret = port_getn(pollcb->fd, pollcb->pollset.port, pollcb->nalloc,
                    &nget, tvptr);

    if (ret == -1) {
        if (errno == ETIME) {
            rv = APR_TIMEUP;
        }
        else {
            rv = apr_get_netos_error();
        }
    }
    else if (nget == 0) {
        rv = APR_TIMEUP;
    }
    else {
        for (i = 0; i < nget; i++) {
            pollfd = (apr_pollfd_t *)(pollcb->pollset.port[i].portev_user);
            pollfd->rtnevents = get_revent(pollcb->pollset.port[i].portev_events);

            rv = func(baton, pollfd);
            if (rv) {
                return rv;
            }
            rv = apr_pollcb_add(pollcb, pollfd);
        }
    }

    return rv;
}

static apr_pollcb_provider_t impl_cb = {
    impl_pollcb_create,
    impl_pollcb_add,
    impl_pollcb_remove,
    impl_pollcb_poll,
    "port"
};

apr_pollcb_provider_t *apr_pollcb_provider_port = &impl_cb;

#endif /* HAVE_PORT_CREATE */
