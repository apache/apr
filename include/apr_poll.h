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

#ifndef APR_POLL_H
#define APR_POLL_H
/**
 * @file apr_poll.h
 * @brief APR Poll interface
 */
#include "apr.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_inherit.h" 
#include "apr_file_io.h" 
#include "apr_network_io.h" 

#if APR_HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup apr_poll Poll Routines
 * @ingroup APR 
 * @{
 */

/**
 * @defgroup apr_poll_opt Poll options
 * @{
 */
#define APR_POLLIN    0x001     /**< Can read without blocking */
#define APR_POLLPRI   0x002     /**< Priority data available */
#define APR_POLLOUT   0x004     /**< Can write without blocking */
#define APR_POLLERR   0x010     /**< Pending error */
#define APR_POLLHUP   0x020     /**< Hangup occurred */
#define APR_POLLNVAL  0x040     /**< Descriptior invalid */
/** @} */

/** Used in apr_pollfd_t to determine what the apr_descriptor is */
typedef enum { 
    APR_NO_DESC,                /**< nothing here */
    APR_POLL_SOCKET,            /**< descriptor refers to a socket */
    APR_POLL_FILE,              /**< descriptor refers to a file */
    APR_POLL_LASTDESC           /**< descriptor is the last one in the list */
} apr_datatype_e ;

/** Union of either an APR file or socket. */
typedef union {
    apr_file_t *f;              /**< file */
    apr_socket_t *s;            /**< socket */
} apr_descriptor;

/** @see apr_pollfd_t */
typedef struct apr_pollfd_t apr_pollfd_t;

/** Poll descriptor set. */
struct apr_pollfd_t {
    apr_pool_t *p;              /**< associated pool */
    apr_datatype_e desc_type;   /**< descriptor type */
    apr_int16_t reqevents;      /**< requested events */
    apr_int16_t rtnevents;      /**< returned events */
    apr_descriptor desc;        /**< @see apr_descriptor */
    void *client_data;          /**< allows app to associate context */
};

/**
 * Setup the memory required for poll to operate properly
 * @param new_poll The poll structure to be used. 
 * @param num The number of socket descriptors to be polled.
 * @param cont The pool to operate on.
 * @deprecated This function is deprecated, APR applications should control the pollset memory themselves.
 */
APR_DECLARE(apr_status_t) apr_poll_setup(apr_pollfd_t **new_poll, 
                                         apr_int32_t num,
                                         apr_pool_t *cont);

/**
 * Poll the sockets in the poll structure
 * @param aprset The poll structure we will be using. 
 * @param numsock The number of sockets we are polling
 * @param nsds The number of sockets signalled.
 * @param timeout The amount of time in microseconds to wait.  This is 
 *                a maximum, not a minimum.  If a socket is signalled, we 
 *                will wake up before this time.  A negative number means 
 *                wait until a socket is signalled.
 * @remark
 * <PRE>
 * The number of sockets signalled is returned in the second argument. 
 *
 *        This is a blocking call, and it will not return until either a 
 *        socket has been signalled, or the timeout has expired. 
 * </PRE>
 */
APR_DECLARE(apr_status_t) apr_poll(apr_pollfd_t *aprset, apr_int32_t numsock,
                                   apr_int32_t *nsds, 
                                   apr_interval_time_t timeout);

/**
 * Add a socket to the poll structure.
 * @param aprset The poll structure we will be using. 
 * @param sock The socket to add to the current poll structure. 
 * @param event The events to look for when we do the poll.  One of:
 * <PRE>
 *            APR_POLLIN       signal if read will not block
 *            APR_POLLPRI      signal if prioirty data is availble to be read
 *            APR_POLLOUT      signal if write will not block
 * </PRE>
 * @deprecated This function is deprecated, APR applications should control the pollset memory themselves.
 */
APR_DECLARE(apr_status_t) apr_poll_socket_add(apr_pollfd_t *aprset, 
                                              apr_socket_t *sock,
                                              apr_int16_t event);

/**
 * Modify a socket in the poll structure with mask.
 * @param aprset The poll structure we will be using. 
 * @param sock The socket to modify in poll structure. 
 * @param events The events to stop looking for during the poll.  One of:
 * <PRE>
 *            APR_POLLIN       signal if read will not block
 *            APR_POLLPRI      signal if priority data is available to be read
 *            APR_POLLOUT      signal if write will not block
 * </PRE>
 * @deprecated This function is deprecated, APR applications should control the pollset memory themselves.
 */
APR_DECLARE(apr_status_t) apr_poll_socket_mask(apr_pollfd_t *aprset,
                                               apr_socket_t *sock,
                                               apr_int16_t events);
/**
 * Remove a socket from the poll structure.
 * @param aprset The poll structure we will be using. 
 * @param sock The socket to remove from the current poll structure. 
 * @deprecated This function is deprecated, APR applications should control the pollset memory themselves.
 */
APR_DECLARE(apr_status_t) apr_poll_socket_remove(apr_pollfd_t *aprset, 
                                                 apr_socket_t *sock);

/**
 * Clear all events in the poll structure.
 * @param aprset The poll structure we will be using. 
 * @param events The events to clear from all sockets.  One of:
 * <PRE>
 *            APR_POLLIN       signal if read will not block
 *            APR_POLLPRI      signal if priority data is available to be read
 *            APR_POLLOUT      signal if write will not block
 * </PRE>
 * @deprecated This function is deprecated, APR applications should control the pollset memory themselves.
 */
APR_DECLARE(apr_status_t) apr_poll_socket_clear(apr_pollfd_t *aprset, 
                                                 apr_int16_t events);

/**
 * Get the return events for the specified socket.
 * @param event The returned events for the socket.  One of:
 * <PRE>
 *            APR_POLLIN       Data is available to be read 
 *            APR_POLLPRI      Priority data is availble to be read
 *            APR_POLLOUT      Write will succeed
 *            APR_POLLERR      An error occurred on the socket
 *            APR_POLLHUP      The connection has been terminated
 *            APR_POLLNVAL     This is an invalid socket to poll on.
 *                             Socket not open.
 * </PRE>
 * @param sock The socket we wish to get information about. 
 * @param aprset The poll structure we will be using. 
 * @deprecated This function is deprecated, APR applications should control the pollset memory themselves.
 */
APR_DECLARE(apr_status_t) apr_poll_revents_get(apr_int16_t *event, 
                                          apr_socket_t *sock,
                                          apr_pollfd_t *aprset);

/* General-purpose poll API for arbitrarily large numbers of
 * file descriptors
 */

/** Opaque structure used for pollset API */
typedef struct apr_pollset_t apr_pollset_t;

/**
 * Setup a pollset object
 * @param pollset  The pointer in which to return the newly created object 
 * @param size The maximum number of descriptors that this pollset can hold
 * @param p The pool from which to allocate the pollset
 * @param flags Optional flags to modify the operation of the pollset
 *              (reserved for future expansion)
 */
APR_DECLARE(apr_status_t) apr_pollset_create(apr_pollset_t **pollset,
                                             apr_uint32_t size,
                                             apr_pool_t *p,
                                             apr_uint32_t flags);

/**
 * Destroy a pollset object
 * @param pollset The pollset to destroy
 */
APR_DECLARE(apr_status_t) apr_pollset_destroy(apr_pollset_t *pollset);

/**
 * Add a socket or file descriptor to a pollset
 * @param pollset The pollset to which to add the descriptor
 * @param descriptor The descriptor to add
 * @remark If you set client_data in the descriptor, that value
 *         will be returned in the client_data field whenever this
 *         descriptor is signalled in apr_pollset_poll().
 */
APR_DECLARE(apr_status_t) apr_pollset_add(apr_pollset_t *pollset,
                                          const apr_pollfd_t *descriptor);

/**
 * Remove a descriptor from a pollset
 * @param pollset The pollset from which to remove the descriptor
 * @param descriptor The descriptor to remove
 */
APR_DECLARE(apr_status_t) apr_pollset_remove(apr_pollset_t *pollset,
                                             const apr_pollfd_t *descriptor);

/**
 * Block for activity on the descriptor(s) in a pollset
 * @param pollset The pollset to use
 * @param timeout Timeout in microseconds
 * @param num Number of signalled descriptors (output parameter)
 * @param descriptors Array of signalled descriptors (output parameter)
 */
APR_DECLARE(apr_status_t) apr_pollset_poll(apr_pollset_t *pollset,
                                           apr_interval_time_t timeout,
                                           apr_int32_t *num,
                                           const apr_pollfd_t **descriptors);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_POLL_H */

