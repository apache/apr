/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "apr_arch_networkio.h"
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include <string.h>

apr_status_t soblock(SOCKET sd)
{
    u_long zero = 0;

    if (ioctlsocket(sd, FIONBIO, &zero) == SOCKET_ERROR) {
        return apr_get_netos_error();
    }
    return APR_SUCCESS;
}

apr_status_t sononblock(SOCKET sd)
{
    u_long one = 1;

    if (ioctlsocket(sd, FIONBIO, &one) == SOCKET_ERROR) {
        return apr_get_netos_error();
    }
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_socket_timeout_set(apr_socket_t *sock, apr_interval_time_t t)
{
    apr_status_t stat;

    if (t == 0) {
        /* Set the socket non-blocking if it was previously blocking */
        if (sock->timeout != 0) {
            if ((stat = sononblock(sock->socketdes)) != APR_SUCCESS)
                return stat;
        }
    }
    else if (t > 0) {
        /* Set the socket to blocking if it was previously non-blocking */
        if (sock->timeout == 0) {
            if ((stat = soblock(sock->socketdes)) != APR_SUCCESS)
                return stat;
        }
        /* Reset socket timeouts if the new timeout differs from the old timeout */
        if (sock->timeout != t) 
        {
            /* Win32 timeouts are in msec, represented as int */
            sock->timeout_ms = (int)apr_time_as_msec(t);
            setsockopt(sock->socketdes, SOL_SOCKET, SO_RCVTIMEO, 
                       (char *) &sock->timeout_ms, 
                       sizeof(sock->timeout_ms));
            setsockopt(sock->socketdes, SOL_SOCKET, SO_SNDTIMEO, 
                       (char *) &sock->timeout_ms, 
                       sizeof(sock->timeout_ms));
        }
    }
    else if (t < 0) {
        int zero = 0;
        /* Set the socket to blocking with infinite timeouts */
        if ((stat = soblock(sock->socketdes)) != APR_SUCCESS)
            return stat;
        setsockopt(sock->socketdes, SOL_SOCKET, SO_RCVTIMEO, 
                   (char *) &zero, sizeof(zero));
        setsockopt(sock->socketdes, SOL_SOCKET, SO_SNDTIMEO, 
                   (char *) &zero, sizeof(zero));
    }
    sock->timeout = t;
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_socket_opt_set(apr_socket_t *sock,
                                             apr_int32_t opt, apr_int32_t on)
{
    int one;
    apr_status_t stat;

    one = on ? 1 : 0;

    switch (opt) {
    case APR_SO_TIMEOUT: 
    {
        /* XXX: To be deprecated */
        return apr_socket_timeout_set(sock, on);
    }
    case APR_SO_KEEPALIVE:
        if (on != apr_is_option_set(sock->netmask, APR_SO_KEEPALIVE)) {
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_KEEPALIVE, 
                           (void *)&one, sizeof(int)) == -1) {
                return apr_get_netos_error();
            }
            apr_set_option(&sock->netmask,APR_SO_KEEPALIVE, on);
        }
        break;
    case APR_SO_DEBUG:
        if (on != apr_is_option_set(sock->netmask, APR_SO_DEBUG)) {
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_DEBUG, 
                           (void *)&one, sizeof(int)) == -1) {
                return apr_get_netos_error();
            }
            apr_set_option(&sock->netmask, APR_SO_DEBUG, on);
        }
        break;
    case APR_SO_REUSEADDR:
        if (on != apr_is_option_set(sock->netmask, APR_SO_REUSEADDR)) {
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_REUSEADDR, 
                           (void *)&one, sizeof(int)) == -1) {
                return apr_get_netos_error();
            }
            apr_set_option(&sock->netmask, APR_SO_REUSEADDR, on);
        }
        break;
    case APR_SO_NONBLOCK:
        if (apr_is_option_set(sock->netmask, APR_SO_NONBLOCK) != on) {
            if (on) {
                if ((stat = sononblock(sock->socketdes)) != APR_SUCCESS) 
                    return stat;
            }
            else {
                if ((stat = soblock(sock->socketdes)) != APR_SUCCESS)
                    return stat;
            }
            apr_set_option(&sock->netmask, APR_SO_NONBLOCK, on);
        }
        break;
    case APR_SO_LINGER:
    {
        if (apr_is_option_set(sock->netmask, APR_SO_LINGER) != on) {
            struct linger li;
            li.l_onoff = on;
            li.l_linger = APR_MAX_SECS_TO_LINGER;
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_LINGER, 
                           (char *) &li, sizeof(struct linger)) == -1) {
                return apr_get_netos_error();
            }
            apr_set_option(&sock->netmask, APR_SO_LINGER, on);
        }
        break;
    }
    case APR_TCP_NODELAY:
        if (apr_is_option_set(sock->netmask, APR_TCP_NODELAY) != on) {
            int optlevel = IPPROTO_TCP;
            int optname = TCP_NODELAY;

#if APR_HAVE_SCTP
            if (sock->protocol == IPPROTO_SCTP) {
                optlevel = IPPROTO_SCTP;
                optname = SCTP_NODELAY;
            }
#endif
            if (setsockopt(sock->socketdes, optlevel, optname,
                           (void *)&on, sizeof(int)) == -1) {
                return apr_get_netos_error();
            }
            apr_set_option(&sock->netmask, APR_TCP_NODELAY, on);
        }
        break;
    case APR_IPV6_V6ONLY:
#if APR_HAVE_IPV6 && defined(IPV6_V6ONLY)
        /* we don't know the initial setting of this option,
         * so don't check/set sock->netmask since that optimization
         * won't work
         */
        if (setsockopt(sock->socketdes, IPPROTO_IPV6, IPV6_V6ONLY,
                       (void *)&on, sizeof(int)) == -1) {
            return apr_get_netos_error();
        }
#else
        return APR_ENOTIMPL;
#endif
        break;
    default:
        return APR_EINVAL;
        break;
    }
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_socket_timeout_get(apr_socket_t *sock, apr_interval_time_t *t)
{
    *t = sock->timeout;
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_socket_opt_get(apr_socket_t *sock,
                                             apr_int32_t opt, apr_int32_t *on)
{
    switch (opt) {
    case APR_SO_TIMEOUT: 
        /* XXX: to be deprecated */
        *on = (apr_int32_t)sock->timeout;
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
        *on = apr_is_option_set(sock->netmask, opt);
    }
    return APR_SUCCESS;
}


/* deprecated */
APR_DECLARE(apr_status_t) apr_setsocketopt(apr_socket_t *sock,
                                           apr_int32_t opt, apr_int32_t on)
{
    return apr_socket_opt_set(sock, opt, on);
}

APR_DECLARE(apr_status_t) apr_getsocketopt(apr_socket_t *sock,
                                           apr_int32_t opt, apr_int32_t *on)
{
    return apr_socket_opt_get(sock, opt, on);
}
                                           

APR_DECLARE(apr_status_t) apr_gethostname(char *buf, int len,
                                          apr_pool_t *cont)
{
    if (gethostname(buf, len) == -1) {
        buf[0] = '\0';
        return apr_get_netos_error();
    }
    else if (!memchr(buf, '\0', len)) { /* buffer too small */
        buf[0] = '\0';
        return APR_ENAMETOOLONG;
    }
    return APR_SUCCESS;
}

