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


#include "apr_general.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_file_io.h"
#include "testutil.h"

#define TEST "Testing\n"
#define TEST2 "Testing again\n"
#define FILEPATH "data/"

static void test_file_dup(abts_case *tc, void *data)
{
    apr_file_t *file1 = NULL;
    apr_file_t *file3 = NULL;
    apr_status_t rv;
    apr_finfo_t finfo;

    /* First, create a new file, empty... */
    rv = apr_file_open(&file1, FILEPATH "testdup.file", 
                       APR_READ | APR_WRITE | APR_CREATE |
                       APR_DELONCLOSE, APR_OS_DEFAULT, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, file1);

    rv = apr_file_dup(&file3, file1, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, file3);

    rv = apr_file_close(file1);
    abts_int_equal(tc, APR_SUCCESS, rv);

    /* cleanup after ourselves */
    rv = apr_file_close(file3);
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_stat(&finfo, FILEPATH "testdup.file", APR_FINFO_NORM, p);
    abts_int_equal(tc, 1, APR_STATUS_IS_ENOENT(rv));
}  

static void test_file_readwrite(abts_case *tc, void *data)
{
    apr_file_t *file1 = NULL;
    apr_file_t *file3 = NULL;
    apr_status_t rv;
    apr_finfo_t finfo;
    apr_size_t txtlen = sizeof(TEST);
    char buff[50];
    apr_off_t fpos;

    /* First, create a new file, empty... */
    rv = apr_file_open(&file1, FILEPATH "testdup.readwrite.file", 
                       APR_READ | APR_WRITE | APR_CREATE |
                       APR_DELONCLOSE, APR_OS_DEFAULT, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, file1);

    rv = apr_file_dup(&file3, file1, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, file3);

    rv = apr_file_write(file3, TEST, &txtlen);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_int_equal(tc, sizeof(TEST), txtlen);

    fpos = 0;
    rv = apr_file_seek(file1, APR_SET, &fpos);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_int_equal(tc, 0, fpos);

    txtlen = 50;
    rv = apr_file_read(file1, buff, &txtlen);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_str_equal(tc, TEST, buff);

    /* cleanup after ourselves */
    rv = apr_file_close(file1);
    abts_int_equal(tc, APR_SUCCESS, rv);

    rv = apr_file_close(file3);
    abts_int_equal(tc, APR_SUCCESS, rv);
    rv = apr_stat(&finfo, FILEPATH "testdup.readwrite.file", APR_FINFO_NORM, p);
    abts_int_equal(tc, 1, APR_STATUS_IS_ENOENT(rv));
}  

static void test_dup2(abts_case *tc, void *data)
{
    apr_file_t *testfile = NULL;
    apr_file_t *errfile = NULL;
    apr_file_t *saveerr = NULL;
    apr_status_t rv;

    rv = apr_file_open(&testfile, FILEPATH "testdup2.file", 
                       APR_READ | APR_WRITE | APR_CREATE |
                       APR_DELONCLOSE, APR_OS_DEFAULT, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, testfile);

    rv = apr_file_open_stderr(&errfile, p);
    abts_int_equal(tc, APR_SUCCESS, rv);

    /* Set aside the real errfile */
    rv = apr_file_dup(&saveerr, errfile, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, saveerr);

    rv = apr_file_dup2(errfile, testfile, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, errfile);

    apr_file_close(testfile);

    rv = apr_file_dup2(errfile, saveerr, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, errfile);
}

static void test_dup2_readwrite(abts_case *tc, void *data)
{
    apr_file_t *errfile = NULL;
    apr_file_t *testfile = NULL;
    apr_file_t *saveerr = NULL;
    apr_status_t rv;
    apr_size_t txtlen = sizeof(TEST);
    char buff[50];
    apr_off_t fpos;

    rv = apr_file_open(&testfile, FILEPATH "testdup2.readwrite.file", 
                       APR_READ | APR_WRITE | APR_CREATE |
                       APR_DELONCLOSE, APR_OS_DEFAULT, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, testfile);

    rv = apr_file_open_stderr(&errfile, p);
    abts_int_equal(tc, APR_SUCCESS, rv);

    /* Set aside the real errfile */
    rv = apr_file_dup(&saveerr, errfile, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, saveerr);

    rv = apr_file_dup2(errfile, testfile, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, errfile);

    txtlen = sizeof(TEST2);
    rv = apr_file_write(errfile, TEST2, &txtlen);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_int_equal(tc, sizeof(TEST2), txtlen);

    fpos = 0;
    rv = apr_file_seek(testfile, APR_SET, &fpos);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_int_equal(tc, 0, fpos);

    txtlen = 50;
    rv = apr_file_read(testfile, buff, &txtlen);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_str_equal(tc, TEST2, buff);
      
    apr_file_close(testfile);

    rv = apr_file_dup2(errfile, saveerr, p);
    abts_int_equal(tc, APR_SUCCESS, rv);
    abts_ptr_notnull(tc, errfile);
}

abts_suite *testdup(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, test_file_dup, NULL);
    abts_run_test(suite, test_file_readwrite, NULL);
    abts_run_test(suite, test_dup2, NULL);
    abts_run_test(suite, test_dup2_readwrite, NULL);

    return suite;
}

