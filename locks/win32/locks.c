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
#include "apr_lib.h"
#include "locks.h"
#include "apr_portable.h"

ap_status_t ap_create_lock(struct ap_lock_t **lock, ap_locktype_e type, 
                           ap_lockscope_e scope, char *fname, 
                           ap_context_t *cont)
{
    struct ap_lock_t *newlock;
    SECURITY_ATTRIBUTES sec;

    newlock = (struct ap_lock_t *)ap_palloc(cont, sizeof(struct ap_lock_t));

    newlock->cntxt = cont;
    /* ToDo:  How to handle the case when no context is available? 
    *         How to cleanup the storage properly?
    */
    newlock->fname = ap_pstrdup(cont, fname);
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

ap_status_t ap_child_init_lock(struct ap_lock_t **lock, char *fname, ap_context_t *cont)
{
    /* This routine should not be called (and OpenMutex will fail if called) 
     * on a INTRAPROCESS lock
     */
    (*lock) = (struct ap_lock_t *)ap_palloc(cont, sizeof(struct ap_lock_t));

    if ((*lock) == NULL) {
        return APR_ENOMEM;
    }
    (*lock)->fname = ap_pstrdup(cont, fname);
    (*lock)->mutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, fname);
    
    if ((*lock)->mutex == NULL) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_lock(struct ap_lock_t *lock)
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
        if (rv == WAIT_TIMEOUT) {
            return APR_TIMEUP;
        }
    }
    return APR_EEXIST;
}

ap_status_t ap_unlock(struct ap_lock_t *lock)
{
    if (lock->scope == APR_INTRAPROCESS) {
        LeaveCriticalSection(&lock->section);
        return APR_SUCCESS;
    } else {
        if (ReleaseMutex(lock->mutex) == 0) {
            return APR_EEXIST;
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_destroy_lock(struct ap_lock_t *lock)
{
    if (lock->scope == APR_INTRAPROCESS) {
        DeleteCriticalSection(&lock->section);
        return APR_SUCCESS;
    } else {
        if (CloseHandle(lock->mutex) == 0) {
            return APR_EEXIST;
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_get_lockdata(struct ap_lock_t *lock, char *key, void *data)
{
    if (lock != NULL) {
        return ap_get_userdata(data, key, lock->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOLOCK;
    }
}

ap_status_t ap_set_lockdata(struct ap_lock_t *lock, void *data, char *key,
                            ap_status_t (*cleanup) (void *))
{
    if (lock != NULL) {
        return ap_set_userdata(data, key, cleanup, lock->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOLOCK;
    }
}

ap_status_t ap_get_os_lock(ap_os_lock_t *thelock, struct ap_lock_t *lock)
{
    if (lock == NULL) {
        return APR_ENOFILE;
    }
    thelock = &(lock->mutex);
    return APR_SUCCESS;
}

ap_status_t ap_put_os_lock(struct ap_lock_t **lock, ap_os_lock_t *thelock, 
                           ap_context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if ((*lock) == NULL) {
        (*lock) = (struct ap_lock_t *)ap_palloc(cont, sizeof(struct ap_lock_t));
        (*lock)->cntxt = cont;
    }
    (*lock)->mutex = *thelock;
    return APR_SUCCESS;
}    
