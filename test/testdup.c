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


#include "apr_general.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_file_io.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "test_apr.h"

#define TEST "Testing\n"
#define TEST2 "Testing again\n"

int main (int argc, char ** argv)
{
    apr_file_t *file1 = NULL;
    apr_file_t *file2 = NULL;
    apr_file_t *file3 = NULL;

    apr_pool_t *p;
    apr_size_t txtlen = sizeof(TEST);
    char buff[50];
    apr_off_t fpos;

    apr_initialize();
    atexit(apr_terminate);

    fprintf(stdout, "APR File Duplication Test\n=========================\n\n");
    STD_TEST_NEQ("Creating a pool", apr_pool_create(&p, NULL))

    printf("\nTesting apr_file_dup\n");
    /* First, create a new file, empty... */
    STD_TEST_NEQ("    Open a new, empty file (#1)", apr_file_open(&file1, "./testdup.file",
                                            APR_READ | APR_WRITE | APR_CREATE,
                                            APR_OS_DEFAULT, p))

    STD_TEST_NEQ("    Simple dup", apr_file_dup(&file3, file1, p))

    STD_TEST_NEQ("    Write to dup'd file (#3)", apr_file_write(file3, TEST, &txtlen))

    fpos = 0;
    STD_TEST_NEQ("    Rewind file #1 to start", apr_file_seek(file1, APR_SET, &fpos))

    txtlen = 50;
    STD_TEST_NEQ("    Read from file #1", 
                 apr_file_read(file1, buff, &txtlen))

    TEST_NEQ("    Checking what we read from #1", strcmp(buff, TEST), 0, "OK", "Failed")


    printf("\nTesting apr_file_dup2\n");
    STD_TEST_NEQ("    Open another new, empty file (#2)",
                 apr_file_open(&file2, "./testdup2.file",
                               APR_READ | APR_WRITE | APR_CREATE,
                               APR_OS_DEFAULT, p))

    STD_TEST_NEQ("    Dup2 test", apr_file_dup2(file3, file2, p))

    txtlen = sizeof(TEST2);
    STD_TEST_NEQ("    Write to dup'd file #3", apr_file_write(file3, TEST2, &txtlen))

    STD_TEST_NEQ("    Rewind file #2 to start", apr_file_seek(file2, APR_SET, &fpos))

    txtlen = 50;
    STD_TEST_NEQ("    Read from file #2",
                 apr_file_read(file2, buff, &txtlen))

    TEST_NEQ("    Checking what we read from #2", strcmp(buff, TEST2), 0, "OK", "Failed")
      
    printf("\nCleaning up\n");
    STD_TEST_NEQ("    Closing file #3", apr_file_close(file3))
    STD_TEST_NEQ("    Closing file #2", apr_file_close(file2))
    STD_TEST_NEQ("    Closing file #1", apr_file_close(file1))

    STD_TEST_NEQ("    Removing first test file", apr_file_remove("./testdup.file", p))
    STD_TEST_NEQ("    Removing first test file", apr_file_remove("./testdup2.file", p))

    printf("\nAll Tests completed - OK!\n");

    return (0);
}

