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

#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

void ocmaint(int reason, void *data, int status)
{
    fprintf(stdout,"[CHILD]  Maintenance routine called....");
    fflush(stdout);
    switch (reason) {
    case APR_OC_REASON_DEATH:
        fprintf(stdout, "Died correctly\n");
        break;
    case APR_OC_REASON_LOST:
        fprintf(stdout, "APR_OC_REASON_LOST\n");
    case APR_OC_REASON_UNWRITABLE:
        fprintf(stdout, "APR_OC_REASON_UNWRITEABLE\n");
    case APR_OC_REASON_RESTART:
        fprintf(stdout, "APR_OC_REASON_RESTART\n");
        fprintf(stdout, "OC maintentance called for reason other than death\n");
        break;
    }
}

int main(int argc, char *argv[])
{
    ap_pool_t *context;
    ap_pool_t *cont2;
    ap_status_t status = 0;
    ap_ssize_t nbytes = 0;
    ap_proc_t *newproc = NULL;
    ap_procattr_t *procattr = NULL;
    char *args[3];

    if (argc > 1) {
        while (1);
    }

    if (ap_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Couldn't initialize.");
        exit(-1);
    }
    atexit(ap_terminate);
    if (ap_create_pool(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate context.");
        exit(-1);
    }
    
    args[0] = ap_pstrdup(context, "testoc");
    args[1] = ap_pstrdup(context, "-X");
    args[2] = NULL;

    fprintf(stdout, "[PARENT] Creating procattr.............");
    fflush(stdout);
    if (ap_createprocattr_init(&procattr, context) != APR_SUCCESS) {
        fprintf(stderr, "Could not create attr\n");
        exit(-1);;
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "[PARENT] Starting other child..........");
    fflush(stdout);
    if (ap_create_process(&newproc, "../testoc", args, NULL, procattr, context) 
                          != APR_SUCCESS) {
        fprintf(stderr, "error starting other child\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");


    ap_register_other_child(newproc, ocmaint, NULL, -1, context);

    fprintf(stdout, "[PARENT] Sending SIGKILL to child......");
    fflush(stdout);
    if (ap_kill(newproc, SIGKILL) != APR_SUCCESS) {
        fprintf(stderr,"couldn't send the signal!\n");
        exit(-1);
    }
    fprintf(stdout,"OK\n");
    
    /* allow time for things to settle... */
    sleep(1);
    
    fprintf(stdout, "[PARENT] Checking on children..........\n");
    ap_check_other_child();
    
    return 1;
}    

