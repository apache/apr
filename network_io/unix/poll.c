/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "networkio.h"
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_POLL    /* We can just use poll to do our socket polling. */

/* ***APRDOC********************************************************
 * ap_status_t ap_setup_poll(ap_context_t *, ap_int32_t, ap_pollfd_t **)
 *    Setup the memory required for poll to operate properly.
 * arg 1) The context to operate on.
 * arg 2) The number of socket descriptors to be polled.
 * arg 3) The poll structure to be used. 
 */
ap_status_t ap_setup_poll(ap_context_t *cont, ap_int32_t num, struct pollfd_t **new)
{
    (*new) = (struct pollfd_t *)ap_palloc(cont, sizeof(struct pollfd_t));
    (*new)->sock = ap_palloc(cont, sizeof(struct socket_t) * num);
    (*new)->events = ap_palloc(cont, sizeof(ap_int16_t) * num);
    (*new)->revents = ap_palloc(cont, sizeof(ap_int16_t) * num);

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont;
    (*new)->curpos = 0;
    return APR_SUCCESS;
}

ap_int16_t get_event(ap_int16_t event)
{
    ap_int16_t rv = 0;

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

ap_int16_t get_revent(ap_int16_t event)
{
    ap_int16_t rv = 0;

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

/* ***APRDOC********************************************************
 * ap_status_t ap_add_poll_socket(ap_pollfd_t *, ap_socket_t *, ap_int16_t)
 *    Add a socket to the poll structure. 
 * arg 1) The poll structure we will be using. 
 * arg 2) The socket to add to the current poll structure. 
 * arg 3) The events to look for when we do the poll.  One of:
 *            APR_POLLIN    -- signal if read will not block
 *            APR_POLLPRI   -- signal if prioirty data is availble to be read
 *            APR_POLLOUT   -- signal if write will not block
 */
ap_status_t ap_add_poll_socket(struct pollfd_t *aprset, 
			       struct socket_t *sock, ap_int16_t event)
{
    int i = 0;
    
    while (i < aprset->curpos && aprset->sock[i] != sock->socketdes) {
        i++;
    }
    if (i >= aprset->curpos) {
        aprset->curpos++;
    } 
    aprset->sock[i] = sock->socketdes;
    aprset->events[i] = get_event(event);

    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_poll(ap_pollfd_t *, ap_int32_t *, ap_int32_t)
 *    Poll the sockets in the poll structure.  This is a blocking call,
 *    and it will not return until either a socket has been signalled, or
 *    the timeout has expired. 
 * arg 1) The poll structure we will be using. 
 * arg 2) The number of sockets we are polling. 
 * arg 3) The amount of time in seconds to wait.  This is a maximum, not 
 *        a minimum.  If a socket is signalled, we will wake up before this
 *        time.  A negative number means wait until a socket is signalled.
 * NOTE:  The number of sockets signalled is returned in the second argument. 
 */
ap_status_t ap_poll(struct pollfd_t *aprset, ap_int32_t *nsds, ap_int32_t timeout)
{
    int i;
    struct pollfd *pollset;
    int rv;

    pollset = (struct pollfd *)ap_palloc(aprset->cntxt, 
                                         sizeof(struct pollfd) * (*nsds));

    for (i = 0; i < (*nsds); i++) {
        pollset[i].fd = aprset->sock[i];
        pollset[i].events = aprset->events[i];
    }

    if (timeout != -1) {
        timeout *= 1000;
    }

    rv = poll(pollset, (*nsds), timeout);
    (*nsds) = rv;
    
    for (i = 0; i < (*nsds); i++) {
        aprset->revents[i] = get_revent(pollset[i].revents);
    }
    
    if ((*nsds) < 0) {
        return errno;
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_revents(ap_pollfd_t *, ap_socket_t *, ap_int_16_t *)
 *    Get the return events for the specified socket. 
 * arg 1) The poll structure we will be using. 
 * arg 2) The socket we wish to get information about. 
 * arg 3) The returned events for the socket.  One of:
 *            APR_POLLIN    -- Data is available to be read 
 *            APR_POLLPRI   -- Prioirty data is availble to be read
 *            APR_POLLOUT   -- Write will succeed
 *            APR_POLLERR   -- An error occurred on the socket
 *            APR_POLLHUP   -- The connection has been terminated
 *            APR_POLLNVAL  -- This is an invalid socket to poll on.
 *                             Socket not open.
 */
