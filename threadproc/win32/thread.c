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

#include "apr_private.h"
#include "win32/apr_arch_threadproc.h"
#include "apr_thread_proc.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#if APR_HAVE_PROCESS_H
#include <process.h>
#endif
#include "apr_arch_misc.h"   

/* Chosen for us by apr_initialize */
DWORD tls_apr_thread = 0;

APR_DECLARE(apr_status_t) apr_threadattr_create(apr_threadattr_t **new,
                                                apr_pool_t *pool)
{
    (*new) = (apr_threadattr_t *)apr_palloc(pool, 
              sizeof(apr_threadattr_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->pool = pool;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_threadattr_detach_set(apr_threadattr_t *attr,
                                                   apr_int32_t on)
{
    attr->detach = on;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_threadattr_detach_get(apr_threadattr_t *attr)
{
    if (attr->detach == 1)
        return APR_DETACH;
    return APR_NOTDETACH;
}

static void *dummy_worker(void *opaque)
{
    apr_thread_t *thd = (apr_thread_t *)opaque;
    TlsSetValue(tls_apr_thread, thd->td);
    return thd->func(thd, thd->data);
}

APR_DECLARE(apr_status_t) apr_thread_create(apr_thread_t **new,
                                            apr_threadattr_t *attr,
                                            apr_thread_start_t func,
                                            void *data, apr_pool_t *pool)
{
    apr_status_t stat;
	unsigned temp;

    (*new) = (apr_thread_t *)apr_palloc(pool, sizeof(apr_thread_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->pool = pool;
    (*new)->data = data;
    (*new)->func = func;
    
    stat = apr_pool_create(&(*new)->pool, pool);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    /* Use 0 for Thread Stack Size, because that will default the stack to the
     * same size as the calling thread. 
     */
#ifndef _WIN32_WCE
    if (((*new)->td = (HANDLE)_beginthreadex(NULL, 0, 
                        (unsigned int (APR_THREAD_FUNC *)(void *))dummy_worker,
                        (*new), 0, &temp)) == 0) {
        return APR_FROM_OS_ERROR(_doserrno);
    }
#else
   if (((*new)->td = CreateThread(NULL, 0, 
                        (unsigned int (APR_THREAD_FUNC *)(void *))dummy_worker,
                        (*new), 0, &temp)) == 0) {
        return apr_get_os_error();
    }
#endif
    if (attr && attr->detach) {
        CloseHandle((*new)->td);
        (*new)->td = NULL;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_exit(apr_thread_t *thd,
                                          apr_status_t retval)
{
    thd->exitval = retval;
    apr_pool_destroy(thd->pool);
#ifndef _WIN32_WCE
    if (thd->td) {
        CloseHandle(thd->td);
    }
    _endthreadex(0);
#else
    ExitThread(0);
#endif
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_join(apr_status_t *retval,
                                          apr_thread_t *thd)
{
    apr_status_t rv;

    rv = WaitForSingleObject(thd->td, INFINITE);
    if ( rv == WAIT_OBJECT_0 || rv == WAIT_ABANDONED) {
        *retval = thd->exitval;
        return APR_SUCCESS;
    }
    /* Wait failed */
    return apr_get_os_error();;
}

APR_DECLARE(apr_status_t) apr_thread_detach(apr_thread_t *thd)
{
    if (thd->td && CloseHandle(thd->td)) {
        thd->td = NULL;
        return APR_SUCCESS;
    }
    else {
        return apr_get_os_error();
    }
}

APR_DECLARE(void) apr_thread_yield()
{
    /* SwitchToThread is not supported on Win9x, but since it's
     * primarily a noop (entering time consuming code, therefore
     * providing more critical threads a bit larger timeslice)
     * we won't worry too much if it's not available.
     */
#ifndef _WIN32_WCE
    if (apr_os_level >= APR_WIN_NT) {
        SwitchToThread();
    }
#endif
}

APR_DECLARE(apr_status_t) apr_thread_data_get(void **data, const char *key,
                                             apr_thread_t *thread)
{
    return apr_pool_userdata_get(data, key, thread->pool);
}

APR_DECLARE(apr_status_t) apr_thread_data_set(void *data, const char *key,
                                             apr_status_t (*cleanup) (void *),
                                             apr_thread_t *thread)
{
    return apr_pool_userdata_set(data, key, cleanup, thread->pool);
}


APR_DECLARE(apr_os_thread_t) apr_os_thread_current(void)
{
    HANDLE hthread = (HANDLE)TlsGetValue(tls_apr_thread);
    HANDLE hproc;

    if (hthread) {
        return hthread;
    }
    
    hproc = GetCurrentProcess();
    hthread = GetCurrentThread();
    if (!DuplicateHandle(hproc, hthread, 
                         hproc, &hthread, 0, FALSE, 
                         DUPLICATE_SAME_ACCESS)) {
        return NULL;
    }
    TlsSetValue(tls_apr_thread, hthread);
    return hthread;
}

APR_DECLARE(apr_status_t) apr_os_thread_get(apr_os_thread_t **thethd,
                                            apr_thread_t *thd)
{
    if (thd == NULL) {
        return APR_ENOTHREAD;
    }
    *thethd = thd->td;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_thread_put(apr_thread_t **thd,
                                            apr_os_thread_t *thethd,
                                            apr_pool_t *pool)
{
    if (pool == NULL) {
        return APR_ENOPOOL;
    }
    if ((*thd) == NULL) {
        (*thd) = (apr_thread_t *)apr_palloc(pool, sizeof(apr_thread_t));
        (*thd)->pool = pool;
    }
    (*thd)->td = thethd;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_once_init(apr_thread_once_t **control,
                                               apr_pool_t *p)
{
    (*control) = apr_pcalloc(p, sizeof(**control));
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_once(apr_thread_once_t *control,
                                          void (*func)(void))
{
    if (!InterlockedExchange(&control->value, 1)) {
        func();
    }
    return APR_SUCCESS;
}

APR_DECLARE(int) apr_os_thread_equal(apr_os_thread_t tid1,
                                     apr_os_thread_t tid2)
{
    /* Since the only tid's we support our are own, and
     * apr_os_thread_current returns the identical handle
     * to the one we created initially, the test is simple.
     */
    return (tid1 == tid2);
}

APR_POOL_IMPLEMENT_ACCESSOR(thread)
