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

#include "testutil.h"
#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_file_io.h"
#include "apr_strings.h"

/* hmmm, what is a truly portable define for the max path
 * length on a platform?
 */
#define PATH_LEN 255
#define TEST_STRING "This is the MMAP data file."APR_EOL_STR

#if !APR_HAS_MMAP
static void not_implemented(abts_case *tc, void *data)
{
    abts_not_impl(tc, "User functions");
}

#else

static apr_mmap_t *themmap = NULL;
static apr_file_t *thefile = NULL;
static char *file1;
static apr_finfo_t finfo;
static int fsize;

static void create_filename(abts_case *tc, void *data)
{
    char *oldfileptr;

    apr_filepath_get(&file1, 0, p);
#ifndef NETWARE
#ifdef WIN32
    abts_true(tc, file1[1] == ':');
#else
    abts_true(tc, file1[0] == '/');
#endif
#endif
    abts_true(tc, file1[strlen(file1) - 1] != '/');

    oldfileptr = file1;
    file1 = apr_pstrcat(p, file1,"/data/mmap_datafile.txt" ,NULL);
    abts_true(tc, oldfileptr != file1);
}

static void test_file_close(abts_case *tc, void *data)
{
    apr_status_t rv;

    rv = apr_file_close(thefile);
    abts_int_equal(tc, rv, APR_SUCCESS);
}
   
static void test_file_open(abts_case *tc, void *data)
{
    apr_status_t rv;

    rv = apr_file_open(&thefile, file1, APR_READ, APR_UREAD | APR_GREAD, p);
    abts_int_equal(tc, rv, APR_SUCCESS);
    abts_ptr_notnull(tc, thefile);
}
   
static void test_get_filesize(abts_case *tc, void *data)
{
    apr_status_t rv;

    rv = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);
    abts_int_equal(tc, rv, APR_SUCCESS);
    abts_int_equal(tc, fsize, finfo.size);
}

static void test_mmap_create(abts_case *tc, void *data)
{
    apr_status_t rv;

    rv = apr_mmap_create(&themmap, thefile, 0, finfo.size, APR_MMAP_READ, p);
    abts_ptr_notnull(tc, themmap);
    abts_int_equal(tc, rv, APR_SUCCESS);
}

static void test_mmap_contents(abts_case *tc, void *data)
{
    
    abts_ptr_notnull(tc, themmap);
    abts_ptr_notnull(tc, themmap->mm);
    abts_int_equal(tc, fsize, themmap->size);

    /* Must use nEquals since the string is not guaranteed to be NULL terminated */
    abts_str_nequal(tc, themmap->mm, TEST_STRING, fsize);
}

static void test_mmap_delete(abts_case *tc, void *data)
{
    apr_status_t rv;

    abts_ptr_notnull(tc, themmap);
    rv = apr_mmap_delete(themmap);
    abts_int_equal(tc, rv, APR_SUCCESS);
}

static void test_mmap_offset(abts_case *tc, void *data)
{
    apr_status_t rv;
    void *addr;

    abts_ptr_notnull(tc, themmap);
    rv = apr_mmap_offset(&addr, themmap, 5);

    /* Must use nEquals since the string is not guaranteed to be NULL terminated */
    abts_str_nequal(tc, addr, TEST_STRING + 5, fsize-5);
}
#endif

abts_suite *testmmap(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

#if APR_HAS_MMAP    
    fsize = strlen(TEST_STRING);

    abts_run_test(suite, create_filename, NULL);
    abts_run_test(suite, test_file_open, NULL);
    abts_run_test(suite, test_get_filesize, NULL);
    abts_run_test(suite, test_mmap_create, NULL);
    abts_run_test(suite, test_mmap_contents, NULL);
    abts_run_test(suite, test_mmap_offset, NULL);
    abts_run_test(suite, test_mmap_delete, NULL);
    abts_run_test(suite, test_file_close, NULL);
#else
    abts_run_test(suite, not_implemented, NULL);
#endif

    return suite;
}

