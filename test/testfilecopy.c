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

#include "test_apr.h"
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_pools.h"

static void copy_helper(CuTest *tc, const char *from, const char * to,
                        apr_fileperms_t perms, int append, apr_pool_t *p)
{
    apr_status_t rv;
    apr_status_t dest_rv;
    apr_finfo_t copy;
    apr_finfo_t orig;
    apr_finfo_t dest;
    
    dest_rv = apr_stat(&dest, to, APR_FINFO_SIZE, p);
    
    if (!append) {
        rv = apr_file_copy(from, to, perms, p);
    }
    else {
        rv = apr_file_append(from, to, perms, p);
    }
    apr_assert_success(tc, "Error copying file", rv);

    rv = apr_stat(&orig, from, APR_FINFO_SIZE, p);
    apr_assert_success(tc, "Couldn't stat original file", rv);

    rv = apr_stat(&copy, to, APR_FINFO_SIZE, p);
    apr_assert_success(tc, "Couldn't stat copy file", rv);

    if (!append) {
        CuAssertIntEquals(tc, orig.size, copy.size);
    }
    else {
        CuAssertIntEquals(tc, 
                          ((dest_rv == APR_SUCCESS) ? dest.size : 0) + orig.size,
                          copy.size);
    }
}

static void copy_short_file(CuTest *tc)
{
    apr_status_t rv;

    /* make absolutely sure that the dest file doesn't exist. */
    apr_file_remove("data/file_copy.txt", p);
    
    copy_helper(tc, "data/file_datafile.txt", "data/file_copy.txt", 
                APR_FILE_SOURCE_PERMS, 0, p);
    rv = apr_file_remove("data/file_copy.txt", p);
    apr_assert_success(tc, "Couldn't remove copy file", rv);
}

static void copy_over_existing(CuTest *tc)
{
    apr_status_t rv;
    
    /* make absolutely sure that the dest file doesn't exist. */
    apr_file_remove("data/file_copy.txt", p);
    
    /* This is a cheat.  I don't want to create a new file, so I just copy
     * one file, then I copy another.  If the second copy succeeds, then
     * this works.
     */
    copy_helper(tc, "data/file_datafile.txt", "data/file_copy.txt", 
                APR_FILE_SOURCE_PERMS, 0, p);
    
    copy_helper(tc, "data/mmap_datafile.txt", "data/file_copy.txt", 
                APR_FILE_SOURCE_PERMS, 0, p);
  
    rv = apr_file_remove("data/file_copy.txt", p);
    apr_assert_success(tc, "Couldn't remove copy file", rv);
}

static void append_nonexist(CuTest *tc)
{
    apr_status_t rv;

    /* make absolutely sure that the dest file doesn't exist. */
    apr_file_remove("data/file_copy.txt", p);

    copy_helper(tc, "data/file_datafile.txt", "data/file_copy.txt", 
                APR_FILE_SOURCE_PERMS, 0, p);
    rv = apr_file_remove("data/file_copy.txt", p);
    apr_assert_success(tc, "Couldn't remove copy file", rv);
}

static void append_exist(CuTest *tc)
{
    apr_status_t rv;
    
    /* make absolutely sure that the dest file doesn't exist. */
    apr_file_remove("data/file_copy.txt", p);
    
    /* This is a cheat.  I don't want to create a new file, so I just copy
     * one file, then I copy another.  If the second copy succeeds, then
     * this works.
     */
    copy_helper(tc, "data/file_datafile.txt", "data/file_copy.txt", 
                APR_FILE_SOURCE_PERMS, 0, p);
    
    copy_helper(tc, "data/mmap_datafile.txt", "data/file_copy.txt", 
                APR_FILE_SOURCE_PERMS, 1, p);
  
    rv = apr_file_remove("data/file_copy.txt", p);
    apr_assert_success(tc, "Couldn't remove copy file", rv);
}

CuSuite *testfilecopy(void)
{
    CuSuite *suite = CuSuiteNew("File Copy");

    SUITE_ADD_TEST(suite, copy_short_file);
    SUITE_ADD_TEST(suite, copy_over_existing);

    SUITE_ADD_TEST(suite, append_nonexist);
    SUITE_ADD_TEST(suite, append_exist);

    return suite;
}

