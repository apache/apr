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
 
#include "beos/apr_arch_thread_mutex.h"
#include "apr_strings.h"
#include "apr_portable.h"

static apr_status_t _thread_mutex_cleanup(void * data)
{
    apr_thread_mutex_t *lock = (apr_thread_mutex_t*)data;
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

APR_DECLARE(apr_status_t) apr_thread_mutex_create(apr_thread_mutex_t **mutex,
                                                  unsigned int flags,
                                                  apr_pool_t *pool)
{
    apr_thread_mutex_t *new_m;
    apr_status_t stat = APR_SUCCESS;
  
    new_m = (apr_thread_mutex_t *)apr_pcalloc(pool, sizeof(apr_thread_mutex_t));
    if (new_m == NULL){
        return APR_ENOMEM;
    }
    
    if ((stat = create_sem(0, "APR_Lock")) < B_NO_ERROR) {
        _thread_mutex_cleanup(new_m);
        return stat;
    }
    new_m->LockCount = 0;
    new_m->Lock = stat;  
    new_m->pool  = pool;

    /* Optimal default is APR_THREAD_MUTEX_UNNESTED, 
     * no additional checks required for either flag.
     */
    new_m->nested = flags & APR_THREAD_MUTEX_NESTED;

    apr_pool_cleanup_register(new_m->pool, (void *)new_m, _thread_mutex_cleanup,
                              apr_pool_cleanup_null);

    (*mutex) = new_m;
    return APR_SUCCESS;
}

#if APR_HAS_CREATE_LOCKS_NP
APR_DECLARE(apr_status_t) apr_thread_mutex_create_np(apr_thread_mutex_t **mutex,
                                                   const char *fname,
                                                   apr_lockmech_e_np mech,
                                                   apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}       
#endif
  
APR_DECLARE(apr_status_t) apr_thread_mutex_lock(apr_thread_mutex_t *mutex)
{
    int32 stat;
    thread_id me = find_thread(NULL);
    
    if (mutex->nested && mutex->owner == me) {
        mutex->owner_ref++;
        return APR_SUCCESS;
    }
    
	if (atomic_add(&mutex->LockCount, 1) > 0) {
		if ((stat = acquire_sem(mutex->Lock)) < B_NO_ERROR) {
            /* Oh dear, acquire_sem failed!!  */
		    atomic_add(&mutex->LockCount, -1);
		    return stat;
		}
	}

    mutex->owner = me;
    mutex->owner_ref = 1;
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_trylock(apr_thread_mutex_t *mutex)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_unlock(apr_thread_mutex_t *mutex)
{
    int32 stat;
        
    if (mutex->nested && mutex->owner == find_thread(NULL)) {
        mutex->owner_ref--;
        if (mutex->owner_ref > 0)
            return APR_SUCCESS;
    }
    
	if (atomic_add(&mutex->LockCount, -1) > 1) {
        if ((stat = release_sem(mutex->Lock)) < B_NO_ERROR) {
            atomic_add(&mutex->LockCount, 1);
            return stat;
        }
    }

    mutex->owner = -1;
    mutex->owner_ref = 0;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_destroy(apr_thread_mutex_t *mutex)
{
    apr_status_t stat;
    if ((stat = _thread_mutex_cleanup(mutex)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(mutex->pool, mutex, _thread_mutex_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_mutex)

