/* Copyright 2004 The Apache Software Foundation
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

#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_poll.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_mmap.h"
#include "test_apr.h"

/* Only enable these tests by default on platforms which support sparse
 * files... just Unixes? */
#if APR_HAS_LARGE_FILES && !defined(WIN32) && !defined(OS2) && !defined(NETWARE)
#define USE_LFS_TESTS

/* Tests which create an 8Gb sparse file and then check it can be used
 * as normal. */

static apr_off_t eightGb = APR_INT64_C(2) << 32;

static int madefile = 0;

#define PRECOND if (!madefile) CuNotImpl(tc, "Large file tests not enabled")

#define TESTDIR "lfstests"
#define TESTFILE "large.bin"
#define TESTFN "lfstests/large.bin"

static void test_open(CuTest *tc)
{
    apr_file_t *f;
    apr_status_t rv;

    rv = apr_dir_make(TESTDIR, APR_OS_DEFAULT, p);
    if (rv && !APR_STATUS_IS_EEXIST(rv)) {
        apr_assert_success(tc, "make test directory", rv);
    }

    apr_assert_success(tc, "open file",
                       apr_file_open(&f, TESTFN, 
                                     APR_CREATE | APR_WRITE | APR_TRUNCATE,
                                     APR_OS_DEFAULT, p));

    rv = apr_file_trunc(f, eightGb);

    apr_assert_success(tc, "close large file", apr_file_close(f));

    /* 8Gb may pass rlimits or filesystem limits */

    if (APR_STATUS_IS_EINVAL(rv)
#ifdef EFBIG
        || rv == EFBIG
#endif
        ) {
        CuNotImpl(tc, "Creation of large file (limited by rlimit or fs?)");
    } 
    else {
        apr_assert_success(tc, "truncate file to 8gb", rv);
    }

    madefile = 1;
}

static void test_reopen(CuTest *tc)
{
    apr_file_t *fh;
    apr_finfo_t finfo;

    PRECOND;
    
    apr_assert_success(tc, "re-open 8Gb file",
                       apr_file_open(&fh, TESTFN, APR_READ, APR_OS_DEFAULT, p));

    apr_assert_success(tc, "file_info_get failed",
                       apr_file_info_get(&finfo, APR_FINFO_NORM, fh));
    
    CuAssert(tc, "file_info_get gave incorrect size",
             finfo.size == eightGb);

    apr_assert_success(tc, "re-close large file", apr_file_close(fh));
}

static void test_stat(CuTest *tc)
{
    apr_finfo_t finfo;

    PRECOND;

    apr_assert_success(tc, "stat large file", 
                       apr_stat(&finfo, TESTFN, APR_FINFO_NORM, p));
    
    CuAssert(tc, "stat gave incorrect size", finfo.size == eightGb);
}

static void test_readdir(CuTest *tc)
{
    apr_dir_t *dh;
    apr_status_t rv;

    PRECOND;

    apr_assert_success(tc, "open test directory", 
                       apr_dir_open(&dh, TESTDIR, p));

    do {
        apr_finfo_t finfo;
        
        rv = apr_dir_read(&finfo, APR_FINFO_NORM, dh);
        
        if (rv == APR_SUCCESS && strcmp(finfo.name, TESTFILE) == 0) {
            CuAssert(tc, "apr_dir_read gave incorrect size for large file", 
                     finfo.size == eightGb);
        }

    } while (rv == APR_SUCCESS);
        
    if (!APR_STATUS_IS_ENOENT(rv)) {
        apr_assert_success(tc, "apr_dir_read failed", rv);
    }
    
    apr_assert_success(tc, "close test directory",
                       apr_dir_close(dh));
}

#define TESTSTR "Hello, world."

static void test_append(CuTest *tc)
{
    apr_file_t *fh;
    apr_finfo_t finfo;
    
    PRECOND;

    apr_assert_success(tc, "open 8Gb file for append",
                       apr_file_open(&fh, TESTFN, APR_WRITE | APR_APPEND, 
                                     APR_OS_DEFAULT, p));

    apr_assert_success(tc, "append to 8Gb file",
                       apr_file_write_full(fh, TESTSTR, strlen(TESTSTR), NULL));

    apr_assert_success(tc, "file_info_get failed",
                       apr_file_info_get(&finfo, APR_FINFO_NORM, fh));
    
    CuAssert(tc, "file_info_get gave incorrect size",
             finfo.size == eightGb + strlen(TESTSTR));

    apr_assert_success(tc, "close 8Gb file", apr_file_close(fh));
}

