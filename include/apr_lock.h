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

/**
 * @package APR lock library
 */

typedef enum {APR_CROSS_PROCESS, APR_INTRAPROCESS, APR_LOCKALL} apr_lockscope_e;

typedef enum {APR_MUTEX, APR_READWRITE} apr_locktype_e;

typedef struct apr_lock_t           apr_lock_t;

/*   Function definitions */

/**
 * Create a new instance of a lock structure.
 * @param lock The newly created lock structure.
 * @param type The type of lock to create, one of:
 * <PRE>
 *            APR_MUTEX
 *            APR_READWRITE
 * </PRE>
 * @param scope The scope of the lock to create, one of:
 * <PRE>
 *            APR_CROSS_PROCESS -- lock processes from the protected area.
 *            APR_INTRAPROCESS  -- lock threads from the protected area.
 *            APR_LOCKALL       -- lock processes and threads from the
 *                                 protected area.
 * </PRE>
 * @param fname A file name to use if the lock mechanism requires one.  This
 *        argument should always be provided.  The lock code itself will
 *        determine if it should be used.
 * @param cont The pool to operate on.
 * @tip APR_CROSS_PROCESS may lock both processes and threads, but it is
 *      only guaranteed to lock processes.
 */
apr_status_t apr_create_lock(apr_lock_t **lock, apr_locktype_e type, 
                           apr_lockscope_e scope, const char *fname, 
                           apr_pool_t *cont);

/**
 * Lock a protected region.
 * @param lock The lock to set.
 */
apr_status_t apr_lock(apr_lock_t *lock);

/**
 * Unlock a protected region.
 * @param lock The lock to reset.
 */
apr_status_t apr_unlock(apr_lock_t *lock);

/**
 * Free the memory associated with a lock.
 * @param lock The lock to free.
 * @tip  If the lock is currently active when it is destroyed, it 
 *       will be unlocked first.
 */
apr_status_t apr_destroy_lock(apr_lock_t *lock);

/**
 * Re-open a lock in a child process.
 * @param lock The newly re-opened lock structure.
 * @param fname A file name to use if the lock mechanism requires one.  This
 *              argument should always be provided.  The lock code itself will
 *              determine if it should be used.  This filename should be the 
 *              same one that was passed to apr_create_lock
 * @param cont The pool to operate on.
 * @tip This function doesn't always do something, it depends on the
 *      locking mechanism chosen for the platform, but it is a good
 *      idea to call it regardless, because it makes the code more
 *      portable. 
 */
apr_status_t apr_child_init_lock(apr_lock_t **lock, const char *fname, 
                               apr_pool_t *cont);

/**
 * Return the pool associated with the current lock.
 * @param lock The currently open lock.
 * @param key The key to use when retreiving data associated with this lock
 * @param data The user data associated with the lock.
 */
apr_status_t apr_get_lockdata(apr_lock_t *lock, const char *key, void *data);

/**
 * Return the pool associated with the current lock.
 * @param lock The currently open lock.
 * @param data The user data to associate with the lock.
 * @param key The key to use when associating data with this lock
 * @param cleanup The cleanup to use when the lock is destroyed.
 */
apr_status_t apr_set_lockdata(apr_lock_t *lock, void *data, const char *key,
                            apr_status_t (*cleanup) (void *));

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_LOCKS_H */
