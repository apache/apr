/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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
#include "apr_file_io.h"
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#ifdef BEOS
#include <unistd.h>
#endif

int test_filedel(apr_pool_t *);
int testdirs(apr_pool_t *);
static void test_read(apr_pool_t *);

int main(void)
{
    apr_pool_t *context;
    apr_pool_t *cont2;
    apr_file_t *thefile = NULL;
    apr_socket_t *testsock = NULL;
    apr_pollfd_t *sdset = NULL;
    apr_status_t status = 0;
    apr_int32_t flag = APR_READ | APR_WRITE | APR_CREATE;
    apr_ssize_t nbytes = 0;
    apr_int32_t rv;
    apr_off_t zer = 0;
    char *buf;
    char *str;
    char *filename = "test.fil";
    char *teststr;

    if (apr_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Couldn't initialize.");
        exit(-1);
    }
    atexit(apr_terminate);
    if (apr_create_pool(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate context.");
        exit(-1);
    }
    if (apr_create_pool(&cont2, context) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate context.");
        exit(-1);
    }

    fprintf(stdout, "Testing file functions.\n");

    fprintf(stdout, "\tOpening file.......");
    if (apr_open(&thefile, filename, flag, APR_UREAD | APR_UWRITE | APR_GREAD, context) != APR_SUCCESS) {
        perror("Didn't open file");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    fprintf(stdout, "\tChecking file.......");
    if (thefile == NULL) {
        fprintf(stderr, "Bad file des\n");
        exit(-1);
    }
    apr_get_filename(&str, thefile);
    if (strcmp(str, filename) != 0) {
        fprintf(stderr, "wrong filename\n");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tWriting to file.......");
    
    nbytes = (apr_ssize_t)strlen("this is a test");
    if (apr_write(thefile, "this is a test", &nbytes) != APR_SUCCESS) {
        perror("something's wrong");
        exit(-1);
    }
    if (nbytes != (apr_ssize_t)strlen("this is a test")) {
        fprintf(stderr, "didn't write properly.\n");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tMoving to start of file.......");
    zer = 0;
    if (apr_seek(thefile, SEEK_SET, &zer) != 0) {
        perror("couldn't seek to beginning of file.");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }

#if APR_FILES_AS_SOCKETS
    fprintf(stdout, "\tThis platform supports files_like_sockets\n");
    fprintf(stdout, "\t\tMaking file look like a socket.......");
    if (apr_socket_from_file(&testsock, thefile) != APR_SUCCESS) {
        perror("Something went wrong");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\t\tChecking for incoming data.......");
    apr_setup_poll(&sdset, 1, context);
    apr_add_poll_socket(sdset, testsock, APR_POLLIN);
    rv = 1;
    if (apr_poll(sdset, &rv, -1) != APR_SUCCESS) {
        fprintf(stderr, "Select caused an error\n");
        exit(-1);
    }
    else if (rv == 0) {
        fprintf(stderr, "I should not return until rv == 1\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
#endif


    fprintf(stdout, "\tReading from the file.......");
    nbytes = (apr_ssize_t)strlen("this is a test");
    buf = (char *)apr_palloc(context, nbytes + 1);
    if (apr_read(thefile, buf, &nbytes) != APR_SUCCESS) {
        perror("something's wrong");
        exit(-1);
    }
    if (nbytes != (apr_ssize_t)strlen("this is a test")) {
        fprintf(stderr, "didn't read properly.\n");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tAdding user data to the file.......");
    status = apr_set_filedata(thefile, "This is a test", "test", apr_null_cleanup);
    if (status  != APR_SUCCESS) {
        fprintf(stderr, "Couldn't add the data\n");
        exit(-1); 
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tGetting user data from the file.......");
    status = apr_get_filedata((void **)&teststr, "test", thefile);
    if (status  != APR_SUCCESS || strcmp(teststr, "This is a test")) {
        fprintf(stderr, "Couldn't get the data\n");
        exit(-1); 
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tClosing File.......");
    status = apr_close(thefile);
    if (status  != APR_SUCCESS) {
        fprintf(stderr, "Couldn't close the file\n");
        exit(-1); 
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    fprintf(stdout, "\tDeleting file.......");
    status = apr_remove_file(filename, context);
    if (status  != APR_SUCCESS) {
        fprintf(stderr, "Couldn't delete the file\n");
        exit(-1); 
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    fprintf(stdout, "\tMaking sure it's gone.......");
    status = apr_open(&thefile, filename, APR_READ, APR_UREAD | APR_UWRITE | APR_GREAD, context);
    if (status == APR_SUCCESS) {
        fprintf(stderr, "I could open the file for some reason?\n");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }

    testdirs(context); 
    test_filedel(context);
    test_read(context);

    return 1;
}

int test_filedel(apr_pool_t *context)
{
    apr_file_t *thefile = NULL;
    apr_int32_t flag = APR_READ | APR_WRITE | APR_CREATE;
    apr_status_t stat;
  
    stat = apr_open(&thefile, "testdel", flag, APR_UREAD | APR_UWRITE | APR_GREAD, context);
    if (stat != APR_SUCCESS) {
         return stat;
    }

    if ((stat = apr_close(thefile))  != APR_SUCCESS) {
        return stat;
    }

    if ((stat = apr_remove_file("testdel", context))  != APR_SUCCESS) {
        return stat;
    }

    stat = apr_open(&thefile, "testdel", APR_READ, APR_UREAD | APR_UWRITE | APR_GREAD, context);
    if (stat == APR_SUCCESS) {
        return stat;
    }
  
    return APR_SUCCESS;
}

int testdirs(apr_pool_t *context)
{
    apr_dir_t *temp;  
    apr_file_t *file = NULL;
    apr_ssize_t bytes;
    apr_filetype_e type;
    char *fname;

    fprintf(stdout, "Testing Directory functions.\n");

    fprintf(stdout, "\tMakeing Directory.......");
    if (apr_make_dir("testdir", APR_UREAD | APR_UWRITE | APR_UEXECUTE | APR_GREAD | APR_GWRITE | APR_GEXECUTE | APR_WREAD | APR_WWRITE | APR_WEXECUTE, context)  != APR_SUCCESS) {
        fprintf(stderr, "Could not create directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    if (apr_open(&file, "testdir/testfile", APR_READ | APR_WRITE | APR_CREATE, APR_UREAD | APR_UWRITE | APR_UEXECUTE, context) != APR_SUCCESS) {;
        return -1;
    }

    bytes = strlen("Another test!!!");
    apr_write(file, "Another test!!", &bytes); 
	apr_close(file);

    fprintf(stdout, "\tOpening Directory.......");
    if (apr_opendir(&temp, "testdir", context) != APR_SUCCESS) {
        fprintf(stderr, "Could not open directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tReading Directory.......");
    if ((apr_readdir(temp))  != APR_SUCCESS) {
        fprintf(stderr, "Could not read directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }
   
    fprintf(stdout, "\tGetting Information about the file.......\n");
    fprintf(stdout, "\t\tFile name.......");
    do {
        /* Because I want the file I created, I am skipping the "." and ".."
         * files that are here. 
         */
        if (apr_readdir(temp) != APR_SUCCESS) {
            fprintf(stderr, "Error reading directory testdir"); 
            return -1;
        }
        apr_get_dir_filename(&fname, temp);
    } while (fname[0] == '.');
    if (strcmp(fname, "testfile")) {
        fprintf(stderr, "Got wrong file name %s\n", fname);
        return -1;
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\t\tFile type.......");
    apr_dir_entry_ftype(&type, temp);
    if (type != APR_REG) {
        fprintf(stderr, "Got wrong file type\n");
        return -1;
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\t\tFile size.......");
    apr_dir_entry_size(&bytes, temp);
    if (bytes != (apr_ssize_t)strlen("Another test!!!")) {
        fprintf(stderr, "Got wrong file size %d\n", bytes);
        return -1;
    }
    fprintf(stdout, "OK\n");
     
    fprintf(stdout, "\tRewinding directory.......");
    apr_rewinddir(temp); 
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tClosing Directory.......");
    if (apr_closedir(temp)  != APR_SUCCESS) {
        fprintf(stderr, "Could not close directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tRemoving file from directory.......");
    if (apr_remove_file("testdir/testfile", context)  != APR_SUCCESS) {
        fprintf(stderr, "Could not remove file\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tRemoving Directory.......");
    if (apr_remove_dir("testdir", context)  != APR_SUCCESS) {
        fprintf(stderr, "Could not remove directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    return 1; 
}    

#define TESTREAD_BLKSIZE 1024
#define APR_BUFFERSIZE   4096 /* This should match APR's buffer size. */

static void create_testread(apr_pool_t *p, const char *fname)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char buf[TESTREAD_BLKSIZE];
    apr_ssize_t nbytes;

    /* Create a test file with known content.
     */
    rv = apr_open(&f, fname, APR_CREATE | APR_WRITE | APR_TRUNCATE, APR_UREAD | APR_UWRITE, p);
    if (rv) {
        fprintf(stderr, "apr_open()->%d/%s\n",
                rv, apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    nbytes = 4;
    rv = apr_write(f, "abc\n", &nbytes);
    assert(!rv && nbytes == 4);
    memset(buf, 'a', sizeof buf);
    nbytes = sizeof buf;
    rv = apr_write(f, buf, &nbytes);
    assert(!rv && nbytes == sizeof buf);
    nbytes = 2;
    rv = apr_write(f, "\n\n", &nbytes);
    assert(!rv && nbytes == 2);
    rv = apr_close(f);
    assert(!rv);
}

static char read_one(apr_file_t *f, int expected)
{
  char bytes[3];
  apr_status_t rv;
  static int counter = 0;
  apr_ssize_t nbytes;

  counter += 1;

  bytes[0] = bytes[2] = 0x01;
  if (counter % 2) {
      rv = apr_getc(bytes + 1, f);
  }
  else {
      nbytes = 1;
      rv = apr_read(f, bytes + 1, &nbytes);
      assert(nbytes == 1);
  }
  assert(!rv);
  assert(bytes[0] == 0x01 && bytes[2] == 0x01);
  if (expected != -1) {
      assert(bytes[1] == expected);
  }
  return bytes[1];
}

static void test_read_guts(apr_pool_t *p, const char *fname, apr_int32_t extra_flags)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    apr_ssize_t nbytes;
    char buf[1024];
    int i;

    rv = apr_open(&f, fname, APR_READ | extra_flags, 0, p);
    assert(!rv);
    read_one(f, 'a');
    read_one(f, 'b');
    if (extra_flags & APR_BUFFERED) {
        fprintf(stdout, 
                "\n\t\tskipping apr_ungetc() for APR_BUFFERED... "
                "doesn't work yet...\n\t\t\t\t ");
    }
    else {
        rv = apr_ungetc('b', f);
        assert(!rv);
        /* Note: some implementations move the file ptr back;
         *       others just save up to one char; it isn't 
         *       portable to unget more than once.
         */
        /* Don't do this: rv = apr_ungetc('a', f); */
        read_one(f, 'b');
    }
    read_one(f, 'c');
    read_one(f, '\n');
    for (i = 0; i < TESTREAD_BLKSIZE; i++) {
        read_one(f, 'a');
    }
    read_one(f, '\n');
    read_one(f, '\n');
    rv = apr_getc(buf, f);
    assert(rv == APR_EOF);
    rv = apr_close(f);
    assert(!rv);

    f = NULL;
    rv = apr_open(&f, fname, APR_READ | extra_flags, 0, p);
    assert(!rv);
    rv = apr_fgets(buf, 10, f);
    assert(!rv);
    assert(!strcmp(buf, "abc\n"));
    /* read first 800 of TESTREAD_BLKSIZE 'a's 
     */
    rv = apr_fgets(buf, 801, f);
    assert(!rv);
    assert(strlen(buf) == 800);
    for (i = 0; i < 800; i++) {
        assert(buf[i] == 'a');
    }
    /* read rest of the 'a's and the first newline
     */
    rv = apr_fgets(buf, sizeof buf, f);
    assert(!rv);
    assert(strlen(buf) == TESTREAD_BLKSIZE - 800 + 1);
    for (i = 0; i < TESTREAD_BLKSIZE - 800; i++) {
        assert(buf[i] == 'a');
    }
    assert(buf[TESTREAD_BLKSIZE - 800] == '\n');
    /* read the last newline
     */
    rv = apr_fgets(buf, sizeof buf, f);
    assert(!rv);
    assert(!strcmp(buf, "\n"));
    /* get APR_EOF
     */
    rv = apr_fgets(buf, sizeof buf, f);
    assert(rv == APR_EOF);
    /* get APR_EOF with apr_getc
     */
    rv = apr_getc(buf, f);
    assert(rv == APR_EOF);
    /* get APR_EOF with apr_read
     */
    nbytes = sizeof buf;
    rv = apr_read(f, buf, &nbytes);
    assert(rv == APR_EOF);
    rv = apr_close(f);
    assert(!rv);
}

static void test_bigread(apr_pool_t *p, const char *fname, apr_int32_t extra_flags)
{
    apr_file_t *f = NULL;
    apr_status_t rv;
    char buf[APR_BUFFERSIZE * 2];
    apr_ssize_t nbytes;

    /* Create a test file with known content.
     */
    rv = apr_open(&f, fname, APR_CREATE | APR_WRITE | APR_TRUNCATE, APR_UREAD | APR_UWRITE, p);
    if (rv) {
        fprintf(stderr, "apr_open()->%d/%s\n",
                rv, apr_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    nbytes = APR_BUFFERSIZE;
    memset(buf, 0xFE, nbytes);
    rv = apr_write(f, buf, &nbytes);
    assert(!rv && nbytes == APR_BUFFERSIZE);
    rv = apr_close(f);
    assert(!rv);

    f = NULL;
    rv = apr_open(&f, fname, APR_READ | extra_flags, 0, p);
    assert(!rv);
    nbytes = sizeof buf;
    rv = apr_read(f, buf, &nbytes);
    assert(!rv);
    assert(nbytes == APR_BUFFERSIZE);
    rv = apr_close(f);
    assert(!rv);
}

static void test_read(apr_pool_t *p)
{
    const char *fname = "testread.dat";
    apr_status_t rv;

    fprintf(stdout, "Testing file read functions.\n");

    create_testread(p, fname);
    fprintf(stdout, "\tBuffered file tests......");
    test_read_guts(p, fname, APR_BUFFERED);
    fprintf(stdout, "OK\n");
    fprintf(stdout, "\tUnbuffered file tests....");
    test_read_guts(p, fname, 0);
    fprintf(stdout, "OK\n");
    fprintf(stdout, "\tMore buffered file tests......");
    test_bigread(p, fname, APR_BUFFERED);
    fprintf(stdout, "OK\n");
    fprintf(stdout, "\tMore unbuffered file tests......");
    test_bigread(p, fname, 0);
    fprintf(stdout, "OK\n");
    rv = apr_remove_file(fname, p);
    assert(!rv);
    fprintf(stdout, "\tAll read tests...........OK\n");
}

