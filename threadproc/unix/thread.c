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
ap_status_t ap_create_threadattr(ap_threadattr_t **new, ap_pool_t *cont)
{
    ap_status_t stat;
  
    (*new) = (ap_threadattr_t *)ap_palloc(cont, 
              sizeof(ap_threadattr_t));
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

ap_status_t ap_setthreadattr_detach(ap_threadattr_t *attr, ap_int32_t on)
{
    ap_status_t stat;
    if ((stat = pthread_attr_setdetachstate(attr->attr, on)) == 0) {
        return APR_SUCCESS;
    }
    else {
        return stat;
    }
}

ap_status_t ap_getthreadattr_detach(ap_threadattr_t *attr)
{
    int state;

    pthread_attr_getdetachstate(attr->attr, &state);
    if (state == 1)
        return APR_DETACH;
    return APR_NOTDETACH;
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
#endif  /* HAVE_PTHREAD_H */
#endif  /* APR_HAS_THREADS */

