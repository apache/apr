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

#ifndef APR_GLOBAL_MUTEX_H
#define APR_GLOBAL_MUTEX_H

/**
 * @file apr_global_mutex.h
 * @brief APR Global Locking Routines
 */

#include "apr.h"
#include "apr_proc_mutex.h"    /* only for apr_lockmech_e */
#include "apr_pools.h"
#include "apr_errno.h"
#if APR_PROC_MUTEX_IS_GLOBAL
#include "apr_proc_mutex.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup APR_GlobalMutex Global Locking Routines
 * @ingroup APR 
 * @{
 */

#if !APR_PROC_MUTEX_IS_GLOBAL || defined(DOXYGEN)

/** Opaque global mutex structure. */
typedef struct apr_global_mutex_t apr_global_mutex_t;

/*   Function definitions */

/**
 * Create and initialize a mutex that can be used to synchronize both
 * processes and threads. Note: There is considerable overhead in using
 * this API if only cross-process or cross-thread mutual exclusion is
 * required. See apr_proc_mutex.h and apr_thread_mutex.h for more
 * specialized lock routines.
 * @param mutex the memory address where the newly created mutex will be
 *        stored.
 * @param fname A file name to use if the lock mechanism requires one.  This
 *        argument should always be provided.  The lock code itself will
 *        determine if it should be used.
 * @param mech The mechanism to use for the interprocess lock, if any; one of
 * <PRE>
 *            APR_LOCK_FCNTL
 *            APR_LOCK_FLOCK
 *            APR_LOCK_SYSVSEM
 *            APR_LOCK_POSIXSEM
 *            APR_LOCK_PROC_PTHREAD
 *            APR_LOCK_DEFAULT     pick the default mechanism for the platform
 * </PRE>
 * @param pool the pool from which to allocate the mutex.
 * @warning Check APR_HAS_foo_SERIALIZE defines to see if the platform supports
 *          APR_LOCK_foo.  Only APR_LOCK_DEFAULT is portable.
 */
APR_DECLARE(apr_status_t) apr_global_mutex_create(apr_global_mutex_t **mutex,
                                                  const char *fname,
                                                  apr_lockmech_e mech,
                                                  apr_pool_t *pool);

/**
 * Re-open a mutex in a child process.
 * @param mutex The newly re-opened mutex structure.
 * @param fname A file name to use if the mutex mechanism requires one.  This
 *              argument should always be provided.  The mutex code itself will
 *              determine if it should be used.  This filename should be the 
 *              same one that was passed to apr_global_mutex_create().
 * @param pool The pool to operate on.
 * @remark This function must be called to maintain portability, even
 *         if the underlying lock mechanism does not require it.
 */
APR_DECLARE(apr_status_t) apr_global_mutex_child_init(
                              apr_global_mutex_t **mutex,
                              const char *fname,
                              apr_pool_t *pool);

/**
 * Acquire the lock for the given mutex. If the mutex is already locked,
 * the current thread will be put to sleep until the lock becomes available.
 * @param mutex the mutex on which to acquire the lock.
 */
APR_DECLARE(apr_status_t) apr_global_mutex_lock(apr_global_mutex_t *mutex);

/**
 * Attempt to acquire the lock for the given mutex. If the mutex has already
 * been acquired, the call returns immediately with APR_EBUSY. Note: it
 * is important that the APR_STATUS_IS_EBUSY(s) macro be used to determine
 * if the return value was APR_EBUSY, for portability reasons.
 * @param mutex the mutex on which to attempt the lock acquiring.
 */
APR_DECLARE(apr_status_t) apr_global_mutex_trylock(apr_global_mutex_t *mutex);

/**
 * Release the lock for the given mutex.
 * @param mutex the mutex from which to release the lock.
 */
APR_DECLARE(apr_status_t) apr_global_mutex_unlock(apr_global_mutex_t *mutex);

/**
 * Destroy the mutex and free the memory associated with the lock.
 * @param mutex the mutex to destroy.
 */
APR_DECLARE(apr_status_t) apr_global_mutex_destroy(apr_global_mutex_t *mutex);

/**
 * Get the pool used by this global_mutex.
 * @return apr_pool_t the pool
 */
APR_POOL_DECLARE_ACCESSOR(global_mutex);

#else /* APR_PROC_MUTEX_IS_GLOBAL */

/* Some platforms [e.g. Win32] have cross process locks that are truly
 * global locks, since there isn't the concept of cross-process locks.
 * Define these platforms in terms of an apr_proc_mutex_t.
 */

#define apr_global_mutex_t          apr_proc_mutex_t
#define apr_global_mutex_create     apr_proc_mutex_create
#define apr_global_mutex_child_init apr_proc_mutex_child_init
#define apr_global_mutex_lock       apr_proc_mutex_lock
#define apr_global_mutex_trylock    apr_proc_mutex_trylock
#define apr_global_mutex_unlock     apr_proc_mutex_unlock
#define apr_global_mutex_destroy    apr_proc_mutex_destroy
#define apr_global_mutex_pool_get   apr_proc_mutex_pool_get

#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* ndef APR_GLOBAL_MUTEX_H */
