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

typedef struct ap_socket_t     ap_socket_t;
typedef struct ap_pollfd_t     ap_pollfd_t;
typedef struct ap_hdtr_t       ap_hdtr_t;
typedef struct in_addr      ap_in_addr;

#if APR_HAS_SENDFILE
/* A structure to encapsulate headers and trailers for ap_sendfile */
struct ap_hdtr_t {
    struct iovec* headers;
    int numheaders;
    struct iovec* trailers;
    int numtrailers;
};
#endif

/* function definitions */

/* ***APRDOC********************************************************
 * ap_status_t ap_create_tcp_socket(ap_socket_t **new, ap_context_t *cont)
 *    Create a socket for tcp communication.
 * arg 1) The new socket that has been setup. 
 * arg 2) The context to use
 */
ap_status_t ap_create_tcp_socket(ap_socket_t **new, ap_pool_t *cont);

/* ***APRDOC********************************************************
 * ap_status_t ap_shutdown(ap_socket_t *thesocket, ap_shutdown_how_e how)
 *    Shutdown either reading, writing, or both sides of a tcp socket.
 * arg 1) The socket to close 
 * arg 2) How to shutdown the socket.  One of:
 *            APR_SHUTDOWN_READ      -- no longer allow read requests
 *            APR_SHUTDOWN_WRITE     -- no longer allow write requests
 *            APR_SHUTDOWN_READWRITE -- no longer allow read or write requests 
 * NOTE:  This does not actually close the socket descriptor, it just
 *        controls which calls are still valid on the socket.
 */
ap_status_t ap_shutdown(ap_socket_t *ithesocket, ap_shutdown_how_e how);

/* ***APRDOC********************************************************
 * ap_status_t ap_close_socket(ap_socket_t *thesocket)
 *    Close a tcp socket.
 * arg 1) The socket to close 
 */
ap_status_t ap_close_socket(ap_socket_t *thesocket);

/* ***APRDOC********************************************************
 * ap_status_t ap_bind(ap_socket_t *sock)
 *    Bind the socket to it's assocaited port
 * arg 1) The socket to bind 
 * NOTE:  This is where we will find out if there is any other process
 *        using the selected port.
 */
ap_status_t ap_bind(ap_socket_t *sock);

/* ***APRDOC********************************************************
 * ap_status_t ap_listen(ap_socket_t *sock, ap_int32_t backlog)
 *    Listen to a bound socketi for connections. 
 * arg 1) The socket to listen on 
 * arg 2) The number of outstanding connections allowed in the sockets
 *        listen queue.  If this value is less than zero, the listen
 *        queue size is set to zero.  
 */
ap_status_t ap_listen(ap_socket_t *sock, ap_int32_t backlog);

/* ***APRDOC********************************************************
 * ap_status_t ap_accept(ap_socket_t **new, ap_socket_t *sock, 
                         ap_context_t *connection_context)
 *    Accept a new connection request
 * arg 1) A copy of the socket that is connected to the socket that
 *        made the connection request.  This is the socket which should
 *        be used for all future communication.
 * arg 2) The socket we are listening on.
 * arg 3) The context for the new socket.
 */
ap_status_t ap_accept(ap_socket_t **new, const ap_socket_t *sock, 
                      ap_pool_t *connection_context);

/* ***APRDOC********************************************************
 * ap_status_t ap_connect(ap_socket_t *sock, char *hostname)
 *    Issue a connection request to a socket either on the same machine
 *    or a different one. 
 * arg 1) The socket we wish to use for our side of the connection 
 * arg 2) The hostname of the machine we wish to connect to.  If NULL,
 *        APR assumes that the sockaddr_in in the apr_socket is completely
 *        filled out.
 */
ap_status_t ap_connect(ap_socket_t *sock, char *hostname);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_remote_hostname(char **name, ap_socket_t *sock)
 *    Get name of the machine we are currently connected to. 
 * arg 1) A buffer to store the hostname in.
 * arg 2) The socket to examine.
 */
