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
#include <signal.h>
#include <kernel/OS.h>

ap_status_t ap_create_signal(ap_signum_t signum, ap_context_t *cont)
{
    return APR_SUCCESS;
}

/* Signals can only be sent to the whole process group because I havne't 
 * figured out how to send to individual children on Winodws yet.  When
 * that is solved, this will change here.
 */
 
ap_status_t ap_send_signal(ap_signum_t signum, ap_context_t *cont)
{
/* this function sends a signal to every thread within the current team
 * except the one calling it! */
 	thread_id me;
 	thread_info tinfo;
 	int32 cookie = 0;
 	me = find_thread(NULL);
 	
 	while (get_next_thread_info(0, &cookie, &tinfo) == B_OK) {
 		if (tinfo.thread != me) {
 			kill(tinfo.thread, signum);
 		}
 	}
}

ap_setup_signal(ap_signum_t signum, Sigfunc *func, ap_context_t *cont)
{
    /* Thanks to Chris Tate at Be for the code below */
    sigset_t newset, oldset;
    struct sigaction action;

    /* create a sigset_t for the given signal */
    sigemptyset(&newset);
    sigaddset(&newset, signum);

    /* install the given signal handler */
    action.sa_handler = func;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(signum, &action, NULL);

    sigprocmask(SIG_UNBLOCK, &newset, NULL);
    
    return APR_SUCCESS;
}
