/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_private.h"
#include "apr_arch_threadproc.h"
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
    (*new)->detach = 0;
    (*new)->stacksize = 0;

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

APR_DECLARE(apr_status_t) apr_threadattr_stacksize_set(apr_threadattr_t *attr,
                                                       apr_size_t stacksize)
{
    attr->stacksize = stacksize;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_threadattr_guardsize_set(apr_threadattr_t *attr,
                                                       apr_size_t size)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_threadattr_max_free_set(apr_threadattr_t *attr, 
                                                      apr_size_t size)
{
    attr->max_free = size;
    return APR_SUCCESS;
}

#if APR_HAS_THREAD_LOCAL
static APR_THREAD_LOCAL apr_thread_t *current_thread = NULL;
#endif

static void *dummy_worker(void *opaque)
{
    apr_thread_t *thd = (apr_thread_t *)opaque;
    void *ret;

#if APR_HAS_THREAD_LOCAL
    current_thread = thd;
#endif

    TlsSetValue(tls_apr_thread, thd->td);
    ret = thd->func(thd, thd->data);
    if (!thd->td) { /* detached? */
        apr_pool_destroy(thd->pool);
    }

    return ret;
}

static apr_status_t alloc_thread(apr_thread_t **new,
                                 apr_threadattr_t *attr,
                                 apr_thread_start_t func, void *data,
                                 apr_pool_t *pool)
{
    apr_status_t stat;
    apr_abortfunc_t abort_fn = apr_pool_abort_get(pool);
    apr_pool_t *p;

    /* The thread can be detached anytime (from the creation or later with
     * apr_thread_detach), so it needs its own pool and allocator to not
     * depend on a parent pool which could be destroyed before the thread
     * exits. The allocator needs no mutex obviously since the pool should
     * not be used nor create children pools outside the thread. Passing
     * NULL allocator will create one like that.
     */
    stat = apr_pool_create_unmanaged_ex(&p, abort_fn, NULL);
    if (stat != APR_SUCCESS) {
        return stat;
    }
    if (attr && attr->max_free) {
        apr_allocator_max_free_set(apr_pool_allocator_get(p), attr->max_free);
    }

    (*new) = (apr_thread_t *)apr_pcalloc(p, sizeof(apr_thread_t));
    if ((*new) == NULL) {
        apr_pool_destroy(p);
        return APR_ENOMEM;
    }

    (*new)->pool = p;
    (*new)->data = data;
    (*new)->func = func;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_create(apr_thread_t **new,
                                            apr_threadattr_t *attr,
                                            apr_thread_start_t func,
                                            void *data, apr_pool_t *pool)
{
    apr_status_t stat;
    unsigned temp;
    HANDLE handle;

    stat = alloc_thread(new, attr, func, data, pool);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    /* Use 0 for default Thread Stack Size, because that will
     * default the stack to the same size as the calling thread.
     */
#ifndef _WIN32_WCE
    if ((handle = (HANDLE)_beginthreadex(NULL,
                        (DWORD) (attr ? attr->stacksize : 0),
                        (unsigned int (APR_THREAD_FUNC *)(void *))dummy_worker,
                        (*new), 0, &temp)) == 0) {
        stat = APR_FROM_OS_ERROR(_doserrno);
        apr_pool_destroy((*new)->pool);
        return stat;
    }
#else
   if ((handle = CreateThread(NULL,
                        attr && attr->stacksize > 0 ? attr->stacksize : 0,
                        (unsigned int (APR_THREAD_FUNC *)(void *))dummy_worker,
                        (*new), 0, &temp)) == 0) {
        stat = apr_get_os_error();
        apr_pool_destroy((*new)->pool);
        return stat;
    }
#endif

    if (attr && attr->detach) {
        CloseHandle(handle);
    }
    else {
        (*new)->td = handle;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_current_create(apr_thread_t **current,
                                                    apr_threadattr_t *attr,
                                                    apr_pool_t *pool)
{
#if APR_HAS_THREAD_LOCAL
    apr_status_t stat;

    *current = apr_thread_current();
    if (*current) {
        return APR_EEXIST;
    }

    stat = alloc_thread(current, attr, NULL, NULL, pool);
    if (stat != APR_SUCCESS) {
        *current = NULL;
        return stat;
    }

    if (!(attr && attr->detach)) {
        (*current)->td = apr_os_thread_current();
    }

    current_thread = *current;
    return APR_SUCCESS;
#else
    return APR_ENOTIMPL;
#endif
}

APR_DECLARE(void) apr_thread_current_after_fork(void)
{
#if APR_HAS_THREAD_LOCAL
    current_thread = NULL;
#endif
}

APR_DECLARE(apr_thread_t *) apr_thread_current(void)
{
#if APR_HAS_THREAD_LOCAL
    return current_thread;
#else
    return NULL;
#endif
}

APR_DECLARE(apr_status_t) apr_thread_exit(apr_thread_t *thd,
                                          apr_status_t retval)
{
    thd->exited = 1;
    thd->exitval = retval;
    if (!thd->td) { /* detached? */
        apr_pool_destroy(thd->pool);
    }
#if APR_HAS_THREAD_LOCAL
    current_thread = NULL;
#endif
#ifndef _WIN32_WCE
    _endthreadex(0);
#else
    ExitThread(0);
#endif
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_join(apr_status_t *retval,
                                          apr_thread_t *thd)
{
    apr_status_t rv = APR_SUCCESS;
    DWORD ret;
    
    if (!thd->td) {
        /* Can not join on detached threads */
        return APR_DETACH;
    }

    ret = WaitForSingleObject(thd->td, INFINITE);
    if (ret == WAIT_OBJECT_0 || ret == WAIT_ABANDONED) {
        /* If the thread_exit has been called */
        if (thd->exited)
            *retval = thd->exitval;
        else
            rv = APR_INCOMPLETE;
    }
    else
        rv = apr_get_os_error();

    if (rv == APR_SUCCESS) {
        CloseHandle(thd->td);
        apr_pool_destroy(thd->pool);
    }

    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_detach(apr_thread_t *thd)
{
    if (!thd->td) {
        return APR_EINVAL;
    }

    if (CloseHandle(thd->td)) {
        thd->td = NULL;
        return APR_SUCCESS;
    }
    else {
        return apr_get_os_error();
    }
}

APR_DECLARE(void) apr_thread_yield(void)
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
    if (thread == NULL) {
        *data = NULL;
        return APR_ENOTHREAD;
    }
    return apr_pool_userdata_get(data, key, thread->pool);
}

APR_DECLARE(apr_status_t) apr_thread_data_set(void *data, const char *key,
                                             apr_status_t (*cleanup) (void *),
                                             apr_thread_t *thread)
{
    if (thread == NULL) {
        return APR_ENOTHREAD;
    }
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
