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
#include "apr_strings.h"
#include "apr_arch_global_mutex.h"
#include "apr_proc_mutex.h"
#include "apr_thread_mutex.h"
#include "apr_portable.h"

static apr_status_t global_mutex_cleanup(void *data)
{
    apr_global_mutex_t *m = (apr_global_mutex_t *)data;
    apr_status_t rv;

    rv = apr_proc_mutex_destroy(m->proc_mutex);

#if APR_HAS_THREADS
    if (m->thread_mutex) {
        if (rv != APR_SUCCESS) {
            (void)apr_thread_mutex_destroy(m->thread_mutex);
        }
        else {
            rv = apr_thread_mutex_destroy(m->thread_mutex);
        }
    }
#endif /* APR_HAS_THREADS */

    return rv;
}

APR_DECLARE(apr_status_t) apr_global_mutex_create(apr_global_mutex_t **mutex,
                                                  const char *fname,
                                                  apr_lockmech_e mech,
                                                  apr_pool_t *pool)
{
    apr_status_t rv;
    apr_global_mutex_t *m;

    m = (apr_global_mutex_t *)apr_palloc(pool, sizeof(*m));
    m->pool = pool;

    rv = apr_proc_mutex_create(&m->proc_mutex, fname, mech, m->pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

#if APR_HAS_THREADS
    if (m->proc_mutex->inter_meth->flags & APR_PROCESS_LOCK_MECH_IS_GLOBAL) {
        m->thread_mutex = NULL; /* We don't need a thread lock. */
    }
    else {
        rv = apr_thread_mutex_create(&m->thread_mutex,
                                     APR_THREAD_MUTEX_DEFAULT, m->pool);
        if (rv != APR_SUCCESS) {
            rv = apr_proc_mutex_destroy(m->proc_mutex);
            return rv;
        }
    }
#endif /* APR_HAS_THREADS */

    apr_pool_cleanup_register(m->pool, (void *)m,
                              global_mutex_cleanup, apr_pool_cleanup_null);
    *mutex = m;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_global_mutex_child_init(
                              apr_global_mutex_t **mutex,
                              const char *fname,
                              apr_pool_t *pool)
{
    apr_status_t rv;

    rv = apr_proc_mutex_child_init(&((*mutex)->proc_mutex), fname, pool);
    return rv;
}

APR_DECLARE(apr_status_t) apr_global_mutex_lock(apr_global_mutex_t *mutex)
{
    apr_status_t rv;

#if APR_HAS_THREADS
    if (mutex->thread_mutex) {
        rv = apr_thread_mutex_lock(mutex->thread_mutex);
        if (rv != APR_SUCCESS) {
            return rv;
        }
    }
#endif /* APR_HAS_THREADS */

    rv = apr_proc_mutex_lock(mutex->proc_mutex);

#if APR_HAS_THREADS
    if (rv != APR_SUCCESS) {
        if (mutex->thread_mutex) {
            (void)apr_thread_mutex_unlock(mutex->thread_mutex);
        }
    }
#endif /* APR_HAS_THREADS */

    return rv;
}

APR_DECLARE(apr_status_t) apr_global_mutex_trylock(apr_global_mutex_t *mutex)
{
    apr_status_t rv;

#if APR_HAS_THREADS
    if (mutex->thread_mutex) {
        rv = apr_thread_mutex_trylock(mutex->thread_mutex);
        if (rv != APR_SUCCESS) {
            return rv;
        }
    }
#endif /* APR_HAS_THREADS */

    rv = apr_proc_mutex_trylock(mutex->proc_mutex);

#if APR_HAS_THREADS
    if (rv != APR_SUCCESS) {
        if (mutex->thread_mutex) {
            (void)apr_thread_mutex_unlock(mutex->thread_mutex);
        }
    }
#endif /* APR_HAS_THREADS */

    return rv;
}

APR_DECLARE(apr_status_t) apr_global_mutex_unlock(apr_global_mutex_t *mutex)
{
    apr_status_t rv;

    rv = apr_proc_mutex_unlock(mutex->proc_mutex);
#if APR_HAS_THREADS
    if (mutex->thread_mutex) {
        if (rv != APR_SUCCESS) {
            (void)apr_thread_mutex_unlock(mutex->thread_mutex);
        }
        else {
            rv = apr_thread_mutex_unlock(mutex->thread_mutex);
        }
    }
#endif /* APR_HAS_THREADS */
    return rv;
}

APR_DECLARE(apr_status_t) apr_os_global_mutex_get(apr_os_global_mutex_t *ospmutex,
                                                apr_global_mutex_t *pmutex)
{
    ospmutex->pool = pmutex->pool;
    ospmutex->proc_mutex = pmutex->proc_mutex;
#if APR_HAS_THREADS
    ospmutex->thread_mutex = pmutex->thread_mutex;
#endif
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_global_mutex_destroy(apr_global_mutex_t *mutex)
{
    return apr_pool_cleanup_run(mutex->pool, mutex, global_mutex_cleanup);
}

APR_POOL_IMPLEMENT_ACCESSOR(global_mutex)
