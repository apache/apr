/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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

#include "networkio.h"
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <string.h>
#include <winsock2.h>

ap_status_t soblock(SOCKET sd)
{
    int one = 1;

    if (ioctlsocket(sd, FIONBIO, &one) == SOCKET_ERROR) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t sononblock(SOCKET sd)
{
    int zero = 0;

    if (ioctlsocket(sd, FIONBIO, &zero) == SOCKET_ERROR) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_setsocketopt(struct socket_t *sock, ap_int32_t opt, ap_int32_t on)
{
    int one;
    struct linger li;
    ap_status_t stat;

    if (on)
        one = 1;
    else
        one = 0;

    if (opt & APR_SO_TIMEOUT) {
        int timeout = on * 1000;  /* Windows needs timeout in mSeconds */
        sock->timeout = timeout;
        if (setsockopt(sock->sock, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, 
                       sizeof(timeout)) == SOCKET_ERROR) {
            return WSAGetLastError();
        }
    }
    if (opt & APR_SO_KEEPALIVE) {
        if (setsockopt(sock->sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int)) == -1) {
            return WSAGetLastError();
        }
    }
    if (opt & APR_SO_DEBUG) {
        if (setsockopt(sock->sock, SOL_SOCKET, SO_DEBUG, (void *)&one, sizeof(int)) == -1) {
            return WSAGetLastError();
        }
    }
    if (opt & APR_SO_REUSEADDR) {
        if (setsockopt(sock->sock, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int)) == -1) {
            return WSAGetLastError();
        }
    }
    if (opt & APR_SO_NONBLOCK) {
        if (on) {
            if ((stat = soblock(sock->sock)) != APR_SUCCESS) 
                return stat;
        }
        else {
            if ((stat = sononblock(sock->sock)) != APR_SUCCESS)
                return stat;
        }
    }
    if (opt & APR_SO_LINGER) {
        li.l_onoff = on;
        li.l_linger = MAX_SECS_TO_LINGER;
        if (setsockopt(sock->sock, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger)) == -1) {
            return APR_EEXIST;
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_gethostname(char *buf, int len, ap_context_t *cont)
{
    if (gethostname(buf, len) == -1)
        return APR_EEXIST;
    else
        return APR_SUCCESS;
}

ap_status_t ap_get_remote_hostname(char **name, struct socket_t *sock)
{
    struct hostent *hptr;

    hptr = gethostbyaddr((char *)&(sock->local_addr->sin_addr),
                         sizeof(struct in_addr), AF_INET);

    if (hptr != NULL) {
        *name = ap_pstrdup(sock->cntxt, hptr->h_name);
        if (*name) {
            return APR_SUCCESS;
        }
        return APR_ENOMEM;
    }

    return status_from_res_error(WSAGetLastError());
}

ap_status_t status_from_res_error(int reserr)
{
    ap_status_t status;
    switch(reserr) {
    case WSAHOST_NOT_FOUND:
        status = APR_EHOSTNOTFOUND;
        break;
    case WSATRY_AGAIN:
        status = APR_EAGAIN;
        break;
    case WSANO_RECOVERY:
        status = APR_ENORECOVERY;
        break;
    case WSANO_DATA:
        status = APR_ENODATA;
        break;
    default:
        status = APR_ENORECOVERY;
    }
    return status;
}


