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

#ifdef HAVE_PTHREAD_H

#if 0 /* some platforms, e.g. FreeBSD 2.2.8, do not have pthread_cancel (they do have an undocumented pthread_kill, though) */
/* ***APRDOC********************************************************
 * ap_status_t ap_cancel_thread(ap_thread_t *)
 *    Asynchronously kill a thread
 * arg 1) The thread to kill.
 */
ap_status_t ap_cancel_thread(struct thread_t *thd)
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
 * ap_status_t ap_setcanceltype(ap_int32_t, ap_context_t *)
 *    Determine how threads are cancelable.
 * arg 1) The context to operate on 
 * arg 2) how are threads cancelable.  One of:
 *            APR_CANCEL_ASYNCH  -- cancel it no matter where it is
 *            APR_CANCEL_DEFER   -- only cancel the thread if it is safe. 
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
 * ap_status_t ap_setcancelstate(ap_int32_t, ap_context_t *)
 *    Determine if threads will be cancelable.
 * arg 1) The context to operate on 
 * arg 2) Are threads cancelable. 
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
#else
ap_status_t ap_cancel_thread(struct thread_t *thd)
{
    return APR_SUCCESS;
}
    
ap_status_t ap_setcanceltype(ap_int32_t type, ap_context_t *cont)
{
    return APR_SUCCESS;
}

ap_status_t ap_setcancelstate(ap_int32_t type, ap_context_t *cont)
{
    return APR_SUCCESS;
}
#endif

