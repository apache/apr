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

#include "apr.h"
#include "apr_private.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "win32/apr_arch_thread_rwlock.h"
#include "apr_portable.h"

static apr_status_t thread_rwlock_cleanup(void *data)
{
    return apr_thread_rwlock_destroy((apr_thread_rwlock_t *) data);
}

APR_DECLARE(apr_status_t)apr_thread_rwlock_create(apr_thread_rwlock_t **rwlock,
                                                  apr_pool_t *pool)
{
    *rwlock = apr_palloc(pool, sizeof(**rwlock));

    (*rwlock)->pool        = pool;
    (*rwlock)->readers     = 0;

    if (! ((*rwlock)->read_event = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        *rwlock = NULL;
        return apr_get_os_error();
    }

    if (! ((*rwlock)->write_mutex = CreateMutex(NULL, FALSE, NULL))) {
        CloseHandle((*rwlock)->read_event);
        *rwlock = NULL;
        return apr_get_os_error();
    }

    apr_pool_cleanup_register(pool, *rwlock, thread_rwlock_cleanup,
                              apr_pool_cleanup_null);

    return APR_SUCCESS;
}

static apr_status_t apr_thread_rwlock_rdlock_core(apr_thread_rwlock_t *rwlock,
                                                  DWORD  milliseconds)
{
    DWORD   code = WaitForSingleObject(rwlock->write_mutex, milliseconds);

    if (code == WAIT_FAILED || code == WAIT_TIMEOUT)
        return APR_FROM_OS_ERROR(code);

    /* We've successfully acquired the writer mutex, we can't be locked
     * for write, so it's OK to add the reader lock.  The writer mutex
     * doubles as race condition protection for the readers counter.   
     */
    InterlockedIncrement(&rwlock->readers);
    
    if (! ResetEvent(rwlock->read_event))
        return apr_get_os_error();
    
    if (! ReleaseMutex(rwlock->write_mutex))
        return apr_get_os_error();
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_rdlock(apr_thread_rwlock_t *rwlock)
{
    return apr_thread_rwlock_rdlock_core(rwlock, INFINITE);
}

APR_DECLARE(apr_status_t) 
apr_thread_rwlock_tryrdlock(apr_thread_rwlock_t *rwlock)
{
    return apr_thread_rwlock_rdlock_core(rwlock, 0);
}

static apr_status_t 
apr_thread_rwlock_wrlock_core(apr_thread_rwlock_t *rwlock, DWORD milliseconds)
{
    DWORD   code = WaitForSingleObject(rwlock->write_mutex, milliseconds);

    if (code == WAIT_FAILED || code == WAIT_TIMEOUT)
        return APR_FROM_OS_ERROR(code);

    /* We've got the writer lock but we have to wait for all readers to
     * unlock before it's ok to use it.
     */
    if (rwlock->readers) {
        /* Must wait for readers to finish before returning, unless this
         * is an trywrlock (milliseconds == 0):
         */
        code = milliseconds
          ? WaitForSingleObject(rwlock->read_event, milliseconds)
          : WAIT_TIMEOUT;
        
        if (code == WAIT_FAILED || code == WAIT_TIMEOUT) {
            /* Unable to wait for readers to finish, release write lock: */
            if (! ReleaseMutex(rwlock->write_mutex))
                return apr_get_os_error();
            
            return APR_FROM_OS_ERROR(code);
        }
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_wrlock(apr_thread_rwlock_t *rwlock)
{
    return apr_thread_rwlock_wrlock_core(rwlock, INFINITE);
}

APR_DECLARE(apr_status_t)apr_thread_rwlock_trywrlock(apr_thread_rwlock_t *rwlock)
{
    return apr_thread_rwlock_wrlock_core(rwlock, 0);
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_unlock(apr_thread_rwlock_t *rwlock)
{
    apr_status_t rv = 0;

    /* First, guess that we're unlocking a writer */
    if (! ReleaseMutex(rwlock->write_mutex))
        rv = apr_get_os_error();
    
    if (rv == APR_FROM_OS_ERROR(ERROR_NOT_OWNER)) {
        /* Nope, we must have a read lock */
        if (rwlock->readers &&
            ! InterlockedDecrement(&rwlock->readers) &&
            ! SetEvent(rwlock->read_event)) {
            rv = apr_get_os_error();
        }
        else {
            rv = 0;
        }
    }

    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_destroy(apr_thread_rwlock_t *rwlock)
{
    if (! CloseHandle(rwlock->read_event))
        return apr_get_os_error();

    if (! CloseHandle(rwlock->write_mutex))
        return apr_get_os_error();
    
    return APR_SUCCESS;
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_rwlock)
