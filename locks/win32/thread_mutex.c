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
#include "apr_arch_thread_mutex.h"
#include "apr_thread_mutex.h"
#include "apr_portable.h"
#include "apr_arch_misc.h"

static apr_status_t thread_mutex_cleanup(void *data)
{
    apr_thread_mutex_t *lock = data;

    if (lock->type == thread_mutex_critical_section) {
        DeleteCriticalSection(&lock->section);
    }
    else {
        if (!CloseHandle(lock->handle)) {
            return apr_get_os_error();
        }
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_create(apr_thread_mutex_t **mutex,
                                                  unsigned int flags,
                                                  apr_pool_t *pool)
{
    (*mutex) = (apr_thread_mutex_t *)apr_palloc(pool, sizeof(**mutex));

    (*mutex)->pool = pool;

    if (flags & APR_THREAD_MUTEX_UNNESTED) {
        /* Use an auto-reset signaled event, ready to accept one
         * waiting thread.
         */
        (*mutex)->type = thread_mutex_unnested_event;
        (*mutex)->handle = CreateEvent(NULL, FALSE, TRUE, NULL);
    }
    else {
#if APR_HAS_UNICODE_FS
        /* Critical Sections are terrific, performance-wise, on NT.
         * On Win9x, we cannot 'try' on a critical section, so we 
         * use a [slower] mutex object, instead.
         */
        IF_WIN_OS_IS_UNICODE {
            (*mutex)->type = thread_mutex_critical_section;
            InitializeCriticalSection(&(*mutex)->section);
        }
#endif
#if APR_HAS_ANSI_FS
        ELSE_WIN_OS_IS_ANSI {
            (*mutex)->type = thread_mutex_nested_mutex;
            (*mutex)->handle = CreateMutex(NULL, FALSE, NULL);

        }
#endif
    }

    apr_pool_cleanup_register((*mutex)->pool, (*mutex), thread_mutex_cleanup,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_lock(apr_thread_mutex_t *mutex)
{
    if (mutex->type == thread_mutex_critical_section) {
        EnterCriticalSection(&mutex->section);
    }
    else {
        DWORD rv = WaitForSingleObject(mutex->handle, INFINITE);
	if ((rv != WAIT_OBJECT_0) && (rv != WAIT_ABANDONED)) {
            return (rv == WAIT_TIMEOUT) ? APR_EBUSY : apr_get_os_error();
	}
    }        
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_trylock(apr_thread_mutex_t *mutex)
{
    if (mutex->type == thread_mutex_critical_section) {
        if (!TryEnterCriticalSection(&mutex->section)) {
            return APR_EBUSY;
        }
    }
    else {
        DWORD rv = WaitForSingleObject(mutex->handle, 0);
	if ((rv != WAIT_OBJECT_0) && (rv != WAIT_ABANDONED)) {
            return (rv == WAIT_TIMEOUT) ? APR_EBUSY : apr_get_os_error();
	}
    }        
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_unlock(apr_thread_mutex_t *mutex)
{
    if (mutex->type == thread_mutex_critical_section) {
        LeaveCriticalSection(&mutex->section);
    }
    else if (mutex->type == thread_mutex_unnested_event) {
        if (!SetEvent(mutex->handle)) {
            return apr_get_os_error();
        }
    }
    else if (mutex->type == thread_mutex_nested_mutex) {
        if (!ReleaseMutex(mutex->handle)) {
            return apr_get_os_error();
        }
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_mutex_destroy(apr_thread_mutex_t *mutex)
{
    return apr_pool_cleanup_run(mutex->pool, mutex, thread_mutex_cleanup);
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_mutex)

