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

#include "apr.h"
#include "apr_private.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_arch_thread_rwlock.h"
#include "apr_portable.h"

APR_DECLARE(apr_status_t) apr_thread_rwlock_create(apr_thread_rwlock_t **rwlock_p,
                                                   apr_pool_t *pool)
{
    apr_thread_rwlock_t* rwlock = apr_palloc(pool, sizeof(*rwlock));

    rwlock->pool = pool;
    rwlock->has_wrlock = 0;
    InitializeSRWLock(&rwlock->lock);

    *rwlock_p = rwlock;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_rdlock(apr_thread_rwlock_t *rwlock)
{
    AcquireSRWLockShared(&rwlock->lock);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t)
apr_thread_rwlock_tryrdlock(apr_thread_rwlock_t *rwlock)
{
    if (!TryAcquireSRWLockShared(&rwlock->lock)) {
        return APR_EBUSY;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_wrlock(apr_thread_rwlock_t *rwlock)
{
    AcquireSRWLockExclusive(&rwlock->lock);
    rwlock->has_wrlock = 1;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t)apr_thread_rwlock_trywrlock(apr_thread_rwlock_t *rwlock)
{
    if (!TryAcquireSRWLockExclusive(&rwlock->lock)) {
        return APR_EBUSY;
    }

    rwlock->has_wrlock = 1;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_unlock(apr_thread_rwlock_t *rwlock)
{
    if (rwlock->has_wrlock) {
        /* Someone holds the write lock. It MUST be the calling thread, since
           the caller wants to unlock. */
        rwlock->has_wrlock = 0;
        ReleaseSRWLockExclusive(&rwlock->lock);
    }
    else {
        /* Calling thread holds the read lock. */
        ReleaseSRWLockShared(&rwlock->lock);
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_destroy(apr_thread_rwlock_t *rwlock)
{
    /* SRW lock is statically allocated. */
    return APR_SUCCESS;
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_rwlock)
