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

#ifndef APR_PROC_MUTEX_H
#define APR_PROC_MUTEX_H

/**
 * @file apr_proc_mutex.h
 * @brief APR Process Locking Routines
 */

#include "apr.h"
#include "apr_pools.h"
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup apr_proc_mutex Process Locking Routines
 * @ingroup APR 
 * @{
 */

/** 
 * Enumerated potential types for APR process locking methods
 * @warning Check APR_HAS_foo_SERIALIZE defines to see if the platform supports
 *          APR_LOCK_foo.  Only APR_LOCK_DEFAULT is portable.
 */
typedef enum {
    APR_LOCK_FCNTL,         /**< fcntl() */
    APR_LOCK_FLOCK,         /**< flock() */
    APR_LOCK_SYSVSEM,       /**< System V Semaphores */
    APR_LOCK_PROC_PTHREAD,  /**< POSIX pthread process-based locking */
    APR_LOCK_POSIXSEM,      /**< POSIX semaphore process-based locking */
    APR_LOCK_DEFAULT        /**< Use the default process lock */
} apr_lockmech_e;

/** Opaque structure representing a process mutex. */
typedef struct apr_proc_mutex_t apr_proc_mutex_t;

/*   Function definitions */

/**
 * Create and initialize a mutex that can be used to synchronize processes.
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
 * @see apr_lockmech_e
 * @warning Check APR_HAS_foo_SERIALIZE defines to see if the platform supports
 *          APR_LOCK_foo.  Only APR_LOCK_DEFAULT is portable.
 */
APR_DECLARE(apr_status_t) apr_proc_mutex_create(apr_proc_mutex_t **mutex,
                                                const char *fname,
                                                apr_lockmech_e mech,
                                                apr_pool_t *pool);

/**
 * Re-open a mutex in a child process.
 * @param mutex The newly re-opened mutex structure.
 * @param fname A file name to use if the mutex mechanism requires one.  This
 *              argument should always be provided.  The mutex code itself will
 *              determine if it should be used.  This filename should be the 
 *              same one that was passed to apr_proc_mutex_create().
 * @param pool The pool to operate on.
 * @remark This function must be called to maintain portability, even
 *         if the underlying lock mechanism does not require it.
 */
APR_DECLARE(apr_status_t) apr_proc_mutex_child_init(apr_proc_mutex_t **mutex,
                                                    const char *fname,
                                                    apr_pool_t *pool);

/**
 * Acquire the lock for the given mutex. If the mutex is already locked,
 * the current thread will be put to sleep until the lock becomes available.
 * @param mutex the mutex on which to acquire the lock.
 */
APR_DECLARE(apr_status_t) apr_proc_mutex_lock(apr_proc_mutex_t *mutex);

/**
 * Attempt to acquire the lock for the given mutex. If the mutex has already
 * been acquired, the call returns immediately with APR_EBUSY. Note: it
 * is important that the APR_STATUS_IS_EBUSY(s) macro be used to determine
 * if the return value was APR_EBUSY, for portability reasons.
 * @param mutex the mutex on which to attempt the lock acquiring.
 */
APR_DECLARE(apr_status_t) apr_proc_mutex_trylock(apr_proc_mutex_t *mutex);

/**
 * Release the lock for the given mutex.
 * @param mutex the mutex from which to release the lock.
 */
APR_DECLARE(apr_status_t) apr_proc_mutex_unlock(apr_proc_mutex_t *mutex);

/**
 * Destroy the mutex and free the memory associated with the lock.
 * @param mutex the mutex to destroy.
 */
APR_DECLARE(apr_status_t) apr_proc_mutex_destroy(apr_proc_mutex_t *mutex);

/**
 * Destroy the mutex and free the memory associated with the lock.
 * @param mutex the mutex to destroy.
 * @note This function is generally used to kill a cleanup on an already
 *       created mutex
 */
APR_DECLARE(apr_status_t) apr_proc_mutex_cleanup(void *mutex);

/**
 * Display the name of the mutex, as it relates to the actual method used.
 * This matches the valid options for Apache's AcceptMutex directive
 * @param mutex the name of the mutex
 */
APR_DECLARE(const char *) apr_proc_mutex_name(apr_proc_mutex_t *mutex);

/**
 * Display the name of the default mutex: APR_LOCK_DEFAULT
 */
APR_DECLARE(const char *) apr_proc_mutex_defname(void);

/**
 * Get the pool used by this proc_mutex.
 * @return apr_pool_t the pool
 */
APR_POOL_DECLARE_ACCESSOR(proc_mutex);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_PROC_MUTEX_H */
