/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include "thread_mutex.h"
#define APR_WANT_MEMFUNC
#include "apr_want.h"

#if APR_HAS_THREADS

static apr_status_t thread_mutex_cleanup(void *data)
{
    apr_thread_mutex_t *mutex = (apr_thread_mutex_t *)data;
    apr_status_t stat;

    pthread_mutex_unlock(&mutex->mutex);
    stat = pthread_mutex_destroy(&mutex->mutex);
#ifdef PTHREAD_SETS_ERRNO
    if (stat) {
        stat = errno;
    }
#endif
    return stat;
} 

APR_DECLARE(apr_status_t) apr_thread_mutex_create(apr_thread_mutex_t **mutex,
                                                  apr_pool_t *pool)
{
    apr_thread_mutex_t *new_mutex;
    pthread_mutexattr_t mattr;
    apr_status_t stat;

    new_mutex = (apr_thread_mutex_t *)apr_pcalloc(pool,
                                                  sizeof(apr_thread_mutex_t));

    if (new_mutex == NULL) {
        return APR_ENOMEM;
    }

    new_mutex->pool = pool;

    if ((stat = pthread_mutexattr_init(&mattr))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        thread_mutex_cleanup(new_mutex);
        return stat;
    }

    if ((stat = pthread_mutex_init(&new_mutex->mutex, &mattr))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        thread_mutex_cleanup(new_mutex);
        return stat;
    }

    if ((stat = pthread_mutexattr_destroy(&mattr))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        thread_mutex_cleanup(new_mutex);
        return stat;
    }

    apr_pool_cleanup_register(new_mutex->pool,
                              (void *)new_mutex, thread_mutex_cleanup,
                              apr_pool_cleanup_null);

    *mutex = new_mutex;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_lock(apr_thread_mutex_t *mutex)
{
    apr_status_t stat;

#if APR_HAS_THREADS
    apr_os_thread_t my_thrid; /* save one call to apr_os_thread_current() */

    if (apr_os_thread_equal(mutex->owner,
                            (my_thrid = apr_os_thread_current()))) {
        mutex->owner_ref++;
        return APR_SUCCESS;
    }
#endif

    stat = pthread_mutex_lock(&mutex->mutex);
    if (stat) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        return stat;
    }

#if APR_HAS_THREADS
    mutex->owner = my_thrid;
    mutex->owner_ref = 1;
#endif

    return stat;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_trylock(apr_thread_mutex_t *mutex)
{
    apr_status_t stat;

#if APR_HAS_THREADS
    apr_os_thread_t my_thrid; /* save one call to apr_os_thread_current() */

    if (apr_os_thread_equal(mutex->owner,
                            (my_thrid = apr_os_thread_current()))) {
        mutex->owner_ref++;
        return APR_SUCCESS;
    }
#endif

    stat = pthread_mutex_trylock(&mutex->mutex);
    if (stat) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        /* Normalize the return code. */
        if (stat == EBUSY)
            stat = APR_EBUSY;

        return stat;
    }

#if APR_HAS_THREADS
    mutex->owner = my_thrid;
    mutex->owner_ref = 1;
#endif

    return stat;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_unlock(apr_thread_mutex_t *mutex)
{
    apr_status_t status;

#if APR_HAS_THREADS
    if (apr_os_thread_equal(mutex->owner, apr_os_thread_current())) {
        mutex->owner_ref--;
        if (mutex->owner_ref > 0)
            return APR_SUCCESS;
    }
#endif

    status = pthread_mutex_unlock(&mutex->mutex);
    if (status) {
#ifdef PTHREAD_SETS_ERRNO
        status = errno;
#endif
        return status;
    }

#if APR_HAS_THREADS
    memset(&mutex->owner, 0, sizeof mutex->owner);
    mutex->owner_ref = 0;
#endif
    
    return status;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_destroy(apr_thread_mutex_t *mutex)
{
    apr_status_t stat;
    if ((stat = thread_mutex_cleanup(mutex)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(mutex->pool, mutex, thread_mutex_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

#endif /* APR_HAS_THREADS */
