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
    apr_thread_rwlock_t *rwlock = data;
    CloseHandle(rwlock->readevent);
    CloseHandle(rwlock->mutex);
    CloseHandle(rwlock->writemutex);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_create(apr_thread_rwlock_t **rwlock,
                                                   apr_pool_t *pool)
{
    (*rwlock) = apr_palloc(pool, sizeof(**rwlock));
    (*rwlock)->pool = pool;
    (*rwlock)->readevent=CreateEvent(NULL,TRUE,FALSE,NULL);
    (*rwlock)->mutex = CreateEvent(NULL,FALSE,TRUE,NULL);
    (*rwlock)->writemutex = CreateMutex(NULL,FALSE,NULL);
    (*rwlock)->counter = -1;
    (*rwlock)->wrcounter = 0;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_rdlock(apr_thread_rwlock_t *rwlock)
{
    if (InterlockedIncrement(&rwlock->counter) == 0) {
        WaitForSingleObject(rwlock->mutex, INFINITE);
        SetEvent(rwlock->readevent);
    }
    WaitForSingleObject(rwlock->readevent,INFINITE);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_tryrdlock(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_wrlock(apr_thread_rwlock_t *rwlock)
{
    WaitForSingleObject(rwlock->writemutex,INFINITE);
    WaitForSingleObject(rwlock->mutex, INFINITE);
    rwlock->wrcounter++;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_trywrlock(apr_thread_rwlock_t *rwlock)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_unlock(apr_thread_rwlock_t *rwlock)
{
    if (rwlock->wrcounter) {
        /* If wrcounter is > 0, then we must have a writer lock */
        rwlock->wrcounter--;
        SetEvent(rwlock->mutex);
        ReleaseMutex(rwlock->writemutex);
    } 
    else {
        if (InterlockedDecrement(&rwlock->counter) < 0) {
            ResetEvent(rwlock->readevent);
            SetEvent(rwlock->mutex);
        }
    } 
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_rwlock_destroy(apr_thread_rwlock_t *rwlock)
{
    return apr_pool_cleanup_run(rwlock->pool, rwlock, thread_rwlock_cleanup);
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_rwlock)

