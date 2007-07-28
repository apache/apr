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

#ifndef APR_MQUEUE_H
#define APR_MQUEUE_H

/**
 * @file apr_mqueue.h
 * @brief APR Message Queues
 */

#include "apr.h"
#include "apr_pools.h"
#include "apr_time.h"

APR_BEGIN_DECLS

/**
 * @defgroup APR_Mqueue APR Message Queues
 * @ingroup APR
 * @{
 */

/** opaque structure */
typedef struct apr_mqueue_t apr_mqueue_t;
/** @see apr_message_t */
typedef struct apr_message_t apr_message_t;

/**
 * The message structure.
 * A single message block.
 */
struct apr_message_t {
    /** message type */
    apr_uint32_t type;
    /** data length */
    apr_size_t length;
    /** message data pointer */
    void *data;
};

/**
 * Create a new anonymous (private) message queue.
 * @param mqueue The pointer in which to return the newly created object
 * @param size Hint on the maximum number of messages on the message queue
 * @param pool The pool from which to allocate the message queue
 */
APR_DECLARE(apr_status_t) apr_mqueue_create(apr_mqueue_t **mqueue,
                                            apr_size_t size, apr_pool_t *pool);

/**
 * Create a new named (public) message queue.
 * @param mqueue The pointer in which to return the newly created object
 * @param pathname The name of the message queue or NULL if anonymous
 * @param pool The pool from which to allocate the message queue
 */
APR_DECLARE(apr_status_t) apr_mqueue_create_named(apr_mqueue_t **mqueue,
                                                  const char *pathname,
                                                  apr_int32_t flags,
                                                  apr_fileperms_t perms,
                                                  apr_pool_t *pool);

/**
 * Open a named message queue.
 * @param mqueue The pointer in which to return the newly created object
 * @param pathname The name of the message queue
 * @param pool The pool from which to allocate the message queue
 */
APR_DECLARE(apr_status_t) apr_mqueue_open(apr_mqueue_t **mqueue,
                                          const char *pathname,
                                          apr_int32_t flags,
                                          apr_pool_t *pool);

/**
 * Remove a message queue.
 * @param name The name of the message queue
 * @param pool The pool used for the operation
 */
APR_DECLARE(apr_status_t) apr_mqueue_remove(const char *name, apr_pool_t *pool);

/**
 * Destroy a message queue.
 * @param mqueue The message queue to destroy
 */
APR_DECLARE(apr_status_t) apr_mqueue_destroy(apr_mqueue_t *mqueue);

/**
 * Send a message to a message queue.
 * @param mqueue The message queue to send the message to
 * @param msg The message to send
 */
APR_DECLARE(apr_status_t) apr_mqueue_send(apr_mqueue_t *mqueue,
                                          apr_message_t *msg);

/**
 * Receive a message from a message queue.
 * @param mqueue The message queue to receive the message from
 * @param msg The address to copy the message to
 */
APR_DECLARE(apr_status_t) apr_mqueue_recv(apr_mqueue_t *mqueue,
                                          apr_message_t *msg);

/**
 * Peek at the topmost message on the message queue
 * @param mqueue The message queue to peek the message from
 * @param msg The address to copy the message to
 * type and size are updated
 */
APR_DECLARE(apr_status_t) apr_mqueue_peek(apr_mqueue_t *mqueue,
                                          apr_message_t *msg);

/**
 * Waits up to timeout microseconds for a message.
 * @param mqueue The message queue to wait
 * @param timeout Timeout in microseconds
 */
APR_DECLARE(apr_status_t) apr_mqueue_wait(apr_mqueue_t *mqueue,
                                          apr_interval_time_t timeout);

/** @} */

APR_END_DECLS

#endif
