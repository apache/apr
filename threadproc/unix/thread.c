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
#include "apr_portable.h"

#if APR_HAS_THREADS

#ifdef HAVE_PTHREAD_H
/* ***APRDOC********************************************************
 * ap_status_t ap_create_threadattr(ap_threadattr_t **new, ap_context_t *cont)
 *    Create and initialize a new threadattr variable
 * arg 1) The newly created threadattr.
 * arg 2) The context to use
 */
ap_status_t ap_create_threadattr(struct threadattr_t **new, ap_context_t *cont)
{
    ap_status_t stat;
  
    (*new) = (struct threadattr_t *)ap_palloc(cont, 
              sizeof(struct threadattr_t));
    (*new)->attr = (pthread_attr_t *)ap_palloc(cont, 
                    sizeof(pthread_attr_t));

    if ((*new) == NULL || (*new)->attr == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    stat = pthread_attr_init((*new)->attr);

    if (stat == 0) {
        return APR_SUCCESS;
    }
    return stat;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_setthreadattr_detach(ap_threadattr_t *attr, ap_int32_t on)
 *    Set if newly created threads should be created in detach mode.
 * arg 1) The threadattr to affect 
 * arg 2) Thread detach state on or off
 */
ap_status_t ap_setthreadattr_detach(struct threadattr_t *attr, ap_int32_t on)
{
    ap_status_t stat;
    if ((stat = pthread_attr_setdetachstate(attr->attr, on)) == 0) {
        return APR_SUCCESS;
    }
    else {
        return stat;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_getthreadattr_detach(ap_threadattr_t *attr)
 *    Get the detach mode for this threadattr.
 * arg 1) The threadattr to reference 
 */
ap_status_t ap_getthreadattr_detach(struct threadattr_t *attr)
{
    int state;

    pthread_attr_getdetachstate(attr->attr, &state);
    if (state == 1)
        return APR_DETACH;
    return APR_NOTDETACH;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_create_thread(ap_thread_t **new, ap_threadattr_t *attr, 
 *                              ap_thread_start_t func, void *data, 
 *                              ap_context_t *cont)
 *    Create a new thread of execution 
 * arg 1) The newly created thread handle.
 * arg 2) The threadattr to use to determine how to create the thread
 * arg 3) The function to start the new thread in
 * arg 4) Any data to be passed to the starting function
 * arg 5) The context to use
 */
ap_status_t ap_create_thread(struct thread_t **new, struct threadattr_t *attr, 
                             ap_thread_start_t func, void *data, 
                             ap_context_t *cont)
{
    ap_status_t stat;
    pthread_attr_t *temp;
 
    (*new) = (struct thread_t *)ap_palloc(cont, sizeof(struct thread_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->td = (pthread_t *)ap_palloc(cont, sizeof(pthread_t));

    if ((*new)->td == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;

    if (attr)
        temp = attr->attr;
    else
        temp = NULL;
    
    stat = ap_create_context(&(*new)->cntxt, cont);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    if ((stat = pthread_create((*new)->td, temp, func, data)) == 0) {
        return APR_SUCCESS;
    }
    else {
        return stat;
    } 
}

/* ***APRDOC********************************************************
 * ap_status_t ap_thread_exit(ap_thread_t *thd, ap_status_t *retval)
 *    stop the current thread 
 * arg 1) The thread to stop
 * arg 2) The return value to pass back to any thread that cares
 */
ap_status_t ap_thread_exit(ap_thread_t *thd, ap_status_t *retval)
{
    ap_destroy_pool(thd->cntxt);
    pthread_exit(retval);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_thread_join(ap_status_t *retval, ap_thread_t *thd)
 *    block until the desired thread stops executing. 
 * arg 1) The return value from the dead thread.
 * arg 2) The thread to join
 */
ap_status_t ap_thread_join(ap_status_t *retval, struct thread_t *thd)
{
    ap_status_t stat;

    if ((stat = pthread_join(*thd->td,(void *)&retval)) == 0) {
        return APR_SUCCESS;
    }
    else {
        return stat;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_thread_detach(ap_thread_t *thd)
 *    detach a thread
 * arg 1) The thread to detach 
 */
ap_status_t ap_thread_detach(struct thread_t *thd)
{
    ap_status_t stat;

    if ((stat = pthread_detach(*thd->td)) == 0) {
        return APR_SUCCESS;
    }
    else {
        return stat;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_threaddata(void **data, char *key, ap_thread_t *thread)
 *    Return the context associated with the current thread.
 * arg 1) The user data associated with the thread.
 * arg 2) The key to associate with the data
 * arg 3) The currently open thread.
 */
ap_status_t ap_get_threaddata(void **data, char *key, struct thread_t *thread)
{
    if (thread != NULL) {
        return ap_get_userdata(data, key, thread->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOTHREAD;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_threaddata(void *data, char *key,
                                 ap_status_t (*cleanup) (void *),
                                 ap_thread_t *thread)
 *    Return the context associated with the current thread.
 * arg 1) The user data to associate with the thread.
 * arg 2) The key to use for associating the data with the tread
 * arg 3) The cleanup routine to use when the thread is destroyed.
 * arg 4) The currently open thread.
 */
ap_status_t ap_set_threaddata(void *data, char *key,
                              ap_status_t (*cleanup) (void *),
                              struct thread_t *thread)
{
    if (thread != NULL) {
        return ap_set_userdata(data, key, cleanup, thread->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOTHREAD;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_thread(ap_thread_t *thethd, ap_os_thread_t *thd)
 *    convert the thread to os specific type from apr type.
 * arg 1) The apr thread to convert
 * arg 2) The os specific thread we are converting to
 */
ap_status_t ap_get_os_thread(ap_os_thread_t *thethd, struct thread_t *thd)
{
    if (thd == NULL) {
        return APR_ENOTHREAD;
    }
    thethd = thd->td;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_thread(ap_thread_t *thd, ap_os_thread_t *thethd,
 *                              ap_context_t *cont)
 *    convert the thread from os specific type to apr type.
 * arg 1) The apr thread we are converting to.
 * arg 2) The os specific thread to convert
 * arg 3) The context to use if it is needed.
 */
ap_status_t ap_put_os_thread(struct thread_t **thd, ap_os_thread_t *thethd, 
                             ap_context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if ((*thd) == NULL) {
        (*thd) = (struct thread_t *)ap_palloc(cont, sizeof(struct thread_t));
        (*thd)->cntxt = cont;
    }
    (*thd)->td = thethd;
    return APR_SUCCESS;
}
#endif  /* HAVE_PTHREAD_H */
#endif  /* APR_HAS_THREADS */