ap_status_t ap_get_revents(struct pollfd_t *aprset, struct socket_t *sock, ap_int16_t *event)
{
    int i = 0;
    
    while (i < aprset->curpos && aprset->sock[i] != sock->socketdes) {
        i++;
    }
    if (i >= aprset->curpos) {
        return APR_INVALSOCK;
    } 
    (*event) = aprset->revents[i];
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_remove_poll_socket(ap_pollfd_t *, ap_socket_t *, ap_int16_t)
 *    Add a socket to the poll structure. 
 * arg 1) The poll structure we will be using. 
 * arg 2) The socket to remove from the current poll structure. 
 * arg 3) The events to stop looking for during the poll.  One of:
 *            APR_POLLIN    -- signal if read will not block
 *            APR_POLLPRI   -- signal if prioirty data is availble to be read
 *            APR_POLLOUT   -- signal if write will not block
 */
ap_status_t ap_remove_poll_socket(struct pollfd_t *aprset, 
                                  struct socket_t *sock, ap_int16_t events)
{
    ap_int16_t newevents;
    int i = 0;
    
    while (i < aprset->curpos && aprset->sock[i] != sock->socketdes) {
        i++;
    }
    if (i >= aprset->curpos) {
        return APR_NOTFOUND;
    } 
    newevents = get_event(events);
    if (aprset->events[i] & newevents) {
        aprset->events[i] ^= newevents;
    }

    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_clear_poll_sockets(ap_pollfd_t *)
 *    Remove all sockets from the poll structure. 
 * arg 1) The poll structure we will be using. 
 * arg 3) The events to clear from all sockets.  One of:
 *            APR_POLLIN    -- signal if read will not block
 *            APR_POLLPRI   -- signal if prioirty data is availble to be read
 *            APR_POLLOUT   -- signal if write will not block
 */
ap_status_t ap_clear_poll_sockets(struct pollfd_t *aprset, ap_int16_t events)
{
    int i = 0;
    ap_int16_t newevents;

    newevents = get_event(events);

    while (i < aprset->curpos) {
        if (aprset->events[i] & newevents) {
            aprset->events[i] ^= newevents;
        }

        return APR_SUCCESS;
        i++;
    }
}

#else    /* Use select to mimic poll */

ap_status_t ap_setup_poll(ap_context_t *cont, ap_int32_t num, struct pollfd_t **
new)
{
    (*new) = (struct pollfd_t *)ap_palloc(cont, sizeof(struct pollfd_t) * num);
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont;
    (*new)->read = (fd_set *)ap_palloc(cont, sizeof(fd_set));
    (*new)->write = (fd_set *)ap_palloc(cont, sizeof(fd_set));
    (*new)->except = (fd_set *)ap_palloc(cont, sizeof(fd_set));
    FD_ZERO((*new)->read);
    FD_ZERO((*new)->write);
    FD_ZERO((*new)->except);
    (*new)->highsock = -1;
    return APR_SUCCESS;
}

ap_status_t ap_add_poll_socket(struct pollfd_t *aprset,
                               struct socket_t *sock, ap_int16_t event)
{
    if (event & APR_POLLIN) {
        FD_SET(sock->socketdes, aprset->read);
    }
    if (event & APR_POLLPRI) {
        FD_SET(sock->socketdes, aprset->read);
    }
    if (event & APR_POLLOUT) {
        FD_SET(sock->socketdes, aprset->write);
    }
    if (sock->socketdes > aprset->highsock) {
        aprset->highsock = sock->socketdes;
    }
    return APR_SUCCESS;
}

