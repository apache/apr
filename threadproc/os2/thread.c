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



static void apr_thread_begin(void *arg)
{
  apr_thread_t *thread = (apr_thread_t *)arg;
  thread->exitval = thread->func(thread, thread->data);
}



APR_DECLARE(apr_status_t) apr_thread_create(apr_thread_t **new, apr_threadattr_t *attr, 
                                            apr_thread_start_t func, void *data, 
                                            apr_pool_t *pool)
{
    apr_status_t stat;
    apr_thread_t *thread;
 
    thread = (apr_thread_t *)apr_palloc(pool, sizeof(apr_thread_t));
    *new = thread;

    if (thread == NULL) {
        return APR_ENOMEM;
    }

    thread->pool = pool;
    thread->attr = attr;
    thread->func = func;
    thread->data = data;
    stat = apr_pool_create(&thread->pool, pool);
    
    if (stat != APR_SUCCESS) {
        return stat;
    }

    if (attr == NULL) {
        stat = apr_threadattr_create(&thread->attr, thread->pool);
        
        if (stat != APR_SUCCESS) {
            return stat;
        }
    }

    thread->tid = _beginthread(apr_thread_begin, NULL, 
                               APR_THREAD_STACKSIZE, thread);
        
    if (thread->tid < 0) {
        return errno;
    }

    return APR_SUCCESS;
}



APR_DECLARE(apr_os_thread_t) apr_os_thread_current()
{
    PIB *ppib;
    TIB *ptib;
    DosGetInfoBlocks(&ptib, &ppib);
    return ptib->tib_ptib2->tib2_ultid;
}



APR_DECLARE(apr_status_t) apr_thread_exit(apr_thread_t *thd, apr_status_t retval)
{
    thd->exitval = retval;
    _endthread();
    return -1; /* If we get here something's wrong */
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
    return APR_OS2_STATUS(rc);
}



APR_DECLARE(apr_status_t) apr_thread_detach(apr_thread_t *thd)
{
    thd->attr->attr |= APR_THREADATTR_DETACHED;
    return APR_SUCCESS;
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
    return apr_pool_userdata_get(data, key, thread->pool);
}



APR_DECLARE(apr_status_t) apr_thread_data_set(void *data, const char *key,
                                              apr_status_t (*cleanup) (void *),
                                              apr_thread_t *thread)
{
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
