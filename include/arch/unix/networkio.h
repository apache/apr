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

#ifndef NETWORK_IO_H
#define NETWORK_IO_H

#include "apr.h"
#include "apr_private.h"
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"

/* System headers the network I/O library needs */
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if APR_HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if APR_HAVE_ERRNO_H
#include <errno.h>
#endif
#if APR_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if APR_HAVE_NETINET_SCTP_UIO_H
#include <netinet/sctp_uio.h>
#endif
#if APR_HAVE_NETINET_SCTP_H
#include <netinet/sctp.h>
#endif
#if APR_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if APR_HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if APR_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if APR_HAVE_NETDB_H
#include <netdb.h>
#endif
#if APR_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if APR_HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif
/* End System Headers */

#ifndef HAVE_POLLIN
#define POLLIN   1
#define POLLPRI  2
#define POLLOUT  4
#define POLLERR  8
#define POLLHUP  16
#define POLLNVAL 32
#endif

struct apr_socket_t {
    apr_pool_t *cntxt;
    int socketdes;
    int type;
    int protocol;
    apr_sockaddr_t *local_addr;
    apr_sockaddr_t *remote_addr;
    apr_interval_time_t timeout; 
#ifndef HAVE_POLL
    int connected;
#endif
    int local_port_unknown;
    int local_interface_unknown;
    int remote_addr_unknown;
    apr_int32_t netmask;
    apr_int32_t inherit;
};

const char *apr_inet_ntop(int af, const void *src, char *dst, apr_size_t size);
int apr_inet_pton(int af, const char *src, void *dst);
void apr_sockaddr_vars_set(apr_sockaddr_t *, int, apr_port_t);

#define apr_is_option_set(mask, option)  ((mask & option) ==option)

#define apr_set_option(mask, option, on) \
    do {                                 \
        if (on)                          \
            *mask |= option;             \
        else                             \
            *mask &= ~option;            \
    } while (0)

#endif  /* ! NETWORK_IO_H */

