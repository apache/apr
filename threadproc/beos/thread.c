/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "threadproc.h"
#include "apr_thread_proc.h"
#include "apr_general.h"


ap_status_t ap_create_threadattr(ap_context_t *cont, struct threadattr_t **new)
{
    ap_status_t stat;
  
    (*new) = (struct threadattr_t *)ap_palloc(cont, 
              sizeof(struct threadattr_t));
    (*new)->attr = (int32)ap_palloc(cont, 
                    sizeof(int32));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
	(*new)->attr = (int32)B_NORMAL_PRIORITY;

    return APR_SUCCESS;
}

ap_status_t ap_setthreadattr_detach(struct threadattr_t *attr, ap_int32_t on)
{
	if (on == 1){
		attr->detached = 1;
	} else {
		attr->detached = 0;
	}    
    return APR_SUCCESS;
}

ap_status_t ap_getthreadattr_detach(struct threadattr_t *attr)
{
	if (attr->detached == 1){
		return APR_DETACH;
	}
	return APR_NOTDETACH;
}

ap_status_t ap_create_thread(ap_context_t *cont, struct threadattr_t *attr,
                                ap_thread_start_t func, void *data,
                                struct thread_t **new)
{
    int32 temp;
    ap_status_t stat;
    
    (*new) = (struct thread_t *)ap_palloc(cont, sizeof(struct thread_t));
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

	(*new)->td = (thread_id) ap_palloc(cont, sizeof(thread_id));
    if ((*new)->td == (thread_id)NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;

    /* First we create the new thread...*/
	if (attr)
	    temp = attr->attr;
	else
	    temp = B_NORMAL_PRIORITY;

    stat = ap_create_context(cont, &(*new)->cntxt);
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

ap_status_t ap_thread_exit(ap_thread_t *thd, ap_status_t *retval)
{
    ap_destroy_pool(thd->cntxt);
	exit_thread ((status_t)retval);
}

ap_status_t ap_thread_join(ap_thread_t *thd, ap_status_t *retval)
{
    if (wait_for_thread(thd->td,(void *)&retval) == B_NO_ERROR) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

ap_status_t ap_thread_detach(ap_thread_t *thd)
{
	if (suspend_thread(thd->td) == B_NO_ERROR){
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}
