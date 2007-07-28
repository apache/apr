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

#include "apr_wqueue.h"
#include "apr_thread_cond.h"

struct apr_wqueue_t {
    apr_pool_t *pool;
    apr_thread_cond_t *cond;
    apr_thread_mutex_t *mutex;
    APR_RING_HEAD(wait_ring_t, apr_wqueue_entry_t) ring;
};

static apr_status_t plain_cleanup(void *data)
{
    apr_wqueue_t *wqueue = data;

    apr_thread_cond_destroy(wqueue->cond);
    apr_thread_mutex_destroy(wqueue->mutex);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_wqueue_create(apr_wqueue_t **wqueue, apr_pool_t *pool)
{
    apr_status_t rv;
    apr_wqueue_t *wq;

    wq = apr_pcalloc(pool, sizeof(*wq));
    if (wq == NULL) {
        return APR_ENOMEM;
    }

    rv = apr_thread_mutex_create(&wq->mutex, APR_THREAD_MUTEX_NESTED, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    rv = apr_thread_cond_create(&wq->cond, pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    *wqueue = wq;
    wq->pool = pool;
    APR_RING_INIT(&wq->ring, apr_wqueue_entry_t, link);
    apr_pool_cleanup_register(pool, wq, plain_cleanup, apr_pool_cleanup_null);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_wqueue_destroy(apr_wqueue_t *wqueue)
{
    return apr_pool_cleanup_run(wqueue->pool, wqueue, plain_cleanup);
}

static APR_INLINE void queue_lock(apr_wqueue_t *wqueue)
{
    apr_status_t rv = apr_thread_mutex_lock(wqueue->mutex);
}

static APR_INLINE void queue_unlock(apr_wqueue_t *wqueue)
{
    apr_status_t rv = apr_thread_mutex_unlock(wqueue->mutex);
}

APR_DECLARE(apr_status_t) apr_wqueue_add(apr_wqueue_t *wqueue, apr_wqueue_entry_t *entry)
{
    APR_RING_ELEM_INIT(entry, link);

    queue_lock(wqueue);
    APR_RING_INSERT_TAIL(&wqueue->ring, entry, apr_wqueue_entry_t, link);
    queue_unlock(wqueue);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_wqueue_remove(apr_wqueue_t *wqueue, apr_wqueue_entry_t *entry)
{
    queue_lock(wqueue);
    APR_RING_REMOVE(entry, link);
    queue_unlock(wqueue);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_wqueue_wake(apr_wqueue_t *wqueue, apr_uint32_t *nwake)
{
    apr_status_t rv;
    apr_int32_t count = *nwake;
    apr_wqueue_entry_t *entry, *next;

    *nwake = 0;

    queue_lock(wqueue);

    APR_RING_FOREACH_SAFE(entry, next, &wqueue->ring,
                          apr_wqueue_entry_t, link) {
        rv = entry->func(wqueue, entry->data);
        if (rv != APR_SUCCESS) {
            continue;
        }

        (*nwake)++;

        APR_RING_REMOVE(entry, link);

        if (count && !--count) {
            break;
        }
    }

    queue_unlock(wqueue);

    return APR_SUCCESS;
}

static apr_status_t wqueue_func(apr_wqueue_t *wqueue, void *data)
{
    return apr_thread_cond_signal(wqueue->cond);
}

APR_DECLARE(apr_status_t) apr_wqueue_wait(apr_wqueue_t *wqueue, apr_interval_time_t timeout)
{
    apr_status_t rv;
    apr_wqueue_entry_t entry;

    entry.func = wqueue_func;
    entry.data = NULL;

    APR_RING_ELEM_INIT(&entry, link);

    queue_lock(wqueue);

    APR_RING_INSERT_TAIL(&wqueue->ring, &entry, apr_wqueue_entry_t, link);

    if (timeout < 0) {
        rv = apr_thread_cond_wait(wqueue->cond, wqueue->mutex);
    } else {
        rv = apr_thread_cond_timedwait(wqueue->cond, wqueue->mutex, timeout);
    }

    APR_RING_REMOVE(&entry, link);

    queue_unlock(queue);

    return rv;
}
