/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#ifndef APR_NETWORK_IO_H
#define APR_NETWORK_IO_H

#ifdef WIN32
#include <winsock2.h>
#endif

#include "apr_general.h"
#include "apr_file_io.h"
#include "apr_errno.h"
#if APR_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MAX_SECS_TO_LINGER
#define MAX_SECS_TO_LINGER 30
#endif

#ifndef APRMAXHOSTLEN
#define APRMAXHOSTLEN 256
#endif

#ifndef APR_ANYADDR
#define APR_ANYADDR "0.0.0.0"
#endif

/* Socket option definitions */
#define APR_SO_LINGER        1
#define APR_SO_KEEPALIVE     2
#define APR_SO_DEBUG         4
#define APR_SO_NONBLOCK      8
#define APR_SO_REUSEADDR     16
#define APR_SO_TIMEOUT       32
#define APR_SO_SNDBUF        64

#define APR_POLLIN    0x001 
#define APR_POLLPRI   0x002
#define APR_POLLOUT   0x004
#define APR_POLLERR   0x010
#define APR_POLLHUP   0x020
#define APR_POLLNVAL  0x040

typedef enum {APR_SHUTDOWN_READ, APR_SHUTDOWN_WRITE, 
	      APR_SHUTDOWN_READWRITE} ap_shutdown_how_e;

/* We need to make sure we always have an in_addr type, so APR will just
 * define it ourselves, if the platform doesn't provide it.
 */
#if !defined(APR_HAVE_IN_ADDR)
struct in_addr {
    ap_uint32_t  s_addr;
}
#endif

/* I guess not everybody uses inet_addr.  This defines ap_inet_addr
 * appropriately.
 */

#if APR_HAVE_INET_ADDR
#define ap_inet_addr    inet_addr
#elif APR_HAVE_INET_NETWORK        /* only DGUX, as far as I know */
#define ap_inet_addr    inet_network
#endif

typedef struct socket_t     ap_socket_t;
typedef struct pollfd_t     ap_pollfd_t;
typedef struct hdtr_t       ap_hdtr_t;
typedef struct in_addr      ap_in_addr;

#if APR_HAS_SENDFILE
/* A structure to encapsulate headers and trailers for ap_sendfile */
struct hdtr_t {
    struct iovec* headers;
    int numheaders;
    struct iovec* trailers;
    int numtrailers;
};
#endif

/* function definitions */

ap_status_t ap_create_tcp_socket(ap_socket_t **new, ap_context_t *cont);
ap_status_t ap_shutdown(ap_socket_t *ithesocket, ap_shutdown_how_e how);
ap_status_t ap_close_socket(ap_socket_t *thesocket);

ap_status_t ap_bind(ap_socket_t *sock);
ap_status_t ap_listen(ap_socket_t *sock, ap_int32_t backlog);
ap_status_t ap_accept(ap_socket_t **new, const ap_socket_t *sock, 
                      ap_context_t *connection_context);
ap_status_t ap_connect(ap_socket_t *sock, char *hostname);

ap_status_t ap_get_remote_hostname(char **name, ap_socket_t *sock);
ap_status_t ap_gethostname(char *buf, int len, ap_context_t *cont);
ap_status_t ap_get_socketdata(void **data, char *key, ap_socket_t *sock);
ap_status_t ap_set_socketdata(ap_socket_t *sock, void *data, char *key,
                              ap_status_t (*cleanup) (void*));

ap_status_t ap_send(ap_socket_t *sock, const char *buf, ap_ssize_t *len);
ap_status_t ap_sendv(ap_socket_t *sock, const struct iovec *vec, 
                     ap_int32_t nvec, ap_int32_t *len);
#if APR_HAS_SENDFILE
ap_status_t ap_sendfile(ap_socket_t *sock, ap_file_t *file, ap_hdtr_t *hdtr, 
                        ap_off_t *offset, ap_size_t *len, ap_int32_t flags);
#endif
ap_status_t ap_recv(ap_socket_t *sock, char *buf, ap_ssize_t *len);

ap_status_t ap_setsocketopt(ap_socket_t *sock, ap_int32_t opt, ap_int32_t on);

ap_status_t ap_set_local_port(ap_socket_t *sock, ap_uint32_t port);
ap_status_t ap_set_remote_port(ap_socket_t *sock, ap_uint32_t port);
ap_status_t ap_get_local_port(ap_uint32_t *port, ap_socket_t *sock);
ap_status_t ap_get_remote_port(ap_uint32_t *port, ap_socket_t *sock);
ap_status_t ap_set_local_ipaddr(ap_socket_t *sock, const char *addr);
ap_status_t ap_set_remote_ipaddr(ap_socket_t *sock, const char *addr);
ap_status_t ap_get_local_ipaddr(char **addr, const ap_socket_t *sock);
ap_status_t ap_get_remote_ipaddr(char **addr, const ap_socket_t *sock);

ap_status_t ap_get_local_name(struct sockaddr_in **name, const ap_socket_t *sock);
ap_status_t ap_get_remote_name(struct sockaddr_in **name, const ap_socket_t *sock);

ap_status_t ap_setup_poll(ap_pollfd_t **new, ap_int32_t num, 
                          ap_context_t *cont);
ap_status_t ap_poll(ap_pollfd_t *aprset, ap_int32_t *nsds, ap_int32_t timeout);
ap_status_t ap_add_poll_socket(ap_pollfd_t *aprset, ap_socket_t *socket, 
                               ap_int16_t event);
ap_status_t ap_remove_poll_socket(ap_pollfd_t *aprset, ap_socket_t *sock,
                                  ap_int16_t events);
ap_status_t ap_clear_poll_sockets(ap_pollfd_t *aprset, ap_int16_t events);
ap_status_t ap_get_revents(ap_int16_t *event, ap_socket_t *sock, 
                           ap_pollfd_t *aprset);
ap_status_t ap_get_polldata(ap_pollfd_t *pollfd, char *key, void *data);
ap_status_t ap_set_polldata(ap_pollfd_t *pollfd, void *data, char *key,
                            ap_status_t (*cleanup) (void *));

/*  accessor functions   */

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_NETWORK_IO_H */

