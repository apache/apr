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

#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_poll.h"
#include "apr_lib.h"
#include "testutil.h"

#define DIRNAME "data"
#define FILENAME DIRNAME "/file_datafile.txt"
#define TESTSTR  "This is the file data file."

#define TESTREAD_BLKSIZE 1024
#define APR_BUFFERSIZE   4096 /* This should match APR's buffer size. */



static void test_open_noreadwrite(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *thefile = NULL;

    rv = apr_file_open(&thefile, FILENAME,
                       APR_CREATE | APR_EXCL, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_TRUE(tc, rv != APR_SUCCESS);
    ABTS_INT_EQUAL(tc, 1, APR_STATUS_IS_EACCES(rv));
    ABTS_PTR_EQUAL(tc, NULL, thefile); 
}

static void test_open_excl(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *thefile = NULL;

    rv = apr_file_open(&thefile, FILENAME,
                       APR_CREATE | APR_EXCL | APR_WRITE, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_TRUE(tc, rv != APR_SUCCESS);
    ABTS_INT_EQUAL(tc, 1, APR_STATUS_IS_EEXIST(rv));
    ABTS_PTR_EQUAL(tc, NULL, thefile); 
}

static void test_open_read(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *filetest = NULL;

    rv = apr_file_open(&filetest, FILENAME, 
                       APR_READ, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, filetest);
    apr_file_close(filetest);
}

static void test_read(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_size_t nbytes = 256;
    char *str = apr_pcalloc(p, nbytes + 1);
    apr_file_t *filetest = NULL;
    
    rv = apr_file_open(&filetest, FILENAME, 
                       APR_READ, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);

    apr_assert_success(tc, "Opening test file " FILENAME, rv);
    rv = apr_file_read(filetest, str, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, strlen(TESTSTR), nbytes);
    ABTS_STR_EQUAL(tc, TESTSTR, str);

    apr_file_close(filetest);
}

static void test_filename(abts_case *tc, void *data)
{
    const char *str;
    apr_status_t rv;
    apr_file_t *filetest = NULL;
    
    rv = apr_file_open(&filetest, FILENAME, 
                       APR_READ, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    apr_assert_success(tc, "Opening test file " FILENAME, rv);

    rv = apr_file_name_get(&str, filetest);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_STR_EQUAL(tc, FILENAME, str);

    apr_file_close(filetest);
}
    
static void test_fileclose(abts_case *tc, void *data)
{
    char str;
    apr_status_t rv;
    apr_size_t one = 1;
    apr_file_t *filetest = NULL;
    
    rv = apr_file_open(&filetest, FILENAME, 
                       APR_READ, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    apr_assert_success(tc, "Opening test file " FILENAME, rv);

    rv = apr_file_close(filetest);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    /* We just closed the file, so this should fail */
    rv = apr_file_read(filetest, &str, &one);
    ABTS_INT_EQUAL(tc, 1, APR_STATUS_IS_EBADF(rv));
}

static void test_file_remove(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *filetest = NULL;

    rv = apr_file_remove(FILENAME, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_open(&filetest, FILENAME, APR_READ, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_INT_EQUAL(tc, 1, APR_STATUS_IS_ENOENT(rv));
}

static void test_open_write(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *filetest = NULL;

    filetest = NULL;
    rv = apr_file_open(&filetest, FILENAME, 
                       APR_WRITE, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_INT_EQUAL(tc, 1, APR_STATUS_IS_ENOENT(rv));
    ABTS_PTR_EQUAL(tc, NULL, filetest);
}

static void test_open_writecreate(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *filetest = NULL;

    filetest = NULL;
    rv = apr_file_open(&filetest, FILENAME, 
                       APR_WRITE | APR_CREATE, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    apr_file_close(filetest);
}

static void test_write(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_size_t bytes = strlen(TESTSTR);
    apr_file_t *filetest = NULL;

    rv = apr_file_open(&filetest, FILENAME, 
                       APR_WRITE | APR_CREATE, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_write(filetest, TESTSTR, &bytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    apr_file_close(filetest);
}

static void test_open_readwrite(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *filetest = NULL;

    filetest = NULL;
    rv = apr_file_open(&filetest, FILENAME, 
                       APR_READ | APR_WRITE, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, filetest);

    apr_file_close(filetest);
}

static void test_seek(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_off_t offset = 5;
    apr_size_t nbytes = 256;
    char *str = apr_pcalloc(p, nbytes + 1);
    apr_file_t *filetest = NULL;

    rv = apr_file_open(&filetest, FILENAME, 
                       APR_READ, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    apr_assert_success(tc, "Open test file " FILENAME, rv);

    rv = apr_file_read(filetest, str, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, strlen(TESTSTR), nbytes);
    ABTS_STR_EQUAL(tc, TESTSTR, str);

    memset(str, 0, nbytes + 1);

    rv = apr_file_seek(filetest, SEEK_SET, &offset);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    
    rv = apr_file_read(filetest, str, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, strlen(TESTSTR) - 5, nbytes);
    ABTS_STR_EQUAL(tc, TESTSTR + 5, str);

    apr_file_close(filetest);

    /* Test for regression of sign error bug with SEEK_END and
       buffered files. */
    rv = apr_file_open(&filetest, FILENAME,
                       APR_READ | APR_BUFFERED,
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    apr_assert_success(tc, "Open test file " FILENAME, rv);

    offset = -5;
    rv = apr_file_seek(filetest, SEEK_END, &offset);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, strlen(TESTSTR) - 5, nbytes);

    memset(str, 0, nbytes + 1);
    nbytes = 256;
    rv = apr_file_read(filetest, str, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, 5, nbytes);
    ABTS_STR_EQUAL(tc, TESTSTR + strlen(TESTSTR) - 5, str);

    apr_file_close(filetest);
}                

static void test_userdata_set(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *filetest = NULL;

    rv = apr_file_open(&filetest, FILENAME, 
                       APR_WRITE, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_data_set(filetest, "This is a test",
                           "test", apr_pool_cleanup_null);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    apr_file_close(filetest);
}

static void test_userdata_get(abts_case *tc, void *data)
{
    apr_status_t rv;
    char *teststr;
    apr_file_t *filetest = NULL;

    rv = apr_file_open(&filetest, FILENAME, 
                       APR_WRITE, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_data_set(filetest, "This is a test",
                           "test", apr_pool_cleanup_null);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_data_get((void **)&teststr, "test", filetest);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_STR_EQUAL(tc, "This is a test", teststr);

    apr_file_close(filetest);
}

static void test_userdata_getnokey(abts_case *tc, void *data)
{
    apr_status_t rv;
    char *teststr;
    apr_file_t *filetest = NULL;

    rv = apr_file_open(&filetest, FILENAME, 
                       APR_WRITE, 
                       APR_UREAD | APR_UWRITE | APR_GREAD, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_data_get((void **)&teststr, "nokey", filetest);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_EQUAL(tc, NULL, teststr);
    apr_file_close(filetest);
}

static void test_getc(abts_case *tc, void *data)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char ch;

    rv = apr_file_open(&f, FILENAME, APR_READ, 0, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    apr_file_getc(&ch, f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, (int)TESTSTR[0], (int)ch);
    apr_file_close(f);
}

static void test_ungetc(abts_case *tc, void *data)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char ch;

    rv = apr_file_open(&f, FILENAME, APR_READ, 0, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    apr_file_getc(&ch, f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, (int)TESTSTR[0], (int)ch);

    apr_file_ungetc('X', f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    apr_file_getc(&ch, f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, 'X', (int)ch);

    apr_file_close(f);
}

static void test_gets(abts_case *tc, void *data)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char *str = apr_palloc(p, 256);

    rv = apr_file_open(&f, FILENAME, APR_READ, 0, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_gets(str, 256, f);
    /* Only one line in the test file, so APR will encounter EOF on the first
     * call to gets, but we should get APR_SUCCESS on this call and
     * APR_EOF on the next.
     */
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_STR_EQUAL(tc, TESTSTR, str);
    rv = apr_file_gets(str, 256, f);
    ABTS_INT_EQUAL(tc, APR_EOF, rv);
    ABTS_STR_EQUAL(tc, "", str);
    apr_file_close(f);
}

static void test_bigread(abts_case *tc, void *data)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char buf[APR_BUFFERSIZE * 2];
    apr_size_t nbytes;

    /* Create a test file with known content.
     */
    rv = apr_file_open(&f, "data/created_file", 
                       APR_CREATE | APR_WRITE | APR_TRUNCATE, 
                       APR_UREAD | APR_UWRITE, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    nbytes = APR_BUFFERSIZE;
    memset(buf, 0xFE, nbytes);

    rv = apr_file_write(f, buf, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, APR_BUFFERSIZE, nbytes);

    rv = apr_file_close(f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    f = NULL;
    rv = apr_file_open(&f, "data/created_file", APR_READ, 0, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    nbytes = sizeof buf;
    rv = apr_file_read(f, buf, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, APR_BUFFERSIZE, nbytes);

    rv = apr_file_close(f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_remove("data/created_file", p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
}

/* This is a horrible name for this function.  We are testing APR, not how
 * Apache uses APR.  And, this function tests _way_ too much stuff.
 */
static void test_mod_neg(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *f;
    const char *s;
    int i;
    apr_size_t nbytes;
    char buf[8192];
    apr_off_t cur;
    const char *fname = "data/modneg.dat";

    rv = apr_file_open(&f, fname, 
                       APR_CREATE | APR_WRITE, APR_UREAD | APR_UWRITE, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    s = "body56789\n";
    nbytes = strlen(s);
    rv = apr_file_write(f, s, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, strlen(s), nbytes);
    
    for (i = 0; i < 7980; i++) {
        s = "0";
        nbytes = strlen(s);
        rv = apr_file_write(f, s, &nbytes);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
        ABTS_INT_EQUAL(tc, strlen(s), nbytes);
    }
    
    s = "end456789\n";
    nbytes = strlen(s);
    rv = apr_file_write(f, s, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, strlen(s), nbytes);

    for (i = 0; i < 10000; i++) {
        s = "1";
        nbytes = strlen(s);
        rv = apr_file_write(f, s, &nbytes);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
        ABTS_INT_EQUAL(tc, strlen(s), nbytes);
    }
    
    rv = apr_file_close(f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_open(&f, fname, APR_READ, 0, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_gets(buf, 11, f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_STR_EQUAL(tc, "body56789\n", buf);

    cur = 0;
    rv = apr_file_seek(f, APR_CUR, &cur);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, 10, cur);

    nbytes = sizeof(buf);
    rv = apr_file_read(f, buf, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, nbytes, sizeof(buf));

    cur = -((apr_off_t)nbytes - 7980);
    rv = apr_file_seek(f, APR_CUR, &cur);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, 7990, cur);

    rv = apr_file_gets(buf, 11, f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_STR_EQUAL(tc, "end456789\n", buf);

    rv = apr_file_close(f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_remove(fname, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
}

static void test_truncate(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_file_t *f;
    const char *fname = "data/testtruncate.dat";
    const char *s;
    apr_size_t nbytes;
    apr_finfo_t finfo;

    apr_file_remove(fname, p);

    rv = apr_file_open(&f, fname,
                       APR_CREATE | APR_WRITE, APR_UREAD | APR_UWRITE, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    
    s = "some data";
    nbytes = strlen(s);
    rv = apr_file_write(f, s, &nbytes);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, strlen(s), nbytes);

    rv = apr_file_close(f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_open(&f, fname,
                       APR_TRUNCATE | APR_WRITE, APR_UREAD | APR_UWRITE, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_file_close(f);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_stat(&finfo, fname, APR_FINFO_SIZE, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_INT_EQUAL(tc, 0, finfo.size);

    rv = apr_file_remove(fname, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
}

abts_suite *testfile(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, test_open_noreadwrite, NULL);
    abts_run_test(suite, test_open_excl, NULL);
    abts_run_test(suite, test_open_read, NULL);
    abts_run_test(suite, test_open_readwrite, NULL);
    abts_run_test(suite, test_read, NULL); 
    abts_run_test(suite, test_seek, NULL);
    abts_run_test(suite, test_filename, NULL);
    abts_run_test(suite, test_fileclose, NULL);
    abts_run_test(suite, test_file_remove, NULL);
    abts_run_test(suite, test_open_write, NULL);
    abts_run_test(suite, test_open_writecreate, NULL);
    abts_run_test(suite, test_write, NULL);
    abts_run_test(suite, test_userdata_set, NULL);
    abts_run_test(suite, test_userdata_get, NULL);
    abts_run_test(suite, test_userdata_getnokey, NULL);
    abts_run_test(suite, test_getc, NULL);
    abts_run_test(suite, test_ungetc, NULL);
    abts_run_test(suite, test_gets, NULL);
    abts_run_test(suite, test_bigread, NULL);
    abts_run_test(suite, test_mod_neg, NULL);
    abts_run_test(suite, test_truncate, NULL);

    return suite;
}

