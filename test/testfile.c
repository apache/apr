/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */
#include <stdio.h>
#include "apr_file_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#ifdef BEOS
#include <unistd.h>
#endif

int test_filedel(ap_context_t *);
int testdirs(ap_context_t *);

int main()
{
    ap_context_t *context;
    ap_context_t *cont2;
    ap_file_t *thefile = NULL;
    ap_status_t status = 0;
    ap_int32_t flag = APR_READ | APR_WRITE | APR_CREATE;
    ap_uint64_t rv = 0;
    ap_ssize_t nbytes = 0;
    ap_off_t zer = 0;
    char *buf;
    char *str;
    char *filename = "test.fil";
    if (ap_create_context(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate context.");
        exit(-1);
    }
    if (ap_create_context(&cont2, context) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate context.");
        exit(-1);
    }

    fprintf(stdout, "Testing file functions.\n");

    fprintf(stdout, "\tOpening file.......");
    if (ap_open(&thefile, filename, flag, APR_UREAD | APR_UWRITE | APR_GREAD, context) != APR_SUCCESS) {
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
    ap_get_filename(&str, thefile);
    if (strcmp(str, filename) != 0) {
        fprintf(stderr, "wrong filename\n");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tWriting to file.......");
    
    nbytes = (ap_ssize_t)strlen("this is a test");
    if (ap_write(thefile, "this is a test", &nbytes) != APR_SUCCESS) {
        perror("something's wrong");
        exit(-1);
    }
    if (nbytes != (ap_ssize_t)strlen("this is a test")) {
        fprintf(stderr, "didn't write properly.\n");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tMoving to start of file.......");
    zer = 0;
    if (ap_seek(thefile, SEEK_SET, &zer) != 0) {
        perror("couldn't seek to beginning of file.");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tReading from the file.......");
    nbytes = (ap_ssize_t)strlen("this is a test");
    buf = (char *)ap_palloc(context, nbytes + 1);
    if (ap_read(thefile, buf, &nbytes) != APR_SUCCESS) {
        perror("something's wrong");
        exit(-1);
    }
    if (nbytes != (ap_ssize_t)strlen("this is a test")) {
        fprintf(stderr, "didn't read properly.\n");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tClosing File.......");
    status = ap_close(thefile);
    if (status  != APR_SUCCESS) {
        fprintf(stderr, "Couldn't close the file\n");
        exit(-1); 
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    fprintf(stdout, "\tDeleting file.......");
    status = ap_remove_file(filename, context);
    if (status  != APR_SUCCESS) {
        fprintf(stderr, "Couldn't delete the file\n");
        exit(-1); 
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    fprintf(stdout, "\tMaking sure it's gone.......");
    status = ap_open(&thefile, filename, APR_READ, APR_UREAD | APR_UWRITE | APR_GREAD, context);
    if (status == APR_SUCCESS) {
        fprintf(stderr, "I could open the file for some reason?\n");
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    testdirs(context); 
    test_filedel(context);
 
    return 1;
}

int test_filedel(ap_context_t *context)
{
    ap_file_t *thefile;
    ap_int32_t flag = APR_READ | APR_WRITE | APR_CREATE;
    ap_status_t stat;
  
    stat = ap_open(&thefile, "testdel", flag, APR_UREAD | APR_UWRITE | APR_GREAD, context);
    if (stat != APR_SUCCESS) {
         return stat;
    }

    if ((stat = ap_close(thefile))  != APR_SUCCESS) {
        return stat;
    }

    if ((stat = ap_remove_file("testdel", context))  != APR_SUCCESS) {
        return stat;
    }

    stat = ap_open(&thefile, "testdel", APR_READ, APR_UREAD | APR_UWRITE | APR_GREAD, context);
    if (stat == APR_SUCCESS) {
        return stat;
    }
  
    return APR_SUCCESS;
}

int testdirs(ap_context_t *context)
{
    ap_dir_t *temp;  
    ap_file_t *file;
    ap_ssize_t bytes;
    ap_filetype_e type;
    char *fname;

    fprintf(stdout, "Testing Directory functions.\n");

    fprintf(stdout, "\tMakeing Directory.......");
    if (ap_make_dir("testdir", APR_UREAD | APR_UWRITE | APR_UEXECUTE | APR_GREAD | APR_GWRITE | APR_GEXECUTE | APR_WREAD | APR_WWRITE | APR_WEXECUTE, context)  != APR_SUCCESS) {
        fprintf(stderr, "Could not create directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    if (ap_open(&file, "testdir/testfile", APR_READ | APR_WRITE | APR_CREATE, APR_UREAD | APR_UWRITE | APR_UEXECUTE, context) != APR_SUCCESS) {;
        return -1;
    }

    bytes = strlen("Another test!!!");
    ap_write(file, "Another test!!", &bytes); 
	ap_close(file);

    fprintf(stdout, "\tOpening Directory.......");
    if (ap_opendir(&temp, "testdir", context) != APR_SUCCESS) {
        fprintf(stderr, "Could not open directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tReading Directory.......");
    if ((ap_readdir(temp))  != APR_SUCCESS) {
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
        if (ap_readdir(temp) != APR_SUCCESS) {
            fprintf(stderr, "Error reading directory testdir"); 
            return -1;
        }
        ap_get_dir_filename(&fname, temp);
    } while (fname[0] == '.');
    if (strcmp(fname, "testfile")) {
        fprintf(stderr, "Got wrong file name %s\n", fname);
        return -1;
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\t\tFile type.......");
    ap_dir_entry_ftype(&type, temp);
    if (type != APR_REG) {
        fprintf(stderr, "Got wrong file type\n");
        return -1;
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\t\tFile size.......");
    ap_dir_entry_size(&bytes, temp);
    if (bytes != (ap_ssize_t)strlen("Another test!!!")) {
        fprintf(stderr, "Got wrong file size %d\n", bytes);
        return -1;
    }
    fprintf(stdout, "OK\n");
     
    fprintf(stdout, "\tRewinding directory.......");
    ap_rewinddir(temp); 
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tClosing Directory.......");
    if (ap_closedir(temp)  != APR_SUCCESS) {
        fprintf(stderr, "Could not close directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tRemoving file from directory.......");
    if (ap_remove_file("testdir/testfile", context)  != APR_SUCCESS) {
        fprintf(stderr, "Could not remove file\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }

    fprintf(stdout, "\tRemoving Directory.......");
    if (ap_remove_dir("testdir", context)  != APR_SUCCESS) {
        fprintf(stderr, "Could not remove directory\n");
        return -1;
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    return 1; 
}    

