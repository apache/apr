/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include "apr.h"
#include "apr_poll.h"
#include "networkio.h"
#include "fileio.h"
#if HAVE_POLL_H
#include <poll.h>
#endif
#if HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

APR_DECLARE(apr_status_t) apr_poll_setup(apr_pollfd_t **new, apr_int32_t num, apr_pool_t *cont)
{
    (*new) = (apr_pollfd_t *)apr_pcalloc(cont, sizeof(apr_pollfd_t) * (num + 1));
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)[num].desc_type = APR_POLL_LASTDESC;
    (*new)[0].p = cont;
    return APR_SUCCESS;
}

APR_DECLARE(apr_pollfd_t*) find_poll_sock(apr_pollfd_t *aprset, apr_socket_t *sock)
{
    apr_pollfd_t *curr = aprset;
    
    while (curr->desc.s != sock) {
        if (curr->desc_type == APR_POLL_LASTDESC) {
            return NULL;
        }
        curr++;
    }

    return curr;
}

APR_DECLARE(apr_status_t) apr_poll_socket_add(apr_pollfd_t *aprset, 
			       apr_socket_t *sock, apr_int16_t event)
{
    apr_pollfd_t *curr = aprset;
    
    while (curr->desc_type != APR_NO_DESC) {
        if (curr->desc_type == APR_POLL_LASTDESC) {
            return APR_ENOMEM;
        }
        curr++;
    }
    curr->desc.s = sock;
    curr->desc_type = APR_POLL_SOCKET;
    curr->events = event;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_revents_get(apr_int16_t *event, apr_socket_t *sock, apr_pollfd_t *aprset)
{
    apr_pollfd_t *curr = find_poll_sock(aprset, sock);
    if (curr == NULL) {
        return APR_NOTFOUND;
    }

    (*event) = curr->revents;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_mask(apr_pollfd_t *aprset, 
                                  apr_socket_t *sock, apr_int16_t events)
{
    apr_pollfd_t *curr = find_poll_sock(aprset, sock);
    if (curr == NULL) {
        return APR_NOTFOUND;
    }
    
    if (curr->events & events) {
        curr->events ^= events;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_remove(apr_pollfd_t *aprset, apr_socket_t *sock)
{
    apr_pollfd_t *curr = find_poll_sock(aprset, sock);
    if (curr == NULL) {
        return APR_NOTFOUND;
    }

    curr->desc_type = APR_NO_DESC;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_clear(apr_pollfd_t *aprset, apr_int16_t events)
{
    apr_pollfd_t *curr = aprset;

    while (curr->desc_type != APR_POLL_LASTDESC) {
        if (curr->events & events) {
            curr->events &= ~events;
        }
    }
    return APR_SUCCESS;
}

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

APR_DECLARE(apr_status_t) apr_poll(apr_pollfd_t *aprset, apr_int32_t num,
                      apr_int32_t *nsds, apr_interval_time_t timeout)
{
    /* obvious optimization, it would be better if this could be allocated
     * on the stack.  For a single file/socket, this can be otpimized
     * very cleanly.
     */
    struct pollfd *pollset = apr_palloc(aprset->p,
                                        sizeof(struct pollfd) * num);
    int i;

    for (i = 0; i < num; i++) {
        if (aprset[i].desc_type == APR_POLL_SOCKET) {
            pollset[i].fd = aprset[i].desc.s->socketdes;
        }
        else if (aprset[i].desc_type == APR_POLL_FILE) {
            pollset[i].fd = aprset[i].desc.f->filedes;
        }
        pollset[i].events = get_event(aprset[i].events);
    }

    if (timeout > 0) {
        timeout /= 1000; /* convert microseconds to milliseconds */
    }

    i = poll(pollset, num, timeout);
    (*nsds) = i;

    for (i = 0; i < num; i++) {
        aprset[i].revents = get_revent(pollset[i].revents);
    }
    
    if ((*nsds) < 0) {
        return errno;
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

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = apr_time_sec(timeout);
        tv.tv_usec = apr_time_usec(timeout);
        tvptr = &tv;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_ZERO(&exceptset);

    for (i = 0; i < num; i++) {
        int fd;

        if (aprset[i].desc_type == APR_POLL_SOCKET) {
            fd = aprset[i].desc.s->socketdes;
        }
        else if (aprset[i].desc_type == APR_POLL_FILE) {
            fd = aprset[i].desc.f->filedes;
        }
        if (aprset[i].events & APR_POLLIN) {
            FD_SET(fd, &readset);
        }
        if (aprset[i].events & APR_POLLOUT) {
            FD_SET(fd, &writeset);
        }
        if (aprset[i].events & 
            (APR_POLLPRI | APR_POLLERR | APR_POLLHUP | APR_POLLNVAL)) {
            FD_SET(fd, &exceptset);
        }
        if (fd > maxfd) {
            maxfd = fd;
        }
    }

    rv = select(maxfd + 1, &readset, &writeset, &exceptset, tvptr);

    (*nsds) = rv;
    if ((*nsds) == 0) {
        return APR_TIMEUP;
    }
    if ((*nsds) < 0) {
        return errno;
    }

    for (i = 0; i < num; i++) {
        int fd;

        if (aprset[i].desc_type == APR_POLL_SOCKET) {
            fd = aprset[i].desc.s->socketdes;
        }
        else if (aprset[i].desc_type == APR_POLL_FILE) {
            fd = aprset[i].desc.f->filedes;
        }
        aprset[i].revents = 0;
        if (FD_ISSET(fd, &readset)) {
            aprset[i].revents |= APR_POLLIN;
        }
        if (FD_ISSET(fd, &writeset)) {
            aprset[i].revents |= APR_POLLOUT;
        }
        if (FD_ISSET(fd, &exceptset)) {
            aprset[i].revents |= APR_POLLERR;
        }
    }

    return APR_SUCCESS;
}

#endif 

#if APR_FILES_AS_SOCKETS
/* I'm not sure if this needs to return an apr_status_t or not, but
 * for right now, we'll leave it this way, and change it later if
 * necessary.
 */
APR_DECLARE(apr_status_t) apr_socket_from_file(apr_socket_t **newsock, apr_file_t *file)
{
    (*newsock) = apr_pcalloc(file->pool, sizeof(**newsock));
    (*newsock)->socketdes = file->filedes;
    (*newsock)->cntxt = file->pool;
    (*newsock)->timeout = file->timeout;
    return APR_SUCCESS;
}
#endif
