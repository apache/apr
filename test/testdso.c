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


#include "apr_general.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_dso.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef NETWARE
# define LIB_NAME "mod_test.nlm"
#else
# ifdef BEOS
#  define LIB_NAME "mod_test.so"
# else
#  ifdef DARWIN
#   define LIB_NAME ".libs/mod_test.so" 
#   define LIB_NAME2 ".libs/libmod_test.dylib" 
#  else
#   define LIB_NAME ".libs/mod_test.so"
#   define LIB_NAME2 ".libs/libmod_test.so"
#  endif
# endif
#endif

void test_shared_library(const char *libname, apr_pool_t *pool)
{
    apr_dso_handle_t *h = NULL;
    apr_dso_handle_sym_t func1 = NULL;
    apr_dso_handle_sym_t func2 = NULL;
    apr_status_t status;
    void (*function)(void);
    void (*function1)(int);
    int *retval;
    char filename[256];   

    getcwd(filename, 256);
    strcat(filename, "/");
    strcat(filename, libname);

    fprintf(stdout,"Trying to load DSO now.....................");
    fflush(stdout);
    if ((status = apr_dso_load(&h, filename, pool)) != APR_SUCCESS){
        char my_error[256];
        apr_strerror(status, my_error, sizeof(my_error));
        fprintf(stderr, "%s!\n", my_error);
        exit (-1);
    }
    fprintf(stdout,"OK\n");

    fprintf(stdout,"Trying to get the DSO's attention..........");
    fflush(stdout);
    if ((status = apr_dso_sym(&func1, h, "print_hello")) != APR_SUCCESS) { 
        char my_error[256];
        apr_dso_error(h, my_error, sizeof(my_error));
        fprintf(stderr, "%s\n", my_error);
        exit (-1);
    }        
    fprintf(stdout,"OK\n");
    
    function = (void *)func1;
    (*function)();

    fprintf(stdout,"Saying farewell 5 times....................");
    fflush(stdout);
    if (apr_dso_sym(&func2, h, "print_goodbye") != APR_SUCCESS) {
        fprintf(stderr, "Failed!\n");
        exit (-1);
    }        
    fprintf(stdout,"OK\n");

    function1 = (void *)(int)func2;
    (*function1)(5);

    fprintf(stdout,"Checking how many times I said goodbye..");
    fflush(stdout);
    if (apr_dso_sym(&func1, h, "goodbyes") != APR_SUCCESS) {
        fprintf(stderr, "Failed!\n");
        exit (-1);
    }
    retval = (int *)func1;
    fprintf(stdout,"%d..", (*retval));
    fflush(stdout);
    if ((*retval) == 5){
        fprintf(stderr,"OK\n");
    } else {
        fprintf(stderr,"Failed!\n");
    }
       
    fprintf(stdout,"Trying to unload DSO now...................");
    if (apr_dso_unload(h) != APR_SUCCESS) {
        fprintf(stderr, "Failed!\n");
        exit (-1);
    }
    fprintf(stdout,"OK\n");

    fprintf(stdout,"Checking it's been unloaded................");
    fflush(stdout);
    if (apr_dso_sym(&func1, h, "print_hello") == APR_SUCCESS) {
        fprintf(stderr, "Failed!\n");
        exit (-1);
    }        
    fprintf(stdout,"OK\n");
}

int main (int argc, char ** argv)
{
    apr_pool_t *pool;

    apr_initialize();
    atexit(apr_terminate);
        
    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate context.");
        exit(-1);
    }

    fprintf(stdout,"=== Checking module library ===\n");
    test_shared_library(LIB_NAME, pool);
#ifdef LIB_NAME2
    fprintf(stdout,"=== Checking non-module library ===\n");
    test_shared_library(LIB_NAME2, pool);
#endif

    return 0;
}
