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

/*Read/Write locking implementation based on the MultiLock code from
 * Stephen Beaulieu <hippo@be.com>
 */
 
#include "beos/locks.h"
#include "apr_strings.h"
#include "apr_portable.h"

#define BIG_NUM 100000

static apr_status_t _lock_cleanup(void * data)
{
    apr_lock_t *lock = (apr_lock_t*)data;
    if (lock->LockCount != 0) {
        /* we're still locked... */
    	while (atomic_add(&lock->LockCount , -1) > 1){
    	    /* OK we had more than one person waiting on the lock so 
    	     * the sem is also locked. Release it until we have no more
    	     * locks left.
    	     */
            release_sem (lock->Lock);
    	}
    }
    delete_sem(lock->Lock);
    return APR_SUCCESS;
}    

static apr_status_t _lock_rw_cleanup(void * data)
{
    apr_lock_t *lock = (apr_lock_t*)data;

    if (lock->ReadCount != 0) {
    	while (atomic_add(&lock->ReadCount , -1) > 1){
            release_sem (lock->Read);
    	}
    }
    if (lock->WriteCount != 0) {
    	while (atomic_add(&lock->WriteCount , -1) > 1){
            release_sem (lock->Write);
    	}
    }
    if (lock->LockCount != 0) {
    	while (atomic_add(&lock->LockCount , -1) > 1){
            release_sem (lock->Lock);
    	}
    }
    
    delete_sem(lock->Read);
    delete_sem(lock->Write);
    delete_sem(lock->Lock);
    return APR_SUCCESS;
}    

