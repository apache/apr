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
#include "apr_lib.h"
#include <sys/time.h>

APR_DECLARE(apr_status_t) apr_socket_send(apr_socket_t *sock, const char *buf,
                                          apr_size_t *len)
{
    apr_ssize_t rv;
    int fds, err = 0;

    do {
        if (!sock->nonblock || err == SOCEWOULDBLOCK) {
            fds = sock->socketdes;
            rv = select(&fds, 0, 1, 0, sock->timeout >= 0 ? sock->timeout/1000 : -1);

            if (rv != 1) {
                *len = 0;
                err = sock_errno();

                if (rv == 0)
                    return APR_TIMEUP;

                if (err == SOCEINTR)
                    continue;

                return APR_OS2_STATUS(err);
            }
        }

        rv = send(sock->socketdes, buf, (*len), 0);
        err = rv < 0 ? sock_errno() : 0;
    } while (err == SOCEINTR || err == SOCEWOULDBLOCK);

    if (err) {
        *len = 0;
        return APR_OS2_STATUS(err);
    }

    (*len) = rv;
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_socket_recv(apr_socket_t *sock, char *buf,
                                          apr_size_t *len)
{
    apr_ssize_t rv;
    int fds, err = 0;

    do {
        if (!sock->nonblock || (err == SOCEWOULDBLOCK && sock->timeout != 0)) {
            fds = sock->socketdes;
            rv = select(&fds, 1, 0, 0, sock->timeout >= 0 ? sock->timeout/1000 : -1);

            if (rv != 1) {
                *len = 0;
                err = sock_errno();

                if (rv == 0)
                    return APR_TIMEUP;

                if (err == SOCEINTR)
                    continue;

                return APR_OS2_STATUS(err);
            }
        }

        rv = recv(sock->socketdes, buf, (*len), 0);
        err = rv < 0 ? sock_errno() : 0;
    } while (err == SOCEINTR || (err == SOCEWOULDBLOCK && sock->timeout != 0));

    if (err) {
        *len = 0;
        return APR_OS2_STATUS(err);
    }

    (*len) = rv;
    return rv == 0 ? APR_EOF : APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_socket_sendv(apr_socket_t *sock, 
                                           const struct iovec *vec, 
                                           apr_int32_t nvec, apr_size_t *len)
{
    apr_status_t rv;
    struct iovec *tmpvec;
    int fds, err = 0;
    int nv_tosend, total = 0;

    /* Make sure writev() only gets fed 64k at a time */
    for ( nv_tosend = 0; nv_tosend < nvec && total + vec[nv_tosend].iov_len < 65536; nv_tosend++ ) {
        total += vec[nv_tosend].iov_len;
    }

    tmpvec = alloca(sizeof(struct iovec) * nv_tosend);
    memcpy(tmpvec, vec, sizeof(struct iovec) * nv_tosend);

    do {
        if (!sock->nonblock || err == SOCEWOULDBLOCK) {
            fds = sock->socketdes;
            rv = select(&fds, 0, 1, 0, sock->timeout >= 0 ? sock->timeout/1000 : -1);

            if (rv != 1) {
                *len = 0;
                err = sock_errno();

                if (rv == 0)
                    return APR_TIMEUP;

                if (err == SOCEINTR)
                    continue;

                return APR_OS2_STATUS(err);
            }
        }

        rv = writev(sock->socketdes, tmpvec, nv_tosend);
        err = rv < 0 ? sock_errno() : 0;
    } while (err == SOCEINTR || err == SOCEWOULDBLOCK);

    if (err) {
        *len = 0;
        return APR_OS2_STATUS(err);
    }

    *len = rv;
    return APR_SUCCESS;
}

/* deprecated */
APR_DECLARE(apr_status_t) apr_send(apr_socket_t *sock, const char *buf,
                                   apr_size_t *len)
{
    return apr_socket_send(sock, buf, len);
}

/* deprecated */
APR_DECLARE(apr_status_t) apr_sendv(apr_socket_t *sock, 
                                    const struct iovec *vec, 
                                    apr_int32_t nvec, apr_size_t *len)
{
    return apr_socket_sendv(sock, vec, nvec, len);
}

/* deprecated */
APR_DECLARE(apr_status_t) apr_recv(apr_socket_t *sock, char *buf,
                                   apr_size_t *len)
{
    return apr_socket_recv(sock, buf, len);
}
