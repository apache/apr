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

#include "locks.h"
#include "apr_portable.h"

/* ***APRDOC********************************************************
 * ap_status_t ap_create_lock(ap_lock_t **lock, ap_locktype_e type,
 *                            ap_lockscope_e scope, char *fname, 
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
ap_status_t ap_create_lock(struct lock_t **lock, ap_locktype_e type, 
                           ap_lockscope_e scope, char *fname, 
                           ap_context_t *cont)
{
    struct lock_t *new;
    ap_status_t stat;

    new = (struct lock_t *)ap_palloc(cont, sizeof(struct lock_t));

    new->cntxt = cont;
    new->type  = type;
    new->scope = scope;
#if defined(USE_FCNTL_SERIALIZE) || defined(USE_FLOCK_SERIALIZE)
    /* file-based serialization primitives */
    if (scope != APR_INTRAPROCESS) {
        if (fname != NULL) {
            new->fname = ap_pstrdup(cont, fname);
        }
        else {
            new->fname = ap_pstrdup(cont, tempnam(NULL, NULL));
            unlink(new->fname);
	}
    }
#endif

    if (scope != APR_CROSS_PROCESS) {
#if APR_HAS_THREADS
        if ((stat = create_intra_lock(new)) != APR_SUCCESS) {
            return stat;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (scope != APR_INTRAPROCESS) {
        if ((stat = create_inter_lock(new)) != APR_SUCCESS) {
            return stat;
        }
    }
    *lock = new;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_lock(ap_lock_t *lock)
 *    Lock a protected region. 
 * arg 1) The lock to set.
 */
ap_status_t ap_lock(struct lock_t *lock)
{
    ap_status_t stat;
    if (lock->scope != APR_CROSS_PROCESS) {
#if APR_HAS_THREADS
        if ((stat = lock_intra(lock)) != APR_SUCCESS) {
            return stat;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (lock->scope != APR_INTRAPROCESS) {
        if ((stat = lock_inter(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_unlock(ap_lock_t *lock)
 *    Unlock a protected region. 
 * arg 1) The lock to reset.
 */
ap_status_t ap_unlock(struct lock_t *lock)
{
    ap_status_t stat;

    if (lock->scope != APR_CROSS_PROCESS) {
#if APR_HAS_THREADS
        if ((stat = unlock_intra(lock)) != APR_SUCCESS) {
            return stat;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (lock->scope != APR_INTRAPROCESS) {
        if ((stat = unlock_inter(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_destroy_lock(ap_lock_t *lock)
 *    Free the memory associated with a lock. 
 * arg 1) The lock to free.
 * NOTE:  If the lock is currently active when it is destroyed, it 
 *        will be unlocked first.
 */
ap_status_t ap_destroy_lock(struct lock_t *lock)
{
    ap_status_t stat;
    if (lock->scope != APR_CROSS_PROCESS) {
#if APR_HAS_THREADS
        if ((stat = destroy_intra_lock(lock)) != APR_SUCCESS) {
            return stat;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (lock->scope != APR_INTRAPROCESS) {
        if ((stat = destroy_inter_lock(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_child_init_lock(ap_lock_t **lock, char *fname, 
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
ap_status_t ap_child_init_lock(struct lock_t **lock, char *fname, ap_context_t *cont)
{
    ap_status_t stat;
    if ((*lock)->scope != APR_CROSS_PROCESS) {
        if ((stat = child_init_lock(lock, cont, fname)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_lockdata(ap_lock_t *lock, char *key, void *data)
 *    Return the context associated with the current lock.
 * arg 1) The currently open lock.
 * arg 2) The key to use when retreiving data associated with this lock
 * arg 3) The user data associated with the lock.
 */
ap_status_t ap_get_lockdata(struct lock_t *lock, char *key, void *data)
{
    if (lock != NULL) {
        return ap_get_userdata(data, key, lock->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOLOCK;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_lockdata(ap_lock_t *lock, void *data, char *key,
                               ap_status_t (*cleanup) (void *))
 *    Return the context associated with the current lock.
 * arg 1) The currently open lock.
 * arg 2) The user data to associate with the lock.
 * arg 3) The key to use when associating data with this lock
 * arg 4) The cleanup to use when the lock is destroyed.
 */
ap_status_t ap_set_lockdata(struct lock_t *lock, void *data, char *key,
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

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_lock(ap_os_lock_t *oslock, ap_lock_t *lock)
 *    onvert the lock from os specific type to apr type
 * arg 1) The os specific lock we are converting to.
 * arg 2) The apr lock to convert.
 */
ap_status_t ap_get_os_lock(ap_os_lock_t *oslock, struct lock_t *lock)
{
    if (lock == NULL) {
        return APR_ENOLOCK;
    }
    oslock->crossproc = lock->interproc;
#if APR_HAS_THREADS
#if USE_PTHREAD_SERIALIZE
    oslock->intraproc = lock->intraproc;
#endif
#endif

    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_lock(ap_lock_t **lock, ap_os_lock_t *,
 *                            ap_context_t *cont)
 *    onvert the lock from os specific type to apr type
 * arg 1) The apr lock we are converting to.
 * arg 2) The os specific lock to convert.
 * arg 3) The context to use if it is needed.
 */
ap_status_t ap_put_os_lock(struct lock_t **lock, ap_os_lock_t *thelock, 
                           ap_context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if ((*lock) == NULL) {
        (*lock) = (struct lock_t *)ap_palloc(cont, sizeof(struct lock_t));
        (*lock)->cntxt = cont;
    }
    (*lock)->interproc = thelock->crossproc;
#if APR_HAS_THREADS
#if defined (USE_PTHREAD_SERIALIZE)
    (*lock)->intraproc = thelock->intraproc;
#endif
#endif
    return APR_SUCCESS;
}
    
