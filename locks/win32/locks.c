/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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

#include "apr_general.h"
#include "apr_strings.h"
#include "locks.h"
#include "apr_portable.h"

apr_status_t apr_create_lock(apr_lock_t **lock, apr_locktype_e type, 
                           apr_lockscope_e scope, const char *fname, 
                           apr_pool_t *cont)
{
    apr_lock_t *newlock;
    SECURITY_ATTRIBUTES sec;

    newlock = (apr_lock_t *)apr_palloc(cont, sizeof(apr_lock_t));

    newlock->cntxt = cont;
    /* ToDo:  How to handle the case when no context is available? 
    *         How to cleanup the storage properly?
    */
    newlock->fname = apr_pstrdup(cont, fname);
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
        InitializeCriticalSection(&newlock->section);
    } else {
        newlock->mutex = CreateMutex(&sec, FALSE, fname);
    }
    *lock = newlock;
    return APR_SUCCESS;
}

apr_status_t apr_child_init_lock(apr_lock_t **lock, const char *fname, 
                               apr_pool_t *cont)
{
    /* This routine should not be called (and OpenMutex will fail if called) 
     * on a INTRAPROCESS lock
     */
    (*lock) = (apr_lock_t *)apr_palloc(cont, sizeof(apr_lock_t));

    if ((*lock) == NULL) {
        return APR_ENOMEM;
    }
    (*lock)->fname = apr_pstrdup(cont, fname);
    (*lock)->mutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, fname);
    
    if ((*lock)->mutex == NULL) {
        return apr_get_os_error();
    }
    return APR_SUCCESS;
}

apr_status_t apr_lock(apr_lock_t *lock)
{
    DWORD rv;
    if (lock->scope == APR_INTRAPROCESS) {
        EnterCriticalSection(&lock->section);
        return APR_SUCCESS;
    } else {
        rv = WaitForSingleObject(lock->mutex, INFINITE);

        if (rv == WAIT_OBJECT_0 || rv == WAIT_ABANDONED) {
            return APR_SUCCESS;
        }
    }
    return apr_get_os_error();
}

apr_status_t apr_unlock(apr_lock_t *lock)
{
    if (lock->scope == APR_INTRAPROCESS) {
        LeaveCriticalSection(&lock->section);
        return APR_SUCCESS;
    } else {
        if (ReleaseMutex(lock->mutex) == 0) {
            return apr_get_os_error();
        }
    }
    return APR_SUCCESS;
}

apr_status_t apr_destroy_lock(apr_lock_t *lock)
{
    if (lock->scope == APR_INTRAPROCESS) {
        DeleteCriticalSection(&lock->section);
        return APR_SUCCESS;
    } else {
        if (CloseHandle(lock->mutex) == 0) {
            return apr_get_os_error();
        }
    }
    return APR_SUCCESS;
}

apr_status_t apr_get_lockdata(apr_lock_t *lock, const char *key, void *data)
{
    return apr_get_userdata(data, key, lock->cntxt);
}

apr_status_t apr_set_lockdata(apr_lock_t *lock, void *data, const char *key,
                            apr_status_t (*cleanup) (void *))
{
    return apr_set_userdata(data, key, cleanup, lock->cntxt);
}

apr_status_t apr_get_os_lock(apr_os_lock_t *thelock, apr_lock_t *lock)
{
    *thelock = lock->mutex;
    return APR_SUCCESS;
}

apr_status_t apr_put_os_lock(apr_lock_t **lock, apr_os_lock_t *thelock, 
                           apr_pool_t *cont)
{
    if (cont == NULL) {
        return APR_ENOPOOL;
    }
    if ((*lock) == NULL) {
        (*lock) = (apr_lock_t *)apr_palloc(cont, sizeof(apr_lock_t));
        (*lock)->cntxt = cont;
    }
    (*lock)->mutex = *thelock;
    return APR_SUCCESS;
}    
