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
#include <string.h>
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "test_apr.h"

static void test_mkdir(CuTest *tc)
{
    apr_status_t rv;
    apr_finfo_t finfo;

    rv = apr_dir_make("data/testdir", APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_stat(&finfo, "data/testdir", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, APR_DIR, finfo.filetype);
}

static void test_mkdir_recurs(CuTest *tc)
{
    apr_status_t rv;
    apr_finfo_t finfo;

    rv = apr_dir_make_recursive("data/one/two/three", 
                                APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_stat(&finfo, "data/one", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, APR_DIR, finfo.filetype);

    rv = apr_stat(&finfo, "data/one/two", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, APR_DIR, finfo.filetype);

    rv = apr_stat(&finfo, "data/one/two/three", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, APR_DIR, finfo.filetype);
}

static void test_remove(CuTest *tc)
{
    apr_status_t rv;
    apr_finfo_t finfo;

    rv = apr_dir_remove("data/testdir", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_stat(&finfo, "data/testdir", APR_FINFO_TYPE, p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));
}

static void test_removeall_fail(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_dir_remove("data/one", p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOTEMPTY(rv));
}

static void test_removeall(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_dir_remove("data/one/two/three", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_remove("data/one/two", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_remove("data/one", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_remove_notthere(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_dir_remove("data/notthere", p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));
}

static void test_mkdir_twice(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_dir_make("data/testdir", APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_make("data/testdir", APR_UREAD | APR_UWRITE | APR_UEXECUTE, p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EEXIST(rv));

    rv = apr_dir_remove("data/testdir", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_opendir(CuTest *tc)
{
    apr_status_t rv;
    apr_dir_t *dir;

    rv = apr_dir_open(&dir, "data", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    apr_dir_close(dir);
}

static void test_opendir_notthere(CuTest *tc)
{
    apr_status_t rv;
    apr_dir_t *dir;

    rv = apr_dir_open(&dir, "notthere", p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));
}

static void test_closedir(CuTest *tc)
{
    apr_status_t rv;
    apr_dir_t *dir;

    rv = apr_dir_open(&dir, "data", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_close(dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_readdir_onedot(CuTest *tc)
{
    apr_status_t rv;
    apr_dir_t *dir;
    apr_finfo_t finfo;

    rv = apr_dir_open(&dir, "data", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_read(&finfo, APR_FINFO_DIRENT, dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ".", finfo.name);

    rv = apr_dir_close(dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_readdir_twodot(CuTest *tc)
{
    apr_status_t rv;
    apr_dir_t *dir;
    apr_finfo_t finfo;

    rv = apr_dir_open(&dir, "data", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_read(&finfo, APR_FINFO_DIRENT, dir);
    rv = apr_dir_read(&finfo, APR_FINFO_DIRENT, dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, "..", finfo.name);

    rv = apr_dir_close(dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_rewind(CuTest *tc)
{
    apr_status_t rv;
    apr_dir_t *dir;
    apr_finfo_t finfo;

    rv = apr_dir_open(&dir, "data", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_read(&finfo, APR_FINFO_DIRENT, dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ".", finfo.name);

    rv = apr_dir_rewind(dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_dir_read(&finfo, APR_FINFO_DIRENT, dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ".", finfo.name);

    rv = apr_dir_close(dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

/* Test for a (fixed) bug in apr_dir_read().  This bug only happened
   in threadless cases. */
static void test_uncleared_errno(CuTest *tc)
{
    apr_file_t *thefile = NULL;
    apr_finfo_t finfo;
    apr_int32_t finfo_flags = APR_FINFO_TYPE | APR_FINFO_NAME;
    apr_dir_t *this_dir;
    apr_status_t rv; 

    rv = apr_dir_make("dir1", APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_make("dir2", APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_open(&thefile, "dir1/file1",
                       APR_READ | APR_WRITE | APR_CREATE, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_close(thefile);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    /* Try to remove dir1.  This should fail because it's not empty.
       However, on a platform with threads disabled (such as FreeBSD),
       `errno' will be set as a result. */
    rv = apr_dir_remove("dir1", p);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOTEMPTY(rv));
    
    /* Read `.' and `..' out of dir2. */
    rv = apr_dir_open(&this_dir, "dir2", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_read(&finfo, finfo_flags, this_dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_read(&finfo, finfo_flags, this_dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    /* Now, when we attempt to do a third read of empty dir2, and the
       underlying system readdir() returns NULL, the old value of
       errno shouldn't cause a false alarm.  We should get an ENOENT
       back from apr_dir_read, and *not* the old errno. */
    rv = apr_dir_read(&finfo, finfo_flags, this_dir);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ENOENT(rv));

    rv = apr_dir_close(this_dir);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
		 
    /* Cleanup */
    rv = apr_file_remove("dir1/file1", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_remove("dir1", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_dir_remove("dir2", p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

}

CuSuite *testdir(void)
{
    CuSuite *suite = CuSuiteNew("Directory");

    SUITE_ADD_TEST(suite, test_mkdir);
    SUITE_ADD_TEST(suite, test_mkdir_recurs);
    SUITE_ADD_TEST(suite, test_remove);
    SUITE_ADD_TEST(suite, test_removeall_fail);
    SUITE_ADD_TEST(suite, test_removeall);
    SUITE_ADD_TEST(suite, test_remove_notthere);
    SUITE_ADD_TEST(suite, test_mkdir_twice);

    SUITE_ADD_TEST(suite, test_readdir_onedot);
    SUITE_ADD_TEST(suite, test_readdir_twodot);
    SUITE_ADD_TEST(suite, test_rewind);

    SUITE_ADD_TEST(suite, test_opendir);
    SUITE_ADD_TEST(suite, test_opendir_notthere);
    SUITE_ADD_TEST(suite, test_closedir);
    SUITE_ADD_TEST(suite, test_uncleared_errno);

    return suite;
}

