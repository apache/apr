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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_poll.h"
#include "apr_lib.h"
#include "test_apr.h"

struct view_fileinfo
{
    apr_int32_t bits;
    char *description;
} vfi[] = {
    {APR_FINFO_MTIME,  "MTIME"},
    {APR_FINFO_CTIME,  "CTIME"},
    {APR_FINFO_ATIME,  "ATIME"},
    {APR_FINFO_SIZE,   "SIZE"},
    {APR_FINFO_DEV,    "DEV"},
    {APR_FINFO_INODE,  "INODE"},
    {APR_FINFO_NLINK,  "NLINK"},
    {APR_FINFO_TYPE,   "TYPE"},
    {APR_FINFO_USER,   "USER"}, 
    {APR_FINFO_GROUP,  "GROUP"}, 
    {APR_FINFO_UPROT,  "UPROT"}, 
    {APR_FINFO_GPROT,  "GPROT"},
    {APR_FINFO_WPROT,  "WPROT"},
    {0,                NULL}
}; 

void test_filedel(apr_pool_t *);
void testdirs(apr_pool_t *);
static void test_read(apr_pool_t *);
static void test_read_seek(apr_int32_t, apr_pool_t *);
static void test_mod_neg(apr_pool_t *, apr_int32_t);

int main(void)
{
    apr_pool_t *pool;
    apr_file_t *thefile = NULL;
    apr_finfo_t finfo;
    apr_socket_t *testsock = NULL;
    apr_pollfd_t *sdset = NULL;
    apr_status_t status;
    apr_int32_t flag = APR_READ | APR_WRITE | APR_CREATE;
    apr_size_t nbytes = 0;
    apr_off_t zer = 0;
    char *buf;
    const char *str;
    char *filename = "test.fil";
    char *teststr;
    apr_uid_t uid;
    apr_gid_t gid;
#if APR_FILES_AS_SOCKETS
    apr_int32_t num;
#endif

    printf("APR File Functions Test\n=======================\n\n");
    
    STD_TEST_NEQ("Initializing APR", apr_initialize())
    atexit(apr_terminate);
    STD_TEST_NEQ("Creating the main pool we'll use", 
                 apr_pool_create(&pool, NULL))
    STD_TEST_NEQ("Creating the second pool we'll use", 
                 apr_pool_create(&pool, NULL))
    

    fprintf(stdout, "Testing file functions.\n");

    STD_TEST_NEQ("    Opening file", 
                 apr_file_open(&thefile, filename, flag, 
                               APR_UREAD | APR_UWRITE | APR_GREAD, pool))
    
    printf("%-60s", "    Checking filename");
    if (thefile == NULL){
        MSG_AND_EXIT("\nBad file descriptor")
    }
    apr_file_name_get(&str, thefile);
    printf("%s\n", str);
    if (strcmp(str, filename) != 0){
        MSG_AND_EXIT("Wrong filename\n")
    }
    
    nbytes = strlen("this is a test");
    STD_TEST_NEQ("    Writing to the file", 
                 apr_file_write(thefile, "this is a test", &nbytes))
    TEST_NEQ("    Checking we wrote everything", nbytes,
             strlen("this is a test"), "OK", "Failed to write everything")

    zer = 0;
    STD_TEST_NEQ("    Moving to the start of file", 
                 apr_file_seek(thefile, SEEK_SET, &zer))
                 
#if APR_FILES_AS_SOCKETS
    printf("    This platform supports files_like_sockets, testing...\n");
    STD_TEST_NEQ("        Making file look like a socket",
                 apr_socket_from_file(&testsock, thefile))

    apr_poll_setup(&sdset, 1, pool);
    apr_poll_socket_add(sdset, testsock, APR_POLLIN);
    num = 1;
    STD_TEST_NEQ_NONFATAL("        Checking for incoming data",
      apr_poll(sdset, 1, &num, apr_time_from_sec(1)));
    if (num == 0) {
        printf("** This platform doesn't return readability on a regular file.**\n");
    }
    printf("    End of files as sockets test.\n");
#endif

    nbytes = strlen("this is a test");
    buf = (char *)apr_palloc(pool, nbytes + 1);
    STD_TEST_NEQ("    Reading from the file",
                 apr_file_read(thefile, buf, &nbytes))
    TEST_NEQ("    Checking what we read", nbytes, strlen("this is a test"),
             "OK", "We didn't read properly.\n")
    STD_TEST_NEQ("    Adding user data to the file",
                 apr_file_data_set(thefile, "This is a test",
                                   "test", apr_pool_cleanup_null))
    STD_TEST_NEQ("    Getting user data from the file",
                 apr_file_data_get((void **)&teststr, "test", thefile))
    TEST_NEQ("    Checking the data we got", strcmp(teststr, "This is a test"), 0,
             "OK", "Got the data, but it was wrong")

    printf("%-60s", "    Getting fileinfo");
    status = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);
    if (status  == APR_INCOMPLETE) {
	int i;
        printf("INCOMPLETE\n");
        for (i = 0; vfi[i].bits; ++i)
            if (vfi[i].bits & ~finfo.valid)
                fprintf(stderr, "\t    Missing %s\n", vfi[i].description);
    }
    else if (status != APR_SUCCESS) {
        printf("OK\n");
        MSG_AND_EXIT("Couldn't get the fileinfo")
    }
    else {
        printf("OK\n");
    }
    gid = finfo.group;
    uid = finfo.user;

    STD_TEST_NEQ("    Closing the file", apr_file_close(thefile))

    printf("%-60s", "    Stat'ing file");
    status = apr_stat(&finfo, filename, APR_FINFO_NORM, pool);
    if (status  == APR_INCOMPLETE) {
	int i;
        printf("INCOMPLETE\n");
        for (i = 0; vfi[i].bits; ++i)
            if (vfi[i].bits & ~finfo.valid)
                fprintf(stderr, "\t    Missing %s\n", vfi[i].description);
    }
    else if (status  != APR_SUCCESS) {
        printf("Failed\n");
        MSG_AND_EXIT("Couldn't stat the file")
    }
    else {
        printf("OK\n");
    }    

    if (finfo.valid & APR_FINFO_GROUP) {
        STD_TEST_NEQ("    Getting groupname", 
                     apr_group_name_get(&buf, finfo.group, pool))
        STD_TEST_NEQ("    Comparing group ID's",
                     apr_compare_groups(finfo.group, gid))
        printf("     (gid's for %s match)\n", buf);
    }

    if (finfo.valid & APR_FINFO_USER) {
        STD_TEST_NEQ("    Getting username", 
                     apr_get_username(&buf, finfo.user, pool))
        STD_TEST_NEQ("    Comparing users",
                     apr_compare_users(finfo.user, uid))
        printf("     (uid's for %s match)\n", buf);
    }

    STD_TEST_NEQ("    Deleting the file", apr_file_remove(filename, pool))
    TEST_EQ("    Making sure it's gone",
           apr_file_open(&thefile, filename, APR_READ, 
                         APR_UREAD | APR_UWRITE | APR_GREAD, pool),
           APR_SUCCESS, "OK", "Failed")

    testdirs(pool); 
    test_filedel(pool);
    test_read(pool);
    test_mod_neg(pool, 0); /* unbuffered */
    test_mod_neg(pool, APR_BUFFERED);

    apr_pool_destroy(pool);

    printf("\nAll tests passed OK\n");
    return 0;
}

