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
#include "apr_strings.h"
#include "apr_arch_threadproc.h"

static int thread_count = 0;

apr_status_t apr_threadattr_create(apr_threadattr_t **new,
                                                apr_pool_t *pool)
{
    (*new) = (apr_threadattr_t *)apr_palloc(pool, 
              sizeof(apr_threadattr_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->pool = pool;
    (*new)->stack_size = APR_DEFAULT_STACK_SIZE;
    (*new)->detach = 0;
    (*new)->thread_name = NULL;
    return APR_SUCCESS;
}

apr_status_t apr_threadattr_detach_set(apr_threadattr_t *attr,apr_int32_t on)
{
    attr->detach = on;
	return APR_SUCCESS;   
}

apr_status_t apr_threadattr_detach_get(apr_threadattr_t *attr)
{
    if (attr->detach == 1)
        return APR_DETACH;
    return APR_NOTDETACH;
}

static void *dummy_worker(void *opaque)
{
    apr_thread_t *thd = (apr_thread_t *)opaque;
    return thd->func(thd, thd->data);
}

apr_status_t apr_thread_create(apr_thread_t **new,
 											apr_threadattr_t *attr, 
                             				apr_thread_start_t func,
 											void *data,
 											apr_pool_t *pool)
{
    apr_status_t stat;
    long flags = NX_THR_BIND_CONTEXT;
  	char threadName[NX_MAX_OBJECT_NAME_LEN+1];
    size_t stack_size = APR_DEFAULT_STACK_SIZE;

    if (attr && attr->thread_name) {
        strncpy (threadName, attr->thread_name, NX_MAX_OBJECT_NAME_LEN);
    }
    else {
	    sprintf(threadName, "APR_thread %04ld", ++thread_count);
    }

    /* An original stack size of 0 will allow NXCreateThread() to
    *   assign a default system stack size.  An original stack
    *   size of less than 0 will assign the APR default stack size.
    *   anything else will be taken as is.
    */
    if (attr && (attr->stack_size >= 0)) {
        stack_size = attr->stack_size;
    }
    
    (*new) = (apr_thread_t *)apr_palloc(pool, sizeof(apr_thread_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    
    (*new)->pool = pool;
    (*new)->data = data;
    (*new)->func = func;
    (*new)->thread_name = (char*)apr_pstrdup(pool, threadName);
    
    stat = apr_pool_create(&(*new)->pool, pool);
    if (stat != APR_SUCCESS) {
        return stat;
    }
    
    if (attr && attr->detach) {
        flags |= NX_THR_DETACHED;
    }
    
    (*new)->ctx = NXContextAlloc(
    	/* void(*start_routine)(void *arg)*/(void (*)(void *)) dummy_worker,
     	/* void *arg */										   (*new),
     	/* int priority */ 									   NX_PRIO_MED,
     	/* NXSize_t stackSize */							   stack_size,
     	/* long flags */									   NX_CTX_NORMAL,
     	/* int *error */									   &stat);
		
     	                                                                   
  	stat = NXContextSetName(
		 	/* NXContext_t ctx */			(*new)->ctx,
			/* const char *name */			threadName);

  	stat = NXThreadCreate(
        	/* NXContext_t context */		(*new)->ctx,
        	/* long flags */				flags,
        	/* NXThreadId_t *thread_id */	&(*new)->td);

    if(stat==0)
     	return APR_SUCCESS;
        
	return(stat);// if error    
}

apr_os_thread_t apr_os_thread_current()
{
    return NXThreadGetId();
}

int apr_os_thread_equal(apr_os_thread_t tid1, apr_os_thread_t tid2)
{
    return (tid1 == tid2);
}

void apr_thread_yield()
{
    NXThreadYield();
}

apr_status_t apr_thread_exit(apr_thread_t *thd,
                             apr_status_t retval)
{
    thd->exitval = retval;
    apr_pool_destroy(thd->pool);
    NXThreadExit(NULL);
    return APR_SUCCESS;
}

apr_status_t apr_thread_join(apr_status_t *retval,
                                          apr_thread_t *thd)
{
    apr_status_t  stat;    
    NXThreadId_t dthr;

    if ((stat = NXThreadJoin(thd->td, &dthr, NULL)) == 0) {
        *retval = thd->exitval;
        return APR_SUCCESS;
    }
    else {
        return stat;
    }
}

apr_status_t apr_thread_detach(apr_thread_t *thd)
{
    return APR_SUCCESS;
}

apr_status_t apr_thread_data_get(void **data, const char *key,
                                             apr_thread_t *thread)
{
    if (thread != NULL) {
            return apr_pool_userdata_get(data, key, thread->pool);
    }
    else {
        data = NULL;
        return APR_ENOTHREAD;
    }
}

apr_status_t apr_thread_data_set(void *data, const char *key,
                              apr_status_t (*cleanup) (void *),
                              apr_thread_t *thread)
{
    if (thread != NULL) {
       return apr_pool_userdata_set(data, key, cleanup, thread->pool);
    }
    else {
        data = NULL;
        return APR_ENOTHREAD;
    }
}

APR_DECLARE(apr_status_t) apr_os_thread_get(apr_os_thread_t **thethd,
                                            apr_thread_t *thd)
{
    if (thd == NULL) {
        return APR_ENOTHREAD;
    }
    *thethd = &(thd->td);
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
        (*thd) = (apr_thread_t *)apr_palloc(pool, sizeof(apr_thread_t));
        (*thd)->pool = pool;
    }
    (*thd)->td = *thethd;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_once_init(apr_thread_once_t **control,
                                               apr_pool_t *p)
{
    (*control) = apr_pcalloc(p, sizeof(**control));
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_thread_once(apr_thread_once_t *control,
                                          void (*func)(void))
{
    if (!atomic_xchg(&control->value, 1)) {
        func();
    }
    return APR_SUCCESS;
}

APR_POOL_IMPLEMENT_ACCESSOR(thread)


