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

#include "locks.h"

#if APR_HAS_THREADS

#if (APR_USE_PTHREAD_SERIALIZE)  

static apr_status_t lock_intra_cleanup(void *data)
{
    apr_lock_t *lock = (apr_lock_t *) data;
    apr_status_t stat;

    pthread_mutex_unlock(lock->intraproc);
    stat = pthread_mutex_destroy(lock->intraproc);
#ifdef PTHREAD_SETS_ERRNO
    if (stat) {
        stat = errno;
    }
#endif
    return stat;
}    

static apr_status_t intra_create(apr_lock_t *new, const char *fname)
{
    apr_status_t stat;
    pthread_mutexattr_t mattr;

    new->intraproc = (pthread_mutex_t *)apr_palloc(new->pool, 
                                                   sizeof(pthread_mutex_t));
    if (new->intraproc == NULL ) {
        return errno;
    }
    if ((stat = pthread_mutexattr_init(&mattr))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        lock_intra_cleanup(new);
        return stat;
    }

    if ((stat = pthread_mutex_init(new->intraproc, &mattr))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        lock_intra_cleanup(new);
        return stat;
    }

    if ((stat = pthread_mutexattr_destroy(&mattr))) {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        lock_intra_cleanup(new);
        return stat;
    }

    new->curr_locked = 0;
    apr_pool_cleanup_register(new->pool, (void *)new, lock_intra_cleanup,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

static apr_status_t intra_acquire(apr_lock_t *lock)
{
    apr_status_t stat;

    stat = pthread_mutex_lock(lock->intraproc);
#ifdef PTHREAD_SETS_ERRNO
    if (stat) {
        stat = errno;
    }
#endif
    return stat;
}

static apr_status_t intra_release(apr_lock_t *lock)
{
    apr_status_t status;

    status = pthread_mutex_unlock(lock->intraproc);
#ifdef PTHREAD_SETS_ERRNO
    if (status) {
        status = errno;
    }
#endif
    return status;
}

static apr_status_t intra_destroy(apr_lock_t *lock)
{
    apr_status_t stat;
    if ((stat = lock_intra_cleanup(lock)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(lock->pool, lock, lock_intra_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

#endif /* APR_USE_PTHREAD_SERIALIZE */

const apr_unix_lock_methods_t apr_unix_intra_methods =
{
    0,
    intra_create,
    intra_acquire,
    NULL, /* no read lock concept */
    NULL, /* no write lock concept */
    intra_release,
    intra_destroy,
    NULL /* no child init */
};

#if APR_HAS_RWLOCK_SERIALIZE
static apr_status_t rwlock_create(apr_lock_t *new, const char *fname)
{
    /* XXX check retcode */
    pthread_rwlock_init(&new->rwlock, NULL);
    return APR_SUCCESS;
}

static apr_status_t rwlock_acquire_read(apr_lock_t *lock)
{
    /* XXX PTHREAD_SETS_ERRNO crap? */
    return pthread_rwlock_rdlock(&lock->rwlock);
}

static apr_status_t rwlock_acquire_write(apr_lock_t *lock)
{
    /* XXX PTHREAD_SETS_ERRNO crap? */
    return pthread_rwlock_wrlock(&lock->rwlock);
}

static apr_status_t rwlock_release(apr_lock_t *lock)
{
    /* XXX PTHREAD_SETS_ERRNO crap? */
    return pthread_rwlock_unlock(&lock->rwlock);
}

static apr_status_t rwlock_destroy(apr_lock_t *lock)
{
    /* XXX PTHREAD_SETS_ERRNO crap? */
    return pthread_rwlock_destroy(&lock->rwlock);
}

const apr_unix_lock_methods_t apr_unix_rwlock_methods =
{
    0,
    rwlock_create,
    NULL, /* no standard acquire method; app better not call :) */
    rwlock_acquire_read,
    rwlock_acquire_write,
    rwlock_release,
    rwlock_destroy,
    NULL /* no child init method */
};
#endif

#endif /* APR_HAS_THREADS */