static apr_status_t _create_lock(apr_lock_t *new)
{
    int32 stat;
    
    if ((stat = create_sem(0, "APR_Lock")) < B_NO_ERROR) {
        _lock_cleanup(new);
        return stat;
    }
    new->LockCount = 0;
    new->Lock = stat;
    apr_pool_cleanup_register(new->pool, (void *)new, _lock_cleanup,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

static apr_status_t _create_rw_lock(apr_lock_t *new)
{
    /* we need to make 3 locks... */
    new->ReadCount = 0;
    new->WriteCount = 0;
    new->LockCount = 0;
    new->Read  = create_sem(0, "APR_ReadLock");
    new->Write = create_sem(0, "APR_WriteLock");
    new->Lock  = create_sem(0, "APR_Lock");
    
    if (new->Lock < 0 || new->Read < 0 || new->Write < 0) {
        _lock_rw_cleanup(new);
        return -1;
    }

    apr_pool_cleanup_register(new->pool, (void *)new, _lock_rw_cleanup,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

static apr_status_t _lock(apr_lock_t *lock)
{
    int32 stat;
    
	if (atomic_add(&lock->LockCount, 1) > 0) {
		if ((stat = acquire_sem(lock->Lock)) < B_NO_ERROR) {
		    atomic_add(&lock->LockCount, -1);
		    return stat;
		}
	}
    return APR_SUCCESS;
}

static apr_status_t _unlock(apr_lock_t *lock)
{
    int32 stat;
    
	if (atomic_add(&lock->LockCount, -1) > 1) {
        if ((stat = release_sem(lock->Lock)) < B_NO_ERROR) {
            atomic_add(&lock->LockCount, 1);
            return stat;
        }
    }
    return APR_SUCCESS;
}

static apr_status_t _read_lock(apr_lock_t *lock)
{
    int32 rv = APR_SUCCESS;

    if (find_thread(NULL) == lock->writer) {
        /* we're the writer - no problem */
        lock->Nested++;
    } else {
        /* we're not the writer */
        int32 r = atomic_add(&lock->ReadCount, 1);
        if (r < 0) {
            /* Oh dear, writer holds lock, wait for sem */
            rv = acquire_sem_etc(lock->Read, 1, B_DO_NOT_RESCHEDULE,
                                 B_INFINITE_TIMEOUT);
        }
    }

    return rv;
}

static apr_status_t _write_lock(apr_lock_t *lock)
{
    int rv = APR_SUCCESS;

    if (find_thread(NULL) == lock->writer) {
        lock->Nested++;
    } else {
        /* we're not the writer... */
        if (atomic_add(&lock->LockCount, 1) >= 1) {
            /* we're locked - acquire the sem */
            rv = acquire_sem_etc(lock->Lock, 1, B_DO_NOT_RESCHEDULE,
                                 B_INFINITE_TIMEOUT);
        }
        if (rv == APR_SUCCESS) {
            /* decrement the ReadCount to a large -ve number so that
             * we block on new readers...
             */
            int32 readers = atomic_add(&lock->ReadCount, -BIG_NUM);
            if (readers > 0) {
                /* readers are holding the lock */
                rv = acquire_sem_etc(lock->Write, readers, B_DO_NOT_RESCHEDULE,
                                     B_INFINITE_TIMEOUT);
            }
            if (rv == APR_SUCCESS)
                lock->writer = find_thread(NULL);
        }
    }
    
    return rv;
}


static apr_status_t _read_unlock(apr_lock_t *lock)
{
    apr_status_t rv = APR_SUCCESS;
    
    /* we know we hold the lock, so don't check it :) */
    if (find_thread(NULL) == lock->writer) {
        /* we're recursively locked */
        lock->Nested--;
        return APR_SUCCESS;
    }
    /* OK so we need to release the sem if we have it :) */
    if (atomic_add(&lock->ReadCount, -1) < 0) {
        /* we have a writer waiting for the lock, so release it */
        rv = release_sem_etc(lock->Write, 1, B_DO_NOT_RESCHEDULE);
    }

    return rv;
}

static apr_status_t _write_unlock(apr_lock_t *lock)
{
    apr_status_t rv = APR_SUCCESS;
    int32 readers;
    
    /* we know we hold the lock, so don't check it :) */
    if (lock->Nested > 1) {
        /* we're recursively locked */
        lock->Nested--;
        return APR_SUCCESS;
    }
    /* OK so we need to release the sem if we have it :) */
    readers = atomic_add(&lock->ReadCount, BIG_NUM) + BIG_NUM;
    if (readers > 0) {
        rv = release_sem_etc(lock->Read, readers, B_DO_NOT_RESCHEDULE);
    }
    if (rv == APR_SUCCESS) {
        lock->writer = -1;
        if (atomic_add(&lock->LockCount, -1) > 1) {
            rv = release_sem_etc(lock->Lock, 1, B_DO_NOT_RESCHEDULE);
        }
    }
    
    return rv;
}

static apr_status_t _destroy_lock(apr_lock_t *lock)
{
    apr_status_t stat;
    if ((stat = _lock_cleanup(lock)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(lock->pool, lock, _lock_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

APR_DECLARE(apr_status_t) apr_lock_create(apr_lock_t **lock, apr_locktype_e type, 
                                          apr_lockscope_e scope, const char *fname, 
                                          apr_pool_t *pool)
{
    apr_lock_t *new;
    apr_status_t stat = APR_SUCCESS;
  
    new = (apr_lock_t *)apr_pcalloc(pool, sizeof(apr_lock_t));
    if (new == NULL){
        return APR_ENOMEM;
    }
    
    new->pool  = pool;
    new->type  = type;
    new->scope = scope;

    if (type == APR_MUTEX) {
        stat = _create_lock(new);
    } else if (type == APR_READWRITE) {
        stat = _create_rw_lock(new);
    }
            
    if (stat != APR_SUCCESS)
        return stat;

    (*lock) = new;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_acquire(apr_lock_t *lock)
{
    apr_status_t stat;

    if (lock->owner == apr_os_thread_current()) {
        lock->owner_ref++;
        return APR_SUCCESS;
    }

    switch (lock->type)
    {
    case APR_MUTEX:
        if ((stat = _lock(lock)) != APR_SUCCESS)
            return stat;
        break;

    case APR_READWRITE:
        return APR_ENOTIMPL;
    }

    lock->owner = apr_os_thread_current();
    lock->owner_ref = 1;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_tryacquire(apr_lock_t *lock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_lock_acquire_rw(apr_lock_t *lock, apr_readerwriter_e e)
{
    switch (lock->type)
    {
    case APR_MUTEX:
        return APR_ENOTIMPL;
    case APR_READWRITE:
        switch (e)
        {
        case APR_READER:
            _read_lock(lock);
            break;
        case APR_WRITER:
            _write_lock(lock);
            break;
        }
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_release(apr_lock_t *lock)
{
    apr_status_t stat = APR_SUCCESS;

    if (lock->owner_ref > 0 && lock->owner == apr_os_thread_current()) {
        lock->owner_ref--;
        if (lock->owner_ref > 0)
            return APR_SUCCESS;
    }

    switch (lock->type)
    {
    case APR_MUTEX:
        stat = _unlock(lock);
        break;
    case APR_READWRITE:
        {
            thread_id me = find_thread(NULL);
            if (me == lock->writer)
                stat = _write_unlock(lock);
            else
                stat = _read_unlock(lock);
        }
        /* if we don't hold the read or write lock then why are
         * we calling release???
         *
         * Just return success.
         */
        break;
    }

    if (stat != APR_SUCCESS)
        return stat;
    
    lock->owner = -1;
    lock->owner_ref = 0;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_destroy(apr_lock_t *lock)
{
    apr_status_t stat; 

    switch (lock->type)
    {
    case APR_MUTEX:
        if ((stat = _destroy_lock(lock)) != APR_SUCCESS)
            return stat;
        break;
    case APR_READWRITE:
        return APR_ENOTIMPL;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_child_init(apr_lock_t **lock, const char *fname, 
			                                  apr_pool_t *pool)
{
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_data_get(apr_lock_t *lock, 
                                            const char *key, void *data)
{
    return apr_pool_userdata_get(data, key, lock->pool);
}

APR_DECLARE(apr_status_t) apr_lock_data_set(apr_lock_t *lock, 
                                            void *data, const char *key,
                                            apr_status_t (*cleanup) (void *))
{
    return apr_pool_userdata_set(data, key, cleanup, lock->pool);
}

APR_DECLARE(apr_status_t) apr_os_lock_get(apr_os_lock_t *oslock, apr_lock_t *lock)
{
    oslock->sem = lock->Lock;
    oslock->ben = lock->LockCount;
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
    (*lock)->Lock = thelock->sem;
    (*lock)->LockCount = thelock->ben;

    return APR_SUCCESS;
}

