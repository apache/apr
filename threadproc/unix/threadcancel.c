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
#if APR_HAS_THREADS

#ifdef HAVE_PTHREAD_H

#if 0 /* some platforms, e.g. FreeBSD 2.2.8, do not have pthread_cancel (they do have an undocumented pthread_kill, though) */
/* ***APRDOC********************************************************
 * ap_status_t ap_cancel_thread(ap_thread_t *thd)
 *    Asynchronously kill a thread
 * arg 1) The thread to kill.
 */
ap_status_t ap_cancel_thread(ap_thread_t *thd)
{
    ap_status_t stat;
    if ((stat = pthread_cancel(*thd->td)) == 0) {
        return APR_SUCCESS;
    }
    else {
        return stat;
    }
}
#endif
    
/* ***APRDOC********************************************************
 * ap_status_t ap_setcanceltype(ap_int32_t type, ap_context_t *cont)
 *    Determine how threads are cancelable.
 * arg 1) how are threads cancelable.  One of:
 *            APR_CANCEL_ASYNCH  -- cancel it no matter where it is
 *            APR_CANCEL_DEFER   -- only cancel the thread if it is safe. 
 * arg 2) The context to operate on 
 */
ap_status_t ap_setcanceltype(ap_int32_t type, ap_context_t *cont)
{
    ap_status_t stat;
    if ((stat = pthread_setcanceltype(type, NULL)) == 0) {
        return APR_SUCCESS;
    }
    else {
        return stat;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_setcancelstate(ap_int32_t type, ap_context_t *cont)
 *    Determine if threads will be cancelable.
 * arg 1) Are threads cancelable. 
 * arg 2) The context to operate on 
 */
ap_status_t ap_setcancelstate(ap_int32_t type, ap_context_t *cont)
{
    ap_status_t stat;
    if ((stat = pthread_setcanceltype(type, NULL)) == 0) {
        return APR_SUCCESS;
    }
    else {
        return stat;
    }
}
#endif
#endif
