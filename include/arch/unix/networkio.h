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
#if HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#if HAVE_POLL_H
#include <poll.h>
#endif
#if APR_HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if APR_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SYS_SENDFILE_H
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
    apr_sockaddr_t *local_addr;
    apr_sockaddr_t *remote_addr;
    apr_interval_time_t timeout; 
#ifndef HAVE_POLL
    int connected;
#endif
    int local_port_unknown;
    int local_interface_unknown;
};

struct apr_pollfd_t {
    apr_pool_t *cntxt;
#ifdef HAVE_POLL
    struct pollfd *pollset;
    int num;
    int curpos;
#else
    fd_set *read;
    fd_set *write;
    fd_set *except;
    int highsock;
#endif
    apr_int16_t *events;
    apr_int16_t *revents;

};

#endif  /* ! NETWORK_IO_H */

