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

#include "apr.h"
#include "apr_portable.h"
#include "apr_arch_threadproc.h"

#if APR_HAS_THREADS

#if APR_HAVE_PTHREAD_H

/* Unfortunately the kernel headers do not export the TASK_COMM_LEN
   macro.  So we have to define it here. Used in apr_thread_name_get and
   apr_thread_name_set functions */
#define TASK_COMM_LEN 16

/* Destroy the threadattr object */
static apr_status_t threadattr_cleanup(void *data)
{
    apr_threadattr_t *attr = data;
    apr_status_t rv;

    rv = pthread_attr_destroy(&attr->attr);
#ifdef HAVE_ZOS_PTHREADS
    if (rv) {
        rv = errno;
    }
#endif
    return rv;
}

APR_DECLARE(apr_status_t) apr_threadattr_create(apr_threadattr_t **new,
                                                apr_pool_t *pool)
{
    apr_status_t stat;

    (*new) = apr_palloc(pool, sizeof(apr_threadattr_t));
    (*new)->pool = pool;
    stat = pthread_attr_init(&(*new)->attr);

    if (stat == 0) {
        apr_pool_cleanup_register(pool, *new, threadattr_cleanup,
                                  apr_pool_cleanup_null);
        return APR_SUCCESS;
    }
#ifdef HAVE_ZOS_PTHREADS
    stat = errno;
#endif

    return stat;
}

#if defined(PTHREAD_CREATE_DETACHED)
#define DETACH_ARG(v) ((v) ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE)
#else
#define DETACH_ARG(v) ((v) ? 1 : 0)
#endif

