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

/**
 * @package APR Network library
 */

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
#define APR_SO_RCVBUF        128
#define APR_SO_DISCONNECTED  256
#define APR_TCP_NODELAY      512

#define APR_POLLIN    0x001 
#define APR_POLLPRI   0x002
#define APR_POLLOUT   0x004
#define APR_POLLERR   0x010
#define APR_POLLHUP   0x020
#define APR_POLLNVAL  0x040

typedef enum {APR_SHUTDOWN_READ, APR_SHUTDOWN_WRITE, 
	      APR_SHUTDOWN_READWRITE} apr_shutdown_how_e;

/* We need to make sure we always have an in_addr type, so APR will just
 * define it ourselves, if the platform doesn't provide it.
 */
#if (!APR_HAVE_IN_ADDR)
struct in_addr {
    apr_uint32_t  s_addr;
}
#endif

/* Enum to tell us if we're interested in remote or local socket */
typedef enum {
    APR_LOCAL,
    APR_REMOTE
} apr_interface_e;

/* I guess not everybody uses inet_addr.  This defines apr_inet_addr
 * appropriately.
 */

#if APR_HAVE_INET_ADDR
#define apr_inet_addr    inet_addr
#elif APR_HAVE_INET_NETWORK        /* only DGUX, as far as I know */
#define apr_inet_addr    inet_network
#endif

typedef struct apr_socket_t     apr_socket_t;
typedef struct apr_pollfd_t     apr_pollfd_t;
typedef struct apr_hdtr_t       apr_hdtr_t;
typedef struct in_addr          apr_in_addr_t;

/* use apr_uint16_t just in case some system has a short that isn't 16 bits... */
typedef apr_uint16_t            apr_port_t;

#if APR_HAS_SENDFILE
/* Define flags passed in on apr_sendfile() */
#define APR_SENDFILE_DISCONNECT_SOCKET      1
#endif

/** A structure to encapsulate headers and trailers for apr_sendfile */
struct apr_hdtr_t {
    /** An iovec to store the headers sent before the file. 
     *  @defvar iovec *headers */
    struct iovec* headers;
    /** number of headers in the iovec */
    int numheaders;
    /** An iovec to store the trailers sent before the file. 
     *  @defvar iovec *trailers */
    struct iovec* trailers;
    /** number of trailers in the iovec */
    int numtrailers;
};

/* function definitions */

/**
 * Create a socket for tcp communication.
 * @param new_sock The new socket that has been setup. 
 * @param cont The pool to use
 */
apr_status_t apr_create_tcp_socket(apr_socket_t **new_sock, apr_pool_t *cont);

/**
 * Shutdown either reading, writing, or both sides of a tcp socket.
 * @param thesocket The socket to close 
 * @param how How to shutdown the socket.  One of:
 * <PRE>
 *            APR_SHUTDOWN_READ      -- no longer allow read requests
 *            APR_SHUTDOWN_WRITE     -- no longer allow write requests
 *            APR_SHUTDOWN_READWRITE -- no longer allow read or write requests 
 * </PRE>
 * @tip This does not actually close the socket descriptor, it just
 *      controls which calls are still valid on the socket.
 */
apr_status_t apr_shutdown(apr_socket_t *thesocket, apr_shutdown_how_e how);

/**
 * Close a tcp socket.
 * @param thesocket The socket to close 
 */
apr_status_t apr_close_socket(apr_socket_t *thesocket);

/**
 * Bind the socket to its associated port
 * @param sock The socket to bind 
 * @tip This is where we will find out if there is any other process
 *      using the selected port.
 */
apr_status_t apr_bind(apr_socket_t *sock);

/**
 * Listen to a bound socket for connections.
 * @param sock The socket to listen on 
 * @param backlog The number of outstanding connections allowed in the sockets
 *                listen queue.  If this value is less than zero, the listen
 *                queue size is set to zero.  
 */
apr_status_t apr_listen(apr_socket_t *sock, apr_int32_t backlog);

/**
 * Accept a new connection request
 * @param new_sock A copy of the socket that is connected to the socket that
 *                 made the connection request.  This is the socket which should
 *                 be used for all future communication.
 * @param sock The socket we are listening on.
 * @param connection_pool The pool for the new socket.
 */
apr_status_t apr_accept(apr_socket_t **new_sock, apr_socket_t *sock, 
                      apr_pool_t *connection_pool);

/**
 * Issue a connection request to a socket either on the same machine 
 * or a different one.
 * @param sock The socket we wish to use for our side of the connection 
 * @param hostname The hostname of the machine we wish to connect to.  If NULL,
 *                 APR assumes that the sockaddr_in in the apr_socket is 
 *                 completely filled out.
 */
