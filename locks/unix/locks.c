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
#include "apr_strings.h"
#include "apr_portable.h"

static apr_status_t lockall_create(apr_lock_t *new, const char *fname)
{
    apr_status_t rv;

    if ((rv = new->inter_meth->create(new, fname)) != APR_SUCCESS) {
        return rv;
    }
    if ((rv = new->intra_meth->create(new, fname)) != APR_SUCCESS) {
        return rv;
    }
    return APR_SUCCESS;
}

static apr_status_t lockall_acquire(apr_lock_t *lock)
{
    apr_status_t rv;

    if ((rv = lock->intra_meth->acquire(lock)) != APR_SUCCESS) {
        return rv;
    }
    if ((rv = lock->inter_meth->acquire(lock)) != APR_SUCCESS) {
        return rv;
    }
    return APR_SUCCESS;
}

static apr_status_t lockall_release(apr_lock_t *lock)
{
    apr_status_t rv;

    if ((rv = lock->intra_meth->release(lock)) != APR_SUCCESS) {
        return rv;
    }
    if ((rv = lock->inter_meth->release(lock)) != APR_SUCCESS) {
        return rv;
    }
    return APR_SUCCESS;
}

static apr_status_t lockall_destroy(apr_lock_t *lock)
{
    apr_status_t rv;

    if ((rv = lock->intra_meth->destroy(lock)) != APR_SUCCESS) {
        return rv;
    }
    if ((rv = lock->inter_meth->destroy(lock)) != APR_SUCCESS) {
        return rv;
    }
    return APR_SUCCESS;
}

static apr_status_t lockall_child_init(apr_lock_t **lock, apr_pool_t *pool,
                                       const char *fname)
{
    /* no child init for intra lock */
    return (*lock)->inter_meth->child_init(lock, pool, fname);
}

static const struct apr_unix_lock_methods_t lockall_methods =
{
    0,
    lockall_create,
    lockall_acquire,
    NULL, /* no tryacquire concept */
    NULL, /* no read lock concept */
    NULL, /* no write lock concept */
    lockall_release,
    lockall_destroy,
    lockall_child_init
};

static apr_status_t choose_method(apr_lock_t *new, apr_lockmech_e mech)
{
    switch (mech) {
    case APR_LOCK_FCNTL:
#if APR_HAS_FCNTL_SERIALIZE
        new->inter_meth = &apr_unix_fcntl_methods;
#else
        return APR_ENOTIMPL;
#endif
        break;
    case APR_LOCK_FLOCK:
#if APR_HAS_FLOCK_SERIALIZE
        new->inter_meth = &apr_unix_flock_methods;
#else
        return APR_ENOTIMPL;
#endif
        break;
    case APR_LOCK_SYSVSEM:
#if APR_HAS_SYSVSEM_SERIALIZE
        new->inter_meth = &apr_unix_sysv_methods;
#else
        return APR_ENOTIMPL;
#endif
        break;
    case APR_LOCK_PROC_PTHREAD:
#if APR_HAS_PROC_PTHREAD_SERIALIZE
        new->inter_meth = &apr_unix_proc_pthread_methods;
#else
        return APR_ENOTIMPL;
#endif
        break;
    case APR_LOCK_DEFAULT:
#if APR_USE_FLOCK_SERIALIZE
        new->inter_meth = &apr_unix_flock_methods;
#elif APR_USE_SYSVSEM_SERIALIZE
        new->inter_meth = &apr_unix_sysv_methods;
#elif APR_USE_FCNTL_SERIALIZE
        new->inter_meth = &apr_unix_fcntl_methods;
#elif APR_USE_PROC_PTHREAD_SERIALIZE
        new->inter_meth = &apr_unix_proc_pthread_methods;
#else
        return APR_ENOTIMPL;
#endif
        break;
    default:
        return APR_ENOTIMPL;
    }
    return APR_SUCCESS;
}