ap_status_t ap_get_remote_hostname(char **name, ap_socket_t *sock);

/* ***APRDOC********************************************************
 * ap_status_t ap_gethostname(char *buf, ap_int32_t len, ap_context_t *cont)
 *    Get name of the current machine 
 * arg 1) A buffer to store the hostname in.
 * arg 2) The maximum length of the hostname that can be stored in the
 *        buffer provided. 
 * arg 3) The context to use.
 */
ap_status_t ap_gethostname(char *buf, int len, ap_pool_t *cont);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_socketdata(void **data, char *key, ap_socket_t *sock)
 *    Return the context associated with the current socket.
 * arg 1) The currently open socket.
 * arg 2) The user data associated with the socket.
 */
ap_status_t ap_get_socketdata(void **data, char *key, ap_socket_t *sock);

/* ***APRDOC********************************************************
 * ap_status_t ap_set_socketdata(ap_socket_t *sock, void *data, char *key,
                                 ap_status_t (*cleanup) (void *))
 *    Set the context associated with the current socket.
 * arg 1) The currently open socket.
 * arg 2) The user data to associate with the socket.
 * arg 3) The key to associate with the data.
 * arg 4) The cleanup to call when the socket is destroyed.
 */
ap_status_t ap_set_socketdata(ap_socket_t *sock, void *data, char *key,
                              ap_status_t (*cleanup) (void*));

/* ***APRDOC********************************************************
 * ap_status_t ap_send(ap_socket_t *sock, const char *buf, ap_ssize_t *len)
 *    Send data over a network.
 * arg 1) The socket to send the data over.
 * arg 2) The buffer which contains the data to be sent. 
 * arg 3) On entry, the number of bytes to send; on exit, the number
 *        of bytes sent.
 * NOTE:  This functions acts like a blocking write by default.  To change 
 *        this behavior, use ap_setsocketopt with the APR_SO_TIMEOUT option.
 *
 * It is possible for both bytes to be sent and an error to be returned.
 *
 * APR_EINTR is never returned.
 */
ap_status_t ap_send(ap_socket_t *sock, const char *buf, ap_ssize_t *len);

/* ***APRDOC********************************************************
 * ap_status_t ap_sendv(ap_socket_t *sock, const struct iovec *vec, 
                        ap_int32_t nvec, ap_int32_t *len)
 *    Send multiple packets of data over a network.
 * arg 1) The socket to send the data over.
 * arg 2) The array of iovec structs containing the data to send 
 * arg 3) The number of iovec structs in the array
 * arg 4) Receives the number of bytes actually written
 * NOTE:  This functions acts like a blocking write by default.  To change 
 *        this behavior, use ap_setsocketopt with the APR_SO_TIMEOUT option.
 *        The number of bytes actually sent is stored in argument 3.
 *
 * It is possible for both bytes to be sent and an error to be returned.
 *
 * APR_EINTR is never returned.
 */
ap_status_t ap_sendv(ap_socket_t *sock, const struct iovec *vec, 
                     ap_int32_t nvec, ap_int32_t *len);

#if APR_HAS_SENDFILE
/* ***APRDOC********************************************************
 * ap_status_t ap_sendfile(ap_socket_t *sock, ap_file_t *file, ap_hdtr_t *hdtr, 
 *                         ap_off_t *offset, ap_size_t *len, ap_int32_t flags)
 *    Send a file from an open file descriptor to a socket, along with 
 *    optional headers and trailers
 * arg 1) The socket to which we're writing
 * arg 2) The open file from which to read
 * arg 3) A structure containing the headers and trailers to send
 * arg 4) Offset into the file where we should begin writing
 * arg 5) Number of bytes to send 
 * arg 6) OS-specific flags to pass to sendfile()
 * NOTE:  This functions acts like a blocking write by default.  To change 
 *        this behavior, use ap_setsocketopt with the APR_SO_TIMEOUT option.
 *        The number of bytes actually sent is stored in argument 5.
 */
