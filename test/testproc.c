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
#ifndef WIN32
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

int test_filedel(void);
int testdirs(void);

int main(int argc, char *argv[])
{
    ap_pool_t *context;
    ap_proc_t newproc;
    ap_procattr_t *attr;
    ap_file_t *testfile = NULL;
    ap_ssize_t length;
    char *buf;
    char *args[3];
    char *teststr;

    if (ap_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Couldn't initialize.");
        exit(-1);
    }
    atexit(ap_terminate);
    ap_create_pool(&context, NULL);


    if (argc > 1) {
        teststr = ap_palloc(context, 256);
        teststr = fgets(teststr, 256, stdin);      
        fprintf(stdout, "%s", teststr);      
        exit(1);
    }
    teststr = ap_pstrdup(context, "Whooo Hoooo\0");

    fprintf(stdout, "Creating directory for later use.......");
    if (ap_make_dir("proctest", APR_UREAD | APR_UWRITE | APR_UEXECUTE, context) != APR_SUCCESS) {
        fprintf(stderr, "Could not create dir\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Creating procattr.......");
    if (ap_createprocattr_init(&attr, context) != APR_SUCCESS) {
        fprintf(stderr, "Could not create attr\n");
        exit(-1);;
    }
    fprintf(stdout, "OK.\n");

    fprintf(stdout, "Setting attr pipes, all three.......");
    if (ap_setprocattr_io(attr, APR_FULL_BLOCK, 
                          APR_CHILD_BLOCK, APR_NO_PIPE) != APR_SUCCESS) {
        fprintf(stderr, "Could not set pipes attr\n");
        exit(-1);
    }
    fprintf(stdout, "OK.\n");
    
    fprintf(stdout, "Setting attr dir.......");
    if (ap_setprocattr_dir(attr, "proctest") != APR_SUCCESS) {
        fprintf(stderr, "Could not set directory attr\n");
        exit(-1);
    }
    fprintf(stdout, "OK.\n");

    fprintf(stdout, "Setting attr cmd type.......");
    if (ap_setprocattr_cmdtype(attr, APR_PROGRAM) != APR_SUCCESS) {
        fprintf(stderr, "Could not set cmd type attr\n");
        exit(-1);
    }
    fprintf(stdout, "OK.\n");

    args[0] = ap_pstrdup(context, "testproc");
    args[1] = ap_pstrdup(context, "-X");
    args[2] = NULL;
    
    fprintf(stdout, "Creating a new process.......");
    if (ap_create_process(&newproc, "../testproc", args, NULL, attr, context) != APR_SUCCESS) {
        fprintf(stderr, "Could not create the new process\n");
        exit(-1);
    }
    fprintf(stdout, "OK.\n");

    fprintf(stdout, "Grabbing child's stdin.......");
    testfile = newproc.stdin;
    fprintf(stdout, "OK.\n");

    length = 256;
    fprintf(stdout, "Writing the data to child.......");
    if (ap_write(testfile, teststr, &length) == APR_SUCCESS) {
        fprintf(stdout,"OK\n");
    }
    else fprintf(stderr, "Write failed.\n");

    fprintf(stdout, "Grabbing child's stdout.......");
    testfile = newproc.stdout;
    fprintf(stdout, "OK.\n");

    length = 256;
    fprintf(stdout, "Checking the data read from pipe to child.......");
    buf = ap_pcalloc(context, length);
    if (ap_read(testfile, buf, &length) == APR_SUCCESS) {
        if (!strcmp(buf, teststr))
            fprintf(stdout,"OK\n");
        else fprintf(stderr, "Uh-Oh\n");
    }
    else fprintf(stderr, "Read failed.\n");

    fprintf(stdout, "Waiting for child to die.......");
    if (ap_wait_proc(&newproc, APR_WAIT) != APR_CHILD_DONE) {
        fprintf(stderr, "Wait for child failed\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "Removing directory.......");
    if (ap_remove_dir("proctest", context) != APR_SUCCESS) {
        fprintf(stderr, "Could not remove directory.\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    return(1);
}

