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
#include "apr_lib.h"
#include "apr_general.h"
#include "apr_win.h"
#include "apr_errno.h"
#include <windows.h>
#include <process.h>

static char    *NewEvent;
static Sigfunc *sig_handler;

volatile int ready = 0;

ap_status_t ap_create_signal(ap_signum_t signum, ap_context_t *cont)
{
    char *EventName;
    int ppid;
    char pidstr[10];
    char sigstr[4];
    SECURITY_ATTRIBUTES sa;

    ppid = _getpid(); 

    _itoa(ppid, pidstr, 10);
    _itoa(signum, sigstr, 10);

    EventName = ap_pstrcat(cont, "APR", pidstr, sigstr, NULL);

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (CreateEvent(&sa, TRUE, FALSE, EventName) == NULL) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_send_signal(ap_signum_t signum, ap_context_t *cont)
{
    HANDLE event;
    char *EventName;
    char *sigstr;
    int ppid;
    char pidstr[10];

    ppid = _getpid(); 

    _itoa(ppid, pidstr, 10);
    _itoa(signum, sigstr, 10);

    EventName = ap_pstrcat(cont, "APR", pidstr, sigstr, NULL);
    event = OpenEvent(EVENT_ALL_ACCESS, FALSE, EventName);

    if (event == NULL) {
        return APR_EEXIST;
    }

    if (SetEvent(event) == 0) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_setup_signal(ap_signum_t signum, Sigfunc *func, ap_context_t *cont)
{
    HANDLE event;
    char ppid[20];
    char *sigstr;

    event = OpenEvent(EVENT_ALL_ACCESS, FALSE, "EventRegister");

    if (GetEnvironmentVariable("parentpid", ppid, 20) == 0) {
        return APR_EEXIST;
    }

    sigstr = (char *)ap_palloc(cont, sizeof(int) * 10);
    _itoa(signum, sigstr, 10);

    NewEvent = ap_pstrcat(cont, "APR", ppid, sigstr, NULL);
    sig_handler = func;

    if (event == NULL) {
        return APR_EEXIST;
    }

    if (SetEvent(event)) {
        return APR_SUCCESS;
    }
    
    return APR_EEXIST;
}

unsigned int __stdcall SignalHandling(void *data)
{    
    int i = 1;
    HANDLE eventlist[MAXIMUM_WAIT_OBJECTS];
    Sigfunc *funclist[MAXIMUM_WAIT_OBJECTS];
    DWORD rv;
    
    if ((eventlist[0] = CreateEvent(NULL, TRUE, FALSE, "EventRegister")) == NULL) {
        return APR_EEXIST;
    }

    sig_handler = NULL;    

    while (1) {
        ready = 1;
        rv = WaitForMultipleObjects(i, eventlist, FALSE, INFINITE);
        if (rv == WAIT_FAILED) {
            exit(-1);
        }
        else if (rv == WAIT_OBJECT_0){
            eventlist[i] = OpenEvent(EVENT_ALL_ACCESS, TRUE, NewEvent);
            funclist[i] = sig_handler;
            i++;
            ResetEvent(eventlist[0]);
        }
        else { 
            funclist[rv - WAIT_OBJECT_0](0);
            ResetEvent(eventlist[rv - WAIT_OBJECT_0]);
        }
    }
}

int thread_ready(void)
{
    return ready;
}
