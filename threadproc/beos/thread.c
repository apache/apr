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

#include "apr_arch_threadproc.h"
#include "apr_portable.h"

APR_DECLARE(apr_status_t) apr_threadattr_create(apr_threadattr_t **new, apr_pool_t *pool)
{
    (*new) = (apr_threadattr_t *)apr_palloc(pool, 
              sizeof(apr_threadattr_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->pool = pool;
	(*new)->attr = (int32)B_NORMAL_PRIORITY;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_threadattr_detach_set(apr_threadattr_t *attr, apr_int32_t on)
{
	if (on == 1){
		attr->detached = 1;
	} else {
		attr->detached = 0;
	}    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_threadattr_detach_get(apr_threadattr_t *attr)
{
	if (attr->detached == 1){
		return APR_DETACH;
	}
	return APR_NOTDETACH;
}

static void *dummy_worker(void *opaque)
{
    apr_thread_t *thd = (apr_thread_t*)opaque;
    return thd->func(thd, thd->data);
}

APR_DECLARE(apr_status_t) apr_thread_create(apr_thread_t **new, apr_threadattr_t *attr,
                                            apr_thread_start_t func, void *data,
                                            apr_pool_t *pool)
{
    int32 temp;
    apr_status_t stat;
    
    (*new) = (apr_thread_t *)apr_palloc(pool, sizeof(apr_thread_t));
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->pool = pool;
    (*new)->data = data;
    (*new)->func = func;
    (*new)->exitval = -1;

    /* First we create the new thread...*/
	if (attr)
	    temp = attr->attr;
	else
	    temp = B_NORMAL_PRIORITY;

    stat = apr_pool_create(&(*new)->pool, pool);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    (*new)->td = spawn_thread((thread_func)dummy_worker, "apr thread", temp, (*new));
    /* Now we try to run it...*/
    if (resume_thread((*new)->td) == B_NO_ERROR) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    } 
}

APR_DECLARE(apr_os_thread_t) apr_os_thread_current(void)
{
    return find_thread(NULL);
}

int apr_os_thread_equal(apr_os_thread_t tid1, apr_os_thread_t tid2)
{
    return tid1 == tid2;
}

APR_DECLARE(apr_status_t) apr_thread_exit(apr_thread_t *thd, apr_status_t retval)
{
    apr_pool_destroy(thd->pool);
    thd->exitval = retval;
    exit_thread ((status_t)(retval));
    /* This will never be reached... */
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_join(apr_status_t *retval, apr_thread_t *thd)
{
    status_t rv = 0, ret;
    if ((ret = wait_for_thread(thd->td, &rv)) == B_NO_ERROR) {
        *retval = rv;
        return APR_SUCCESS;
    }
    else {
        /* if we've missed the thread's death, did we set an exit value prior
         * to it's demise?  If we did return that.
         */
        if (thd->exitval != -1) {
            *retval = thd->exitval;
            return APR_SUCCESS;
        } else 
            return ret;
    }
}

APR_DECLARE(apr_status_t) apr_thread_detach(apr_thread_t *thd)
{
	if (suspend_thread(thd->td) == B_NO_ERROR){
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

void apr_thread_yield()
{
}

APR_DECLARE(apr_status_t) apr_thread_data_get(void **data, const char *key, apr_thread_t *thread)
{
    return apr_pool_userdata_get(data, key, thread->pool);
}

APR_DECLARE(apr_status_t) apr_thread_data_set(void *data, const char *key,
                                              apr_status_t (*cleanup) (void *),
                                              apr_thread_t *thread)
{
    return apr_pool_userdata_set(data, key, cleanup, thread->pool);
}

APR_DECLARE(apr_status_t) apr_os_thread_get(apr_os_thread_t **thethd, apr_thread_t *thd)
{
    *thethd = &thd->td;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_thread_put(apr_thread_t **thd, apr_os_thread_t *thethd, 
                                            apr_pool_t *pool)
{
    if (pool == NULL) {
        return APR_ENOPOOL;
    }
    if ((*thd) == NULL) {
        (*thd) = (apr_thread_t *)apr_pcalloc(pool, sizeof(apr_thread_t));
        (*thd)->pool = pool;
    }
    (*thd)->td = *thethd;
    return APR_SUCCESS;
}

static apr_status_t thread_once_cleanup(void *vcontrol)
{
    apr_thread_once_t *control = (apr_thread_once_t *)vcontrol;

    if (control->sem) {
        release_sem(control->sem);
        delete_sem(control->sem);
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_once_init(apr_thread_once_t **control,
                                               apr_pool_t *p)
{
    int rc;
    *control = (apr_thread_once_t *)apr_pcalloc(p, sizeof(apr_thread_once_t));
    (*control)->hit = 0; /* we haven't done it yet... */
    rc = ((*control)->sem = create_sem(1, "thread_once"));
    if (rc != 0) {
        return rc;
    }
    apr_pool_cleanup_register(p, control, thread_once_cleanup, apr_pool_cleanup_null);
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_thread_once(apr_thread_once_t *control, 
                                          void (*func)(void))
{
    if (!control->hit) {
        if (acquire_sem(control->sem) == B_OK) {
            control->hit = 1;
            func();
        }
    }
    return APR_SUCCESS;
}

APR_POOL_IMPLEMENT_ACCESSOR(thread)
