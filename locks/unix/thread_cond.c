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

#if APR_HAS_THREADS

#include "apr_arch_thread_mutex.h"
#include "apr_arch_thread_cond.h"

static apr_status_t thread_cond_cleanup(void *data)
{
    apr_thread_cond_t *cond = (apr_thread_cond_t *)data;
    apr_status_t rv;

    rv = pthread_cond_destroy(cond->cond);
#ifdef PTHREAD_SETS_ERRNO
    if (rv) {
        rv = errno;
    }
#endif
    return rv;
} 

APR_DECLARE(apr_status_t) apr_thread_cond_create(apr_thread_cond_t **cond,
                                                 apr_pool_t *pool)
{
    apr_thread_cond_t *new_cond;
    apr_status_t rv;

    new_cond = (apr_thread_cond_t *)apr_pcalloc(pool,
                                                sizeof(apr_thread_cond_t));

    if (new_cond == NULL) {
        return APR_ENOMEM;
    }

    new_cond->pool = pool;
    new_cond->cond = (pthread_cond_t *)apr_palloc(pool, 
                                                  sizeof(pthread_cond_t));

    if (new_cond->cond == NULL) {
        return APR_ENOMEM;
    }

    if ((rv = pthread_cond_init(new_cond->cond, NULL))) {
#ifdef PTHREAD_SETS_ERRNO
        rv = errno;
#endif
        thread_cond_cleanup(new_cond);
        return rv;
    }

    apr_pool_cleanup_register(new_cond->pool,
                              (void *)new_cond, thread_cond_cleanup,
                              apr_pool_cleanup_null);

    *cond = new_cond;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_cond_wait(apr_thread_cond_t *cond,
                                               apr_thread_mutex_t *mutex)
{
    apr_status_t rv;

    rv = pthread_cond_wait(cond->cond, &mutex->mutex);
#ifdef PTHREAD_SETS_ERRNO
    if (rv) {
        rv = errno;
    }
#endif
    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_cond_timedwait(apr_thread_cond_t *cond,
                                                    apr_thread_mutex_t *mutex,
                                                    apr_interval_time_t timeout)
{
    apr_status_t rv;
    apr_time_t then;
    struct timespec abstime;

    then = apr_time_now() + timeout;
    abstime.tv_sec = apr_time_sec(then);
    abstime.tv_nsec = apr_time_usec(then) * 1000; /* nanoseconds */

    rv = pthread_cond_timedwait(cond->cond, &mutex->mutex, &abstime);
#ifdef PTHREAD_SETS_ERRNO
    if (rv) {
        rv = errno;
    }
#endif
    if (ETIMEDOUT == rv) {
        return APR_TIMEUP;
    }
    return rv;
}


APR_DECLARE(apr_status_t) apr_thread_cond_signal(apr_thread_cond_t *cond)
{
    apr_status_t rv;

    rv = pthread_cond_signal(cond->cond);
#ifdef PTHREAD_SETS_ERRNO
    if (rv) {
        rv = errno;
    }
#endif
    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_cond_broadcast(apr_thread_cond_t *cond)
{
    apr_status_t rv;

    rv = pthread_cond_broadcast(cond->cond);
#ifdef PTHREAD_SETS_ERRNO
    if (rv) {
        rv = errno;
    }
#endif
    return rv;
}

APR_DECLARE(apr_status_t) apr_thread_cond_destroy(apr_thread_cond_t *cond)
{
    apr_status_t rv;
    if ((rv = thread_cond_cleanup(cond)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(cond->pool, cond, thread_cond_cleanup);
        return APR_SUCCESS;
    }
    return rv;
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_cond)

#endif /* APR_HAS_THREADS */