void test_filedel(apr_pool_t *pool)
{
    apr_file_t *thefile = NULL;
    apr_int32_t flag = APR_READ | APR_WRITE | APR_CREATE;
  
    STD_TEST_NEQ("    Creating the file",
                 apr_file_open(&thefile, "testdel", flag, 
                               APR_UREAD | APR_UWRITE | APR_GREAD, pool))
    STD_TEST_NEQ("    Closing the file",
                 apr_file_close(thefile))
    STD_TEST_NEQ("    Removing the file", apr_file_remove("testdel", pool))
    TEST_EQ("    Checking it's gone",
            apr_file_open(&thefile, "testdel", APR_READ, 
                          APR_UREAD | APR_UWRITE | APR_GREAD, pool),
            APR_SUCCESS,
            "OK", "Failed")
}

void testdirs(apr_pool_t *pool)
{
    apr_dir_t *temp;  
    apr_file_t *file = NULL;
    apr_size_t bytes;
    apr_finfo_t dirent;

    fprintf(stdout, "Testing Directory functions.\n");

    STD_TEST_NEQ("    Making directory",
                 apr_dir_make("tmpdir", 
                               APR_UREAD | APR_UWRITE | APR_UEXECUTE |
                               APR_GREAD | APR_GWRITE | APR_GEXECUTE | 
                               APR_WREAD | APR_WWRITE | APR_WEXECUTE, pool))

    STD_TEST_NEQ("    Creating a file in the new directory",
                 apr_file_open(&file, "tmpdir/testfile", 
                               APR_READ | APR_WRITE | APR_CREATE, 
                               APR_UREAD | APR_UWRITE | APR_UEXECUTE, pool))

    bytes = strlen("Another test!!!");
    apr_file_write(file, "Another test!!", &bytes); 
	apr_file_close(file);

    STD_TEST_NEQ("    Opening directory", apr_dir_open(&temp, "tmpdir", pool))
    STD_TEST_NEQ("    Reading directory", 
                 apr_dir_read(&dirent, APR_FINFO_DIRENT, temp))
       
    printf("    Getting Information about the file...\n");
    do {
        /* Because I want the file I created, I am skipping the "." and ".."
         * files that are here. 
         */
        if (apr_dir_read(&dirent, APR_FINFO_DIRENT | APR_FINFO_TYPE
                                | APR_FINFO_SIZE | APR_FINFO_MTIME, temp) 
                != APR_SUCCESS) {
            fprintf(stderr, "Error reading directory tmpdir"); 
            exit(-1);
        }
    } while (dirent.name[0] == '.');
    TEST_NEQ("        File name",
             strcmp(dirent.name, "testfile"), 0,
             "OK", "Got wrong file name");
    TEST_NEQ("        File type", dirent.filetype, APR_REG,
             "OK", "Got wrong file type")
    TEST_NEQ("        File size", dirent.size, bytes,
             "OK", "Got wrong file size")
    printf("    Done checking file information\n");
    STD_TEST_NEQ("    Rewind directory", apr_dir_rewind(temp))
    STD_TEST_NEQ("    Closing directory", apr_dir_close(temp))
    STD_TEST_NEQ("    Removing file from directory",
                 apr_file_remove("tmpdir/testfile", pool))
    STD_TEST_NEQ("    Removing directory", apr_dir_remove("tmpdir", pool))
}    

