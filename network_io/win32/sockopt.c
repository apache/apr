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
#include "apr_strings.h"
#include <string.h>

apr_status_t soblock(SOCKET sd)
{
    int zero = 0;

    if (ioctlsocket(sd, FIONBIO, &zero) == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    return APR_SUCCESS;
}

apr_status_t sononblock(SOCKET sd)
{
    int one = 1;

    if (ioctlsocket(sd, FIONBIO, &one) == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    return APR_SUCCESS;
}

apr_status_t apr_setsocketopt(apr_socket_t *sock, apr_int32_t opt, apr_int32_t on)
{
    int one;
    apr_status_t stat;

    one = on ? 1 : 0;

    switch (opt) {
    case APR_SO_TIMEOUT: 
    {
        int new_timeout;
        if (on <= 0)
            new_timeout = on;
        else
            /* Convert from APR units (microseconds) to windows units 
             * (milliseconds) */
            new_timeout = on/1000;

        if (new_timeout == 0) {
            /* Set the socket non-blocking if it was previously blocking */
            if (sock->timeout != 0) {
                if ((stat = sononblock(sock->sock)) != APR_SUCCESS)
                    return stat;
            }
        }
        else if (new_timeout > 0) {
            /* Set the socket to blocking if it was previously non-blocking */
            if (sock->timeout == 0) {
                if ((stat = soblock(sock->sock)) != APR_SUCCESS)
                    return stat;
            }
            /* Reset socket timeouts if the new timeout differs from the old timeout */
            if (sock->timeout != new_timeout) {
                setsockopt(sock->sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &new_timeout, sizeof(new_timeout));
                setsockopt(sock->sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &new_timeout, sizeof(new_timeout));
            }
        }
        else if (new_timeout < 0) {
            int zero = 0;
            /* Set the socket to blocking with infinite timeouts */
            if ((stat = soblock(sock->sock)) != APR_SUCCESS)
                return stat;
            setsockopt(sock->sock, SOL_SOCKET, SO_RCVTIMEO, (char *) &zero, sizeof(zero));
            setsockopt(sock->sock, SOL_SOCKET, SO_SNDTIMEO, (char *) &zero, sizeof(zero));
        }
        sock->timeout = new_timeout;
        break;
    }
    case APR_SO_KEEPALIVE:
        if (setsockopt(sock->sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int)) == -1) {
            return WSAGetLastError();
        }
        break;
    case APR_SO_DEBUG:
        if (setsockopt(sock->sock, SOL_SOCKET, SO_DEBUG, (void *)&one, sizeof(int)) == -1) {
            return WSAGetLastError();
        }
        break;
    case APR_SO_REUSEADDR:
        if (setsockopt(sock->sock, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int)) == -1) {
            return WSAGetLastError();
        }
        break;
    case APR_SO_NONBLOCK:
        if (on) {
            if ((stat = sononblock(sock->sock)) != APR_SUCCESS) 
                return stat;
        }
        else {
            if ((stat = soblock(sock->sock)) != APR_SUCCESS)
                return stat;
        }
        break;
    case APR_SO_LINGER:
    {
        struct linger li;
        li.l_onoff = on;
        li.l_linger = MAX_SECS_TO_LINGER;
        if (setsockopt(sock->sock, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger)) == -1) {
            return WSAGetLastError();
        }
        break;
    }
    default:
        return APR_EINVAL;
        break;
    }
    return APR_SUCCESS;
}

apr_status_t apr_getsocketopt(apr_socket_t *sock, apr_int32_t opt, apr_int32_t *on)
{
    switch (opt) {
    case APR_SO_TIMEOUT: 
        /* Convert from milliseconds (windows units) to microseconds 
         * (APR units) */
        *on = sock->timeout * 1000;
        break;
    case APR_SO_DISCONNECTED:
        *on = sock->disconnected;
        break;
    case APR_SO_KEEPALIVE:
    case APR_SO_DEBUG:
    case APR_SO_REUSEADDR:
    case APR_SO_NONBLOCK:
    case APR_SO_LINGER:
    default:
        return APR_ENOTIMPL;
        break;
    }
    return APR_SUCCESS;
}


apr_status_t apr_gethostname(char *buf, int len, apr_pool_t *cont)
{
    if (gethostname(buf, len) == -1)
        return WSAGetLastError();
    else
        return APR_SUCCESS;
}

apr_status_t apr_get_remote_hostname(char **name, apr_socket_t *sock)
{
    struct hostent *hptr;

    hptr = gethostbyaddr((char *)&(sock->local_addr->sin_addr),
                         sizeof(struct in_addr), AF_INET);

    if (hptr != NULL) {
        *name = apr_pstrdup(sock->cntxt, hptr->h_name);
        if (*name) {
            return APR_SUCCESS;
        }
        return APR_ENOMEM;
    }

    return WSAGetLastError();
}


