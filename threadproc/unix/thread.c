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
#include "apr_portable.h"
#include "apr_arch_threadproc.h"

#if APR_HAS_THREADS

#if APR_HAVE_PTHREAD_H
APR_DECLARE(apr_status_t) apr_threadattr_create(apr_threadattr_t **new,
                                                apr_pool_t *pool)
{
    apr_status_t stat;

    (*new) = (apr_threadattr_t *)apr_pcalloc(pool, sizeof(apr_threadattr_t));
    (*new)->attr = (pthread_attr_t *)apr_pcalloc(pool, sizeof(pthread_attr_t));

    if ((*new) == NULL || (*new)->attr == NULL) {
        return APR_ENOMEM;
    }

    (*new)->pool = pool;
    stat = pthread_attr_init((*new)->attr);

    if (stat == 0) {
        return APR_SUCCESS;
    }
#ifdef PTHREAD_SETS_ERRNO
    stat = errno;
#endif

    return stat;
}

APR_DECLARE(apr_status_t) apr_threadattr_detach_set(apr_threadattr_t *attr,
                                                    apr_int32_t on)
{
    apr_status_t stat;
#ifdef PTHREAD_ATTR_SETDETACHSTATE_ARG2_ADDR
    int arg = on;

    if ((stat = pthread_attr_setdetachstate(attr->attr, &arg)) == 0) {
#else
    if ((stat = pthread_attr_setdetachstate(attr->attr, on)) == 0) {
#endif

        return APR_SUCCESS;
    }
    else {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif

        return stat;
    }
}

APR_DECLARE(apr_status_t) apr_threadattr_detach_get(apr_threadattr_t *attr)
{
    int state;

#ifdef PTHREAD_ATTR_GETDETACHSTATE_TAKES_ONE_ARG
    state = pthread_attr_getdetachstate(attr->attr);
#else
    pthread_attr_getdetachstate(attr->attr, &state);
#endif
    if (state == 1)
        return APR_DETACH;
    return APR_NOTDETACH;
}

static void *dummy_worker(void *opaque)
{
    apr_thread_t *thread = (apr_thread_t*)opaque;
    return thread->func(thread, thread->data);
}

APR_DECLARE(apr_status_t) apr_thread_create(apr_thread_t **new,
                                            apr_threadattr_t *attr,
                                            apr_thread_start_t func,
                                            void *data,
                                            apr_pool_t *pool)
{
    apr_status_t stat;
    pthread_attr_t *temp;

    (*new) = (apr_thread_t *)apr_pcalloc(pool, sizeof(apr_thread_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->td = (pthread_t *)apr_pcalloc(pool, sizeof(pthread_t));

    if ((*new)->td == NULL) {
        return APR_ENOMEM;
    }

    (*new)->pool = pool;
    (*new)->data = data;
    (*new)->func = func;

    if (attr)
        temp = attr->attr;
    else
        temp = NULL;

    stat = apr_pool_create(&(*new)->pool, pool);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    if ((stat = pthread_create((*new)->td, temp, dummy_worker, (*new))) == 0) {
        return APR_SUCCESS;
    }
    else {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif

        return stat;
    }
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

APR_DECLARE(apr_status_t) apr_thread_exit(apr_thread_t *thd,
                                          apr_status_t retval)
{
    thd->exitval = retval;
    apr_pool_destroy(thd->pool);
    pthread_exit(NULL);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_join(apr_status_t *retval,
                                          apr_thread_t *thd)
{
    apr_status_t stat;
    apr_status_t *thread_stat;

    if ((stat = pthread_join(*thd->td,(void *)&thread_stat)) == 0) {
        *retval = thd->exitval;
        return APR_SUCCESS;
    }
    else {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif

        return stat;
    }
}

APR_DECLARE(apr_status_t) apr_thread_detach(apr_thread_t *thd)
{
    apr_status_t stat;

#ifdef PTHREAD_DETACH_ARG1_ADDR
    if ((stat = pthread_detach(thd->td)) == 0) {
#else
    if ((stat = pthread_detach(*thd->td)) == 0) {
#endif

        return APR_SUCCESS;
    }
    else {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif

        return stat;
    }
}

void apr_thread_yield()
{
}

APR_DECLARE(apr_status_t) apr_thread_data_get(void **data, const char *key,
                                              apr_thread_t *thread)
{
    return apr_pool_userdata_get(data, key, thread->pool);
}

APR_DECLARE(apr_status_t) apr_thread_data_set(void *data, const char *key,
                              apr_status_t (*cleanup)(void *),
                              apr_thread_t *thread)
{
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
