/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_file_io.h"
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* hmmm, what is a truly portable define for the max path
 * length on a platform?
 */
#define PATH_LEN 255

int main(void)
{
#if APR_HAS_MMAP    
    apr_pool_t *context;
    apr_mmap_t *themmap = NULL;
    apr_file_t *thefile = NULL;
    apr_finfo_t finfo;
    apr_int32_t flag = APR_READ;
    apr_status_t rv;
    char *file1;
    char errmsg[120];
    
    fprintf (stdout,"APR MMAP Test\n*************\n\n");
    
    fprintf(stdout,"Initializing........................");
    if (apr_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Failed.\n");
        exit(-1);
    }
    fprintf(stdout,"OK\n");
    atexit(apr_terminate);

    fprintf(stdout,"Creating context....................");    
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Failed.\n");
        exit(-1);
    }
    fprintf(stdout,"OK\n");
    
    file1 = (char*) apr_palloc(context, sizeof(char) * PATH_LEN);
    getcwd(file1, PATH_LEN);
    strncat(file1,"/testmmap.c",11);  

    fprintf(stdout, "Opening file........................");
    rv = apr_file_open(&thefile, file1, flag, APR_UREAD | APR_GREAD, context);
    if (rv != APR_SUCCESS) {
        fprintf(stderr,
                "couldn't open file `%s': %d/%s\n",
                file1, rv, apr_strerror(rv, errmsg, sizeof errmsg));
        exit(-1);
    }
    else {
        fprintf(stdout, "OK\n");
    }
    
    fprintf(stderr, "Getting file size...................");
    rv = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);
    if (rv != APR_SUCCESS) {
        fprintf(stderr,
                "Didn't get file information: %d/%s\n",
                rv, apr_strerror(rv, errmsg, sizeof errmsg));
        exit(-1);
    }
    else {
        fprintf(stdout, "%d bytes\n", (int)finfo.size);
    }  
    
    fprintf(stdout,"Trying to mmap the file.............");
    if (apr_mmap_create(&themmap, thefile, 0, finfo.size, APR_MMAP_READ, context) != APR_SUCCESS) {
        fprintf(stderr,"Failed!\n");
        exit(-1);
    }
    fprintf(stdout,"OK\n");

    fprintf(stdout,"Trying to delete the mmap file......");
    if (apr_mmap_delete(themmap) != APR_SUCCESS) {
        fprintf(stderr,"Failed!\n");
        exit (-1);
    }
    fprintf(stdout,"OK\n");
    
    fprintf (stdout,"\nTest Complete\n");

    return 1;
#else    
    fprintf(stdout,"APR MMAP Test\n*************\n\n");
    fprintf(stdout,"Failed!  APR was not built with MMAP.\n");
    return -1;
#endif
}
