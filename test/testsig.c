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

#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "errno.h"
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <signal.h>
#include <string.h>

int time_to_die = 0;

void hup_handler(int sig)
{
    fprintf(stdout, "I got the signal\n");
    time_to_die++;    
}

int main(int argc, char *argv[])
{
    ap_context_t *context;
    ap_proc_t *newproc;
    ap_procattr_t *attr;
    char *args[3];

    ap_initialize();

    ap_create_context(&context, NULL);

    if (argc > 1) {

        ap_setup_signal(context, APR_SIGHUP, hup_handler);

        while(time_to_die == 0) {
            sleep(1);
        }
        return(1);
    }

    fprintf(stdout, "Creating new signal.......");
    if (ap_create_signal(context, APR_SIGHUP) != APR_SUCCESS) {
        fprintf(stderr, "Could not create attr\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    if (ap_createprocattr_init(&attr, context) != APR_SUCCESS) {
        fprintf(stderr, "Could not create attr\n");
        exit(-1);;
    }
    ap_setprocattr_detach(attr, FALSE);

    args[0] = ap_pstrdup(context, "testsig");
    args[1] = ap_pstrdup(context, "-X");
    args[2] = NULL;
    
    if (ap_create_process(context, "./testsig", args, NULL, attr, &newproc) != APR_SUCCESS) {
        fprintf(stderr, "Could not create the new process\n");
        exit(-1);
    }

    fprintf(stdout, "Sending the signal.......");
    fflush(stdout);
    ap_send_signal(context, APR_SIGHUP);    

    ap_wait_proc(newproc, APR_WAIT);

    return(1);
}