ap_status_t ap_sendfile(ap_socket_t *sock, ap_file_t *file, ap_hdtr_t *hdtr, 
                        ap_off_t *offset, ap_size_t *len, ap_int32_t flags);
#endif

/* ***APRDOC********************************************************
 * ap_status_t ap_recv(ap_socket_t *sock, char *buf, ap_ssize_t *len)
 *    Read data from a network.
 * arg 1) The socket to read the data from.
 * arg 2) The buffer to store the data in. 
 * arg 3) On entry, the number of bytes to receive; on exit, the number
 *        of bytes received.
 * NOTE:  This functions acts like a blocking write by default.  To change 
 *        this behavior, use ap_setsocketopt with the APR_SO_TIMEOUT option.
 *        The number of bytes actually sent is stored in argument 3.
 *
 * It is possible for both bytes to be received and an APR_EOF or
 * other error to be returned.
 *
 * APR_EINTR is never returned.
 */
ap_status_t ap_recv(ap_socket_t *sock, char *buf, ap_ssize_t *len);

/* ***APRDOC********************************************************
 * ap_status_t ap_setsocketopt(ap_socket_t *sock, ap_int32_t opt, ap_int32_t on)
 *    Setup socket options for the specified socket 
 * arg 1) The socket to set up.
 * arg 2) The option we would like to configure.  One of:
 *            APR_SO_DEBUG      --  turn on debugging information 
 *            APR_SO_KEEPALIVE  --  keep connections active
 *            APR_SO_LINGER     --  lingers on close if data is present
 *            APR_SO_NONBLOCK   --  Turns blocking on/off for socket
 *            APR_SO_REUSEADDR  --  The rules used in validating addresses
 *                                  supplied to bind should allow reuse
 *                                  of local addresses.
 *            APR_SO_TIMEOUT    --  Set the timeout value in seconds.
 *                                  values < 0 mean wait forever.  0 means
 *                                  don't wait at all.
 *            APR_SO_SNDBUF     --  Set the SendBufferSize
 * arg 3) Are we turning the option on or off.
 */
ap_status_t ap_setsocketopt(ap_socket_t *sock, ap_int32_t opt, ap_int32_t on);

/* ***APRDOC********************************************************
 * ap_status_t ap_set_local_port(ap_socket_t *sock, ap_uint32_t port)
 *    Assocaite a local port with a socket.
 * arg 1) The socket to set
 * arg 2) The local port this socket will be dealing with.
 * NOTE:  This does not bind the two together, it is just telling apr 
 *        that this socket is going to use this port if possible.  If
 *        the port is already used, we won't find out about it here.
 */
ap_status_t ap_set_local_port(ap_socket_t *sock, ap_uint32_t port);

/* ***APRDOC********************************************************
 * ap_status_t ap_set_remote_port(ap_socket_t *sock, ap_uint32_t port)
 *    Assocaite a remote port with a socket.
 * arg 1) The socket to enquire about.
 * arg 2) The local port this socket will be dealing with.
 * NOTE:  This does not make a connection to the remote port, it is just 
 *        telling apr which port ap_connect() should attempt to connect to.
 */
ap_status_t ap_set_remote_port(ap_socket_t *sock, ap_uint32_t port);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_local_port(ap_uint32_t *port, ap_socket_t *sock)
 *    Return the local port with a socket.
 * arg 1) The local port this socket is associated with.
 * arg 2) The socket to enquire about.
 */
ap_status_t ap_get_local_port(ap_uint32_t *port, ap_socket_t *sock);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_remote_port(ap_uint32_t *port, ap_socket_t *sock)
 *    Return the remote port associated with a socket.
 * arg 1) The remote port this socket is associated with.
 * arg 2) The socket to enquire about.
 */
