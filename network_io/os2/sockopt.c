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
#include "apr_lib.h"
#include "apr_strings.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/so_ioctl.h>


APR_DECLARE(apr_status_t) apr_socket_timeout_set(apr_socket_t *sock, 
                                                 apr_interval_time_t t)
{
    sock->timeout = t;
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_socket_opt_set(apr_socket_t *sock, 
                                             apr_int32_t opt, apr_int32_t on)
{
    int one;
    struct linger li;

    if (on)
        one = 1;
    else
        one = 0;

    if (opt & APR_SO_KEEPALIVE) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int)) == -1) {
            return APR_OS2_STATUS(sock_errno());
        }
    }
    if (opt & APR_SO_DEBUG) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_DEBUG, (void *)&one, sizeof(int)) == -1) {
            return APR_OS2_STATUS(sock_errno());
        }
    }
    if (opt & APR_SO_REUSEADDR) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int)) == -1) {
            return APR_OS2_STATUS(sock_errno());
        }
    }
    if (opt & APR_SO_SNDBUF) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_SNDBUF, (void *)&on, sizeof(int)) == -1) {
            return APR_OS2_STATUS(sock_errno());
        }
    }
    if (opt & APR_SO_NONBLOCK) {
        if (ioctl(sock->socketdes, FIONBIO, (caddr_t)&one, sizeof(one)) == -1) {
            return APR_OS2_STATUS(sock_errno());
        } else {
            sock->nonblock = one;
        }
    }
    if (opt & APR_SO_LINGER) {
        li.l_onoff = on;
        li.l_linger = APR_MAX_SECS_TO_LINGER;
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger)) == -1) {
            return APR_OS2_STATUS(sock_errno());
        }
    }
    if (opt & APR_SO_TIMEOUT) {
        /* XXX: To be deprecated */
        return apr_socket_timeout_set(sock, on);
    }
    if (opt & APR_TCP_NODELAY) {
        if (setsockopt(sock->socketdes, IPPROTO_TCP, TCP_NODELAY, (void *)&on, sizeof(int)) == -1) {
            return APR_OS2_STATUS(sock_errno());
        }
    }
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_socket_timeout_get(apr_socket_t *sock, 
                                                 apr_interval_time_t *t)
{
    *t = sock->timeout;
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_socket_opt_get(apr_socket_t *sock, 
                                             apr_int32_t opt, apr_int32_t *on)
{
    switch(opt) {
    case APR_SO_TIMEOUT:
        /* XXX: To be deprecated */
        *on = (apr_int32_t)sock->timeout;
        break;
    default:
        return APR_EINVAL;
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
                                           

APR_DECLARE(apr_status_t) apr_gethostname(char *buf, apr_int32_t len, 
                                          apr_pool_t *cont)
{
    if (gethostname(buf, len) == -1) {
        buf[0] = '\0';
        return APR_OS2_STATUS(sock_errno());
    }
    else if (!memchr(buf, '\0', len)) { /* buffer too small */
        buf[0] = '\0';
        return APR_ENAMETOOLONG;
    }
    return APR_SUCCESS;
}