apr_status_t apr_connect(apr_socket_t *sock, const char *hostname);

/**
 * Get name of a machine we are currently connected to.
 * @param name A buffer to store the hostname in.
 * @param which Which interface do we wnt the hostname for?
 * @param sock The socket to examine.
 */
apr_status_t apr_get_hostname(char **name, apr_interface_e which, apr_socket_t *sock);

/**
 * Get name of the current machine
 * @param buf A buffer to store the hostname in.
 * @param len The maximum length of the hostname that can be stored in the
 *            buffer provided. 
 * @param cont The pool to use.
 */
apr_status_t apr_gethostname(char *buf, int len, apr_pool_t *cont);

/**
 * Return the pool associated with the current socket>
 * @param data The user data associated with the socket.
 * @param key The key to associate with the user data.
 * @param sock The currently open socket.
 */
apr_status_t apr_get_socketdata(void **data, const char *key, apr_socket_t *sock);

/**
 * Set the pool associated with the current socket.
 * @param sock The currently open socket.
 * @param data The user data to associate with the socket.
 * @param key The key to associate with the data.
 * @param cleanup The cleanup to call when the socket is destroyed.
 */
apr_status_t apr_set_socketdata(apr_socket_t *sock, void *data, const char *key,
                              apr_status_t (*cleanup) (void*));

/**
 * Send data over a network.
 * @param sock The socket to send the data over.
 * @param buf The buffer which contains the data to be sent. 
 * @param len On entry, the number of bytes to send; on exit, the number
 *            of bytes sent.
 * @tip
 * <PRE>
 * This functions acts like a blocking write by default.  To change 
 * this behavior, use apr_setsocketopt with the APR_SO_TIMEOUT option.
 *
 * It is possible for both bytes to be sent and an error to be returned.
 *
 * APR_EINTR is never returned.
 * </PRE>
 */
apr_status_t apr_send(apr_socket_t *sock, const char *buf, apr_size_t *len);

/**
 * Send multiple packets of data over a network.
 * @param sock The socket to send the data over.
 * @param vec The array of iovec structs containing the data to send 
 * @param nvec The number of iovec structs in the array
 * @param len Receives the number of bytes actually written
 * @tip
 * <PRE>
 * This functions acts like a blocking write by default.  To change 
 * this behavior, use apr_setsocketopt with the APR_SO_TIMEOUT option.
 * The number of bytes actually sent is stored in argument 3.
 *
 * It is possible for both bytes to be sent and an error to be returned.
 *
 * APR_EINTR is never returned.
 * </PRE>
 */
apr_status_t apr_sendv(apr_socket_t *sock, const struct iovec *vec, 
                     apr_int32_t nvec, apr_size_t *len);

#if APR_HAS_SENDFILE
/**
 * Send a file from an open file descriptor to a socket, along with 
 * optional headers and trailers
 * @param sock The socket to which we're writing
 * @param file The open file from which to read
 * @param hdtr A structure containing the headers and trailers to send
 * @param offset Offset into the file where we should begin writing
 * @param len Number of bytes to send 
 * @param flags APR flags that are mapped to OS specific flags
 * @tip This functions acts like a blocking write by default.  To change 
 *      this behavior, use apr_setsocketopt with the APR_SO_TIMEOUT option.
 *      The number of bytes actually sent is stored in argument 5.
 */
apr_status_t apr_sendfile(apr_socket_t *sock, apr_file_t *file, apr_hdtr_t *hdtr, 
                          apr_off_t *offset, apr_size_t *len, apr_int32_t flags);
#endif

/**
 * Read data from a network.
 * @param sock The socket to read the data from.
 * @param buf The buffer to store the data in. 
 * @param len On entry, the number of bytes to receive; on exit, the number
 *            of bytes received.
 * @tip
 * <PRE>
 * This functions acts like a blocking read by default.  To change 
 * this behavior, use apr_setsocketopt with the APR_SO_TIMEOUT option.
 * The number of bytes actually sent is stored in argument 3.
 *
 * It is possible for both bytes to be received and an APR_EOF or
 * other error to be returned.
 *
 * APR_EINTR is never returned.
 * </PRE>
 */
apr_status_t apr_recv(apr_socket_t *sock, char *buf, apr_size_t *len);

