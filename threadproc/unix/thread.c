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
#include "apr_portable.h"
#include "apr_lib.h"

#ifdef HAVE_PTHREAD_H
/* ***APRDOC********************************************************
 * ap_status_t ap_create_threadattr(ap_threadattr_t **, ap_context_t *)
 *    Create and initialize a new threadattr variable
 * arg 1) The context to use
 * arg 2) The newly created threadattr.
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
 * ap_status_t ap_setthreadattr_detach(ap_threadattr_t *, ap_int32_t)
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
 * ap_status_t ap_getthreadattr_detach(ap_threadattr_t *, ap_int32_t *)
 *    Get the detach mode for this threadattr.
 * arg 1) The threadattr to reference 
 * arg 2) Thread detach state on or off
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
 * ap_status_t ap_create_thread(ap_context_t *, ap_threadattr_t *, 
 *                              ap_thread_start_t, coid *, ap_thread_t **)
 *    Create a new thread of execution 
 * arg 1) The context to use
 * arg 2) The threadattr to use to determine how to create the thread
 * arg 3) The function to start the new thread in
 * arg 4) Any data to be passed to the starting function
 * arg 5) The newly created thread handle.
 */
ap_status_t ap_create_thread(ap_context_t *cont, struct threadattr_t *attr, 
                             ap_thread_start_t func, void *data, 
                             struct thread_t **new)
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
 * ap_status_t ap_thread_exit(ap_thread_t *, ap_status_t *)
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
 * ap_status_t ap_thread_join(ap_thread_t *, ap_status_t *)
 *    block until the desired thread stops executing. 
 * arg 1) The thread to join
 * arg 2) The return value from the dead thread.
 */
ap_status_t ap_thread_join(struct thread_t *thd, ap_status_t *retval)
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
 * ap_status_t ap_thread_detach(ap_thread_t *)
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
 * ap_status_t ap_get_threaddata(ap_thread_t *, char *, void *)
 *    Return the context associated with the current thread.
 * arg 1) The currently open thread.
 * arg 2) The user data associated with the thread.
 */
ap_status_t ap_get_threaddata(struct thread_t *thread, char *key, void *data)
{
    if (thread != NULL) {
        return ap_get_userdata(&data, thread->cntxt, key);
    }
    else {
        data = NULL;
        return APR_ENOTHREAD;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_threaddata(ap_thread_t *, void *, char *key,
                                 ap_status_t (*cleanup) (void *))
 *    Return the context associated with the current thread.
 * arg 1) The currently open thread.
 * arg 2) The user data to associate with the thread.
 */
ap_status_t ap_set_threaddata(struct thread_t *thread, void *data, char *key,
                              ap_status_t (*cleanup) (void *))
{
    if (thread != NULL) {
        return ap_set_userdata(thread->cntxt, data, key, cleanup);
    }
    else {
        data = NULL;
        return APR_ENOTHREAD;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_thread(ap_thread_t *, ap_os_thread_t *)
 *    convert the thread to os specific type from apr type.
 * arg 1) The apr thread to convert
 * arg 2) The os specific thread we are converting to
 */
ap_status_t ap_get_os_thread(struct thread_t *thd, ap_os_thread_t *thethd)
{
    if (thd == NULL) {
        return APR_ENOTHREAD;
    }
    thethd = thd->td;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_thread(ap_context_t *, ap_thread_t *, ap_os_thread_t *)
 *    convert the thread from os specific type to apr type.
 * arg 1) The context to use if it is needed.
 * arg 2) The apr thread we are converting to.
 * arg 3) The os specific thread to convert
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
#else
    /* No pthread.h, no threads for right now.*/
ap_status_t ap_create_threadattr(struct threadattr_t **new, ap_context_t *cont)
{
    *new = NULL;
    return APR_SUCCESS;
}

ap_status_t ap_setthreadattr_detach(struct threadattr_t *attr, ap_int32_t on)
{
    return APR_SUCCESS;
}

ap_status_t ap_getthreadattr_detach(struct threadattr_t *attr)
{
    return APR_NOTDETACH;
}

ap_status_t ap_create_thread(ap_context_t *cont, struct threadattr_t *attr, 
                             ap_thread_start_t func, void *data, 
                             struct thread_t **new)
{
    *new = NULL;
    return stat;
}

ap_status_t ap_thread_exit(ap_thread_t *thd, ap_status_t *retval)
{
    APR_SUCCESS;
}

ap_status_t ap_thread_join(struct thread_t *thd, ap_status_t *retval)
{
    return APR_SUCCESS;
}

ap_status_t ap_thread_detach(struct thread_t *thd)
{
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_threaddata(ap_thread_t *, char *, void *)
 *    Return the context associated with the current thread.
 * arg 1) The currently open thread.
 * arg 2) The user data associated with the thread.
 */
ap_status_t ap_get_threaddata(struct thread_t *thread, char *key, void *data)
{
    data = NULL;
    return APR_ENOTHREAD;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_threaddata(ap_thread_t *, void *, char *,
                                 ap_status_t (*cleanup) (void *))
 *    Return the context associated with the current thread.
 * arg 1) The currently open thread.
 * arg 2) The user data to associate with the thread.
 */
ap_status_t ap_set_threaddata(struct thread_t *thread, void *data, char *key,
                              ap_status_t (*cleanup) (void *))
{
    return APR_ENOTHREAD;
}

ap_status_t ap_get_os_thread(struct thread_t *thd, ap_os_thread_t *thethd)
{
    thethd = NULL;
    return APR_SUCCESS;
}

ap_status_t ap_put_os_thread(ap_context_t *cont, struct thread_t **thd,
                             ap_os_thread_t *thethd)
{
    return APR_SUCCESS;
}
#endif

