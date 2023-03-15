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

#define INCL_DOSERRORS
#define INCL_DOS
#include "apr_arch_threadproc.h"
#include "apr_thread_proc.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include "apr_arch_file_io.h"
#include <stdlib.h>



APR_DECLARE(apr_status_t) apr_threadattr_create(apr_threadattr_t **new, apr_pool_t *pool)
{
    (*new) = (apr_threadattr_t *)apr_palloc(pool, sizeof(apr_threadattr_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->pool = pool;
    (*new)->attr = 0;
    (*new)->stacksize = 0;
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_threadattr_detach_set(apr_threadattr_t *attr, apr_int32_t on)
{
    attr->attr |= APR_THREADATTR_DETACHED;
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_threadattr_detach_get(apr_threadattr_t *attr)
{
    return (attr->attr & APR_THREADATTR_DETACHED) ? APR_DETACH : APR_NOTDETACH;
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

static void dummy_worker(void *opaque)
{
#if APR_HAS_THREAD_LOCAL
  current_thread = thread;
#endif

  apr_thread_t *thread = (apr_thread_t *)opaque;
  apr_pool_owner_set(thread->pool, 0);
  thread->exitval = thread->func(thread, thread->data);
  if (thd->attr->attr & APR_THREADATTR_DETACHED) {
      apr_pool_destroy(thread->pool);
  }
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
    (*new)->func = func;
    (*new)->data = data;
    if (attr == NULL) {
        stat = apr_threadattr_create(&attr, p);
        if (stat != APR_SUCCESS) {
            apr_pool_destroy(p);
            return stat;
        }
    }
    (*new)->attr = attr;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_create(apr_thread_t **new,
                                            apr_threadattr_t *attr,
                                            apr_thread_start_t func, void *data,
                                            apr_pool_t *pool)
{
    apr_status_t stat;

    stat = alloc_thread(new, attr, func, data, pool);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    (*new)->tid = _beginthread(dummy_worker, NULL,
                               (*new)->attr->stacksize > 0 ?
                               (*new)->attr->stacksize : APR_THREAD_STACKSIZE,
                               (*new));
    if ((*new)->tid < 0) {
        stat = errno;
        apr_pool_destroy((*new)->pool);
        return stat;
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

    (*current)->tid = apr_os_thread_current();

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


APR_DECLARE(apr_os_thread_t) apr_os_thread_current()
{
    PIB *ppib;
    TIB *ptib;
    DosGetInfoBlocks(&ptib, &ppib);
    return ptib->tib_ptib2->tib2_ultid;
}



APR_DECLARE(void) apr_thread_exit(apr_thread_t *thd, apr_status_t retval)
{
    thd->exitval = retval;
    if (thd->attr->attr & APR_THREADATTR_DETACHED) {
        apr_pool_destroy(thd->pool);
    }
    _endthread();
}



APR_DECLARE(apr_status_t) apr_thread_join(apr_status_t *retval, apr_thread_t *thd)
{
    ULONG rc;
    TID waittid = thd->tid;

    if (thd->attr->attr & APR_THREADATTR_DETACHED)
        return APR_EINVAL;

    rc = DosWaitThread(&waittid, DCWW_WAIT);

    if (rc == ERROR_INVALID_THREADID)
        rc = 0; /* Thread had already terminated */

    *retval = thd->exitval;
    if (rc == 0) {
        apr_pool_destroy(thd->pool);
    }
    return APR_FROM_OS_ERROR(rc);
}



APR_DECLARE(apr_status_t) apr_thread_detach(apr_thread_t *thd)
{
    if (thd->attr->attr & APR_THREADATTR_DETACHED) {
        return APR_EINVAL;
    }

    thd->attr->attr |= APR_THREADATTR_DETACHED;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_name_set(const char *name,
                                              apr_thread_t *thread,
                                              apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_thread_name_get(char ** name,
                                              apr_thread_t *thread,
                                              apr_pool_t *pool)
{ 
    return APR_ENOTIMPL;
}



void apr_thread_yield()
{
    DosSleep(0);
}



APR_DECLARE(apr_status_t) apr_os_thread_get(apr_os_thread_t **thethd, apr_thread_t *thd)
{
    *thethd = &thd->tid;
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_os_thread_put(apr_thread_t **thd, apr_os_thread_t *thethd,
                                            apr_pool_t *pool)
{
    if ((*thd) == NULL) {
        (*thd) = (apr_thread_t *)apr_pcalloc(pool, sizeof(apr_thread_t));
        (*thd)->pool = pool;
    }
    (*thd)->tid = *thethd;
    return APR_SUCCESS;
}



int apr_os_thread_equal(apr_os_thread_t tid1, apr_os_thread_t tid2)
{
    return tid1 == tid2;
}



APR_DECLARE(apr_status_t) apr_thread_data_get(void **data, const char *key, apr_thread_t *thread)
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

APR_POOL_IMPLEMENT_ACCESSOR(thread)



static apr_status_t thread_once_cleanup(void *vcontrol)
{
    apr_thread_once_t *control = (apr_thread_once_t *)vcontrol;

    if (control->sem) {
        DosCloseEventSem(control->sem);
    }

    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_thread_once_init(apr_thread_once_t **control,
                                               apr_pool_t *p)
{
    ULONG rc;
    *control = (apr_thread_once_t *)apr_pcalloc(p, sizeof(apr_thread_once_t));
    rc = DosCreateEventSem(NULL, &(*control)->sem, 0, TRUE);
    apr_pool_cleanup_register(p, control, thread_once_cleanup, apr_pool_cleanup_null);
    return APR_FROM_OS_ERROR(rc);
}



APR_DECLARE(apr_status_t) apr_thread_once(apr_thread_once_t *control,
                                          void (*func)(void))
{
    if (!control->hit) {
        ULONG count, rc;
        rc = DosResetEventSem(control->sem, &count);

        if (rc == 0 && count) {
            control->hit = 1;
            func();
        }
    }

    return APR_SUCCESS;
}
