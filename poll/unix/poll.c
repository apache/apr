/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
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
#include "apr_arch_networkio.h"
#include "apr_arch_file_io.h"
#if HAVE_POLL_H
#include <poll.h>
#endif
#if HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

#ifdef HAVE_KQUEUE
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#endif

#ifdef HAVE_EPOLL
#include <sys/epoll.h>
#endif

#ifdef NETWARE
#define HAS_SOCKETS(dt) (dt == APR_POLL_SOCKET) ? 1 : 0
#define HAS_PIPES(dt) (dt == APR_POLL_FILE) ? 1 : 0
#endif

#ifdef HAVE_KQUEUE
static apr_int16_t get_kqueue_revent(apr_int16_t event, apr_int16_t flags)
{
    apr_int16_t rv = 0;

    if (event & EVFILT_READ)
        rv |= APR_POLLIN;
    if (event & EVFILT_WRITE)
        rv |= APR_POLLOUT;
    if (flags & EV_ERROR || flags & EV_EOF)
        rv |= APR_POLLERR;

    return rv;
}

#endif

#ifdef HAVE_EPOLL
static apr_int16_t get_epoll_event(apr_int16_t event)
{
    apr_int16_t rv = 0;

    if (event & APR_POLLIN)
        rv |= EPOLLIN;
    if (event & APR_POLLPRI)
        rv |= EPOLLPRI;
    if (event & APR_POLLOUT)
        rv |= EPOLLOUT;
    if (event & APR_POLLERR)
        rv |= EPOLLERR;
    if (event & APR_POLLHUP)
        rv |= EPOLLHUP;
    /* APR_POLLNVAL is not handled by epoll. */

    return rv;
}

static apr_int16_t get_epoll_revent(apr_int16_t event)
{
    apr_int16_t rv = 0;

    if (event & EPOLLIN)
        rv |= APR_POLLIN;
    if (event & EPOLLPRI)
        rv |= APR_POLLPRI;
    if (event & EPOLLOUT)
        rv |= APR_POLLOUT;
    if (event & EPOLLERR)
        rv |= APR_POLLERR;
    if (event & EPOLLHUP)
        rv |= APR_POLLHUP;
    /* APR_POLLNVAL is not handled by epoll. */

    return rv;
}
#endif

#ifdef HAVE_POLL    /* We can just use poll to do our socket polling. */

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

#define SMALL_POLLSET_LIMIT  8

