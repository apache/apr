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

#include "apr.h"
#include "apr_portable.h"
#include "threadproc.h"

#if APR_HAS_THREADS

#if APR_HAVE_PTHREAD_H
apr_status_t apr_create_threadattr(apr_threadattr_t **new, apr_pool_t *cont)
{
    apr_status_t stat;
  
    (*new) = (apr_threadattr_t *)apr_pcalloc(cont, sizeof(apr_threadattr_t));
    (*new)->attr = (pthread_attr_t *)apr_pcalloc(cont, sizeof(pthread_attr_t));

    if ((*new) == NULL || (*new)->attr == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    stat = pthread_attr_init((*new)->attr);

    if (stat == 0) {
        return APR_SUCCESS;
    }
#ifdef PTHREAD_SETS_ERRNO
    stat = errno;
#endif
    return stat;
}

apr_status_t apr_setthreadattr_detach(apr_threadattr_t *attr, apr_int32_t on)
{
    apr_status_t stat;
#ifdef PTHREAD_ATTR_SETDETACHSTATE_ARG2_ADDR
    int arg = on;

    if ((stat = pthread_attr_setdetachstate(attr->attr, &arg)) == 0) {
#else
    if ((stat = pthread_attr_setdetachstate(attr->attr, on)) == 0) {
#endif
        return APR_SUCCESS;
    }
    else {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        return stat;
    }
}

apr_status_t apr_getthreadattr_detach(apr_threadattr_t *attr)
{
    int state;

#ifdef PTHREAD_ATTR_GETDETACHSTATE_TAKES_ONE_ARG
    state = pthread_attr_getdetachstate(attr->attr);
#else
    pthread_attr_getdetachstate(attr->attr, &state);
#endif
    if (state == 1)
        return APR_DETACH;
    return APR_NOTDETACH;
}

apr_status_t apr_create_thread(apr_thread_t **new, apr_threadattr_t *attr, 
                             apr_thread_start_t func, void *data, 
                             apr_pool_t *cont)
{
    apr_status_t stat;
    pthread_attr_t *temp;
 
    (*new) = (apr_thread_t *)apr_pcalloc(cont, sizeof(apr_thread_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->td = (pthread_t *)apr_pcalloc(cont, sizeof(pthread_t));

    if ((*new)->td == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;

    if (attr)
        temp = attr->attr;
    else
        temp = NULL;
    
    stat = apr_create_pool(&(*new)->cntxt, cont);
    if (stat != APR_SUCCESS) {
        return stat;
    }

    if ((stat = pthread_create((*new)->td, temp, func, data)) == 0) {
        return APR_SUCCESS;
    }
    else {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        return stat;
    } 
}

apr_status_t apr_thread_exit(apr_thread_t *thd, apr_status_t *retval)
{
    apr_destroy_pool(thd->cntxt);
    pthread_exit(retval);
    return APR_SUCCESS;
}

apr_status_t apr_thread_join(apr_status_t *retval, apr_thread_t *thd)
{
    apr_status_t stat;

    if ((stat = pthread_join(*thd->td,(void *)&retval)) == 0) {
        return APR_SUCCESS;
    }
    else {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        return stat;
    }
}

apr_status_t apr_thread_detach(apr_thread_t *thd)
{
    apr_status_t stat;

#ifdef PTHREAD_DETACH_ARG1_ADDR
    if ((stat = pthread_detach(thd->td)) == 0) {
#else
    if ((stat = pthread_detach(*thd->td)) == 0) {
#endif
        return APR_SUCCESS;
    }
    else {
#ifdef PTHREAD_SETS_ERRNO
        stat = errno;
#endif
        return stat;
    }
}

apr_status_t apr_get_threaddata(void **data, const char *key, apr_thread_t *thread)
{
    return apr_get_userdata(data, key, thread->cntxt);
}

apr_status_t apr_set_threaddata(void *data, const char *key,
                              apr_status_t (*cleanup) (void *),
                              apr_thread_t *thread)
{
    return apr_set_userdata(data, key, cleanup, thread->cntxt);
}

apr_status_t apr_get_os_thread(apr_os_thread_t **thethd, apr_thread_t *thd)
{
    *thethd = thd->td;
    return APR_SUCCESS;
}

apr_status_t apr_put_os_thread(apr_thread_t **thd, apr_os_thread_t *thethd, 
                             apr_pool_t *cont)
{
    if (cont == NULL) {
        return APR_ENOPOOL;
    }
    if ((*thd) == NULL) {
        (*thd) = (apr_thread_t *)apr_pcalloc(cont, sizeof(apr_thread_t));
        (*thd)->cntxt = cont;
    }
    (*thd)->td = thethd;
    return APR_SUCCESS;
}
#endif  /* HAVE_PTHREAD_H */
#endif  /* APR_HAS_THREADS */