ap_status_t ap_get_remote_port(ap_uint32_t *port, ap_socket_t *sock);

/* ***APRDOC********************************************************
 * ap_status_t ap_set_local_ipaddr(ap_socket_t *sock, cont char *addr)
 *    Assocaite a local socket addr with an apr socket.
 * arg 1) The socket to use 
 * arg 2) The IP address to attach to the socket.
 *        Use APR_ANYADDR to use any IP addr on the machine.
 * NOTE:  This does not bind the two together, it is just telling apr 
 *        that this socket is going to use this address if possible. 
 */
ap_status_t ap_set_local_ipaddr(ap_socket_t *sock, const char *addr);

/* ***APRDOC********************************************************
 * ap_status_t ap_set_remote_ipaddr(ap_socket_t *sock, cont char *addr)
 *    Assocaite a remote socket addr with an apr socket.
 * arg 1) The socket to use 
 * arg 2) The IP address to attach to the socket.
 * NOTE:  This does not make a connection to the remote address, it is just
 *        telling apr which address ap_connect() should attempt to connect to.
 */
ap_status_t ap_set_remote_ipaddr(ap_socket_t *sock, const char *addr);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_local_ipaddr(char **addr, const ap_socket_t *sock)
 *    Return the local IP address associated with an apr socket.
 * arg 1) The local IP address associated with the socket.
 * arg 2) The socket to use 
 */
ap_status_t ap_get_local_ipaddr(char **addr, const ap_socket_t *sock);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_remote_ipaddr(char **addr, const ap_socket_t *sock)
 *    Return the remote IP address associated with an apr socket.
 * arg 1) The remote IP address associated with the socket.
 * arg 2) The socket to use 
 */
ap_status_t ap_get_remote_ipaddr(char **addr, const ap_socket_t *sock);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_local_name(struct sockaddr_in **name, 
 *                               const ap_socket_t *sock)
 *    Return the local socket name as a BSD style struct sockaddr_in.
 * arg 1) The local name associated with the socket.
 * arg 2) The socket to use 
 */
ap_status_t ap_get_local_name(struct sockaddr_in **name, const ap_socket_t *sock);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_remote_name(struct sockaddr_in **name, 
 *                                const ap_socket_t *sock)
 *    Return the remote socket name as a BSD style struct sockaddr_in.
 * arg 1) The remote name associated with the socket.
 * arg 2) The socket to use 
 */
ap_status_t ap_get_remote_name(struct sockaddr_in **name, const ap_socket_t *sock);

/* ***APRDOC********************************************************
 * ap_status_t ap_setup_poll(ap_pollfd_t **new, ap_int32_t num, 
 *                           ap_context_t *cont)
 *    Setup the memory required for poll to operate properly.
 * arg 1) The poll structure to be used. 
 * arg 2) The number of socket descriptors to be polled.
 * arg 3) The context to operate on.
 */
ap_status_t ap_setup_poll(ap_pollfd_t **new, ap_int32_t num, 
                          ap_pool_t *cont);

/* ***APRDOC********************************************************
 * ap_status_t ap_poll(ap_pollfd_t *aprset, ap_int32_t *nsds,
 *                     ap_interval_time_t timeout)
 *    Poll the sockets in the poll structure.  This is a blocking call,
 *    and it will not return until either a socket has been signalled, or
 *    the timeout has expired. 
 * arg 1) The poll structure we will be using. 
 * arg 2) The number of sockets we are polling. 
 * arg 3) The amount of time in microseconds to wait.  This is a maximum, not 
 *        a minimum.  If a socket is signalled, we will wake up before this
 *        time.  A negative number means wait until a socket is signalled.
 * NOTE:  The number of sockets signalled is returned in the second argument. 
 */
ap_status_t ap_poll(ap_pollfd_t *aprset, ap_int32_t *nsds, 
		    ap_interval_time_t timeout);