/**
 * Setup socket options for the specified socket
 * @param sock The socket to set up.
 * @param opt The option we would like to configure.  One of:
 * <PRE>
 *            APR_SO_DEBUG      --  turn on debugging information 
 *            APR_SO_KEEPALIVE  --  keep connections active
 *            APR_SO_LINGER     --  lingers on close if data is present
 *            APR_SO_NONBLOCK   --  Turns blocking on/off for socket
 *            APR_SO_REUSEADDR  --  The rules used in validating addresses
 *                                  supplied to bind should allow reuse
 *                                  of local addresses.
 *            APR_SO_TIMEOUT    --  Set the timeout value in microseconds.
 *                                  values < 0 mean wait forever.  0 means
 *                                  don't wait at all.
 *            APR_SO_SNDBUF     --  Set the SendBufferSize
 *            APR_SO_RCVBUF     --  Set the ReceiveBufferSize
 * </PRE>
 * @param on Are we turning the option on or off.
 */
apr_status_t apr_setsocketopt(apr_socket_t *sock, apr_int32_t opt, apr_int32_t on);

/**
 * Query socket options for the specified socket
 * @param sock The socket to query
 * @param opt The option we would like to query.  One of:
 * <PRE>
 *            APR_SO_DEBUG      --  turn on debugging information 
 *            APR_SO_KEEPALIVE  --  keep connections active
 *            APR_SO_LINGER     --  lingers on close if data is present
 *            APR_SO_NONBLOCK   --  Turns blocking on/off for socket
 *            APR_SO_REUSEADDR  --  The rules used in validating addresses
 *                                  supplied to bind should allow reuse
 *                                  of local addresses.
 *            APR_SO_TIMEOUT    --  Set the timeout value in microseconds.
 *                                  values < 0 mean wait forever.  0 means
 *                                  don't wait at all.
 *            APR_SO_SNDBUF     --  Set the SendBufferSize
 *            APR_SO_RCVBUF     --  Set the ReceiveBufferSize
 *            APR_SO_DISCONNECTED -- Query the disconnected state of the socket.
 *                                  (Currently only used on Windows)
 * </PRE>
 * @param on Socket option returned on the call.
 */
apr_status_t apr_getsocketopt(apr_socket_t *sock, apr_int32_t opt, apr_int32_t* on);

/**
 * Associate a port with a socket.
 * @param sock The socket to set.
 * @param which Which socket do we want to set the port for?
 * @param port The local port this socket will be dealing with.
 * @tip This does not bind the two together, it is just telling apr 
 *      that this socket is going to use this port if possible.  If
 *      the port is already used, we won't find out about it here.
 */
apr_status_t apr_set_port(apr_socket_t *sock, apr_interface_e which, apr_port_t port);

/**
 * Return the port associated with a socket.
 * @param port The local port this socket is associated with.
 * @param which Which interface are we getting the port for?
 * @param sock The socket to enquire about.
 */
apr_status_t apr_get_port(apr_port_t *port, apr_interface_e which, apr_socket_t *sock);

/**
 * Associate a socket addr with an apr socket.
 * @param sock The socket to use 
 * @param which Which interface should we set?
 * @param addr The IP address to attach to the socket.
 *             Use APR_ANYADDR to use any IP addr on the machine.
 * @tip This does not bind the two together, it is just telling apr 
 *      that this socket is going to use this address if possible. 
 */
apr_status_t apr_set_ipaddr(apr_socket_t *sock, apr_interface_e which, const char *addr);

/**
 * Return the IP address associated with an apr socket.
 * @param addr The local IP address associated with the socket.
 * @param sock The socket to use 
 */
apr_status_t apr_get_ipaddr(char **addr, apr_interface_e which, apr_socket_t *sock);

/**
 * Return the local socket name as a BSD style struct sockaddr_in.
 * @param name The local name associated with the socket.
 * @param sock The socket to use 
 */
apr_status_t apr_get_local_name(struct sockaddr_in **name, apr_socket_t *sock);

/**
 * Return the remote socket name as a BSD style struct sockaddr_in.
 * @param name The remote name associated with the socket.
 * @param sock The socket to use 
 */
apr_status_t apr_get_remote_name(struct sockaddr_in **name, apr_socket_t *sock);

/**
 * Setup the memory required for poll to operate properly>
 * @param new_poll The poll structure to be used. 
 * @param num The number of socket descriptors to be polled.
 * @param cont The pool to operate on.
 */
apr_status_t apr_setup_poll(apr_pollfd_t **new_poll, apr_int32_t num, 
                          apr_pool_t *cont);

/**
 * Poll the sockets in the poll structure
 * @param aprset The poll structure we will be using. 
 * @param nsds The number of sockets we are polling. 
 * @param timeout The amount of time in microseconds to wait.  This is 
 *                a maximum, not a minimum.  If a socket is signalled, we 
 *                will wake up before this time.  A negative number means 
 *                wait until a socket is signalled.
 * @tip
 * <PRE>
 * The number of sockets signalled is returned in the second argument. 
 *
 *        This is a blocking call, and it will not return until either a 
 *        socket has been signalled, or the timeout has expired. 
 * </PRE>
 */
