/* ====================================================================
 * Copyright (c) 1996-2000 The Apache Software Foundation.  All rights reserved.
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
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
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
 * individuals on behalf of the Apache Software Foundation and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "networkio.h"

ap_status_t ap_send(struct socket_t *sock, const char *buf, ap_ssize_t *len)
{
    ssize_t rv;
	
    do {
        rv = send(sock->socketdes, buf, (*len), 0);
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout > 0) {
        struct timeval *tv;
        fd_set fdset;
        int srv;

        do {
            FD_ZERO(&fdset);
            FD_SET(sock->socketdes, &fdset);
            if (sock->timeout == -1)
                tv = NULL;
            else {
                tv = (struct timeval *)ap_palloc(sock->cntxt, sizeof(struct timeval));
                tv->tv_sec  = sock->timeout;
                tv->tv_usec = 0;
            }
            
            srv = select(FD_SETSIZE, NULL, &fdset, NULL, tv);
        } while (srv == -1 && errno == EINTR);

        if (srv == 0) {
            (*len) = -1;
            return APR_TIMEUP;
        }
        if (srv < 1) {
            (*len) = -1;
            return errno;
        }
        else {
            do {
                rv = send(sock->socketdes, buf, (*len),0);
            } while (rv == -1 && errno == EINTR);
        }
    }
    (*len) = rv;
    return APR_SUCCESS;
}

ap_status_t ap_recv(struct socket_t *sock, char *buf, ap_ssize_t *len)
{
    ap_ssize_t rv;
   
    do {
        rv = recv(sock->socketdes, buf, (*len), 0);
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout > 0) {
        struct timeval *tv;
        fd_set fdset;
        int srv;

        do {
            FD_ZERO(&fdset);
            FD_SET(sock->socketdes, &fdset);
            if (sock->timeout == -1)
                tv = NULL;
            else {
                tv = (struct timeval *)ap_palloc(sock->cntxt, sizeof(struct timeval));
                tv->tv_sec  = sock->timeout;
                tv->tv_usec = 0;
            }
            
            srv = select(FD_SETSIZE, &fdset, NULL, NULL, tv);
        } while (srv == -1 && errno == EINTR);

        if (srv == 0) {
            (*len) = -1;
            return APR_TIMEUP;
        }
        if (srv < 1) {
            (*len) = -1;
            return errno;
        }
        else {
            do {
                rv = recv(sock->socketdes, buf, (*len), 0);
            } while (rv == -1 && errno == EINTR);
        }
    }
    (*len) = rv;
    return APR_SUCCESS;
}