APR_DECLARE(apr_status_t) apr_threadattr_detach_set(apr_threadattr_t *attr,
                                                    apr_int32_t on)
{
    apr_status_t stat;
#ifdef HAVE_ZOS_PTHREADS
    int arg = DETACH_ARG(on);

    if ((stat = pthread_attr_setdetachstate(&attr->attr, &arg)) == 0) {
#else
    if ((stat = pthread_attr_setdetachstate(&attr->attr,
                                            DETACH_ARG(on))) == 0) {
#endif
        return APR_SUCCESS;
    }
    else {
#ifdef HAVE_ZOS_PTHREADS
        stat = errno;
#endif

        return stat;
    }
}

APR_DECLARE(apr_status_t) apr_threadattr_detach_get(apr_threadattr_t *attr)
{
    int state;

#ifdef PTHREAD_ATTR_GETDETACHSTATE_TAKES_ONE_ARG
    state = pthread_attr_getdetachstate(&attr->attr);
#else
    pthread_attr_getdetachstate(&attr->attr, &state);
#endif
    if (state == DETACH_ARG(1))
        return APR_DETACH;
    return APR_NOTDETACH;
}

APR_DECLARE(apr_status_t) apr_threadattr_stacksize_set(apr_threadattr_t *attr,
                                                       apr_size_t stacksize)
{
    int stat;

    stat = pthread_attr_setstacksize(&attr->attr, stacksize);
    if (stat == 0) {
        return APR_SUCCESS;
    }
#ifdef HAVE_ZOS_PTHREADS
    stat = errno;
#endif

    return stat;
}

APR_DECLARE(apr_status_t) apr_threadattr_guardsize_set(apr_threadattr_t *attr,
                                                       apr_size_t size)
{
#ifdef HAVE_PTHREAD_ATTR_SETGUARDSIZE
    apr_status_t rv;

    rv = pthread_attr_setguardsize(&attr->attr, size);
    if (rv == 0) {
        return APR_SUCCESS;
    }
#ifdef HAVE_ZOS_PTHREADS
    rv = errno;
#endif
    return rv;
#else
    return APR_ENOTIMPL;
#endif
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
    apr_thread_t *thread = (apr_thread_t*)opaque;
    void *ret;

#if APR_HAS_THREAD_LOCAL
    current_thread = thread;
#endif

    apr_pool_owner_set(thread->pool, 0);
    ret = thread->func(thread, thread->data);
    if (thread->detached) {
        apr_pool_destroy(thread->pool);
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
    (*new)->detached = (attr && apr_threadattr_detach_get(attr) == APR_DETACH);
    (*new)->td = (pthread_t *)apr_pcalloc(p, sizeof(pthread_t));
    if ((*new)->td == NULL) {
        apr_pool_destroy(p);
        return APR_ENOMEM;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_create(apr_thread_t **new,
                                            apr_threadattr_t *attr,
                                            apr_thread_start_t func,
                                            void *data,
                                            apr_pool_t *pool)
{
    apr_status_t stat;
    pthread_attr_t *temp;

    stat = alloc_thread(new, attr, func, data, pool);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    if (attr)
        temp = &attr->attr;
    else
        temp = NULL;

    if ((stat = pthread_create((*new)->td, temp, dummy_worker, (*new)))) {
#ifdef HAVE_ZOS_PTHREADS
        stat = errno;
#endif
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

    *(*current)->td = apr_os_thread_current();

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

APR_DECLARE(apr_status_t) apr_thread_name_set(const char *name,
                                              apr_thread_t *thread,
                                              apr_pool_t *pool)
{
#if HAVE_PTHREAD_SETNAME_NP
    pthread_t td;

    size_t name_len;
    if (!name) {
        return APR_BADARG;
    }

    if (thread) {
        td = *thread->td;
    }
    else {
        td = pthread_self();
    }

    name_len = strlen(name);
    if (name_len >= TASK_COMM_LEN) {
        name = name + name_len - TASK_COMM_LEN + 1;
    }

    return pthread_setname_np(td, name);
#else
    return APR_ENOTIMPL;
#endif
}

APR_DECLARE(apr_status_t) apr_thread_name_get(char **name,
                                              apr_thread_t *thread,
                                              apr_pool_t *pool)
{
#if HAVE_PTHREAD_SETNAME_NP
    pthread_t td;
    if (thread) {
        td = *thread->td;
    }
    else {
        td = pthread_self();
    }

    *name = apr_pcalloc(pool, TASK_COMM_LEN);
    return pthread_getname_np(td, *name, TASK_COMM_LEN);
#else
    return APR_ENOTIMPL;
#endif
}

APR_DECLARE(apr_os_thread_t) apr_os_thread_current(void)
{
    return pthread_self();
}

APR_DECLARE(int) apr_os_thread_equal(apr_os_thread_t tid1,
                                     apr_os_thread_t tid2)
{
    return pthread_equal(tid1, tid2);
}

APR_DECLARE(void) apr_thread_exit(apr_thread_t *thd,
                                  apr_status_t retval)
{
    thd->exitval = retval;
    if (thd->detached) {
        apr_pool_destroy(thd->pool);
    }
    pthread_exit(NULL);
}

APR_DECLARE(apr_status_t) apr_thread_join(apr_status_t *retval,
                                          apr_thread_t *thd)
{
    apr_status_t stat;
    void *thread_stat;

    if (thd->detached) {
        return APR_EINVAL;
    }

    if ((stat = pthread_join(*thd->td, &thread_stat))) {
#ifdef HAVE_ZOS_PTHREADS
        stat = errno;
#endif
        return stat;
    }

    *retval = thd->exitval;
    apr_pool_destroy(thd->pool);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_detach(apr_thread_t *thd)
{
    apr_status_t stat;

    if (thd->detached) {
        return APR_EINVAL;
    }

#ifdef HAVE_ZOS_PTHREADS
    if ((stat = pthread_detach(thd->td)) == 0) {
#else
    if ((stat = pthread_detach(*thd->td)) == 0) {
#endif
        thd->detached = 1;

        return APR_SUCCESS;
    }
    else {
#ifdef HAVE_ZOS_PTHREADS
        stat = errno;
#endif

        return stat;
    }
}

APR_DECLARE(void) apr_thread_yield(void)
{
#ifdef HAVE_PTHREAD_YIELD
#ifdef HAVE_ZOS_PTHREADS
    pthread_yield(NULL);
#else
    pthread_yield();
#endif /* HAVE_ZOS_PTHREADS */
#else
#ifdef HAVE_SCHED_YIELD
    sched_yield();
#endif
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
                              apr_status_t (*cleanup)(void *),
                              apr_thread_t *thread)
{
    if (thread == NULL) {
        return APR_ENOTHREAD;
    }
    return apr_pool_userdata_set(data, key, cleanup, thread->pool);
}

APR_DECLARE(apr_status_t) apr_os_thread_get(apr_os_thread_t **thethd,
                                            apr_thread_t *thd)
{
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
        (*thd) = (apr_thread_t *)apr_pcalloc(pool, sizeof(apr_thread_t));
        (*thd)->pool = pool;
    }

    (*thd)->td = thethd;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_once_init(apr_thread_once_t **control,
                                               apr_pool_t *p)
{
    static const pthread_once_t once_init = PTHREAD_ONCE_INIT;

    *control = apr_palloc(p, sizeof(**control));
    (*control)->once = once_init;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_once(apr_thread_once_t *control,
                                          void (*func)(void))
{
    return pthread_once(&control->once, func);
}

APR_POOL_IMPLEMENT_ACCESSOR(thread)

#endif  /* HAVE_PTHREAD_H */
#endif  /* APR_HAS_THREADS */

#if !APR_HAS_THREADS

/* avoid warning for no prototype */
APR_DECLARE(apr_status_t) apr_os_thread_get(void);

APR_DECLARE(apr_status_t) apr_os_thread_get(void)
{
    return APR_ENOTIMPL;
}

#endif
