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

#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include "apr_arch_thread_rwlock.h"
#include "apr_arch_file_io.h"
#include <string.h>

static apr_status_t thread_rwlock_cleanup(void *therwlock)
{
    apr_thread_rwlock_t *rwlock = therwlock;
    return apr_thread_rwlock_destroy(rwlock);
}



APR_DECLARE(apr_status_t) apr_thread_rwlock_create(apr_thread_rwlock_t **rwlock,
                                                   apr_pool_t *pool)
{
    apr_thread_rwlock_t *new_rwlock;
    ULONG rc;

    new_rwlock = (apr_thread_rwlock_t *)apr_palloc(pool, sizeof(apr_thread_rwlock_t));
    new_rwlock->pool = pool;
    new_rwlock->readers = 0;

    rc = DosCreateMutexSem(NULL, &(new_rwlock->write_lock), 0, FALSE);

    if (rc)
        return APR_FROM_OS_ERROR(rc);

    rc = DosCreateEventSem(NULL, &(new_rwlock->read_done), 0, FALSE);

    if (rc)
        return APR_FROM_OS_ERROR(rc);

    *rwlock = new_rwlock;

    if (!rc)
        apr_pool_cleanup_register(pool, new_rwlock, thread_rwlock_cleanup,
                                  apr_pool_cleanup_null);

    return APR_FROM_OS_ERROR(rc);
}



APR_DECLARE(apr_status_t) apr_thread_rwlock_rdlock(apr_thread_rwlock_t *rwlock)
{
    ULONG rc, posts;

    rc = DosRequestMutexSem(rwlock->write_lock, SEM_INDEFINITE_WAIT);

    if (rc)
        return APR_FROM_OS_ERROR(rc);

    /* We've successfully acquired the writer mutex so we can't be locked
     * for write which means it's ok to add a reader lock. The writer mutex
     * doubles as race condition protection for the readers counter.
     */
    rwlock->readers++;
    DosResetEventSem(rwlock->read_done, &posts);
    rc = DosReleaseMutexSem(rwlock->write_lock);
    return APR_FROM_OS_ERROR(rc);
}



APR_DECLARE(apr_status_t) apr_thread_rwlock_tryrdlock(apr_thread_rwlock_t *rwlock)
{
    /* As above but with different wait time */
    ULONG rc, posts;

    rc = DosRequestMutexSem(rwlock->write_lock, SEM_IMMEDIATE_RETURN);

    if (rc)
        return APR_FROM_OS_ERROR(rc);

    rwlock->readers++;
    DosResetEventSem(rwlock->read_done, &posts);
    rc = DosReleaseMutexSem(rwlock->write_lock);
    return APR_FROM_OS_ERROR(rc);
}



APR_DECLARE(apr_status_t) apr_thread_rwlock_wrlock(apr_thread_rwlock_t *rwlock)
{
    ULONG rc;

    rc = DosRequestMutexSem(rwlock->write_lock, SEM_INDEFINITE_WAIT);

    if (rc)
        return APR_FROM_OS_ERROR(rc);

    /* We've got the writer lock but we have to wait for all readers to
     * unlock before it's ok to use it
     */

    if (rwlock->readers) {
        rc = DosWaitEventSem(rwlock->read_done, SEM_INDEFINITE_WAIT);

        if (rc)
            DosReleaseMutexSem(rwlock->write_lock);
    }

    return APR_FROM_OS_ERROR(rc);
}



APR_DECLARE(apr_status_t) apr_thread_rwlock_trywrlock(apr_thread_rwlock_t *rwlock)
{
    ULONG rc;

    rc = DosRequestMutexSem(rwlock->write_lock, SEM_IMMEDIATE_RETURN);

    if (rc)
        return APR_FROM_OS_ERROR(rc);

    /* We've got the writer lock but we have to wait for all readers to
     * unlock before it's ok to use it
     */

    if (rwlock->readers) {
        /* There are readers active, give up */
        DosReleaseMutexSem(rwlock->write_lock);
        rc = ERROR_TIMEOUT;
    }

    return APR_FROM_OS_ERROR(rc);
}



APR_DECLARE(apr_status_t) apr_thread_rwlock_unlock(apr_thread_rwlock_t *rwlock)
{
    ULONG rc;

    /* First, guess that we're unlocking a writer */
    rc = DosReleaseMutexSem(rwlock->write_lock);

    if (rc == ERROR_NOT_OWNER) {
        /* Nope, we must have a read lock */
        if (rwlock->readers) {
            DosEnterCritSec();
            rwlock->readers--;

            if (rwlock->readers == 0) {
                DosPostEventSem(rwlock->read_done);
            }

            DosExitCritSec();
            rc = 0;
        }
    }

    return APR_FROM_OS_ERROR(rc);
}



APR_DECLARE(apr_status_t) apr_thread_rwlock_destroy(apr_thread_rwlock_t *rwlock)
{
    ULONG rc;

    if (rwlock->write_lock == 0)
        return APR_SUCCESS;

    while (DosReleaseMutexSem(rwlock->write_lock) == 0);

    rc = DosCloseMutexSem(rwlock->write_lock);

    if (!rc) {
        rwlock->write_lock = 0;
        DosCloseEventSem(rwlock->read_done);
        return APR_SUCCESS;
    }

    return APR_FROM_OS_ERROR(rc);
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_rwlock)

