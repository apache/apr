/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "apr_general.h"
#include "apr_lib.h"
#include "locks.h"
#include "apr_portable.h"

ap_status_t ap_create_lock(ap_context_t *cont, ap_locktype_e type, 
                           ap_lockscope_e scope, char *fname, 
                           struct lock_t **lock)
{
    struct lock_t *newlock;
    SECURITY_ATTRIBUTES sec;

    newlock = (struct lock_t *)ap_palloc(cont, sizeof(struct lock_t));

    newlock->cntxt = cont;
    newlock->fname = strdup(fname);

    sec.nLength = sizeof(SECURITY_ATTRIBUTES);
    sec.lpSecurityDescriptor = NULL;

    if (type == APR_CROSS_PROCESS || type == APR_LOCKALL) {
        sec.bInheritHandle = TRUE;
    }
    else {
        sec.bInheritHandle = FALSE;
    }

    newlock->mutex = CreateMutex(&sec, FALSE, fname);
    *lock = newlock;
    return APR_SUCCESS;
}

ap_status_t ap_child_init_lock(ap_context_t *cont, char *fname, struct lock_t **lock)
{
    (*lock) = (struct lock_t *)ap_palloc(cont, sizeof(struct lock_t));

    if ((*lock) == NULL) {
        return APR_ENOMEM;
    }

    (*lock)->fname = strdup(fname);
    (*lock)->mutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, fname);
    
    if ((*lock)->mutex == NULL) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_lock(struct lock_t *lock)
{
    DWORD rv;

    rv = WaitForSingleObject(lock->mutex, INFINITE);

    if (rv == WAIT_OBJECT_0 || rv == WAIT_ABANDONED) {
        return APR_SUCCESS;
    }
    if (rv == WAIT_TIMEOUT) {
        return APR_TIMEUP;
    }

    return APR_EEXIST;
}

ap_status_t ap_unlock(struct lock_t *lock)
{
    if (ReleaseMutex(lock->mutex) == 0) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_destroy_lock(struct lock_t *lock)
{
    if (CloseHandle(lock->mutex) == 0) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_get_lockdata(struct lock_t *lock, char *key, void *data)
{
    if (lock != NULL) {
        return ap_get_userdata(lock->cntxt, key, &data);
    }
    else {
        data = NULL;
        return APR_ENOLOCK;
    }
}

ap_status_t ap_set_lockdata(struct lock_t *lock, void *data, char *key,
                            ap_status_t (*cleanup) (void *))
{
    if (lock != NULL) {
        return ap_set_userdata(lock->cntxt, data, key, cleanup);
    }
    else {
        data = NULL;
        return APR_ENOLOCK;
    }
}

ap_status_t ap_get_os_lock(struct lock_t *lock, ap_os_lock_t *thelock)
{
    if (lock == NULL) {
        return APR_ENOFILE;
    }
    thelock = &(lock->mutex);
    return APR_SUCCESS;
}

ap_status_t ap_put_os_lock(ap_context_t *cont, struct lock_t **lock, 
                            ap_os_lock_t *thelock)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if ((*lock) == NULL) {
        (*lock) = (struct lock_t *)ap_palloc(cont, sizeof(struct lock_t));
        (*lock)->cntxt = cont;
    }
    (*lock)->mutex = *thelock;
    return APR_SUCCESS;
}    
