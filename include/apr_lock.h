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

#ifndef APR_LOCKS_H
#define APR_LOCKS_H

#include "apr_general.h"
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {APR_CROSS_PROCESS, APR_INTRAPROCESS, APR_LOCKALL} ap_lockscope_e;

typedef enum {APR_MUTEX, APR_READWRITE} ap_locktype_e;

typedef struct ap_lock_t           ap_lock_t;

/*   Function definitions */
/* ***APRDOC********************************************************
 * ap_status_t ap_create_lock(ap_lock_t **lock, ap_locktype_e type,
 *                            ap_lockscope_e scope, const char *fname, 
 *                            ap_context_t *cont)
 *    Create a new instance of a lock structure. 
 * arg 1) The newly created lock structure.
 * arg 2) The type of lock to create, one of:
 *            APR_MUTEX
 *            APR_READWRITE
 * arg 3) The scope of the lock to create, one of:
 *            APR_CROSS_PROCESS -- lock processes from the protected area.
 *            APR_INTRAPROCESS  -- lock threads from the protected area.
 *            APR_LOCKALL       -- lock processes and threads from the
 *                                 protected area.
 * arg 4) A file name to use if the lock mechanism requires one.  This
 *        argument should always be provided.  The lock code itself will
 *        determine if it should be used.
 * arg 5) The context to operate on.
 * NOTE:  APR_CROSS_PROCESS may lock both processes and threads, but it is
 *        only guaranteed to lock processes.
 */
ap_status_t ap_create_lock(ap_lock_t **lock, ap_locktype_e type, 
                           ap_lockscope_e scope, char *fname, 
                           ap_pool_t *cont);

/* ***APRDOC********************************************************
 * ap_status_t ap_lock(ap_lock_t *lock)
 *    Lock a protected region. 
 * arg 1) The lock to set.
 */
ap_status_t ap_lock(ap_lock_t *lock);

/* ***APRDOC********************************************************
 * ap_status_t ap_unlock(ap_lock_t *lock)
 *    Unlock a protected region. 
 * arg 1) The lock to reset.
 */
ap_status_t ap_unlock(ap_lock_t *lock);

/* ***APRDOC********************************************************
 * ap_status_t ap_destroy_lock(ap_lock_t *lock)
 *    Free the memory associated with a lock. 
 * arg 1) The lock to free.
 * NOTE:  If the lock is currently active when it is destroyed, it 
 *        will be unlocked first.
 */
ap_status_t ap_destroy_lock(ap_lock_t *lock);

/* ***APRDOC********************************************************
 * ap_status_t ap_child_init_lock(ap_lock_t **lock, const char *fname, 
 *                                ap_context_t *cont)
 *    Re-open a lock in a child process. 
 * arg 1) The newly re-opened lock structure.
 * arg 2) A file name to use if the lock mechanism requires one.  This
 *        argument should always be provided.  The lock code itself will
 *        determine if it should be used.  This filename should be the same
 *        one that was passed to ap_create_lock
 * arg 3) The context to operate on.
 * NOTE:  This function doesn't always do something, it depends on the
 *        locking mechanism chosen for the platform, but it is a good
 *        idea to call it regardless, because it makes the code more
 *        portable. 
 */
ap_status_t ap_child_init_lock(ap_lock_t **lock, const char *fname, 
                               ap_pool_t *cont);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_lockdata(ap_lock_t *lock, char *key, void *data)
 *    Return the context associated with the current lock.
 * arg 1) The currently open lock.
 * arg 2) The key to use when retreiving data associated with this lock
 * arg 3) The user data associated with the lock.
 */
ap_status_t ap_get_lockdata(ap_lock_t *lock, char *key, void *data);

/* ***APRDOC********************************************************
 * ap_status_t ap_set_lockdata(ap_lock_t *lock, void *data, char *key,
 *                              ap_status_t (*cleanup) (void *))
 *    Return the context associated with the current lock.
 * arg 1) The currently open lock.
 * arg 2) The user data to associate with the lock.
 * arg 3) The key to use when associating data with this lock
 * arg 4) The cleanup to use when the lock is destroyed.
 */
ap_status_t ap_set_lockdata(ap_lock_t *lock, void *data, char *key,
                            ap_status_t (*cleanup) (void *));

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_LOCKS_H */
