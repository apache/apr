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

#include "apr_memory_system.h"
#include "apr_tracking_memory_system.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_time.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#define LUMPS 10
#define LUMP_SIZE 1024
char *ptrs[LUMPS];

static void do_test(apr_memory_system_t *ams)
{
    int cntr,cntr2;
    
    printf("\tCreating %d lumps of memory, each %d bytes........", 
           LUMPS, LUMP_SIZE);
    for (cntr = 0;cntr < LUMPS;cntr ++){
        ptrs[cntr] = apr_memory_system_malloc(ams, LUMP_SIZE);
        if (!ptrs[cntr]){
            printf("Failed @ lump %d of %d\n", cntr + 1, LUMPS);
            exit (-1);
        }
    }
    printf ("OK\n");

    printf("\tWriting to the lumps of memory......................");
    for (cntr = 0;cntr < LUMPS;cntr ++){
        if (memset(ptrs[cntr], cntr, LUMP_SIZE) != ptrs[cntr]){
            printf("Failed to write into lump %d\n", cntr + 1);
            exit(-1);
        }
    }
    printf("OK\n");    

    printf("\tCheck what we wrote.................................");
    for (cntr = 0;cntr < LUMPS;cntr++){
        for (cntr2 = 0;cntr2 < LUMP_SIZE; cntr2 ++){
            if (*(ptrs[cntr] + cntr2) != cntr){
                printf("Got %d instead of %d at byte %d\n",
                       *(ptrs[cntr] + cntr2), cntr, cntr2 + 1);
                exit (-1);
            }
        }
    } 
    printf("OK\n");   
}

static void do_free(apr_memory_system_t *ams)
{
    int cntr;
    
    printf("\tFreeing the memory we created.......................");
    for (cntr = 0;cntr < LUMPS;cntr ++){
        if (apr_memory_system_free(ams, ptrs[cntr]) != APR_SUCCESS){
            printf("Failed to free block %d\n", cntr + 1);
            exit (-1);
        }
    }
    printf("OK\n");
}

int main(void)
{
    apr_memory_system_t *ams, *ams2;
    apr_initialize();
    
    printf("APR Memory Test\n");
    printf("===============\n\n");

    printf("Standard Memory\n");
    printf("\tCreating the memory area............................");
    if (apr_standard_memory_system_create(&ams) != APR_SUCCESS){
        printf("Failed.\n");
        exit(-1);
    }
    printf("OK\n");

    do_test(ams);
    do_free(ams);
    
    printf("Tracking Memory\n");
    printf("\tCreating the memory area............................");
    if (apr_tracking_memory_system_create(&ams2, ams) != APR_SUCCESS){
        printf("Failed.\n");
        exit(-1);
    }
    printf("OK\n");

    do_test(ams2);
    printf("\tAbout to reset the tracking memory..................\n");
    if (apr_memory_system_reset(ams2) != APR_SUCCESS){
        printf("Failed.\n");
        exit(-1);
    }
    printf("OK\n");
    do_test(ams2);
    do_free(ams2);
    
    printf("Trying to destroy the tracking memory segment...............");
    if (apr_memory_system_destroy(ams2) != APR_SUCCESS){
        printf("Failed.\n");
        exit (-1);
    }
    printf("OK\n");

    printf("Trying to destroy the standard memory segment...............");
    if (apr_memory_system_destroy(ams) != APR_SUCCESS){
        printf("Failed.\n");
        exit (-1);
    }
    printf("OK\n\n");

    printf("Memory test complete.\n");
    return (0);          
}
