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

#include "apr_private.h"
#include "threadproc.h"
#include "apr_thread_proc.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include <process.h>


ap_status_t ap_create_threadattr(ap_threadattr_t **new, ap_pool_t *cont)
{
    (*new) = (ap_threadattr_t *)ap_palloc(cont, 
              sizeof(ap_threadattr_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    return APR_SUCCESS;
}

ap_status_t ap_setthreadattr_detach(ap_threadattr_t *attr, ap_int32_t on)
{
    attr->detach = on;
	return APR_SUCCESS;
}

ap_status_t ap_getthreadattr_detach(ap_threadattr_t *attr)
{
    if (attr->detach == 1)
        return APR_DETACH;
    return APR_NOTDETACH;
}

ap_status_t ap_create_thread(ap_thread_t **new, ap_threadattr_t *attr, 
                             ap_thread_start_t func, void *data, 
                             ap_pool_t *cont)
{
    ap_status_t stat;
	unsigned temp;
    int lasterror;
 
    (*new) = (ap_thread_t *)ap_palloc(cont, sizeof(ap_thread_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    
    stat = ap_create_pool(&(*new)->cntxt, cont);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    /* Use 0 for Thread Stack Size, because that will default the stack to the
     * same size as the calling thread. 
     */
    if (((*new)->td = (HANDLE *)_beginthreadex(NULL, 0, (unsigned int (API_THREAD_FUNC *)(void *))func,
                                               data, 0, &temp)) == 0) {
        lasterror = GetLastError();
        return APR_EEXIST; /* MSVC++ doc doesn't mention any additional error info */
    }

    if (attr && attr->detach) {
        CloseHandle((*new)->td);
    }

    return APR_SUCCESS;
}

ap_status_t ap_thread_exit(ap_thread_t *thd, ap_status_t *retval)
{
    ap_destroy_pool(thd->cntxt);
    _endthreadex(*retval);
	return APR_SUCCESS;
}

ap_status_t ap_thread_join(ap_status_t *retval, ap_thread_t *thd)
{
    ap_status_t stat;

    if ((stat = WaitForSingleObject(thd->td, INFINITE)) == WAIT_OBJECT_0) {
        if (GetExitCodeThread(thd->td, retval) == 0) {
            return APR_SUCCESS;
        }
        return GetLastError();
    }
    else {
        return stat;
    }
}

ap_status_t ap_thread_detach(ap_thread_t *thd)
{
    if (CloseHandle(thd->td)) {
        return APR_SUCCESS;
    }
    else {
        return GetLastError();
    }
}

ap_status_t ap_get_threaddata(void **data, char *key, ap_thread_t *thread)
{
    if (thread != NULL) {
        return ap_get_userdata(data, key, thread->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOTHREAD;
    }
}

ap_status_t ap_set_threaddata(void *data, char *key,
                              ap_status_t (*cleanup) (void *),
                              ap_thread_t *thread)
{
    if (thread != NULL) {
        return ap_set_userdata(data, key, cleanup, thread->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOTHREAD;
    }
}

ap_status_t ap_get_os_thread(ap_os_thread_t *thethd, ap_thread_t *thd)
{
    if (thd == NULL) {
        return APR_ENOTHREAD;
    }
    thethd = thd->td;
    return APR_SUCCESS;
}

ap_status_t ap_put_os_thread(ap_thread_t **thd, ap_os_thread_t *thethd, 
                             ap_pool_t *cont)
{
    if (cont == NULL) {
        return APR_ENOPOOL;
    }
    if ((*thd) == NULL) {
        (*thd) = (ap_thread_t *)ap_palloc(cont, sizeof(ap_thread_t));
        (*thd)->cntxt = cont;
    }
    (*thd)->td = thethd;
    return APR_SUCCESS;
}

