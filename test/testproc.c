/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

int main(int argc, char *argv[])
{
    apr_pool_t *pool;
    apr_proc_t newproc;
    apr_procattr_t *attr;
    apr_file_t *testfile = NULL;
    apr_size_t length;
    char *buf;
    const char *args[3];
    char *teststr;

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
        exit(1);
    }
    teststr = apr_pstrdup(pool, "Whooo Hoooo\0");
    
    printf("APR Process Test\n================\n\n");
    
    STD_TEST_NEQ("Creating directory for later use", 
                 apr_dir_make("proctest", APR_UREAD | APR_UWRITE | APR_UEXECUTE, pool))
    STD_TEST_NEQ("Creating procattr", apr_procattr_create(&attr, pool))
    STD_TEST_NEQ("Setting attr pipes, all three", apr_procattr_io_set(attr, APR_FULL_BLOCK, 
                 APR_CHILD_BLOCK, APR_NO_PIPE))
    STD_TEST_NEQ("Setting attr dir", apr_procattr_dir_set(attr, "proctest"))
    STD_TEST_NEQ("Setting attr cmd type", apr_procattr_cmdtype_set(attr, APR_PROGRAM))

    args[0] = "testproc";
    args[1] = "-X";
    args[2] = NULL;
    
    STD_TEST_NEQ("Creating a new process", apr_proc_create(&newproc,
                 "../testproc", args, NULL, attr, pool))

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
    if (apr_file_read(testfile, buf, &length) == APR_SUCCESS) {
        if (!strcmp(buf, teststr))
            printf("OK\n");
        else {
            printf( "Uh-Oh\n");
            printf("  (I actually got %s_\n", buf);
        }
    }
    else printf( "Read failed.\n");

    TEST_NEQ("Waiting for child to die",
             apr_proc_wait(&newproc, NULL, NULL, APR_WAIT),
             APR_CHILD_DONE, "OK", "Failed")   
    STD_TEST_NEQ("Removing directory", apr_dir_remove("proctest", pool))

    printf("\nTest completed succesfully\n");
    return 0;
}

