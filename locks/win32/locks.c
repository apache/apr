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
#include "win32/locks.h"
#include "apr_portable.h"
#include "misc.h"

static apr_status_t lock_cleanup(void *lock_)
{
    apr_lock_t *lock = lock_;

    switch (lock->type)
    {
    case APR_MUTEX:
        if (lock->scope == APR_INTRAPROCESS) {
            DeleteCriticalSection(&lock->section);
            return APR_SUCCESS;
        } else {
            if (CloseHandle(lock->mutex) == 0) {
                return apr_get_os_error();
            }
        }
        break;
    case APR_READWRITE:
        return APR_ENOTIMPL;
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_create(apr_lock_t **lock, 
                                          apr_locktype_e type, 
                                          apr_lockscope_e scope, 
                                          apr_lockmech_e mech,
                                          const char *fname,
                                          apr_pool_t *pool)
{
    apr_lock_t *newlock;
    SECURITY_ATTRIBUTES sec;

    /* FIXME: Remove when read write locks implemented. */
    if (type == APR_READWRITE)
        return APR_ENOTIMPL;

    if (mech != APR_LOCK_DEFAULT) {
        return APR_ENOTIMPL;
    }

    newlock = (apr_lock_t *)apr_palloc(pool, sizeof(apr_lock_t));

    newlock->pool = pool;
    /* ToDo:  How to handle the case when no pool is available? 
    *         How to cleanup the storage properly?
    */
    newlock->type = type;
    newlock->scope = scope;
    sec.nLength = sizeof(SECURITY_ATTRIBUTES);
    sec.lpSecurityDescriptor = NULL;

    if (scope == APR_CROSS_PROCESS || scope == APR_LOCKALL) {
        sec.bInheritHandle = TRUE;
    }
    else {
        sec.bInheritHandle = FALSE;
    }

    if (scope == APR_INTRAPROCESS) {
        if (fname) {
            newlock->fname = apr_pstrdup(pool, fname);
        }
        else {
            newlock->fname = NULL;
        }
        InitializeCriticalSection(&newlock->section);
    } else {
        /* With Win2000 Terminal Services, the Mutex name can have a 
         * "Global\" or "Local\" prefix to explicitly create the object 
         * in the global or session name space.  Without Terminal Service
         * running on Win2000, Global\ and Local\ are ignored.  These
         * prefixes are only valid on Win2000+
         */
        if (fname) {
            if (apr_os_level >= APR_WIN_2000) {
                newlock->fname = apr_pstrcat(pool, "Global\\", fname, NULL);
            }
            else {
                newlock->fname = apr_pstrdup(pool, fname);
            }
        }
        else {
            newlock->fname = NULL;
        }

        newlock->mutex = CreateMutex(&sec, FALSE, newlock->fname);
        if (!newlock->mutex) {
	    return apr_get_os_error();
        }
    }
    *lock = newlock;
    apr_pool_cleanup_register(newlock->pool, newlock, lock_cleanup,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_child_init(apr_lock_t **lock, 
                                              const char *fname, 
                                              apr_pool_t *pool)
{
    /* This routine should not be called (and OpenMutex will fail if called) 
     * on a INTRAPROCESS lock
     */
    (*lock) = (apr_lock_t *)apr_palloc(pool, sizeof(apr_lock_t));

    if ((*lock) == NULL) {
        return APR_ENOMEM;
    }
    if (fname) {
        if (apr_os_level >= APR_WIN_2000) {
            (*lock)->fname = apr_pstrcat(pool, "Global\\", fname, NULL);
        }
        else {
            (*lock)->fname = apr_pstrdup(pool, fname);
        }
    }
    else {
        return APR_EINVAL;
    }

    (*lock)->mutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, fname);
    
    if ((*lock)->mutex == NULL) {
        return apr_get_os_error();
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_acquire(apr_lock_t *lock)
{
    DWORD rv;
    switch (lock->type)
    {
    case APR_MUTEX:
        if (lock->scope == APR_INTRAPROCESS) {
            EnterCriticalSection(&lock->section);
            return APR_SUCCESS;
        } else {
            rv = WaitForSingleObject(lock->mutex, INFINITE);

            if (rv == WAIT_OBJECT_0 || rv == WAIT_ABANDONED) {
                return APR_SUCCESS;
            }
        }
        break;
    case APR_READWRITE:
        return APR_ENOTIMPL;
    }

    return apr_get_os_error();
}

APR_DECLARE(apr_status_t) apr_lock_tryacquire(apr_lock_t *lock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_lock_acquire_rw(apr_lock_t *lock,
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
            break;
        case APR_WRITER:
            break;
        } 
        return APR_ENOTIMPL;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_release(apr_lock_t *lock)
{
    switch (lock->type)
    {
    case APR_MUTEX:
        if (lock->scope == APR_INTRAPROCESS) {
            LeaveCriticalSection(&lock->section);
            return APR_SUCCESS;
        } else {
            if (ReleaseMutex(lock->mutex) == 0) {
                return apr_get_os_error();
            }
        }
        break;
    case APR_READWRITE:
        return APR_ENOTIMPL;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lock_destroy(apr_lock_t *lock)
{
    apr_status_t stat;

    stat = lock_cleanup(lock);
    if (stat == APR_SUCCESS) {
        apr_pool_cleanup_kill(lock->pool, lock, lock_cleanup);
    }
    return stat;
}

APR_DECLARE(apr_status_t) apr_lock_data_get(apr_lock_t *lock, const char *key,
                                           void *data)
{
    return apr_pool_userdata_get(data, key, lock->pool);
}

APR_DECLARE(apr_status_t) apr_lock_data_set(apr_lock_t *lock, void *data,
                                           const char *key,
                                           apr_status_t (*cleanup) (void *))
{
    return apr_pool_userdata_set(data, key, cleanup, lock->pool);
}

APR_DECLARE(apr_status_t) apr_os_lock_get(apr_os_lock_t *thelock,
                                          apr_lock_t *lock)
{
    *thelock = lock->mutex;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_lock_put(apr_lock_t **lock,
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
    (*lock)->mutex = *thelock;
    return APR_SUCCESS;
}    
