/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

/*Read/Write locking implementation based on the MultiLock code from
 * Stephen Beaulieu <hippo@be.com>
 */
 
#include "beos/thread_mutex.h"
#include "beos/thread_cond.h"
#include "apr_strings.h"
#include "apr_portable.h"

static apr_status_t thread_cond_cleanup(void *data)
{
    struct waiter *w;
    apr_thread_cond_t *cond = (apr_thread_cond_t *)data;

    acquire_sem(cond->lock);
    /* Go through waiters list and delete the sem's so we don't leak. */
    while (cond->list) {
        w = cond->list;
        cond->list = w->next;
        delete_sem(w->sem);
    }
    delete_sem(cond->lock);

    return APR_SUCCESS;
}
    
APR_DECLARE(apr_status_t) apr_thread_cond_create(apr_thread_cond_t **cond,
                                                 apr_pool_t *pool)
{
    apr_thread_cond_t *new_cond;
    sem_id rv;

    new_cond = (apr_thread_cond_t *)apr_palloc(pool, sizeof(apr_thread_cond_t));

    if (new_cond == NULL)
        return APR_ENOMEM;

    if ((rv = create_sem(1, "apr conditional lock")) < B_OK)
        return rv;
    
    new_cond->lock = rv;
    new_cond->pool = pool;
    new_cond->list = NULL;
    
    apr_pool_cleanup_register(new_cond->pool,
                              (void *)new_cond, thread_cond_cleanup,
                              apr_pool_cleanup_null);

    *cond = new_cond;
    return APR_SUCCESS;
}

static apr_status_t do_wait(apr_thread_cond_t *cond, apr_thread_mutex_t *mutex,
                            int timeout)
{
    struct waiter *wait = apr_palloc(cond->pool, sizeof(struct waiter));
    thread_id cth = find_thread(NULL);
    apr_status_t rv;

    if (!wait)
        return APR_ENOMEM;
        
    if (cond->owner > 0 && cth != cond->owner) {
        /* What should we return??? */
        return APR_EINVAL;
    }
    
    wait->sem  = create_sem(0, "apr conditional waiter");
    wait->next = NULL;
    wait->pool = cond->pool;
    cond->condlock = mutex;
           
    acquire_sem(cond->lock);
    cond->owner = -1;

    if (!cond->list)
        cond->list = wait;
    else
        cond->tail->next = wait;
    cond->tail = wait;

    release_sem(cond->lock);
        
    apr_thread_mutex_unlock(cond->condlock);

    rv = acquire_sem_etc(wait->sem, 1, B_RELATIVE_TIMEOUT, timeout);
    if (rv != B_OK)
        if (rv == B_TIMED_OUT)
            return APR_TIMEUP;
        return rv;
        
    apr_thread_mutex_lock(cond->condlock);

    acquire_sem(cond->lock);
    cond->owner = find_thread(NULL);
    release_sem(cond->lock);

    delete_sem(wait->sem);
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_cond_wait(apr_thread_cond_t *cond,
                                               apr_thread_mutex_t *mutex)
{
    return do_wait(cond, mutex, 0);
}

APR_DECLARE(apr_status_t) apr_thread_cond_timedwait(apr_thread_cond_t *cond,
                                                    apr_thread_mutex_t *mutex,
                                                    apr_interval_time_t timeout)
{
    return do_wait(cond, mutex, timeout);
}

APR_DECLARE(apr_status_t) apr_thread_cond_signal(apr_thread_cond_t *cond)
{
    struct waiter *wake;

    acquire_sem(cond->lock);    
    if (cond->list) {
        wake = cond->list;
        cond->list = wake->next;
        release_sem(wake->sem);
    }
    release_sem(cond->lock);
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_cond_broadcast(apr_thread_cond_t *cond)
{
    struct waiter *wake;
    
    acquire_sem(cond->lock);
    while (cond->list) {
        wake = cond->list;
        cond->list = wake->next;
        release_sem(wake->sem);
    }
    release_sem(cond->lock);
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_cond_destroy(apr_thread_cond_t *cond)
{
    apr_status_t stat;
    if ((stat = thread_cond_cleanup(cond)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(cond->pool, cond, thread_cond_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

APR_POOL_IMPLEMENT_ACCESSOR(thread_cond)