apr_status_t apr_poll(apr_pollfd_t *aprset, apr_int32_t *nsds, apr_interval_time_t timeout);

/**
 * Add a socket to the poll structure.
 * @param aprset The poll structure we will be using. 
 * @param socket The socket to add to the current poll structure. 
 * @param event The events to look for when we do the poll.  One of:
 * <PRE>
 *            APR_POLLIN    -- signal if read will not block
 *            APR_POLLPRI   -- signal if prioirty data is availble to be read
 *            APR_POLLOUT   -- signal if write will not block
 * </PRE>
 */
apr_status_t apr_add_poll_socket(apr_pollfd_t *aprset, apr_socket_t *socket, 
                               apr_int16_t event);

/**
 * Modify a socket in the poll structure with mask.
 * @param aprset The poll structure we will be using. 
 * @param sock The socket to modify in poll structure. 
 * @param events The events to stop looking for during the poll.  One of:
 * <PRE>
 *            APR_POLLIN    -- signal if read will not block
 *            APR_POLLPRI   -- signal if prioirty data is availble to be read
 *            APR_POLLOUT   -- signal if write will not block
 * </PRE>
 */
apr_status_t apr_mask_poll_socket(apr_pollfd_t *aprset, apr_socket_t *sock,
                                  apr_int16_t events);
/**
 * Remove a socket from the poll structure.
 * @param aprset The poll structure we will be using. 
 * @param sock The socket to remove from the current poll structure. 
 */
apr_status_t apr_remove_poll_socket(apr_pollfd_t *aprset, apr_socket_t *sock);

/**
 * Remove all sockets from the poll structure.
 * @param aprset The poll structure we will be using. 
 * @param events The events to clear from all sockets.  One of:
 * <PRE>
 *            APR_POLLIN    -- signal if read will not block
 *            APR_POLLPRI   -- signal if prioirty data is availble to be read
 *            APR_POLLOUT   -- signal if write will not block
 * </PRE>
 */
apr_status_t apr_clear_poll_sockets(apr_pollfd_t *aprset, apr_int16_t events);

/**
 * Get the return events for the specified socket.
 * @param event The returned events for the socket.  One of:
 * <PRE>
 *            APR_POLLIN    -- Data is available to be read 
 *            APR_POLLPRI   -- Prioirty data is availble to be read
 *            APR_POLLOUT   -- Write will succeed
 *            APR_POLLERR   -- An error occurred on the socket
 *            APR_POLLHUP   -- The connection has been terminated
 *            APR_POLLNVAL  -- This is an invalid socket to poll on.
 *                             Socket not open.
 * </PRE>
 * @param sock The socket we wish to get information about. 
 * @param aprset The poll structure we will be using. 
 */
apr_status_t apr_get_revents(apr_int16_t *event, apr_socket_t *sock, 
                           apr_pollfd_t *aprset);

/**
 * Return the data associated with the current poll.
 * @param pollfd The currently open pollfd.
 * @param key The key to use for retreiving data associated with a poll struct.
 * @param data The user data associated with the pollfd.
 */
apr_status_t apr_get_polldata(apr_pollfd_t *pollfd, const char *key, void *data);

/**
 * Set the data associated with the current poll.
 * @param pollfd The currently open pollfd.
 * @param data The key to associate with the data.
 * @param key The user data to associate with the pollfd.
 * @param cleanup The cleanup function
 */
apr_status_t apr_set_polldata(apr_pollfd_t *pollfd, void *data, const char *key,
                            apr_status_t (*cleanup) (void *));

/**
 * Convert a File type to a socket so that it can be used in a poll operation.
 * @param newsock the newly created socket which represents a file.
 * @param file the file to mask as a socket.
 * @tip This is not available on all platforms.  Platforms that have the
 *      ability to poll files for data to be read/written/exceptions will
 *      have the APR_FILES_AS_SOCKETS macro defined as true.
 */
apr_status_t apr_socket_from_file(apr_socket_t **newsock, apr_file_t *file);

/**
 * Given a hostname and a port, create an apr_in_addr for it...
 * @param addr The apr_in_addr_t structure to return.
 * @param hostname The hostname to lookup.
 */
apr_status_t apr_get_inaddr(apr_in_addr_t *addr, char *hostname);

/**
 * Given an apr_socket_t get the apr_in_addr_t for the requested interface
 * @param addr The apr_in_addr_t structure to return
 * @param which The interface to return for
 * @param sock The apr_socket_t to use
 */
apr_status_t apr_get_socket_inaddr(apr_in_addr_t *addr, apr_interface_e which,
                  apr_socket_t *sock);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_NETWORK_IO_H */