APR_DECLARE(apr_status_t) apr_poll(apr_pollfd_t *aprset, apr_int32_t num,
                      apr_int32_t *nsds, apr_interval_time_t timeout)
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

    for (i = 0; i < num; i++) {
        aprset[i].rtnevents = get_revent(pollset[i].revents);
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


#else    /* Use select to mimic poll */

APR_DECLARE(apr_status_t) apr_poll(apr_pollfd_t *aprset, int num, apr_int32_t *nsds, 
		    apr_interval_time_t timeout)
{
    fd_set readset, writeset, exceptset;
    int rv, i;
    int maxfd = -1;
    struct timeval tv, *tvptr;
#ifdef NETWARE
    apr_datatype_e set_type = APR_NO_DESC;
#endif

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = (long)apr_time_sec(timeout);
        tv.tv_usec = (long)apr_time_usec(timeout);
        tvptr = &tv;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_ZERO(&exceptset);

    for (i = 0; i < num; i++) {
        apr_os_sock_t fd;

        aprset[i].rtnevents = 0;

        if (aprset[i].desc_type == APR_POLL_SOCKET) {
#ifdef NETWARE
            if (HAS_PIPES(set_type)) {
                return APR_EBADF;
            }
            else {
                set_type = APR_POLL_SOCKET;
            }
#endif
            fd = aprset[i].desc.s->socketdes;
        }
        else if (aprset[i].desc_type == APR_POLL_FILE) {
#if !APR_FILES_AS_SOCKETS
            return APR_EBADF;
#else
#ifdef NETWARE
            if (aprset[i].desc.f->is_pipe && !HAS_SOCKETS(set_type)) {
                set_type = APR_POLL_FILE;
            }
            else
                return APR_EBADF;
#endif /* NETWARE */

            fd = aprset[i].desc.f->filedes;

#endif /* APR_FILES_AS_SOCKETS */
        }
        else {
            break;
        }
#if !defined(WIN32) && !defined(NETWARE) /* socket sets handled with array of handles */
        if (fd >= FD_SETSIZE) {
            /* XXX invent new error code so application has a clue */
            return APR_EBADF;
        }
#endif
        if (aprset[i].reqevents & APR_POLLIN) {
            FD_SET(fd, &readset);
        }
        if (aprset[i].reqevents & APR_POLLOUT) {
            FD_SET(fd, &writeset);
        }
        if (aprset[i].reqevents & 
            (APR_POLLPRI | APR_POLLERR | APR_POLLHUP | APR_POLLNVAL)) {
            FD_SET(fd, &exceptset);
        }
        if ((int)fd > maxfd) {
            maxfd = (int)fd;
        }
    }

#ifdef NETWARE
    if (HAS_PIPES(set_type)) {
        rv = pipe_select(maxfd + 1, &readset, &writeset, &exceptset, tvptr);
    }
    else {
#endif

    rv = select(maxfd + 1, &readset, &writeset, &exceptset, tvptr);

#ifdef NETWARE
    }
#endif

    (*nsds) = rv;
    if ((*nsds) == 0) {
        return APR_TIMEUP;
    }
    if ((*nsds) < 0) {
        return apr_get_netos_error();
    }

    for (i = 0; i < num; i++) {
        apr_os_sock_t fd;

        if (aprset[i].desc_type == APR_POLL_SOCKET) {
            fd = aprset[i].desc.s->socketdes;
        }
        else if (aprset[i].desc_type == APR_POLL_FILE) {
#if !APR_FILES_AS_SOCKETS
            return APR_EBADF;
#else
            fd = aprset[i].desc.f->filedes;
#endif
        }
        else {
            break;
        }
        if (FD_ISSET(fd, &readset)) {
            aprset[i].rtnevents |= APR_POLLIN;
        }
        if (FD_ISSET(fd, &writeset)) {
            aprset[i].rtnevents |= APR_POLLOUT;
        }
        if (FD_ISSET(fd, &exceptset)) {
            aprset[i].rtnevents |= APR_POLLERR;
        }
    }

    return APR_SUCCESS;
}

#endif 

struct apr_pollset_t {
    apr_pool_t *pool;

    apr_uint32_t nelts;
    apr_uint32_t nalloc;
#ifdef HAVE_KQUEUE
    int kqueue_fd;
    struct kevent kevent;
    struct kevent *ke_set;
#elif defined(HAVE_EPOLL)
    int epoll_fd;
    struct epoll_event *pollset;
#elif defined(HAVE_POLL)
    struct pollfd *pollset;
#else
    fd_set readset, writeset, exceptset;
    int maxfd;
#endif
    apr_pollfd_t *query_set;
    apr_pollfd_t *result_set;

#ifdef NETWARE
    int set_type;
#endif
};

#if defined(HAVE_KQUEUE) || defined(HAVE_EPOLL)
static apr_status_t backend_cleanup(void *p_)
{
    apr_pollset_t *pollset = (apr_pollset_t *)p_;
#ifdef HAVE_KQUEUE
    close(pollset->kqueue_fd);
#elif defined(HAVE_EPOLL)
    close(pollset->epoll_fd);
#endif
    return APR_SUCCESS;
}
#endif /* HAVE_KQUEUE || HAVE_EPOLL */

APR_DECLARE(apr_status_t) apr_pollset_create(apr_pollset_t **pollset,
                                             apr_uint32_t size,
                                             apr_pool_t *p,
                                             apr_uint32_t flags)
{
#if !defined(HAVE_KQUEUE) && !defined(HAVE_EPOLL) && !defined(HAVE_POLL) && defined(FD_SETSIZE)
    if (size > FD_SETSIZE) {
        *pollset = NULL;
        return APR_EINVAL;
    }
#endif
    *pollset = apr_palloc(p, sizeof(**pollset));
    (*pollset)->nelts = 0;
    (*pollset)->nalloc = size;
    (*pollset)->pool = p;
#ifdef HAVE_KQUEUE
    (*pollset)->ke_set = (struct kevent*)apr_palloc(p, size * sizeof(struct  kevent));
    memset((*pollset)->ke_set, 0, size * sizeof(struct kevent));
    (*pollset)->kqueue_fd = kqueue();
    if ((*pollset)->kqueue_fd == -1) {
         return APR_ENOMEM;
    }
    apr_pool_cleanup_register(p, (void*)(*pollset), backend_cleanup, 
        apr_pool_cleanup_null);
#elif defined(HAVE_EPOLL)
    (*pollset)->epoll_fd = epoll_create(size);
    (*pollset)->pollset = apr_palloc(p, size * sizeof(struct epoll_event));
    apr_pool_cleanup_register(p, (void*)(*pollset), backend_cleanup, 
        apr_pool_cleanup_null);
#elif defined(HAVE_POLL)
    (*pollset)->pollset = apr_palloc(p, size * sizeof(struct pollfd));
#else
    FD_ZERO(&((*pollset)->readset));
    FD_ZERO(&((*pollset)->writeset));
    FD_ZERO(&((*pollset)->exceptset));
    (*pollset)->maxfd = 0;
#ifdef NETWARE
    (*pollset)->set_type = APR_NO_DESC;
#endif
#endif
    (*pollset)->query_set = apr_palloc(p, size * sizeof(apr_pollfd_t));
    (*pollset)->result_set = apr_palloc(p, size * sizeof(apr_pollfd_t));

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollset_destroy(apr_pollset_t *pollset)
{
#if defined(HAVE_KQUEUE) || defined(HAVE_EPOLL)
    return apr_pool_cleanup_run(pollset->pool, pollset, backend_cleanup);
#else
    return APR_SUCCESS;
#endif
}

APR_DECLARE(apr_status_t) apr_pollset_add(apr_pollset_t *pollset,
                                          const apr_pollfd_t *descriptor)
{
#ifdef HAVE_KQUEUE
    apr_os_sock_t fd;
#elif defined(HAVE_EPOLL)
    struct epoll_event ev;
    int ret = -1;
#else
#if !defined(HAVE_POLL)
    apr_os_sock_t fd;
#endif
#endif

    if (pollset->nelts == pollset->nalloc) {
        return APR_ENOMEM;
    }

    pollset->query_set[pollset->nelts] = *descriptor;

#ifdef HAVE_KQUEUE
    if (descriptor->desc_type == APR_POLL_SOCKET) {
        fd = descriptor->desc.s->socketdes;
    }
    else {
        fd = descriptor->desc.f->filedes;
    }

    if (descriptor->reqevents & APR_POLLIN) {
        EV_SET(&pollset->kevent, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

        if (kevent(pollset->kqueue_fd, &pollset->kevent, 1, NULL, 0,
                   NULL) == -1) {
            return APR_ENOMEM;
        }
    }

    if (descriptor->reqevents & APR_POLLOUT) {
        EV_SET(&pollset->kevent, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);

        if (kevent(pollset->kqueue_fd, &pollset->kevent, 1, NULL, 0,
                   NULL) == -1) {
            return APR_ENOMEM;
        }
    }

#elif defined(HAVE_EPOLL)
    ev.events = get_epoll_event(descriptor->reqevents);
    if (descriptor->desc_type == APR_POLL_SOCKET) {
        ev.data.fd = descriptor->desc.s->socketdes;
        ret = epoll_ctl(pollset->epoll_fd, EPOLL_CTL_ADD,
                        descriptor->desc.s->socketdes, &ev);
    }
    else {
        ev.data.fd = descriptor->desc.f->filedes;
        ret = epoll_ctl(pollset->epoll_fd, EPOLL_CTL_ADD,
                        descriptor->desc.f->filedes, &ev);
    }
    if (0 != ret) {
        return APR_EBADF;
    }
#elif defined(HAVE_POLL)

    if (descriptor->desc_type == APR_POLL_SOCKET) {
        pollset->pollset[pollset->nelts].fd = descriptor->desc.s->socketdes;
    }
    else {
        pollset->pollset[pollset->nelts].fd = descriptor->desc.f->filedes;
    }

    pollset->pollset[pollset->nelts].events = get_event(descriptor->reqevents);
#else
    if (descriptor->desc_type == APR_POLL_SOCKET) {
#ifdef NETWARE
        /* NetWare can't handle mixed descriptor types in select() */
        if (HAS_PIPES(pollset->set_type)) {
            return APR_EBADF;
        }
        else {
            pollset->set_type = APR_POLL_SOCKET;
        }
#endif
        fd = descriptor->desc.s->socketdes;
    }
    else {
#if !APR_FILES_AS_SOCKETS
        return APR_EBADF;
#else
#ifdef NETWARE
        /* NetWare can't handle mixed descriptor types in select() */
        if (descriptor->desc.f->is_pipe && !HAS_SOCKETS(pollset->set_type)) {
            pollset->set_type = APR_POLL_FILE;
            fd = descriptor->desc.f->filedes;
        }
        else {
            return APR_EBADF;
        }
#else
        fd = descriptor->desc.f->filedes;
#endif
#endif
    }
#if !defined(WIN32) && !defined(NETWARE) /* socket sets handled with array of handles */
    if (fd >= FD_SETSIZE) {
        /* XXX invent new error code so application has a clue */
        return APR_EBADF;
    }
#endif
    if (descriptor->reqevents & APR_POLLIN) {
        FD_SET(fd, &(pollset->readset));
    }
    if (descriptor->reqevents & APR_POLLOUT) {
        FD_SET(fd, &(pollset->writeset));
    }
    if (descriptor->reqevents &
        (APR_POLLPRI | APR_POLLERR | APR_POLLHUP | APR_POLLNVAL)) {
        FD_SET(fd, &(pollset->exceptset));
    }
    if ((int)fd > pollset->maxfd) {
        pollset->maxfd = (int)fd;
    }
#endif
    pollset->nelts++;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pollset_remove(apr_pollset_t *pollset,
                                             const apr_pollfd_t *descriptor)
{
    apr_uint32_t i;
#ifdef HAVE_KQUEUE
    apr_os_sock_t fd;
#elif defined(HAVE_EPOLL)
    struct epoll_event ev;
    int ret = -1;
#elif !defined(HAVE_POLL)
    apr_os_sock_t fd;
#endif

#ifdef HAVE_KQUEUE
    for (i = 0; i < pollset->nelts; i++) {
        if (descriptor->desc.s == pollset->query_set[i].desc.s) {
            /* Found an instance of the fd: remove this and any other copies  */
            apr_uint32_t dst = i;
            apr_uint32_t old_nelts = pollset->nelts;
            pollset->nelts--;
            for (i++; i < old_nelts; i++) {
                if (descriptor->desc.s == pollset->query_set[i].desc.s) {
                    pollset->nelts--;
                }
                else {
                    pollset->query_set[dst] = pollset->query_set[i];
                    dst++;
                }
            }

            if (descriptor->desc_type == APR_POLL_SOCKET) {
                fd = descriptor->desc.s->socketdes;
            }
            else {
                fd = descriptor->desc.f->filedes;
            }

            if (descriptor->reqevents & APR_POLLIN) {
                EV_SET(&pollset->kevent, fd,
                       EVFILT_READ, EV_DELETE, 0, 0, NULL);

                if (kevent(pollset->kqueue_fd, &pollset->kevent, 1, NULL, 0,
                          NULL) == -1) {
                    return APR_EBADF;
                }
            }

            if (descriptor->reqevents & APR_POLLOUT) {
                EV_SET(&pollset->kevent, fd,
                       EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

                if (kevent(pollset->kqueue_fd, &pollset->kevent, 1, NULL, 0,
                          NULL) == -1) {
                    return APR_EBADF;
                }
            }

            return APR_SUCCESS;
        }
    }
#elif defined(HAVE_EPOLL)
    for (i = 0; i < pollset->nelts; i++) {
        if (descriptor->desc.s == pollset->query_set[i].desc.s) {
            /* Found an instance of the fd: remove this and any other copies  */
            apr_uint32_t dst = i;
            apr_uint32_t old_nelts = pollset->nelts;
            pollset->nelts--;
            for (i++; i < old_nelts; i++) {
                if (descriptor->desc.s == pollset->query_set[i].desc.s) {
                    pollset->nelts--;
                }
                else {
                    pollset->query_set[dst] = pollset->query_set[i];
                    dst++;
                }
            }
            ev.events = get_epoll_event(descriptor->reqevents);
            if (descriptor->desc_type == APR_POLL_SOCKET) {
                ev.data.fd = descriptor->desc.s->socketdes;
                ret = epoll_ctl(pollset->epoll_fd, EPOLL_CTL_DEL,
                                descriptor->desc.s->socketdes, &ev);
            }
            else {
                ev.data.fd = descriptor->desc.f->filedes;
                ret = epoll_ctl(pollset->epoll_fd, EPOLL_CTL_DEL,
                                descriptor->desc.f->filedes, &ev);
            }
            if (ret < 0) {
                return APR_EBADF;
            }

            return APR_SUCCESS;
        }
    }
#elif defined(HAVE_POLL)
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

#else /* no poll */
    if (descriptor->desc_type == APR_POLL_SOCKET) {
        fd = descriptor->desc.s->socketdes;
    }
    else {
#if !APR_FILES_AS_SOCKETS
        return APR_EBADF;
#else
        fd = descriptor->desc.f->filedes;
#endif
    }

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
                    pollset->query_set[dst] = pollset->query_set[i];
                    dst++;
                }
            }
            FD_CLR(fd, &(pollset->readset));
            FD_CLR(fd, &(pollset->writeset));
            FD_CLR(fd, &(pollset->exceptset));
            if (((int)fd == pollset->maxfd) && (pollset->maxfd > 0)) {
                pollset->maxfd--;
            }
            return APR_SUCCESS;
        }
    }
#endif /* no poll */

    return APR_NOTFOUND;
}
#ifdef HAVE_KQUEUE
APR_DECLARE(apr_status_t) apr_pollset_poll(apr_pollset_t *pollset,
                                           apr_interval_time_t timeout,
                                           apr_int32_t *num,
                                           const apr_pollfd_t **descriptors)
{
    int rv;
    apr_uint32_t i, j, r = 0;
    struct timespec tv, *tvptr;

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = (long)apr_time_sec(timeout);
        tv.tv_nsec = (long)apr_time_msec(timeout);
        tvptr = &tv;
    }

    rv = kevent(pollset->kqueue_fd, NULL, 0, pollset->ke_set, pollset->nelts,
                tvptr);
    (*num) = rv;
    if (rv < 0) {
        return apr_get_netos_error();
    }
    if (rv == 0) {
        return APR_TIMEUP;
    }

    /* TODO: Is there a better way to re-associate our data? */
    for (i = 0; i < pollset->nelts; i++) {
        apr_os_sock_t fd;
        if (pollset->query_set[i].desc_type == APR_POLL_SOCKET) {
            fd = pollset->query_set[i].desc.s->socketdes;
        }
        else {
            fd = pollset->query_set[i].desc.f->filedes;
        }
        for (j = 0; j < rv; j++) {
            if (pollset->ke_set[j].ident == fd ) {
                pollset->result_set[r] = pollset->query_set[i];
                pollset->result_set[r].rtnevents =
                    get_kqueue_revent(pollset->ke_set[j].filter,
                                      pollset->ke_set[j].flags);
                r++;
            }
        }
    }

    (*num) = r;

    if (descriptors) {
        *descriptors = pollset->result_set;
    }

    return APR_SUCCESS;
}