static void test_seek(CuTest *tc)
{
    apr_file_t *fh;
    apr_off_t pos;

    PRECOND;
    
    apr_assert_success(tc, "open 8Gb file for writing",
                       apr_file_open(&fh, TESTFN, APR_WRITE, 
                                     APR_OS_DEFAULT, p));

    pos = eightGb;
    apr_assert_success(tc, "seek to 8Gb", apr_file_seek(fh, APR_SET, &pos));
    CuAssert(tc, "seek gave 8Gb offset", pos == eightGb);

    pos = 0;
    apr_assert_success(tc, "relative seek to 0", apr_file_seek(fh, APR_CUR, &pos));
    CuAssert(tc, "relative seek gave 8Gb offset", pos == eightGb);

    apr_file_close(fh);
}

static void test_write(CuTest *tc)
{
    apr_file_t *fh;
    apr_off_t pos = eightGb - 4;

    PRECOND;

    apr_assert_success(tc, "re-open 8Gb file",
                       apr_file_open(&fh, TESTFN, APR_WRITE, APR_OS_DEFAULT, p));

    apr_assert_success(tc, "seek to 8Gb - 4", 
                       apr_file_seek(fh, APR_SET, &pos));
    CuAssert(tc, "seek gave 8Gb-4 offset", pos == eightGb - 4);

    apr_assert_success(tc, "write magic string to 8Gb-4",
                       apr_file_write_full(fh, "FISH", 4, NULL));

    apr_assert_success(tc, "close 8Gb file", apr_file_close(fh));
}


#if APR_HAS_MMAP
static void test_mmap(CuTest *tc)
{
    apr_mmap_t *map;
    apr_file_t *fh;
    apr_size_t len = 16384; /* hopefully a multiple of the page size */
    apr_off_t off = eightGb - len; 
    void *ptr;

    PRECOND;

    apr_assert_success(tc, "open 8gb file for mmap",
                       apr_file_open(&fh, TESTFN, APR_READ, APR_OS_DEFAULT, p));
    
    apr_assert_success(tc, "mmap 8Gb file",
                       apr_mmap_create(&map, fh, off, len, APR_MMAP_READ, p));

    apr_assert_success(tc, "close file", apr_file_close(fh));

    CuAssert(tc, "mapped a 16K block", map->size == len);
    
    apr_assert_success(tc, "get pointer into mmaped region",
                       apr_mmap_offset(&ptr, map, len - 4));
    CuAssert(tc, "pointer was not NULL", ptr != NULL);

    CuAssert(tc, "found the magic string", memcmp(ptr, "FISH", 4) == 0);

    apr_assert_success(tc, "delete mmap handle", apr_mmap_delete(map));
}
#endif /* APR_HAS_MMAP */

static void test_format(CuTest *tc)
{
    apr_off_t off;

    PRECOND;

    off = apr_atoi64(apr_off_t_toa(p, eightGb));

    CuAssert(tc, "apr_atoi64 parsed apr_off_t_toa result incorrectly",
             off == eightGb);
}

#else
static void test_nolfs(CuTest *tc)
{
    CuNotImpl(tc, "Large Files not supported");
}
#endif

CuSuite *testlfs(void)
{
    CuSuite *suite = CuSuiteNew("Large File Support");

#ifdef USE_LFS_TESTS
    SUITE_ADD_TEST(suite, test_open);
    SUITE_ADD_TEST(suite, test_reopen);
    SUITE_ADD_TEST(suite, test_stat);
    SUITE_ADD_TEST(suite, test_readdir);
    SUITE_ADD_TEST(suite, test_append);
    SUITE_ADD_TEST(suite, test_seek);
    SUITE_ADD_TEST(suite, test_write);
#if APR_HAS_MMAP
    SUITE_ADD_TEST(suite, test_mmap);
#endif
    SUITE_ADD_TEST(suite, test_format);
#else
    SUITE_ADD_TEST(suite, test_nolfs);
#endif

    return suite;
}

