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

#if defined(POLL_USES_POLL) || defined(POLLSET_USES_POLL)

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

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

#endif /* POLL_USES_POLL || POLLSET_USES_POLL */


#ifdef POLL_USES_POLL

#define SMALL_POLLSET_LIMIT  8

APR_DECLARE(apr_status_t) apr_poll(apr_pollfd_t *aprset, apr_int32_t num,
                                   apr_int32_t *nsds, 
                                   apr_interval_time_t timeout)
{
    int i, num_to_poll;
#ifdef HAVE_VLA
    /* XXX: I trust that this is a segv when insufficient stack exists? */
    struct pollfd pollset[num];
#elif defined(HAVE_ALLOCA)
    struct pollfd *pollset = alloca(sizeof(struct pollfd) * num);
    if (!pollset)
        return APR_ENOMEM;
#else
    struct pollfd tmp_pollset[SMALL_POLLSET_LIMIT];
    struct pollfd *pollset;

    if (num <= SMALL_POLLSET_LIMIT) {
        pollset = tmp_pollset;
    }
    else {
        /* This does require O(n) to copy the descriptors to the internal
         * mapping.
         */
        pollset = malloc(sizeof(struct pollfd) * num);
        /* The other option is adding an apr_pool_abort() fn to invoke
         * the pool's out of memory handler
         */
        if (!pollset)
            return APR_ENOMEM;
    }
#endif
    for (i = 0; i < num; i++) {
        if (aprset[i].desc_type == APR_POLL_SOCKET) {
            pollset[i].fd = aprset[i].desc.s->socketdes;
        }
        else if (aprset[i].desc_type == APR_POLL_FILE) {
            pollset[i].fd = aprset[i].desc.f->filedes;
        }
        else {
            break;
        }
        pollset[i].events = get_event(aprset[i].reqevents);
    }
    num_to_poll = i;

    if (timeout > 0) {
        timeout /= 1000; /* convert microseconds to milliseconds */
    }

    i = poll(pollset, num_to_poll, timeout);
    (*nsds) = i;

    if (i > 0) { /* poll() sets revents only if an event was signalled;
                  * we don't promise to set rtnevents unless an event
                  * was signalled
                  */
        for (i = 0; i < num; i++) {
            aprset[i].rtnevents = get_revent(pollset[i].revents);
        }
    }
    
#if !defined(HAVE_VLA) && !defined(HAVE_ALLOCA)
    if (num > SMALL_POLLSET_LIMIT) {
        free(pollset);
    }
#endif

    if ((*nsds) < 0) {
        return apr_get_netos_error();
    }
    if ((*nsds) == 0) {
        return APR_TIMEUP;
    }
    return APR_SUCCESS;
}


#endif /* POLL_USES_POLL */


#ifdef POLLSET_USES_POLL

struct apr_pollset_t
{
    apr_pool_t *pool;
    apr_uint32_t nelts;
    apr_uint32_t nalloc;
    apr_uint32_t flags;
    /* Pipe descriptors used for wakeup */
    apr_file_t *wakeup_pipe[2];    
    struct pollfd *pollset;
    apr_pollfd_t *query_set;
    apr_pollfd_t *result_set;
};

/* Create a dummy wakeup pipe for interrupting the poller
 */
static apr_status_t create_wakeup_pipe(apr_pollset_t *pollset)
{
    apr_status_t rv;
    apr_pollfd_t fd;

    if ((rv = apr_file_pipe_create(&pollset->wakeup_pipe[0],
                                   &pollset->wakeup_pipe[1],
                                   pollset->pool)) != APR_SUCCESS)
        return rv;
    fd.reqevents = APR_POLLIN;
    fd.desc_type = APR_POLL_FILE;
    fd.desc.f = pollset->wakeup_pipe[0];
    /* Add the pipe to the pollset
     */
    return apr_pollset_add(pollset, &fd);
}

/* Read and discard what's ever in the wakeup pipe.
 */
static void drain_wakeup_pipe(apr_pollset_t *pollset)
{
    char rb[512];
    apr_size_t nr = sizeof(rb);

    while (apr_file_read(pollset->wakeup_pipe[0], rb, &nr) == APR_SUCCESS) {
        /* Although we write just one byte to the other end of the pipe
         * during wakeup, multiple treads could call the wakeup.
         * So simply drain out from the input side of the pipe all
         * the data.
         */
        if (nr != sizeof(rb))
            break;
    }
}

