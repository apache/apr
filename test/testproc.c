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

#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "test_apr.h"

/* XXX I'm sure there has to be a better way to do this ... */
#ifdef WIN32
#define EXTENSION ".exe"
#elif NETWARE
#define EXTENSION ".nlm"
#else
#define EXTENSION
#endif

#define TESTSTR "This is a test"

static apr_proc_t newproc;

static void test_create_proc(CuTest *tc)
{
    const char *args[2];
    apr_procattr_t *attr;
    apr_file_t *testfile = NULL;
    apr_status_t rv;
    apr_size_t length;
    char *buf;

    rv = apr_procattr_create(&attr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_io_set(attr, APR_FULL_BLOCK, APR_FULL_BLOCK, 
                             APR_NO_PIPE);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_dir_set(attr, "data");
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_cmdtype_set(attr, APR_PROGRAM);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    args[0] = "proc_child" EXTENSION;
    args[1] = NULL;
    
    rv = apr_proc_create(&newproc, "../proc_child" EXTENSION, args, NULL, 
                         attr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    testfile = newproc.in;

    length = strlen(TESTSTR);
    rv = apr_file_write(testfile, TESTSTR, &length);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, strlen(TESTSTR), length);

    testfile = newproc.out;
    length = 256;
    buf = apr_pcalloc(p, length);
    rv = apr_file_read(testfile, buf, &length);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, TESTSTR, buf);
}

static void test_proc_wait(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_proc_wait(&newproc, NULL, NULL, APR_WAIT);
    CuAssertIntEquals(tc, APR_CHILD_DONE, rv);
}

static void test_file_redir(CuTest *tc)
{
    apr_file_t *testout = NULL;
    apr_file_t *testerr = NULL;
    apr_off_t offset;
    apr_status_t rv;
    const char *args[2];
    apr_procattr_t *attr;
    apr_file_t *testfile = NULL;
    apr_size_t length;
    char *buf;

    testfile = NULL;
    rv = apr_file_open(&testfile, "data/stdin",
                       APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                       APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_open(&testout, "data/stdout",
                       APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                       APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_open(&testerr, "data/stderr",
                       APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                       APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    length = strlen(TESTSTR);
    apr_file_write(testfile, TESTSTR, &length);
    offset = 0;
    rv = apr_file_seek(testfile, APR_SET, &offset);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 0, offset);

    rv = apr_procattr_create(&attr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_child_in_set(attr, testfile, NULL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_child_out_set(attr, testout, NULL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_child_err_set(attr, testerr, NULL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_dir_set(attr, "data");
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_procattr_cmdtype_set(attr, APR_PROGRAM);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    args[0] = "proc_child";
    args[1] = NULL;

    rv = apr_proc_create(&newproc, "../proc_child" EXTENSION, args, NULL, 
                         attr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_proc_wait(&newproc, NULL, NULL, APR_WAIT);
    CuAssertIntEquals(tc, APR_CHILD_DONE, rv);

    offset = 0;
    rv = apr_file_seek(testout, APR_SET, &offset);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    length = 256;
    buf = apr_pcalloc(p, length);
    rv = apr_file_read(testout, buf, &length);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, TESTSTR, buf);


    apr_file_close(testfile);
    apr_file_close(testout);
    apr_file_close(testerr);

    rv = apr_file_remove("data/stdin", p);;
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_remove("data/stdout", p);;
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_remove("data/stderr", p);;
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

CuSuite *testproc(void)
{
    CuSuite *suite = CuSuiteNew("Process control");

    SUITE_ADD_TEST(suite, test_create_proc);
    SUITE_ADD_TEST(suite, test_proc_wait);
    SUITE_ADD_TEST(suite, test_file_redir);

    return suite;
}

