/* ====================================================================
 * Copyright (c) 1996-1999 The Apache Group.  All rights reserved.
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
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "networkio.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_lib.h"
#include <time.h>

ap_status_t ap_send(struct socket_t *sock, const char *buf, ap_ssize_t *len)
{
    ap_ssize_t rv;
    WSABUF data;
    int lasterror;    

    data.len = *len;
    data.buf = ap_pstrdup(sock->cntxt, buf);
    do {
        rv = WSASend(sock->sock, &data, 1, len, 0, NULL, NULL);
        if (rv == SOCKET_ERROR) {
            lasterror = WSAGetLastError();
        } 
    } while (rv == SOCKET_ERROR && lasterror == WSAEINTR);

    if (rv == SOCKET_ERROR && lasterror == WSAEWOULDBLOCK && sock->timeout > 0) {
        struct timeval tv;
        fd_set fdset;
        int srv;

        do {
            FD_ZERO(&fdset);
            FD_SET(sock->sock, &fdset);
            tv.tv_sec  = sock->timeout;
            tv.tv_usec = 0;

            srv = select(FD_SETSIZE, NULL, &fdset, NULL, &tv);
            if (srv == SOCKET_ERROR) {
                lasterror = WSAGetLastError();
            }
        } while (srv == SOCKET_ERROR && errno == WSAEINTR);

        if (srv == 0) {
            (*len) = -1;
            return APR_TIMEUP;
        }
        if (srv < 0) {
            (*len) = -1;
            return APR_EEXIST;
        }
        else {
            do {
                rv = WSASend(sock->sock, &data, 1, len, 0, NULL, NULL);
                if (rv == SOCKET_ERROR) {
                    lasterror = WSAGetLastError();
                } 
            } while (rv == SOCKET_ERROR && lasterror == WSAEINTR);
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_recv(struct socket_t *sock, char *buf, ap_ssize_t *len)
{
    ap_ssize_t rv;
    int lasterror;

    do {
        rv = recv(sock->sock, buf, *len, 0);
        if (rv == SOCKET_ERROR) {
            lasterror = WSAGetLastError();
        } 
    } while (rv == SOCKET_ERROR && lasterror == WSAEINTR);

    if (rv == SOCKET_ERROR && lasterror == WSAEWOULDBLOCK && sock->timeout > 0) {
        struct timeval tv;
        fd_set fdset;
        int srv;

        do {
            FD_ZERO(&fdset);
            FD_SET(sock->sock, &fdset);
            tv.tv_sec  = sock->timeout;
            tv.tv_usec = 0;

            srv = select(FD_SETSIZE, &fdset, NULL, NULL, &tv);
            if (srv == SOCKET_ERROR) {
                lasterror = WSAGetLastError();
            } 
        } while (srv == SOCKET_ERROR && errno == WSAEINTR);

        if (srv == 0) {
            (*len) = -1;
            return APR_TIMEUP;
        }
        else if (srv < 0) {
            (*len) = -1;
            return errno;
        }
        else {
            do {
                rv = recv(sock->sock, buf, *len, 0);
                if (rv == SOCKET_ERROR) {
                    lasterror = WSAGetLastError();
                } 
            } while (rv == SOCKET_ERROR && lasterror == WSAEINTR);
        }
    }
    (*len) = rv;
    return APR_SUCCESS;
}

