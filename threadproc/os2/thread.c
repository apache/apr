/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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
#include "threadproc.h"
#include "apr_thread_proc.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "fileio.h"
#include <stdlib.h>
#include <os2.h>

apr_status_t apr_create_threadattr(apr_threadattr_t **new, apr_pool_t *cont)
{
    (*new) = (apr_threadattr_t *)apr_palloc(cont, sizeof(apr_threadattr_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    (*new)->attr = 0;
    return APR_SUCCESS;
}



apr_status_t apr_setthreadattr_detach(apr_threadattr_t *attr, apr_int32_t on)
{
    attr->attr |= APR_THREADATTR_DETACHED;
    return APR_SUCCESS;
}



apr_status_t apr_getthreadattr_detach(apr_threadattr_t *attr)
{
    return (attr->attr & APR_THREADATTR_DETACHED) ? APR_DETACH : APR_NOTDETACH;
}



static void apr_thread_begin(void *arg)
{
  apr_thread_t *thread = (apr_thread_t *)arg;
  thread->rv = thread->func(thread->data);
}



apr_status_t apr_create_thread(apr_thread_t **new, apr_threadattr_t *attr, 
                             apr_thread_start_t func, void *data, 
                             apr_pool_t *cont)
{
    apr_status_t stat;
    apr_thread_t *thread;
 
    thread = (apr_thread_t *)apr_palloc(cont, sizeof(apr_thread_t));
    *new = thread;

    if (thread == NULL) {
        return APR_ENOMEM;
    }

    thread->cntxt = cont;
    thread->attr = attr;
    thread->func = func;
    thread->data = data;
    stat = apr_create_pool(&thread->cntxt, cont);
    
    if (stat != APR_SUCCESS) {
        return stat;
    }

    if (attr == NULL) {
        stat = apr_create_threadattr(&thread->attr, thread->cntxt);
        
        if (stat != APR_SUCCESS) {
            return stat;
        }
    }
    
    if (thread->attr->attr & APR_THREADATTR_DETACHED)
        thread->tid = _beginthread((os2_thread_start_t)func, NULL, 
                                   APR_THREAD_STACKSIZE, data);
    else
        thread->tid = _beginthread(apr_thread_begin, NULL, 
                                   APR_THREAD_STACKSIZE, thread);
        
    if (thread->tid < 0) {
        return errno;
    }

    return APR_SUCCESS;
}



apr_status_t apr_thread_exit(apr_thread_t *thd, apr_status_t *retval)
{
    thd->rv = retval;
    _endthread();
    return -1; /* If we get here something's wrong */
}



apr_status_t apr_thread_join(apr_status_t *retval, apr_thread_t *thd)
{
    ULONG rc;
    TID waittid = thd->tid;

    if (thd->attr->attr & APR_THREADATTR_DETACHED)
        return APR_EINVAL;

    rc = DosWaitThread(&waittid, DCWW_WAIT);

    if (rc == ERROR_INVALID_THREADID)
        rc = 0; /* Thread had already terminated */

    *retval = (apr_status_t)thd->rv;
    return APR_OS2_STATUS(rc);
}



apr_status_t apr_thread_detach(apr_thread_t *thd)
{
    thd->attr->attr |= APR_THREADATTR_DETACHED;
    return APR_SUCCESS;
}


