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
#include "apr_file_io.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "test_apr.h"

static void alloc_bytes(apr_pool_t *p, int bytes)
{
    int i;
    char *alloc;
    
    printf("apr_palloc for %d bytes\n", bytes);
    printf("%-60s", "    apr_palloc");
    alloc = apr_palloc(p, bytes);
    if (!alloc) {
        printf("Failed\n");
        exit(-1);
    }
    printf("OK\n");
    
    printf("%-60s", "    Checking entire allocation is writable");
    for (i=0;i<bytes;i++) {
        char *ptr = alloc + i;
        *ptr = 0xa;
    }
    printf("OK\n");
}

static void calloc_bytes(apr_pool_t *p, int bytes)
{
    int i;
    char *alloc;
    
    printf("apr_pcalloc for %d bytes\n", bytes);
    printf("%-60s", "    apr_pcalloc");
    alloc = apr_pcalloc(p, bytes);
    if (!alloc) {
        printf("Failed\n");
        exit(-1);
    }
    printf("OK\n");
    
    printf("%-60s", "    Checking entire allocation is set to 0");
    for (i=0;i<bytes;i++) {
        char *ptr = alloc + i;
        if (*ptr != 0x0) {
            printf("Error at byte %d (%d vs 0)\n", i, (*ptr));
            exit (-1);
        }
    }
    printf("OK\n");
}


int main (int argc, char ** argv)
{
    apr_pool_t *pmain, *pchild;
    
    apr_initialize();
    atexit(apr_terminate);

    fprintf(stdout, "APR Pools Test\n==============\n\n");
    STD_TEST_NEQ("Creating a top level pool (no parent)", apr_pool_create(&pmain, NULL))

    STD_TEST_NEQ("Create a child pool from the top level one",
                 apr_pool_create(&pchild, pmain))

    printf("\nMain Pool\n");
    alloc_bytes(pmain, 1024);   

    printf("\nChild Pool\n");

    alloc_bytes(pchild, 1024);   
    calloc_bytes(pchild, 1024);   
    alloc_bytes(pchild, 4096);   
       
    printf("\nClearing the child pool\n\n");
    apr_pool_clear(pchild);

    alloc_bytes(pchild, 2048);    
    calloc_bytes(pchild, 1024);   
    alloc_bytes(pchild, 4096);   
    
    printf("\nDestroying the child pool\n");
    apr_pool_destroy(pchild);
    printf("Destroying the main pool\n");
    apr_pool_destroy(pmain);
    
    printf("\nAll Tests completed - OK!\n");

    return (0);
}

