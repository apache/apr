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

#ifndef APR_WQUEUE_H
#define APR_WQUEUE_H

/**
 * @file apr_wqueue.h
 * @brief APR Wait Queues
 */

#include "apr.h"
#include "apr_ring.h"
#include "apr_pools.h"
#include "apr_time.h"

APR_BEGIN_DECLS

/**
 * @defgroup APR_Wqueue APR Wait Queues
 * @ingroup APR
 * @{
 */

/** opaque structure */
typedef struct apr_wqueue_t apr_wqueue_t;
/** @see apr_wqueue_entry_t */
typedef struct apr_wqueue_entry_t apr_wqueue_entry_t;

/**
 * Wake function prototype
 * @param wqueue the wait queue for this entry
 * @param data user private data pointer
 */
typedef apr_status_t (*apr_wqueue_func_t)(apr_wqueue_t *wqueue, void *data);

/**
 * The wait queue entry structure.
 */
struct apr_wqueue_entry_t {
    /** link to the wait queue ring */
    APR_RING_ENTRY(apr_wqueue_entry_t) link;
    /** wake up callback */
    apr_wqueue_func_t func;
    /** user private data pointer */
    void *data;
};

/**
 * Create a new wait queue.
 * @param wqueue The pointer in which to return the newly created object
 * @param pool The pool from which to allocate the wait queue
 */
APR_DECLARE(apr_status_t) apr_wqueue_create(apr_wqueue_t **wqueue,
                                            apr_pool_t *pool);

/**
 * Destroy a wait queue.
 * @param wqueue The wait queue to destroy
 */
APR_DECLARE(apr_status_t) apr_wqueue_destroy(apr_wqueue_t *wqueue);

/**
 * Add an entry into a wait queue.
 * @param wqueue The wait queue to add the entry to
 * @param entry The wait queue entry
 */
APR_DECLARE(apr_status_t) apr_wqueue_add(apr_wqueue_t *wqueue,
                                         apr_wqueue_entry_t *entry);

/**
 * Remove an entry from a wait queue
 * @param wqueue The wait queue to remove the entry from
 * @param entry The wait queue entry
 */
APR_DECLARE(apr_status_t) apr_wqueue_remove(apr_wqueue_t *wqueue,
                                            apr_wqueue_entry_t *entry);

/**
 * Wake up @nwake processes sleeping on the wait queue.
 * @param wqueue The wait queue
 * @param nwake (input) Number of process to wake or 0 to wake up all
 * (output) - Number of processes woken up
 * @remark The woken up entries are not removed from the wait queue
 */
APR_DECLARE(apr_status_t) apr_wqueue_wake(apr_wqueue_t *wqueue,
                                          apr_uint32_t *nwake);

/**
 * Waits up to timeout microseconds for a wake up.
 * @param wqueue The wait queue to wait on
 * @param timeout Timeout in microseconds, a negative value is treated
 * as an infinite timeout.
 */
APR_DECLARE(apr_status_t) apr_wqueue_wait(apr_wqueue_t *wqueue,
                                          apr_interval_time_t timeout);

/** @} */

APR_END_DECLS

#endif
