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

/*  OS/2 doesn't have a poll function, implement using select */
 
ap_status_t ap_setup_poll(ap_context_t *cont, ap_int32_t num, struct pollfd_t **new)
{
    (*new) = (struct pollfd_t *)ap_palloc(cont, sizeof(struct pollfd_t) * num);

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    (*new)->curpos = 0;
    return APR_SUCCESS;
}



ap_status_t ap_add_poll_socket(struct pollfd_t *aprset, 
			       struct socket_t *sock, ap_int16_t event)
{
    int i = 0;
    
    while (i < aprset->curpos && aprset[i].sock->socketdes != sock->socketdes) {
        i++;
    }
    if (i >= aprset->curpos) {
        aprset->curpos++;
    } 
    aprset[i].sock = sock;
    aprset[i].events = event;

    return APR_SUCCESS;
}



ap_status_t ap_poll(struct pollfd_t *pollfdset, ap_int32_t *nsds, ap_int32_t timeout)
{
    int i;
    int rv = 0, maxfd = 0;
    time_t starttime;
    struct timeval tv;
    fd_set readfds, writefds, exceptfds;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&exceptfds);
    
    for (i = 0; i < *nsds; i++) {
        if (pollfdset[i].sock->socketdes > maxfd)
            maxfd = pollfdset[i].sock->socketdes;

        if (pollfdset[i].events & APR_POLLIN)
            FD_SET(pollfdset[i].sock->socketdes, &readfds);

        if (pollfdset[i].events & APR_POLLOUT)
            FD_SET(pollfdset[i].sock->socketdes, &writefds);

        if (pollfdset[i].events & APR_POLLPRI)
            FD_SET(pollfdset[i].sock->socketdes, &exceptfds);
    }

    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    time(&starttime);

    do {
        rv = select(maxfd + 1, &readfds, &writefds, &exceptfds, timeout >= 0 ? &tv : NULL);

        if (rv < 0 && errno == EINTR && timeout >= 0 ) {
            time_t elapsed = time(NULL) - starttime;

            if (timeout <= elapsed)
                break;

            tv.tv_sec = timeout - elapsed;
        }
    } while ( rv < 0 && errno == EINTR );

    if (rv >= 0) {
        for (i = 0; i < *nsds; i++) {
            pollfdset[i].revents =
                (FD_ISSET(pollfdset[i].sock->socketdes, &readfds) ? APR_POLLIN : 0) +
                (FD_ISSET(pollfdset[i].sock->socketdes, &writefds) ? APR_POLLOUT : 0) +
                (FD_ISSET(pollfdset[i].sock->socketdes, &exceptfds) ? APR_POLLPRI : 0);
        }
    }
    
    (*nsds) = rv;
    return rv < 0 ? errno : APR_SUCCESS;
}



ap_status_t ap_get_revents(struct pollfd_t *aprset, struct socket_t *sock, ap_int16_t *event)
{
    int i = 0;
    
    while (i < aprset->curpos && aprset[i].sock->socketdes != sock->socketdes) {
        i++;
    }
    if (i >= aprset->curpos) {
        return APR_INVALSOCK;
    } 
    (*event) = aprset[i].revents;
    return APR_SUCCESS;
}