static apr_status_t wakeup_pipe_cleanup(void *p)
{
    apr_pollset_t *pollset = (apr_pollset_t *) p;
    if (pollset->flags & APR_POLLSET_WAKEABLE) {
        /* Close both sides of the wakeup pipe */
        if (pollset->wakeup_pipe[0]) {
            apr_file_close(pollset->wakeup_pipe[0]);
            pollset->wakeup_pipe[0] = NULL;
        }
        if (pollset->wakeup_pipe[1]) {
            apr_file_close(pollset->wakeup_pipe[1]);
            pollset->wakeup_pipe[1] = NULL;
        }
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollset_create(apr_pollset_t **pollset,
                                             apr_uint32_t size,
                                             apr_pool_t *p,
                                             apr_uint32_t flags)
{
    if (flags & APR_POLLSET_THREADSAFE) {                
        *pollset = NULL;
        return APR_ENOTIMPL;
    }
    if (flags & APR_POLLSET_WAKEABLE) {
        /* Add room for wakeup descriptor */
        size++;
    }

    *pollset = apr_palloc(p, sizeof(**pollset));
    (*pollset)->nelts = 0;
    (*pollset)->nalloc = size;
    (*pollset)->pool = p;
    (*pollset)->flags = flags;
    (*pollset)->pollset = apr_palloc(p, size * sizeof(struct pollfd));
    (*pollset)->query_set = apr_palloc(p, size * sizeof(apr_pollfd_t));
    (*pollset)->result_set = apr_palloc(p, size * sizeof(apr_pollfd_t));

    if (flags & APR_POLLSET_WAKEABLE) {
        apr_status_t rv;
        /* Create wakeup pipe */
        if ((rv = create_wakeup_pipe(*pollset)) != APR_SUCCESS) {
            *pollset = NULL;
            return rv;
        }
        apr_pool_cleanup_register(p, *pollset, wakeup_pipe_cleanup,
                                  apr_pool_cleanup_null);
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollset_destroy(apr_pollset_t *pollset)
{
    if (pollset->flags & APR_POLLSET_WAKEABLE) 
        return apr_pool_cleanup_run(pollset->pool, pollset,
                                    wakeup_pipe_cleanup);
    else
        return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollset_add(apr_pollset_t *pollset,
                                          const apr_pollfd_t *descriptor)
{
    if (pollset->nelts == pollset->nalloc) {
        return APR_ENOMEM;
    }

    pollset->query_set[pollset->nelts] = *descriptor;

    if (descriptor->desc_type == APR_POLL_SOCKET) {
        pollset->pollset[pollset->nelts].fd = descriptor->desc.s->socketdes;
    }
    else {
        pollset->pollset[pollset->nelts].fd = descriptor->desc.f->filedes;
    }

    pollset->pollset[pollset->nelts].events =
        get_event(descriptor->reqevents);
    pollset->nelts++;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollset_remove(apr_pollset_t *pollset,
                                             const apr_pollfd_t *descriptor)
{
    apr_uint32_t i;

    for (i = 0; i < pollset->nelts; i++) {
        if (descriptor->desc.s == pollset->query_set[i].desc.s) {
            /* Found an instance of the fd: remove this and any other copies */
            apr_uint32_t dst = i;
            apr_uint32_t old_nelts = pollset->nelts;
            pollset->nelts--;
            for (i++; i < old_nelts; i++) {
                if (descriptor->desc.s == pollset->query_set[i].desc.s) {
                    pollset->nelts--;
                }
                else {
                    pollset->pollset[dst] = pollset->pollset[i];
                    pollset->query_set[dst] = pollset->query_set[i];
                    dst++;
                }
            }
            return APR_SUCCESS;
        }
    }

    return APR_NOTFOUND;
}

APR_DECLARE(apr_status_t) apr_pollset_poll(apr_pollset_t *pollset,
                                           apr_interval_time_t timeout,
                                           apr_int32_t *num,
                                           const apr_pollfd_t **descriptors)
{
    int ret;
    apr_status_t rv = APR_SUCCESS;
    apr_uint32_t i, j;

    if (timeout > 0) {
        timeout /= 1000;
    }
    ret = poll(pollset->pollset, pollset->nelts, timeout);
    (*num) = ret;
    if (ret < 0) {
        return apr_get_netos_error();
    }
    else if (ret == 0) {
        return APR_TIMEUP;
    }
    else {
        for (i = 0, j = 0; i < pollset->nelts; i++) {
            if (pollset->pollset[i].revents != 0) {
                /* Check if the polled descriptor is our
                 * wakeup pipe. In that case do not put it result set.
                 */
                if ((pollset->flags & APR_POLLSET_WAKEABLE) &&
                    pollset->query_set[i].desc_type == APR_POLL_FILE &&
                    pollset->query_set[i].desc.f == pollset->wakeup_pipe[0]) {
                        drain_wakeup_pipe(pollset);
                        rv = APR_EINTR;
                }
                else {
                    pollset->result_set[j] = pollset->query_set[i];
                    pollset->result_set[j].rtnevents =
                        get_revent(pollset->pollset[i].revents);
                    j++;
                }
            }
        }
        if (((*num) = j)) {
            rv = APR_SUCCESS;
        }
    }
    if (descriptors && (*num))
        *descriptors = pollset->result_set;
    return rv;
}

APR_DECLARE(apr_status_t) apr_pollset_wakeup(apr_pollset_t *pollset)
{
    if (pollset->flags & APR_POLLSET_WAKEABLE)
        return apr_file_putc(1, pollset->wakeup_pipe[1]);
    else
        return APR_EINIT;
}

APR_DECLARE(apr_status_t) apr_pollcb_create(apr_pollcb_t **pollcb,
                                            apr_uint32_t size,
                                            apr_pool_t *p,
                                            apr_uint32_t flags)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_pollcb_add(apr_pollcb_t *pollcb,
                                         apr_pollfd_t *descriptor)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_pollcb_remove(apr_pollcb_t *pollcb,
                                            apr_pollfd_t *descriptor)
{
    return APR_ENOTIMPL;
}


APR_DECLARE(apr_status_t) apr_pollcb_poll(apr_pollcb_t *pollcb,
                                          apr_interval_time_t timeout,
                                          apr_pollcb_cb_t func,
                                          void *baton)
{
    return APR_ENOTIMPL;
}

#endif /* POLLSET_USES_POLL */
