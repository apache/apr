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

#include "apr_general.h"
#include <string.h>

#ifdef HAVE_PTHREAD_H

#include <pthread.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

/* ***APRDOC********************************************************
 * ap_status_t ap_create_signal(ap_signum_t, ap_context_t *)
 *    Create a signal for use later on. 
 * arg 1) The context to operate on.
 * arg 2) The signal we are creating.  One of:
 *            List to come.  :)
 * NOTE: This function must be called before the desired signal can be sent.
 *       This is for Windows to be able to send signals, so your program
 *       won't be portable without it. 
 */                                                                             
ap_status_t ap_create_signal(ap_signum_t signum, ap_context_t *cont)
{
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_send_signal(ap_signum_t, ap_context_t *)
 *    Send a signal to your process group 
 * arg 1) The context to operate on.
 * arg 2) The signal we are sending.  Same as above list
 * NOTE:  Signals can only be sent to the whole process group because I haven't 
 *        figured out how to send to individual children on Windows yet.  When
 *        that is solved, this will change here.
 */
ap_status_t ap_send_signal(ap_signum_t signum, ap_context_t *cont)
{
    killpg(0, signum);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_setup_signal(ap_context_t *, ap_signum_t)
 *    Setup the response when a process receives a particular signal. 
 * arg 1) The context to operate on.
 * arg 2) The signal we are expecting to receive.  Same as above list
 * arg 3) The function to execute when this signal is received.
 */
ap_status_t ap_setup_signal(ap_signum_t signum, Sigfunc *func,
                            ap_context_t *cont)
{
    sigset_t newset;

    sigemptyset(&newset);
    sigaddset(&newset, signum);

    signal(signum, func);

    pthread_sigmask(SIG_UNBLOCK, &newset, NULL);

    return APR_SUCCESS;
}
#endif
