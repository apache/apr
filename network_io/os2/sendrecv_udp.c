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
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_support.h"
#include "apr_lib.h"
#include <sys/time.h>


APR_DECLARE(apr_status_t) apr_socket_sendto(apr_socket_t *sock, 
                                            apr_sockaddr_t *where,
                                            apr_int32_t flags, const char *buf,
                                            apr_size_t *len)
{
    apr_ssize_t rv;
    int serrno;

    do {
        rv = sendto(sock->socketdes, buf, (*len), flags, 
                    (struct sockaddr*)&where->sa,
                    where->salen);
    } while (rv == -1 && (serrno = sock_errno()) == EINTR);

    if (rv == -1 && serrno == SOCEWOULDBLOCK && sock->timeout != 0) {
        apr_status_t arv = apr_wait_for_io_or_timeout(NULL, sock, 0);

        if (arv != APR_SUCCESS) {
            *len = 0;
            return arv;
        } else {
            do {
                rv = sendto(sock->socketdes, buf, *len, flags,
                            (const struct sockaddr*)&where->sa,
                            where->salen);
            } while (rv == -1 && (serrno = sock_errno()) == SOCEINTR);
        }
    }

    if (rv == -1) {
        *len = 0;
        return APR_FROM_OS_ERROR(serrno);
    }

    *len = rv;
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_socket_recvfrom(apr_sockaddr_t *from,
                                              apr_socket_t *sock,
                                              apr_int32_t flags, char *buf,
                                              apr_size_t *len)
{
    apr_ssize_t rv;
    int serrno;

    do {
        rv = recvfrom(sock->socketdes, buf, (*len), flags, 
                      (struct sockaddr*)&from->sa, &from->salen);
    } while (rv == -1 && (serrno = sock_errno()) == EINTR);

    if (rv == -1 && serrno == SOCEWOULDBLOCK && sock->timeout != 0) {
        apr_status_t arv = apr_wait_for_io_or_timeout(NULL, sock, 1);

        if (arv != APR_SUCCESS) {
            *len = 0;
            return arv;
        } else {
            do {
                rv = recvfrom(sock->socketdes, buf, *len, flags,
                              (struct sockaddr*)&from->sa, &from->salen);
                } while (rv == -1 && (serrno = sock_errno()) == EINTR);
        }
    }

    if (rv == -1) {
        (*len) = 0;
        return APR_FROM_OS_ERROR(serrno);
    }

    (*len) = rv;

    if (rv == 0 && sock->type == SOCK_STREAM)
        return APR_EOF;

    return APR_SUCCESS;
}

/* deprecated */
APR_DECLARE(apr_status_t) apr_sendto(apr_socket_t *sock, apr_sockaddr_t *where,
                                     apr_int32_t flags, const char *buf,
                                     apr_size_t *len)
{
    return apr_socket_sendto(sock, where, flags, buf, len);
}



APR_DECLARE(apr_status_t) apr_recvfrom(apr_sockaddr_t *from,
                                       apr_socket_t *sock,
                                       apr_int32_t flags, char *buf,
                                       apr_size_t *len)
{
    return apr_socket_recvfrom(from, sock, flags, buf, len);
}