#define TESTREAD_BLKSIZE 1024
#define APR_BUFFERSIZE   4096 /* This should match APR's buffer size. */

static void create_testread(apr_pool_t *p, const char *fname)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char buf[TESTREAD_BLKSIZE];
    apr_size_t nbytes;

    /* Create a test file with known content.
     */
    
    rv = apr_file_open(&f, fname, APR_CREATE | APR_WRITE | APR_TRUNCATE, APR_UREAD | APR_UWRITE, p);
    if (rv) {
        fprintf(stderr, "apr_file_open()->%d/%s\n",
                rv, apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    nbytes = 4;
    rv = apr_file_write(f, "abc\n", &nbytes);
    assert(!rv && nbytes == 4);
    memset(buf, 'a', sizeof buf);
    nbytes = sizeof buf;
    rv = apr_file_write(f, buf, &nbytes);
    assert(!rv && nbytes == sizeof buf);
    nbytes = 2;
    rv = apr_file_write(f, "\n\n", &nbytes);
    assert(!rv && nbytes == 2);
    rv = apr_file_close(f);
    assert(!rv);
}

static char read_one(apr_file_t *f, int expected)
{
  char bytes[3];
  apr_status_t rv;
  static int counter = 0;
  apr_size_t nbytes;

  counter += 1;

  bytes[0] = bytes[2] = 0x01;
  if (counter % 2) {
      rv = apr_file_getc(bytes + 1, f);
  }
  else {
      nbytes = 1;
      rv = apr_file_read(f, bytes + 1, &nbytes);
      assert(nbytes == 1);
  }
  assert(!rv);
  assert(bytes[0] == 0x01 && bytes[2] == 0x01);
  if (expected != -1) {
      assert(bytes[1] == expected);
  }
  return bytes[1];
}

static int test_read_guts(apr_pool_t *p, const char *fname, apr_int32_t extra_flags)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    apr_size_t nbytes;
    char buf[1024];
    int i;

    rv = apr_file_open(&f, fname, APR_READ | extra_flags, 0, p);
    assert(!rv);
    read_one(f, 'a');
    read_one(f, 'b');
    rv = apr_file_ungetc('b', f);
    assert(!rv);
    /* Note: some implementations move the file ptr back;
     *       others just save up to one char; it isn't 
     *       portable to unget more than once.
     */
    /* Don't do this: rv = apr_file_ungetc('a', f); */
    read_one(f, 'b');
    read_one(f, 'c');
    read_one(f, '\n');
    for (i = 0; i < TESTREAD_BLKSIZE; i++) {
        read_one(f, 'a');
    }
    read_one(f, '\n');
    read_one(f, '\n');
    rv = apr_file_getc(buf, f);
    assert(rv == APR_EOF);
    rv = apr_file_close(f);
    assert(!rv);

    f = NULL;
    rv = apr_file_open(&f, fname, APR_READ | extra_flags, 0, p);
    assert(!rv);
    rv = apr_file_gets(buf, 10, f);
    assert(!rv);
    assert(!strcmp(buf, "abc\n"));
    /* read first 800 of TESTREAD_BLKSIZE 'a's 
     */
    rv = apr_file_gets(buf, 801, f);
    assert(!rv);
    assert(strlen(buf) == 800);
    for (i = 0; i < 800; i++) {
        assert(buf[i] == 'a');
    }
    /* read rest of the 'a's and the first newline
     */
    rv = apr_file_gets(buf, sizeof buf, f);
    assert(!rv);
    assert(strlen(buf) == TESTREAD_BLKSIZE - 800 + 1);
    for (i = 0; i < TESTREAD_BLKSIZE - 800; i++) {
        assert(buf[i] == 'a');
    }
    assert(buf[TESTREAD_BLKSIZE - 800] == '\n');
    /* read the last newline
     */
    rv = apr_file_gets(buf, sizeof buf, f);
    assert(!rv);
    assert(!strcmp(buf, "\n"));
    /* get APR_EOF
     */
    rv = apr_file_gets(buf, sizeof buf, f);
    assert(rv == APR_EOF);
    /* get APR_EOF with apr_file_getc
     */
    rv = apr_file_getc(buf, f);
    assert(rv == APR_EOF);
    /* get APR_EOF with apr_file_read
     */
    nbytes = sizeof buf;
    rv = apr_file_read(f, buf, &nbytes);
    assert(rv == APR_EOF);
    rv = apr_file_close(f);
    assert(!rv);
    return (1);
}