ap_status_t ap_poll(struct pollfd_t *aprset, ap_int32_t *nsds, ap_int32_t timeout)
{
    int rv;
    struct timeval *thetime;

    if (timeout == -1) {
        thetime = NULL;
    }
    else {
        /* Convert milli-seconds into seconds and micro-seconds. */
        thetime = (struct timeval *)ap_palloc(aprset->cntxt, sizeof(struct timeval));
        thetime->tv_sec = timeout;
        thetime->tv_usec = 0;
    }

    rv = select(aprset->highsock + 1, aprset->read, aprset->write, 
                    aprset->except, thetime);
    
    (*nsds) = rv;
    if ((*nsds) == 0) {
        return APR_TIMEUP;
    }
    if ((*nsds) < 0) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_get_revents(struct pollfd_t *aprset, struct socket_t *sock, ap_int16_t *event)
{
    ap_int16_t revents = 0;
    char data[256];
    int dummy = 256;
    int flags = MSG_PEEK;

    /* We just want to PEEK at the data, so I am setting up a dummy WSABUF
     * variable here.
     */
    if (FD_ISSET(sock->socketdes, aprset->read)) {
        revents |= APR_POLLIN;
        if (recv(sock->socketdes, &data, dummy, flags) == -1) {
            switch (errno) {
                case ECONNRESET:
                case ECONNABORTED:
                case ESHUTDOWN:
                case ENETRESET: {
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLHUP;
                    break;
                }
                case ENOTSOCK: {
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLNVAL;
                }
                default: {
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLERR;
                }
            }
        }
    }
    if (FD_ISSET(sock->socketdes, aprset->write)) {
        revents |= APR_POLLOUT;
    }
    /* I am assuming that the except is for out of band data, not a failed
     * connection on a non-blocking socket.  Might be a bad assumption, but
     * it works for now. rbb.
     */
    if (FD_ISSET(sock->socketdes, aprset->except)) {
        revents |= APR_POLLPRI;
    }

    (*event) = revents;
    return APR_SUCCESS;
}

ap_status_t ap_remove_poll_socket(struct pollfd_t *aprset, 
                                  struct socket_t *sock, ap_int16_t events)
{
    if (events & APR_POLLIN) {
        FD_CLR(sock->socketdes, aprset->read);
    }
    if (events & APR_POLLPRI) {
        FD_CLR(sock->socketdes, aprset->read);
    }
    if (events & APR_POLLOUT) {
        FD_CLR(sock->socketdes, aprset->write);
    }
    return APR_SUCCESS;
}

ap_status_t ap_clear_poll_sockets(struct pollfd_t *aprset, ap_int16_t event)
{
    if (event & APR_POLLIN) {
        FD_ZERO(aprset->read);
    }
    if (event & APR_POLLPRI) {
        FD_ZERO(aprset->read);
    }
    if (event & APR_POLLOUT) {
        FD_ZERO(aprset->write);
    }
    aprset->highsock = 0;
    return APR_SUCCESS;
}

#endif 

/* ***APRDOC********************************************************
 * ap_status_t ap_get_polldata(ap_pollfd_t *, void *)
 *    Return the context associated with the current poll.
 * arg 1) The currently open pollfd.
 * arg 2) The user data associated with the pollfd.
 */
ap_status_t ap_get_polldata(struct pollfd_t *pollfd, void *data)
{
    if (pollfd != NULL) {
        return ap_get_userdata(pollfd->cntxt, &data);
    }
    else {
        data = NULL;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_polldata(ap_pollfd_t *, void *)
 *    Return the context associated with the current poll.
 * arg 1) The currently open pollfd.
 * arg 2) The user data to associate with the pollfd.
 */
ap_status_t ap_set_polldata(struct pollfd_t *pollfd, void *data)
{
    if (pollfd != NULL) {
        return ap_set_userdata(pollfd->cntxt, data);
    }
    else {
        data = NULL;
        return APR_ENOFILE;
    }
}

