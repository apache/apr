/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
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
#include "apr_strings.h"
#include <errno.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "test_apr.h"

int test_filedel(void);
int testdirs(void);

/* XXX I'm sure there has to be a better way to do this ... */
#ifdef WIN32
#define EXTENSION ".exe"
#else
#define EXTENSION
#endif

int main(int argc, char *argv[])
{
    apr_pool_t *pool;
    apr_proc_t newproc;
    apr_procattr_t *attr;
    apr_file_t *testfile = NULL;
    apr_file_t *testout = NULL;
    apr_file_t *testerr = NULL;
    apr_size_t length;
    apr_off_t offset;
    char *buf;
    char msgbuf[120];
    const char *args[3];
    char *teststr;
    apr_status_t rv;

    if (apr_initialize() != APR_SUCCESS){
        printf("Failed to initialize APR\n");
        exit(-1);
    }   
    atexit(apr_terminate);
    apr_pool_create(&pool, NULL);

    if (argc > 1) {
        teststr = apr_palloc(pool, 256);
        teststr = fgets(teststr, 256, stdin);      
        printf("%s", teststr);      
        if (!strcmp("--to-stderr", argv[1]))
            fprintf(stderr, "%s", teststr);
        exit(1);
    }
    teststr = apr_pstrdup(pool, "Whooo Hoooo\0");
    
    printf("APR Process Test\n================\n\n");
    
    STD_TEST_NEQ("Creating directory for later use", 
                 apr_dir_make("proctest", APR_UREAD | APR_UWRITE | APR_UEXECUTE, pool))

    /* =================================================================== */

    printf("\nTesting process pipes ...\n\n");

    STD_TEST_NEQ("Creating procattr", apr_procattr_create(&attr, pool))
    STD_TEST_NEQ("Setting attr pipes, all three", apr_procattr_io_set(attr, APR_FULL_BLOCK, 
                 APR_CHILD_BLOCK, APR_NO_PIPE))
    STD_TEST_NEQ("Setting attr dir", apr_procattr_dir_set(attr, "proctest"))
    STD_TEST_NEQ("Setting attr cmd type", apr_procattr_cmdtype_set(attr, APR_PROGRAM))

    args[0] = "testproc";
    args[1] = "-X";
    args[2] = NULL;
    
    STD_TEST_NEQ("Creating a new process", apr_proc_create(&newproc,
                 "../testproc" EXTENSION, args, NULL, attr, pool))

    printf("%-60s","Grabbing child's stdin");
    testfile = newproc.in;
    printf("OK\n");

    length = 256;
    printf("%-60s", "Writing the data to child");
    if (apr_file_write(testfile, teststr, &length) == APR_SUCCESS) {
        printf("OK\n");
    }
    else printf("Write failed.\n");

    printf("%-60s", "Grabbing child's stdout");
    testfile = newproc.out;
    printf("OK\n");

    length = 256;
    printf("%-60s", "Checking the data read from pipe to child");
    buf = apr_pcalloc(pool, length);
    if ((rv = apr_file_read(testfile, buf, &length)) == APR_SUCCESS) {
        if (!strcmp(buf, teststr))
            printf("OK\n");
        else {
            printf( "Uh-Oh\n");
            printf("  (I actually got %s_\n", buf);
        }
    }
    else {
        printf("Read failed - (%d) %s\n",
               rv, apr_strerror(rv, msgbuf, sizeof msgbuf));
    }

    TEST_NEQ("Waiting for child to die",
             apr_proc_wait(&newproc, NULL, NULL, APR_WAIT),
             APR_CHILD_DONE, "OK", "Failed")   

    /* =================================================================== */

    printf("\nTesting file redirection ...\n\n");

    testfile = NULL;
    STD_TEST_NEQ("Creating input file",
                 apr_file_open(&testfile, "proctest/stdin",
                               APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                               APR_OS_DEFAULT, pool))
    STD_TEST_NEQ("Creating output file",
                 apr_file_open(&testout, "proctest/stdout",
                               APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                               APR_OS_DEFAULT, pool))
    STD_TEST_NEQ("Creating error file",
                 apr_file_open(&testerr, "proctest/stderr",
                               APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                               APR_OS_DEFAULT, pool))

    length = strlen(teststr);
    STD_TEST_NEQ("Writing input file",
                 apr_file_write(testfile, teststr, &length))
    offset = 0;
    STD_TEST_NEQ("Rewinding input file",
                 apr_file_seek(testfile, APR_SET, &offset))

    STD_TEST_NEQ("Creating procattr", apr_procattr_create(&attr, pool))
    STD_TEST_NEQ("Setting attr input file",
                 apr_procattr_child_in_set(attr, testfile, NULL))
    STD_TEST_NEQ("Setting attr output file",
                 apr_procattr_child_out_set(attr, testout, NULL))
    STD_TEST_NEQ("Setting attr error file",
                 apr_procattr_child_err_set(attr, testerr, NULL))
    STD_TEST_NEQ("Setting attr dir", apr_procattr_dir_set(attr, "proctest"))
    STD_TEST_NEQ("Setting attr cmd type", apr_procattr_cmdtype_set(attr, APR_PROGRAM))

    args[0] = "testproc";
    args[1] = "--to-stderr";
    args[2] = NULL;

    STD_TEST_NEQ("Creating a new process", apr_proc_create(&newproc,
                 "../testproc" EXTENSION, args, NULL, attr, pool))

    TEST_NEQ("Waiting for child to die",
             apr_proc_wait(&newproc, NULL, NULL, APR_WAIT),
             APR_CHILD_DONE, "OK", "Failed")

    offset = 0;
    STD_TEST_NEQ("Rewinding output file",
                 apr_file_seek(testout, APR_SET, &offset))
    length = 256;
    printf("%-60s", "Checking the data read from child's stdout");
    buf = apr_pcalloc(pool, length);
    if ((rv = apr_file_read(testout, buf, &length)) == APR_SUCCESS) {
        if (!strcmp(buf, teststr))
            printf("OK\n");
        else {
            printf( "Uh-Oh\n");
            printf("  (I actually got %s_\n", buf);
        }
    }
    else {
        printf("Read failed - (%d) %s\n",
               rv, apr_strerror(rv, msgbuf, sizeof msgbuf));
    }

    offset = 0;
    STD_TEST_NEQ("Rewinding error file",
                 apr_file_seek(testerr, APR_SET, &offset))
    length = 256;
    printf("%-60s", "Checking the data read from child's stderr");
    buf = apr_pcalloc(pool, length);
    if ((rv = apr_file_read(testerr, buf, &length)) == APR_SUCCESS) {
        if (!strcmp(buf, teststr))
            printf("OK\n");
        else {
            printf( "Uh-Oh\n");
            printf("  (I actually got %s_\n", buf);
        }
    }
    else {
        printf("Read failed - (%d) %s\n",
               rv, apr_strerror(rv, msgbuf, sizeof msgbuf));
    }

    STD_TEST_NEQ("Closing input file", apr_file_close(testfile));
    STD_TEST_NEQ("Closing output file", apr_file_close(testout));
    STD_TEST_NEQ("Closing error file", apr_file_close(testerr));

    STD_TEST_NEQ("Removing input file", apr_file_remove("proctest/stdin", pool));
    STD_TEST_NEQ("Removing output file", apr_file_remove("proctest/stdout", pool));
    STD_TEST_NEQ("Removing error file", apr_file_remove("proctest/stderr", pool));

    /* =================================================================== */

    printf("\n");
    STD_TEST_NEQ("Removing directory", apr_dir_remove("proctest", pool))

    printf("\nTest completed succesfully\n");
    return 0;
}