static void test_bigread(apr_pool_t *p, const char *fname, apr_int32_t extra_flags)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char buf[APR_BUFFERSIZE * 2];
    apr_size_t nbytes;

    /* Create a test file with known content.
     */
    rv = apr_file_open(&f, fname, APR_CREATE | APR_WRITE | APR_TRUNCATE, APR_UREAD | APR_UWRITE, p);
    if (rv) {
        fprintf(stderr, "apr_file_open()->%d/%s\n",
                rv, apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    nbytes = APR_BUFFERSIZE;
    memset(buf, 0xFE, nbytes);
    rv = apr_file_write(f, buf, &nbytes);
    assert(!rv && nbytes == APR_BUFFERSIZE);
    rv = apr_file_close(f);
    assert(!rv);

    f = NULL;
    rv = apr_file_open(&f, fname, APR_READ | extra_flags, 0, p);
    assert(!rv);
    nbytes = sizeof buf;
    rv = apr_file_read(f, buf, &nbytes);
    assert(!rv);
    assert(nbytes == APR_BUFFERSIZE);
    rv = apr_file_close(f);
    assert(!rv);
}

static void test_read(apr_pool_t *p)
{
    const char *fname = "testread.dat";
    apr_status_t rv;

    printf("Testing file read functions.\n");

    create_testread(p, fname);
    printf("%-60s", "    Buffered file tests");
    if (test_read_guts(p, fname, APR_BUFFERED))
        printf("OK\n");
    printf("%-60s", "    Unbuffered file tests");
    test_read_guts(p, fname, 0);
    printf("OK\n");
    printf("%-60s", "    More buffered file tests");
    test_bigread(p, fname, APR_BUFFERED);
    printf("OK\n");
    printf("%-60s", "    More unbuffered file tests");
    test_bigread(p, fname, 0);
    printf("OK\n");
    printf("%-60s", "    Even more buffered file tests");
    test_read_seek(APR_BUFFERED, p);
    printf("OK\n");
    printf("%-60s", "    Even more unbuffered file tests");
    test_read_seek(0, p);
    printf("OK\n");
    rv = apr_file_remove(fname, p);
    assert(!rv);
    printf("%-60s", "    All read tests");
    printf("OK\n");
}

static void test_read_seek(apr_int32_t moreflags, apr_pool_t *p)
{
    const char *fname = "readseek.dat";
    apr_status_t rv;
    apr_file_t *f;
    apr_size_t nbytes;
    const char *str1, *str2, *str3;
    char buf[8192];
    apr_off_t seek_amt;

    /* create the content */

    rv = apr_file_open(&f, fname, APR_CREATE | APR_WRITE, APR_UREAD | APR_UWRITE, p);
    assert(!rv);

    str1 = "abcdefghijklmnopqrstuvwxyz\n";
    str2 = "1234567890\n";
    str3 = "1234567890-=+_)(*&^%$#@!\n";

    nbytes = strlen(str1);
    rv = apr_file_write(f, str1, &nbytes);
    assert(!rv && nbytes == strlen(str1));

    nbytes = strlen(str2);
    rv = apr_file_write(f, str2, &nbytes);
    assert(!rv && nbytes == strlen(str2));

    nbytes = strlen(str3);
    rv = apr_file_write(f, str3, &nbytes);
    assert(!rv && nbytes == strlen(str3));

    nbytes = strlen(str1);
    rv = apr_file_write(f, str1, &nbytes);
    assert(!rv && nbytes == strlen(str1));

    nbytes = strlen(str2);
    rv = apr_file_write(f, str2, &nbytes);
    assert(!rv && nbytes == strlen(str2));

    nbytes = strlen(str3);
    rv = apr_file_write(f, str3, &nbytes);
    assert(!rv && nbytes == strlen(str3));

    rv = apr_file_close(f);
    assert(!rv);

    rv = apr_file_open(&f, fname, APR_READ | moreflags, 0, p);
    assert(!rv);

    rv = apr_file_gets(buf, sizeof buf, f);
    assert(!rv);
    assert(!strcmp(buf, str1));

    rv = apr_file_gets(buf, sizeof buf, f);
    assert(!rv);
    assert(!strcmp(buf, str2));

    nbytes = sizeof buf;
    rv = apr_file_read(f, buf, &nbytes);
    assert(!rv);
    assert(nbytes == strlen(str3) + strlen(str1) + strlen(str2) + strlen(str3));
    assert(!memcmp(buf, str3, strlen(str3)));

    /* seek back a couple of strings */
    seek_amt = -(apr_off_t)(nbytes - strlen(str3) - strlen(str1));
    rv = apr_file_seek(f, APR_CUR, &seek_amt);
    assert(!rv);
    /* seek_amt should be updated with the current offset into the file */
    assert(seek_amt == strlen(str1) + strlen(str2) + strlen(str3) + strlen(str1));

    rv = apr_file_gets(buf, sizeof buf, f);
    assert(!rv);
    assert(!strcmp(buf, str2));

    rv = apr_file_gets(buf, sizeof buf, f);
    assert(!rv);
    assert(!strcmp(buf, str3));

    rv = apr_file_close(f);
    assert(!rv);

    rv = apr_file_remove(fname, p);
    assert(!rv);
}

static void test_mod_neg(apr_pool_t *p, apr_int32_t flags)
{
    apr_status_t rv;
    apr_file_t *f;
    const char *s;
    int i;
    apr_size_t nbytes;
    char buf[8192];
    apr_off_t cur;
    const char *fname = "modneg.dat";

    printf("    Testing mod_negotiation-style file access (%sbuffered)...\n",
           !flags ? "un" : "");
    
    rv = apr_file_open(&f, fname, APR_CREATE | APR_WRITE, APR_UREAD | APR_UWRITE, p);
    assert(!rv);

    s = "body56789\n";
    nbytes = strlen(s);
    rv = apr_file_write(f, s, &nbytes);
    assert(!rv);
    assert(nbytes == strlen(s));
    
    for (i = 0; i < 7980; i++) {
        s = "0";
        nbytes = strlen(s);
        rv = apr_file_write(f, s, &nbytes);
        assert(!rv);
        assert(nbytes == strlen(s));
    }
    
    s = "end456789\n";
    nbytes = strlen(s);
    rv = apr_file_write(f, s, &nbytes);
    assert(!rv);
    assert(nbytes == strlen(s));

    for (i = 0; i < 10000; i++) {
        s = "1";
        nbytes = strlen(s);
        rv = apr_file_write(f, s, &nbytes);
        assert(!rv);
        assert(nbytes == strlen(s));
    }
    
    rv = apr_file_close(f);
    assert(!rv);

    rv = apr_file_open(&f, fname, APR_READ | flags, 0, p);
    assert(!rv);

    rv = apr_file_gets(buf, 11, f);
    assert(!rv);
    assert(!strcmp(buf, "body56789\n"));

    cur = 0;
    rv = apr_file_seek(f, APR_CUR, &cur);
    assert(!rv);
    assert(cur == 10);

    nbytes = sizeof(buf);
    rv = apr_file_read(f, buf, &nbytes);
    assert(!rv);
    assert(nbytes == sizeof(buf));

    cur = -((apr_off_t)nbytes - 7980);
    rv = apr_file_seek(f, APR_CUR, &cur);
    assert(!rv);
    assert(cur == 7990);

    rv = apr_file_gets(buf, 11, f);
    assert(!rv);
    assert(!strcmp(buf, "end456789\n"));

    rv = apr_file_close(f);
    assert(!rv);

    rv = apr_file_remove(fname, p);
    assert(!rv);
}
