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

#include "threadproc.h"

apr_status_t apr_create_threadattr(apr_threadattr_t **new, apr_pool_t *cont)
{
    (*new) = (apr_threadattr_t *)apr_palloc(cont, 
              sizeof(apr_threadattr_t));
    (*new)->attr = (int32)apr_palloc(cont, 
                    sizeof(int32));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
	(*new)->attr = (int32)B_NORMAL_PRIORITY;

    return APR_SUCCESS;
}

apr_status_t apr_setthreadattr_detach(apr_threadattr_t *attr, apr_int32_t on)
{
	if (on == 1){
		attr->detached = 1;
	} else {
		attr->detached = 0;
	}    
    return APR_SUCCESS;
}

apr_status_t apr_getthreadattr_detach(apr_threadattr_t *attr)
{
	if (attr->detached == 1){
		return APR_DETACH;
	}
	return APR_NOTDETACH;
}

apr_status_t apr_create_thread(apr_thread_t **new, apr_threadattr_t *attr,
                             apr_thread_start_t func, void *data,
                             apr_pool_t *cont)
{
    int32 temp;
    apr_status_t stat;
    
    (*new) = (apr_thread_t *)apr_palloc(cont, sizeof(apr_thread_t));
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

	(*new)->td = (thread_id) apr_palloc(cont, sizeof(thread_id));
    if ((*new)->td == (thread_id)NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;

    /* First we create the new thread...*/
	if (attr)
	    temp = attr->attr;
	else
	    temp = B_NORMAL_PRIORITY;

    stat = apr_create_pool(&(*new)->cntxt, cont);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    (*new)->td = spawn_thread((thread_func)func, "apr thread", temp, data);
    /* Now we try to run it...*/
    if (resume_thread((*new)->td) == B_NO_ERROR) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    } 
}

apr_status_t apr_thread_exit(apr_thread_t *thd, apr_status_t *retval)
{
    apr_destroy_pool(thd->cntxt);
	exit_thread ((status_t)retval);
	return APR_SUCCESS;
}

apr_status_t apr_thread_join(apr_status_t *retval, apr_thread_t *thd)
{
    if (wait_for_thread(thd->td,(void *)&retval) == B_NO_ERROR) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

apr_status_t apr_thread_detach(apr_thread_t *thd)
{
	if (suspend_thread(thd->td) == B_NO_ERROR){
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}