#elif defined(HAVE_EPOLL)

APR_DECLARE(apr_status_t) apr_pollset_poll(apr_pollset_t *pollset,
                                           apr_interval_time_t timeout,
                                           apr_int32_t *num,
                                           const apr_pollfd_t **descriptors)
{
    int rv;
    apr_uint32_t i, j, k;

    if (timeout > 0) {
        timeout /= 1000;
    }

    rv = epoll_wait(pollset->epoll_fd, pollset->pollset, pollset->nelts,
                    timeout);
    (*num) = rv;
    if (rv < 0) {
        return apr_get_netos_error();
    }
    if (rv == 0) {
        return APR_TIMEUP;
    }
    j = 0;
    for (i = 0; i < pollset->nelts; i++) {
        if (pollset->pollset[i].events != 0) {
            /* TODO: Is there a better way to re-associate our data? */
            for (k = 0; k < pollset->nelts; k++) {
                if (pollset->query_set[k].desc_type == APR_POLL_SOCKET &&
                    pollset->query_set[k].desc.s->socketdes ==
                        pollset->pollset[i].data.fd) {
                    pollset->result_set[j] = pollset->query_set[k];
                    pollset->result_set[j].rtnevents =
                        get_epoll_revent(pollset->pollset[i].events);
                    j++;
                    break;
                }
                else if (pollset->query_set[k].desc_type == APR_POLL_FILE 
                         && pollset->query_set[k].desc.f->filedes ==
                            pollset->pollset[i].data.fd) {
                    pollset->result_set[j] = pollset->query_set[k];
                    pollset->result_set[j].rtnevents =
                        get_epoll_revent(pollset->pollset[i].events);
                    j++;
                    break;
                }
            }
        }
    }
    if (descriptors) {
        *descriptors = pollset->result_set;
    }
    return APR_SUCCESS;
}
#elif defined(HAVE_POLL)
APR_DECLARE(apr_status_t) apr_pollset_poll(apr_pollset_t *pollset,
                                           apr_interval_time_t timeout,
                                           apr_int32_t *num,
                                           const apr_pollfd_t **descriptors)
{
    int rv;
    apr_uint32_t i, j;

    if (timeout > 0) {
        timeout /= 1000;
    }
    rv = poll(pollset->pollset, pollset->nelts, timeout);
    (*num) = rv;
    if (rv < 0) {
        return apr_get_netos_error();
    }
    if (rv == 0) {
        return APR_TIMEUP;
    }
    j = 0;
    for (i = 0; i < pollset->nelts; i++) {
        if (pollset->pollset[i].revents != 0) {
            pollset->result_set[j] = pollset->query_set[i];
            pollset->result_set[j].rtnevents =
                get_revent(pollset->pollset[i].revents);
            j++;
        }
    }
    if (descriptors)
        *descriptors = pollset->result_set;
    return APR_SUCCESS;
}

