/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APR_EVENTSET_H
#define APR_EVENTSET_H

/**
 * @file apr_eventset.h
 * @brief Generic event handling and dispatching.
 * @note Some features may not be available due to host system limitations.
 */

#include "apr.h"
#include "apr_poll.h"
#include "apr_time.h"
#include "apr_network_io.h"

APR_BEGIN_DECLS

/**
 * @defgroup APR_Event Generic event handling and dispatching
 * @ingroup APR
 * @{
 */

/** @see apr_event_t */
typedef struct apr_event_t apr_event_t;
/** @see apr_iocb_t */
typedef struct apr_iocb_t apr_iocb_t;
/** @see apr_event_data_t */
typedef struct apr_event_data_t apr_event_data_t;
/** opaque structure */
typedef struct apr_eventset_t apr_eventset_t;

/**
 * eventset flags
 */
#define APR_EVENTSET 0x0001 /**< Runs a single iteration for the eventset loop. */

/**
 * Supported event types (asynchronous operations)
 */
typedef enum {
    /** poll for activity on a socket */
    APR_EVENT_POLL,
    /** receive a message from a socket */
    APR_EVENT_RECV,
    /** send a message on a socket */
    APR_EVENT_SEND,
    /** accept a connection on a socket */
    APR_EVENT_ACCEPT,
    /** establish a connection on a socket */
    APR_EVENT_CONNECT,
    /** read from file */
    APR_EVENT_READ,
    /** write on a file */
    APR_EVENT_WRITE,
    /** file change notification */
    APR_EVENT_WATCH,
    /** timer event */
    APR_EVENT_TIMER,
    /** transfer file data */
    APR_EVENT_SENDFILE,
    /** signal delivery notification */
    APR_EVENT_SIGNAL,
    /** address resolution */
    APR_EVENT_DNS,
} apr_event_type_e;

/**
 * Event flags
 */
typedef enum {
    /** event is notified only once */
    APR_EVENT_ONESHOT   = (0 << 1),
} apr_event_flags_e;

/**
 * Return values for the event callback function
 */
typedef enum {
    /** event is not to be rearmed  */
    APR_EVENT_NOREARM,
    /** event must be rearmed */
    APR_EVENT_REARM,
} apr_event_status_e;

/**
 * I/O control block
 */
struct apr_iocb_t {
    /** data buffer */
    apr_byte_t *data;
    /** data buffer length */
    apr_size_t nbytes;
    /** file position */
    apr_off_t offset;
};

/**
 * The event data union.
 * This union is used to send/receive event type-specific data.
 */
struct apr_event_data_t {
    union {
        /** an open file */
        apr_file_t *file;
        /** a socket */
        apr_socket_t *socket;
        /** poll descriptor */
        apr_pollfd_t *pfd;
        /** signal number */
        int signal;
    };
    union {
        /** i/o control block */
        apr_iocb_t *iocb;
        /** new connection */
        apr_socket_t *new_socket;
        /** address to connect to */
        apr_sockaddr_t *sa;
    };
};

/**
 * The event structure.
 * This structure holds all information necessary to process an
 * event from/to any supported component (event types).
 */
struct apr_event_t {
    /** associated pool */
    apr_pool_t *pool;
    /** event type */
    apr_uint16_t type;
    /** event flags */
    apr_uint16_t flags;
    /** return status */
    apr_status_t status;
    /** event type-dependent data */
    apr_event_data_t data;
    /** event callback function */
    apr_event_status_e (*cb)(apr_event_t *);
    /** user data variable */
    void *cookie;
};

/**
 * Initialize eventset internal structures and global work queue
 * @warning This must be called before any eventset is created
 */
APR_DECLARE(apr_status_t) apr_eventset_initialize(void);

/**
 * Setup a eventset object
 * @param eventset The pointer in which to return the newly created object
 * @param pool The pool from which to allocate the eventset
 * @param size The size is just a hint about how to dimension internal structures
 * @param flags Optional flags to modify the operation of the eventset
 */
APR_DECLARE(apr_status_t) apr_eventset_create(apr_eventset_t **eventset,
                                              apr_pool_t *pool, apr_uint32_t size,
                                              apr_uint32_t flags);

/**
 * Destroy a eventset
 * @param eventset The eventset to destroy
 */
APR_DECLARE(apr_status_t) apr_eventset_destroy(apr_eventset_t *eventset);

/**
 * Add (register) an event descriptor to the eventset
 * @param eventset The eventset to use
 * @param event The event to add
 * @remark Events added to the eventset are not copied. They must be kept in
 * memory until removed from the eventset.
 */
APR_DECLARE(apr_status_t) apr_eventset_add(apr_eventset_t *eventset,
                                           apr_event_t *event);

/**
 * Remove an event from the eventset
 * @param eventset The eventset to destroy
 * @param event The event to remove
 * @remark This will only work if the event is still in its waiting stage.
 */
APR_DECLARE(apr_status_t) apr_eventset_remove(apr_eventset_t *eventset,
                                              apr_event_t *event);

/**
 * Runs the eventset main loop
 * @param eventset The evenset to use
 * @param flags Optional flags to modify the operation of the eventset
 */
APR_DECLARE(apr_status_t) apr_eventset_loop(apr_eventset_t *eventset,
                                            apr_uint32_t flags);

/**
 * Stops a eventset loop from running
 * @param eventset The evenset to stop
 */
APR_DECLARE(void) apr_eventset_stop(apr_eventset_t *eventset);

/** @} */

APR_END_DECLS

#endif
