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

#include "apr.h"
#include "apr_private.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "locks.h"
#include "apr_portable.h"

static apr_status_t lock_cleanup(void *lock_)
{
    apr_lock_t *lock = lock_;

    switch (lock->type)
    {
    case APR_MUTEX:
        NXMutexFree(lock->mutex);        
        break;
    case APR_READWRITE:
    	NXRwLockFree (lock->rwlock);
    	break;
    }
    return APR_SUCCESS;
}

apr_status_t apr_lock_create(apr_lock_t **lock, apr_locktype_e type, apr_lockscope_e scope, 
                             apr_lockmech_e mech, const char *fname, apr_pool_t *pool)
{
   
    apr_lock_t *newlock = NULL;
   
   /* struct apr_lock_t {
    apr_pool_t *pool;
    apr_locktype_e type;
    apr_lockscope_e scope;
    NXMutex_t *mutex;
    NXRwLock_t *rwlock;
    char *fname;
	};
   */
	NXHierarchy_t hierarchy=0;	   //for libc NKS NXRwLockAlloc
	NXLockInfo_t *info;			   //for libc NKS NXRwLockAlloc
    apr_status_t status;
    long flags = 0;

    if (mech != APR_LOCK_DEFAULT) {
        return APR_ENOTIMPL;
    }

    newlock = (apr_lock_t *)apr_palloc(pool, sizeof(apr_lock_t));
	
	if(newlock ==NULL) {
        return APR_ENOMEM;
    }     
    newlock->pool = pool;
    /* ToDo:  How to handle the case when no pool is available? 
    *         How to cleanup the storage properly?
    */
    newlock->fname = apr_pstrdup(pool, fname);
    newlock->type = type;
    newlock->scope = scope;

//srj fill in scope later
//srj   if (scope == APR_INTRAPROCESS) {
//srj      InitializeCriticalSection(&newlock->section);
//srj   } else {

        switch (type)
        {
            case APR_MUTEX:
                flags=NX_MUTEX_RECURSIVE;
                newlock->mutex = NXMutexAlloc(flags,NULL, NULL);
                
                if(newlock->mutex == NULL)
                	return APR_ENOMEM;
                break;
            case APR_READWRITE:
                
                info = (NXLockInfo_t *)apr_palloc(pool, sizeof(NXLockInfo_t));
                hierarchy=1;
                //srj NXRwLockAlloc Allocates and initializes a reader/writer lock
                //srj RWLocks are not recursive
                newlock->rwlock = NXRwLockAlloc(hierarchy,info);
                if(newlock->rwlock == NULL)
                	return APR_ENOMEM;
                break;
        } 

//	}	end of else for scope

   *lock = newlock;
    apr_pool_cleanup_register(newlock->pool, newlock, lock_cleanup,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

apr_status_t apr_lock_child_init(apr_lock_t **lock, 
                                              const char *fname, 
                                              apr_pool_t *pool)
{
    /* This routine should not be called ( OpenMutex will fail if called) 
     * on a INTRAPROCESS lock
     */
    (*lock) = (apr_lock_t *)apr_palloc(pool, sizeof(apr_lock_t));

    if ((*lock) == NULL) {
        return APR_ENOMEM;
    }
    (*lock)->fname = apr_pstrdup(pool, fname);
    
    if ((*lock)->mutex == NULL) {
        return apr_get_os_error();
    }
    return APR_SUCCESS;
}

apr_status_t apr_lock_acquire(apr_lock_t *lock)
{
    DWORD rv;
    switch (lock->type)
    {
    case APR_MUTEX:
    	if(NXLock(lock->mutex)==0)    
        	return APR_SUCCESS;
        break;
    //srj APR_READWRITE should not be called here. Not Needed
    //srj since we have apr_lock_acquire_rw function

    case APR_READWRITE:
    	return APR_ENOTIMPL;
    default:
        return APR_EINVAL;
    } 

    return apr_get_os_error();
}

APR_DECLARE(apr_status_t) apr_lock_tryacquire(apr_lock_t *lock)
{
    return APR_ENOTIMPL;
}

apr_status_t apr_lock_acquire_rw(apr_lock_t *lock,
                                 apr_readerwriter_e e)
{
    switch (lock->type)
    {
    case APR_MUTEX:
        return APR_ENOTIMPL;
    
    case APR_READWRITE:
        
        switch (e)
        {
        case APR_READER:
#if 0
            //srj NXRdLock specifies the reader/writer lock in the read mode
            //No return values
            NXRdLock (lock->rwlock);
#endif
            break;
        case APR_WRITER:
#if 0
            //srj NXWrLock specifies the reader/writer lock in the write mode
            //No return values
            NXWrLock (lock->rwlock);
#endif
            break;
       	}
	
	default:
        return APR_EINVAL;
    }

    return APR_SUCCESS;
}

apr_status_t apr_lock_release(apr_lock_t *lock)
{
    switch (lock->type)
    {
    case APR_MUTEX:
//srj will work on scope later
//
//        if (lock->scope == APR_INTRAPROCESS) {
//            LeaveCriticalSection(&lock->section);
//            return APR_SUCCESS;
//        } else {
            if(NXUnlock(lock->mutex)==0);    
            	return APR_SUCCESS;
//        }
        break;
    
    case APR_READWRITE:
        return APR_ENOTIMPL;
        /*NXRwUnlock (lock->rwlock);*/
    }

    return apr_get_os_error();
}

apr_status_t apr_lock_destroy(apr_lock_t *lock)
{
    apr_status_t stat;

    stat = lock_cleanup(lock);
    if (stat == APR_SUCCESS) {
        apr_pool_cleanup_kill(lock->pool, lock, lock_cleanup);
    }
    return stat;
}

apr_status_t apr_lock_data_get(apr_lock_t *lock, const char *key,
                                           void *data)
{
    return apr_pool_userdata_get(data, key, lock->pool);
}

apr_status_t apr_lock_data_set(apr_lock_t *lock, void *data,
                               const char *key,
                               apr_status_t (*cleanup) (void *))
{
    return apr_pool_userdata_set(data, key, cleanup, lock->pool);
}

apr_status_t apr_os_lock_get(apr_os_lock_t *thelock,
                             apr_lock_t *lock)
{
    switch (lock->type)
    {
    case APR_MUTEX:
        thelock = lock->mutex;
        break;
    case APR_READWRITE:
        return APR_ENOTIMPL;
        /* thelock = lock->rwlock;*/
        break;
    }

    return APR_SUCCESS;
}

apr_status_t apr_os_lock_put(apr_lock_t **lock,
                             apr_os_lock_t *thelock,
                             apr_pool_t *pool)
{
    if (pool == NULL) {
        return APR_ENOPOOL;
    }
    if ((*lock) == NULL) {
        (*lock) = (apr_lock_t *)apr_palloc(pool, sizeof(apr_lock_t));
        (*lock)->pool = pool;
    }
    switch ((*lock)->type)
    {
    case APR_MUTEX:
        (*lock)->mutex = thelock;
        break;
    case APR_READWRITE:
        return APR_ENOTIMPL;
        /*(*lock)->rwlock = *thelock;*/
        break;
    }
    return APR_SUCCESS;
}    
