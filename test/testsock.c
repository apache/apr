/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"

#define STRLEN 15

static int run_basic_test(apr_pool_t *context)
{
    apr_procattr_t *attr1 = NULL;
    apr_procattr_t *attr2 = NULL;
    apr_proc_t proc1;
    apr_proc_t proc2;
    apr_status_t s1;
    apr_status_t s2;
    const char *args[2];

    fprintf(stdout, "Creating children to run network tests.......\n");
    s1 = apr_procattr_create(&attr1, context);
    s2 = apr_procattr_create(&attr2, context);

    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS) {
        fprintf(stderr, "Problem creating proc attrs\n");
        exit(-1);
    }

    args[0] = apr_pstrdup(context, "server");
    args[1] = NULL; 
    s1 = apr_proc_create(&proc1, "./server", args, NULL, attr1, context);

    /* Sleep for 5 seconds to ensure the server is setup before we begin */
    apr_sleep(5000000);
    args[0] = apr_pstrdup(context, "client");
    s2 = apr_proc_create(&proc2, "./client", args, NULL, attr2, context);

    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS) {
        fprintf(stderr, "Problem spawning new process\n");
        exit(-1);
    }

    while ((s1 = apr_proc_wait(&proc1, NULL, NULL, APR_NOWAIT)) == APR_CHILD_NOTDONE && 
           (s2 = apr_proc_wait(&proc2, NULL, NULL, APR_NOWAIT)) == APR_CHILD_NOTDONE) {
        continue;
    }

    if (s1 == APR_SUCCESS) {
        apr_proc_kill(&proc2, SIGTERM);
        while (apr_proc_wait(&proc2, NULL, NULL, APR_WAIT) == APR_CHILD_NOTDONE);
    }
    else {
        apr_proc_kill(&proc1, SIGTERM);
        while (apr_proc_wait(&proc1, NULL, NULL, APR_WAIT) == APR_CHILD_NOTDONE);
    }
    fprintf(stdout, "Network test completed.\n");   

    return 1;
}

static int run_sendfile(apr_pool_t *context, int number)
{
    apr_procattr_t *attr1 = NULL;
    apr_procattr_t *attr2 = NULL;
    apr_proc_t proc1;
    apr_proc_t proc2;
    apr_status_t s1;
    apr_status_t s2;
    const char *args[4];

    fprintf(stdout, "Creating children to run network tests.......\n");
    s1 = apr_procattr_create(&attr1, context);
    s2 = apr_procattr_create(&attr2, context);

    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS) {
        fprintf(stderr, "Problem creating proc attrs\n");
        exit(-1);
    }

    args[0] = apr_pstrdup(context, "sendfile");
    args[1] = apr_pstrdup(context, "server");
    args[2] = NULL; 
    s1 = apr_proc_create(&proc1, "./sendfile", args, NULL, attr1, context);

    /* Sleep for 5 seconds to ensure the server is setup before we begin */
    apr_sleep(5000000);
    args[1] = apr_pstrdup(context, "client");
    switch (number) {
        case 0: {
            args[2] = apr_pstrdup(context, "blocking");
            break;
        }
        case 1: {
            args[2] = apr_pstrdup(context, "nonblocking");
            break;
        }
        case 2: {
            args[2] = apr_pstrdup(context, "timeout");
            break;
        }
    }
    args[3] = NULL;
    s2 = apr_proc_create(&proc2, "./sendfile", args, NULL, attr2, context);

    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS) {
        fprintf(stderr, "Problem spawning new process\n");
        exit(-1);
    }

    while ((s1 = apr_proc_wait(&proc1, NULL, NULL, APR_NOWAIT)) == APR_CHILD_NOTDONE && 
           (s2 = apr_proc_wait(&proc2, NULL, NULL, APR_NOWAIT)) == APR_CHILD_NOTDONE) {
        continue;
    }

    if (s1 == APR_SUCCESS) {
        apr_proc_kill(&proc2, SIGTERM);
        while (apr_proc_wait(&proc2, NULL, NULL, APR_WAIT) == APR_CHILD_NOTDONE);
    }
    else {
        apr_proc_kill(&proc1, SIGTERM);
        while (apr_proc_wait(&proc1, NULL, NULL, APR_WAIT) == APR_CHILD_NOTDONE);
    }
    fprintf(stdout, "Network test completed.\n");   

    return 1;
}

int main(int argc, char *argv[])
{
    apr_pool_t *context = NULL;

    fprintf(stdout, "Initializing.........");
    if (apr_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Something went wrong\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    atexit(apr_terminate);

    fprintf(stdout, "Creating context.......");
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Could not create context\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "This test relies on the process test working.  Please\n");
    fprintf(stdout, "run that test first, and only run this test when it\n");
    fprintf(stdout, "completes successfully.  Alternatively, you could run\n");
    fprintf(stdout, "server and client by yourself.\n");
    run_basic_test(context);
    run_sendfile(context, 0);
    run_sendfile(context, 1);
    run_sendfile(context, 2);

    return 0;
}
