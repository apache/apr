/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "testutil.h"

#define TESTSTR "This is a test"

static apr_proc_t newproc;

static void test_create_proc(abts_case *tc, void *data)
{
    const char *args[2];
    apr_procattr_t *attr;
    apr_file_t *testfile = NULL;
    apr_status_t rv;
    apr_size_t length;
    char *buf;

    rv = apr_procattr_create(&attr, p);
    abts_int_equal(tc, APR_SUCCESS, rv);

    rv = apr_procattr_io_set(attr, APR_FULL_BLOCK, APR_FULL_BLOCK, 
                             APR_NO_PIPE);
    abts_int_equal(tc, APR_SUCCESS, rv);

    rv = apr_procattr_dir_set(attr, "data");
    abts_int_equal(tc, APR_SUCCESS, rv);

    rv = apr_procattr_cmdtype_set(attr, APR_PROGRAM);
    abts_int_equal(tc, APR_SUCCESS, rv);

    args[0] = "proc_child" EXTENSION;
    args[1] = NULL;
    
    rv = apr_proc_create(&newproc, "../proc_child" EXTENSION, args, NULL, 
                         attr, p);
    abts_int_equal(tc, APR_SUCCESS, rv);

    testfile = newproc.in;

    length = strlen(TESTSTR);
    rv = apr_file_write(testfile, TESTSTR, &length);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_int_equal(tc, strlen(TESTSTR), length);

    testfile = newproc.out;
    length = 256;
    buf = apr_pcalloc(p, length);
    rv = apr_file_read(testfile, buf, &length);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_str_equal(tc, TESTSTR, buf);
}

static void test_proc_wait(abts_case *tc, void *data)
{
    apr_status_t rv;

    rv = apr_proc_wait(&newproc, NULL, NULL, APR_WAIT);
    abts_int_equal(tc, APR_CHILD_DONE, rv);
}

static void test_file_redir(abts_case *tc, void *data)
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
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_file_open(&testout, "data/stdout",
                       APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                       APR_OS_DEFAULT, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_file_open(&testerr, "data/stderr",
                       APR_READ | APR_WRITE | APR_CREATE | APR_EXCL,
                       APR_OS_DEFAULT, p);
    abts_int_equal(tc, APR_SUCCESS, rv);

    length = strlen(TESTSTR);
    apr_file_write(testfile, TESTSTR, &length);
    offset = 0;
    rv = apr_file_seek(testfile, APR_SET, &offset);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_int_equal(tc, 0, offset);

    rv = apr_procattr_create(&attr, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_procattr_child_in_set(attr, testfile, NULL);
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_procattr_child_out_set(attr, testout, NULL);
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_procattr_child_err_set(attr, testerr, NULL);
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_procattr_dir_set(attr, "data");
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_procattr_cmdtype_set(attr, APR_PROGRAM);
    abts_int_equal(tc, APR_SUCCESS, rv);

    args[0] = "proc_child";
    args[1] = NULL;

    rv = apr_proc_create(&newproc, "../proc_child" EXTENSION, args, NULL, 
                         attr, p);
    abts_int_equal(tc, APR_SUCCESS, rv);

    rv = apr_proc_wait(&newproc, NULL, NULL, APR_WAIT);
    abts_int_equal(tc, APR_CHILD_DONE, rv);

    offset = 0;
    rv = apr_file_seek(testout, APR_SET, &offset);
    abts_int_equal(tc, APR_SUCCESS, rv);

    length = 256;
    buf = apr_pcalloc(p, length);
    rv = apr_file_read(testout, buf, &length);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_str_equal(tc, TESTSTR, buf);


    apr_file_close(testfile);
    apr_file_close(testout);
    apr_file_close(testerr);

    rv = apr_file_remove("data/stdin", p);;
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_file_remove("data/stdout", p);;
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_file_remove("data/stderr", p);;
    abts_int_equal(tc, APR_SUCCESS, rv);
}

abts_suite *testproc(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, test_create_proc, NULL);
    abts_run_test(suite, test_proc_wait, NULL);
    abts_run_test(suite, test_file_redir, NULL);

    return suite;
}

