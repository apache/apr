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

#include "apr_arch_thread_mutex.h"
#define APR_WANT_MEMFUNC
#include "apr_want.h"

#if APR_HAS_THREADS

static apr_status_t thread_mutex_cleanup(void *data)
{
    apr_thread_mutex_t *mutex = data;
    apr_status_t rv;

    rv = pthread_mutex_destroy(&mutex->mutex);
#ifdef PTHREAD_SETS_ERRNO
    if (rv) {
        rv = errno;
    }
#endif
    return rv;
} 

APR_DECLARE(apr_status_t) apr_thread_mutex_create(apr_thread_mutex_t **mutex,
                                                  unsigned int flags,
                                                  apr_pool_t *pool)
{
    apr_thread_mutex_t *new_mutex;
    apr_status_t rv;

    new_mutex = apr_pcalloc(pool, sizeof(apr_thread_mutex_t));

    new_mutex->pool = pool;

    /* Optimal default is APR_THREAD_MUTEX_UNNESTED, 
     * no additional checks required for either flag.
     */
    new_mutex->nested = flags & APR_THREAD_MUTEX_NESTED;

    if ((rv = pthread_mutex_init(&new_mutex->mutex, NULL))) {
#ifdef PTHREAD_SETS_ERRNO
        rv = errno;
#endif
        return rv;
    }

    apr_pool_cleanup_register(new_mutex->pool,
                              new_mutex, thread_mutex_cleanup,
                              apr_pool_cleanup_null);

    *mutex = new_mutex;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_lock(apr_thread_mutex_t *mutex)
{
    apr_status_t rv;

    if (mutex->nested) {
        /*
         * Although threadsafe, this test is NOT reentrant.  
         * The thread's main and reentrant attempts will both mismatch 
         * testing the mutex is owned by this thread, so a deadlock is expected.
         */
        if (apr_os_thread_equal(mutex->owner, apr_os_thread_current())) {
            apr_atomic_inc(&mutex->owner_ref);
            return APR_SUCCESS;
        }

        rv = pthread_mutex_lock(&mutex->mutex);
        if (rv) {
#ifdef PTHREAD_SETS_ERRNO
            rv = errno;
#endif
            return rv;
        }

        if (apr_atomic_cas(&mutex->owner_ref, 1, 0) != 0) {
            /* The owner_ref should be zero when the lock is not held,
             * if owner_ref was non-zero we have a mutex reference bug.
             * XXX: so now what?
             */
            mutex->owner_ref = 1;
        }
        /* Note; do not claim ownership until the owner_ref has been
         * incremented; limits a subtle race in reentrant code.
         */
        mutex->owner = apr_os_thread_current();
        return rv;
    }
    else {
        rv = pthread_mutex_lock(&mutex->mutex);
#ifdef PTHREAD_SETS_ERRNO
        if (rv) {
            rv = errno;
        }
#endif
        return rv;
    }
}

APR_DECLARE(apr_status_t) apr_thread_mutex_trylock(apr_thread_mutex_t *mutex)
{
    apr_status_t rv;

    if (mutex->nested) {
        /*
         * Although threadsafe, this test is NOT reentrant.  
         * The thread's main and reentrant attempts will both mismatch 
         * testing the mutex is owned by this thread, so one will fail 
         * the trylock.
         */
        if (apr_os_thread_equal(mutex->owner, apr_os_thread_current())) {
            apr_atomic_inc(&mutex->owner_ref);
            return APR_SUCCESS;
        }

        rv = pthread_mutex_trylock(&mutex->mutex);
        if (rv) {
#ifdef PTHREAD_SETS_ERRNO
            rv = errno;
#endif
            return (rv == EBUSY) ? APR_EBUSY : rv;
        }

        if (apr_atomic_cas(&mutex->owner_ref, 1, 0) != 0) {
            /* The owner_ref should be zero when the lock is not held,
             * if owner_ref was non-zero we have a mutex reference bug.
             * XXX: so now what?
             */
            mutex->owner_ref = 1;
        }
        /* Note; do not claim ownership until the owner_ref has been
         * incremented; limits a subtle race in reentrant code.
         */
        mutex->owner = apr_os_thread_current();
    }
    else {
        rv = pthread_mutex_trylock(&mutex->mutex);
        if (rv) {
#ifdef PTHREAD_SETS_ERRNO
            rv = errno;
#endif
            return (rv == EBUSY) ? APR_EBUSY : rv;
        }
    }

    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_unlock(apr_thread_mutex_t *mutex)
{
    apr_status_t status;

    if (mutex->nested) {
        /*
         * The code below is threadsafe and reentrant.
         */
        if (apr_os_thread_equal(mutex->owner, apr_os_thread_current())) {
            /*
             * This should never occur, and indicates an application error
             */
            if (mutex->owner_ref == 0) {
                return APR_EINVAL;
            }

            if (apr_atomic_dec(&mutex->owner_ref) != 0)
                return APR_SUCCESS;
            mutex->owner = 0;
        }
        /*
         * This should never occur, and indicates an application error
         */
        else {
            return APR_EINVAL;
        }
    }

    status = pthread_mutex_unlock(&mutex->mutex);
#ifdef PTHREAD_SETS_ERRNO
    if (status) {
        status = errno;
    }
#endif

    return status;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_destroy(apr_thread_mutex_t *mutex)
{
    return apr_pool_cleanup_run(mutex->pool, mutex, thread_mutex_cleanup);
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_mutex)

#endif /* APR_HAS_THREADS */
