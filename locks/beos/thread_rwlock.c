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

/*Read/Write locking implementation based on the MultiLock code from
 * Stephen Beaulieu <hippo@be.com>
 */
 
#include "beos/apr_arch_thread_rwlock.h"
#include "apr_strings.h"
#include "apr_portable.h"

#define BIG_NUM 100000

static apr_status_t _thread_rw_cleanup(void * data)
{
    apr_thread_rwlock_t *mutex = (apr_thread_rwlock_t*)data;

    if (mutex->ReadCount != 0) {
    	while (atomic_add(&mutex->ReadCount , -1) > 1){
            release_sem (mutex->Read);
    	}
    }
    if (mutex->WriteCount != 0) {
    	while (atomic_add(&mutex->WriteCount , -1) > 1){
            release_sem (mutex->Write);
    	}
    }
    if (mutex->LockCount != 0) {
    	while (atomic_add(&mutex->LockCount , -1) > 1){
            release_sem (mutex->Lock);
    	}
    }
    
    delete_sem(mutex->Read);
    delete_sem(mutex->Write);
    delete_sem(mutex->Lock);
    return APR_SUCCESS;
}    

APR_DECLARE(apr_status_t) apr_thread_rwlock_create(apr_thread_rwlock_t **rwlock,
                                                   apr_pool_t *pool)
{
    apr_thread_rwlock_t *new;
  
    new = (apr_thread_rwlock_t *)apr_pcalloc(pool, sizeof(apr_thread_rwlock_t));
    if (new == NULL){
        return APR_ENOMEM;
    }
    
    new->pool  = pool;
    /* we need to make 3 locks... */
    new->ReadCount = 0;
    new->WriteCount = 0;
    new->LockCount = 0;
    new->Read  = create_sem(0, "APR_ReadLock");
    new->Write = create_sem(0, "APR_WriteLock");
    new->Lock  = create_sem(0, "APR_Lock");
    
    if (new->Lock < 0 || new->Read < 0 || new->Write < 0) {
        _thread_rw_cleanup(new);
        return -1;
    }

    apr_pool_cleanup_register(new->pool, (void *)new, _thread_rw_cleanup,
                              apr_pool_cleanup_null);
    (*rwlock) = new;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_rdlock(apr_thread_rwlock_t *rwlock)
{
    int32 rv = APR_SUCCESS;

    if (find_thread(NULL) == rwlock->writer) {
        /* we're the writer - no problem */
        rwlock->Nested++;
    } else {
        /* we're not the writer */
        int32 r = atomic_add(&rwlock->ReadCount, 1);
        if (r < 0) {
            /* Oh dear, writer holds lock, wait for sem */
            rv = acquire_sem_etc(rwlock->Read, 1, B_DO_NOT_RESCHEDULE,
                                 B_INFINITE_TIMEOUT);
        }
    }

    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_tryrdlock(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_wrlock(apr_thread_rwlock_t *rwlock)
{
    int rv = APR_SUCCESS;

    if (find_thread(NULL) == rwlock->writer) {
        rwlock->Nested++;
    } else {
        /* we're not the writer... */
        if (atomic_add(&rwlock->LockCount, 1) >= 1) {
            /* we're locked - acquire the sem */
            rv = acquire_sem_etc(rwlock->Lock, 1, B_DO_NOT_RESCHEDULE,
                                 B_INFINITE_TIMEOUT);
        }
        if (rv == APR_SUCCESS) {
            /* decrement the ReadCount to a large -ve number so that
             * we block on new readers...
             */
            int32 readers = atomic_add(&rwlock->ReadCount, -BIG_NUM);
            if (readers > 0) {
                /* readers are holding the lock */
                rv = acquire_sem_etc(rwlock->Write, readers, B_DO_NOT_RESCHEDULE,
                                     B_INFINITE_TIMEOUT);
            }
            if (rv == APR_SUCCESS)
                rwlock->writer = find_thread(NULL);
        }
    }
    
    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_trywrlock(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_unlock(apr_thread_rwlock_t *rwlock)
{
    apr_status_t rv = APR_SUCCESS;
    int32 readers;

    /* we know we hold the lock, so don't check it :) */
    if (find_thread(NULL) == rwlock->writer) {
    /* we know we hold the lock, so don't check it :) */
        if (rwlock->Nested > 1) {
            /* we're recursively locked */
            rwlock->Nested--;
            return APR_SUCCESS;
        }
        /* OK so we need to release the sem if we have it :) */
        readers = atomic_add(&rwlock->ReadCount, BIG_NUM) + BIG_NUM;
        if (readers > 0) {
            rv = release_sem_etc(rwlock->Read, readers, B_DO_NOT_RESCHEDULE);
        }
        if (rv == APR_SUCCESS) {
            rwlock->writer = -1;
            if (atomic_add(&rwlock->LockCount, -1) > 1) {
                rv = release_sem_etc(rwlock->Lock, 1, B_DO_NOT_RESCHEDULE);
            }
        }
    } else {
       /* We weren't the Writer, so just release the ReadCount... */
       if (atomic_add(&rwlock->ReadCount, -1) < 0) {
            /* we have a writer waiting for the lock, so release it */
            rv = release_sem_etc(rwlock->Write, 1, B_DO_NOT_RESCHEDULE);
        }
    }

    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_destroy(apr_thread_rwlock_t *rwlock)
{
    apr_status_t stat;
    if ((stat = _thread_rw_cleanup(rwlock)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(rwlock->pool, rwlock, _thread_rw_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_rwlock)