#else /* no poll */

APR_DECLARE(apr_status_t) apr_pollset_poll(apr_pollset_t *pollset,
                                           apr_interval_time_t timeout,
                                           apr_int32_t *num,
                                           const apr_pollfd_t **descriptors)
{
    int rv;
    apr_uint32_t i, j;
    struct timeval tv, *tvptr;
    fd_set readset, writeset, exceptset;

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = (long)apr_time_sec(timeout);
        tv.tv_usec = (long)apr_time_usec(timeout);
        tvptr = &tv;
    }

    memcpy(&readset, &(pollset->readset), sizeof(fd_set));
    memcpy(&writeset, &(pollset->writeset), sizeof(fd_set));
    memcpy(&exceptset, &(pollset->exceptset), sizeof(fd_set));

#ifdef NETWARE
    if (HAS_PIPES(pollset->set_type)) {
        rv = pipe_select(pollset->maxfd + 1, &readset, &writeset, &exceptset, tvptr);
    }
    else
#endif
    rv = select(pollset->maxfd + 1, &readset, &writeset, &exceptset, tvptr);

    (*num) = rv;
    if (rv < 0) {
        return apr_get_netos_error();
    }
    if (rv == 0) {
        return APR_TIMEUP;
    }
    j = 0;
    for (i = 0; i < pollset->nelts; i++) {
        apr_os_sock_t fd;
        if (pollset->query_set[i].desc_type == APR_POLL_SOCKET) {
            fd = pollset->query_set[i].desc.s->socketdes;
        }
        else {
#if !APR_FILES_AS_SOCKETS
            return APR_EBADF;
#else
            fd = pollset->query_set[i].desc.f->filedes;
#endif
        }
        if (FD_ISSET(fd, &readset) || FD_ISSET(fd, &writeset) ||
            FD_ISSET(fd, &exceptset)) {
            pollset->result_set[j] = pollset->query_set[i];
            pollset->result_set[j].rtnevents = 0;
            if (FD_ISSET(fd, &readset)) {
                pollset->result_set[j].rtnevents |= APR_POLLIN;
            }
            if (FD_ISSET(fd, &writeset)) {
                pollset->result_set[j].rtnevents |= APR_POLLOUT;
            }
            if (FD_ISSET(fd, &exceptset)) {
                pollset->result_set[j].rtnevents |= APR_POLLERR;
            }
            j++;
        }
    }

    if (descriptors)
        *descriptors = pollset->result_set;
    return APR_SUCCESS;
}

#endif /* no poll */