/* ***APRDOC********************************************************
 * ap_status_t ap_add_poll_socket(ap_pollfd_t *aprset, ap_socket_t *sock, 
 *                                ap_int16_t event)
 *    Add a socket to the poll structure. 
 * arg 1) The poll structure we will be using. 
 * arg 2) The socket to add to the current poll structure. 
 * arg 3) The events to look for when we do the poll.  One of:
 *            APR_POLLIN    -- signal if read will not block
 *            APR_POLLPRI   -- signal if prioirty data is availble to be read
 *            APR_POLLOUT   -- signal if write will not block
 */
ap_status_t ap_add_poll_socket(ap_pollfd_t *aprset, ap_socket_t *socket, 
                               ap_int16_t event);

/* ***APRDOC********************************************************
 * ap_status_t ap_remove_poll_socket(ap_pollfd_t *aprset, ap_socket_t *sock,
 *                                   ap_int16_t events)
 *    Add a socket to the poll structure. 
 * arg 1) The poll structure we will be using. 
 * arg 2) The socket to remove from the current poll structure. 
 * arg 3) The events to stop looking for during the poll.  One of:
 *            APR_POLLIN    -- signal if read will not block
 *            APR_POLLPRI   -- signal if prioirty data is availble to be read
 *            APR_POLLOUT   -- signal if write will not block
 */
ap_status_t ap_remove_poll_socket(ap_pollfd_t *aprset, ap_socket_t *sock,
                                  ap_int16_t events);

/* ***APRDOC********************************************************
 * ap_status_t ap_clear_poll_sockets(ap_pollfd_t *aprset, ap_int16_t events)
 *    Remove all sockets from the poll structure. 
 * arg 1) The poll structure we will be using. 
 * arg 2) The events to clear from all sockets.  One of:
 *            APR_POLLIN    -- signal if read will not block
 *            APR_POLLPRI   -- signal if prioirty data is availble to be read
 *            APR_POLLOUT   -- signal if write will not block
 */
ap_status_t ap_clear_poll_sockets(ap_pollfd_t *aprset, ap_int16_t events);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_revents(ap_int_16_t *event, ap_socket_t *sock,
 *                            ap_pollfd_t *aprset)
 *    Get the return events for the specified socket. 
 * arg 1) The returned events for the socket.  One of:
 *            APR_POLLIN    -- Data is available to be read 
 *            APR_POLLPRI   -- Prioirty data is availble to be read
 *            APR_POLLOUT   -- Write will succeed
 *            APR_POLLERR   -- An error occurred on the socket
 *            APR_POLLHUP   -- The connection has been terminated
 *            APR_POLLNVAL  -- This is an invalid socket to poll on.
 *                             Socket not open.
 * arg 2) The socket we wish to get information about. 
 * arg 3) The poll structure we will be using. 
 */
ap_status_t ap_get_revents(ap_int16_t *event, ap_socket_t *sock, 
                           ap_pollfd_t *aprset);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_polldata(ap_pollfd_t *pollfd, char *key, void *data)
 *    Return the context associated with the current poll.
 * arg 1) The currently open pollfd.
 * arg 2) The key to use for retreiving data associated with a poll struct.
 * arg 3) The user data associated with the pollfd.
 */
ap_status_t ap_get_polldata(ap_pollfd_t *pollfd, char *key, void *data);

/* ***APRDOC********************************************************
 * ap_status_t ap_set_polldata(ap_pollfd_t *pollfd, void *data, char *key,
                               ap_status_t (*cleanup) (void *))
 *    Return the context associated with the current poll.
 * arg 1) The currently open pollfd.
 * arg 2) The user data to associate with the pollfd.

 */
ap_status_t ap_set_polldata(ap_pollfd_t *pollfd, void *data, char *key,
                            ap_status_t (*cleanup) (void *));

/*  accessor functions   */

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_NETWORK_IO_H */