static apr_status_t create_lock(apr_lock_t *new, apr_lockmech_e mech, const char *fname)
{
    apr_status_t stat;

    if (new->scope != APR_INTRAPROCESS) {
        if ((stat = choose_method(new, mech)) != APR_SUCCESS) {
            return stat;
        }
    }

    if (new->scope != APR_CROSS_PROCESS) {
#if APR_HAS_THREADS
        if (new->type == APR_READWRITE) {
#if APR_HAS_RWLOCK_SERIALIZE
            new->intra_meth = &apr_unix_rwlock_methods;
#else
            return APR_ENOTIMPL; /* 'cause we don't have rwlocks */
#endif
        }
        else {
            new->intra_meth = &apr_unix_intra_methods;
        }
#else
        if (new->scope == APR_INTRAPROCESS) {
            return APR_ENOTIMPL; /* 'cause we don't have threads */
        }
#endif
    }

    switch (new->scope) {
    case APR_LOCKALL:
        if (new->inter_meth->flags & APR_PROCESS_LOCK_MECH_IS_GLOBAL) {
            new->meth = new->inter_meth;
        }
        else {
            new->meth = &lockall_methods;
        }
        break;
    case APR_CROSS_PROCESS:
        new->meth = new->inter_meth;
        break;
    case APR_INTRAPROCESS:
        new->meth = new->intra_meth;
    }

    if ((stat = new->meth->create(new, fname)) != APR_SUCCESS) {
        return stat;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_create(apr_lock_t **lock, apr_locktype_e type, 
                                          apr_lockscope_e scope, apr_lockmech_e mech,
                                          const char *fname, apr_pool_t *pool)
{
    apr_lock_t *new;
    apr_status_t stat;

    new = (apr_lock_t *)apr_pcalloc(pool, sizeof(apr_lock_t));

    new->pool  = pool;
    new->type  = type;
    new->scope = scope;
#if APR_HAS_SYSVSEM_SERIALIZE || APR_HAS_FCNTL_SERIALIZE || APR_HAS_FLOCK_SERIALIZE
    new->interproc = NULL;
#endif

    if ((stat = create_lock(new, mech, fname)) != APR_SUCCESS)
        return stat;

    *lock = new;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_acquire(apr_lock_t *lock)
{
    apr_status_t stat;

#if APR_HAS_THREADS
    if (apr_os_thread_equal(lock->owner, apr_os_thread_current())) {
        lock->owner_ref++;
        return APR_SUCCESS;
    }
#endif

    if ((stat = lock->meth->acquire(lock)) != APR_SUCCESS) {
        return stat;
    }

#if APR_HAS_THREADS
    lock->owner = apr_os_thread_current();
    lock->owner_ref = 1;
#endif

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_tryacquire(apr_lock_t *lock)
{
    apr_status_t stat;

#if APR_HAS_THREADS
    if (apr_os_thread_equal(lock->owner, apr_os_thread_current())) {
        lock->owner_ref++;
        return APR_SUCCESS;
    }
#endif

    if ((stat = lock->meth->tryacquire(lock)) != APR_SUCCESS) {
        return stat;
    }

#if APR_HAS_THREADS
    lock->owner = apr_os_thread_current();
    lock->owner_ref = 1;
#endif

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_acquire_rw(apr_lock_t *lock, 
                                              apr_readerwriter_e e)
{
    switch (e)
    {
    case APR_READER:
        return lock->meth->acquire_read(lock);
    case APR_WRITER:
        return lock->meth->acquire_write(lock);
    }
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_lock_release(apr_lock_t *lock)
{
    apr_status_t stat;

#if APR_HAS_THREADS
    if (apr_os_thread_equal(lock->owner, apr_os_thread_current())) {
        lock->owner_ref--;
        if (lock->owner_ref > 0)
            return APR_SUCCESS;
    }
#endif

    if ((stat = lock->meth->release(lock)) != APR_SUCCESS) {
        return stat;
    }

#if APR_HAS_THREADS
    memset(&lock->owner, 0, sizeof lock->owner);
    lock->owner_ref = 0;
#endif
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_destroy(apr_lock_t *lock)
{
    return lock->meth->destroy(lock);
}

APR_DECLARE(apr_status_t) apr_lock_child_init(apr_lock_t **lock, const char *fname, 
                               apr_pool_t *cont)
{
    if ((*lock)->scope != APR_INTRAPROCESS)
        return (*lock)->meth->child_init(lock, cont, fname);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_data_get(apr_lock_t *lock, const char *key, void *data)
{
    return apr_pool_userdata_get(data, key, lock->pool);
}

APR_DECLARE(apr_status_t) apr_lock_data_set(apr_lock_t *lock, void *data, const char *key,
                            apr_status_t (*cleanup) (void *))
{
    return apr_pool_userdata_set(data, key, cleanup, lock->pool);
}

APR_DECLARE(apr_status_t) apr_os_lock_get(apr_os_lock_t *oslock, apr_lock_t *lock)
{
#if APR_HAS_SYSVSEM_SERIALIZE || APR_HAS_FCNTL_SERIALIZE || APR_HAS_FLOCK_SERIALIZE
    oslock->crossproc = lock->interproc->filedes;
#endif
#if APR_HAS_PROC_PTHREAD_SERIALIZE
    oslock->pthread_interproc = lock->pthread_interproc;
#endif
#if APR_HAS_THREADS
#if APR_USE_PTHREAD_SERIALIZE
    oslock->intraproc = lock->intraproc;
#endif
#endif

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_lock_put(apr_lock_t **lock, apr_os_lock_t *thelock, 
                                          apr_pool_t *pool)
{
    if (pool == NULL) {
        return APR_ENOPOOL;
    }
    if ((*lock) == NULL) {
        (*lock) = (apr_lock_t *)apr_pcalloc(pool, sizeof(apr_lock_t));
        (*lock)->pool = pool;
    }
#if APR_HAS_SYSVSEM_SERIALIZE || APR_HAS_FCNTL_SERIALIZE || APR_HAS_FLOCK_SERIALIZE
    apr_os_file_put(&(*lock)->interproc, &thelock->crossproc, pool);
#endif
#if APR_HAS_PROC_PTHREAD_SERIALIZE
    (*lock)->pthread_interproc = thelock->pthread_interproc;
#endif
#if APR_HAS_THREADS
#if (APR_USE_PTHREAD_SERIALIZE)
    (*lock)->intraproc = thelock->intraproc;
#endif
#endif
    return APR_SUCCESS;
}
 
