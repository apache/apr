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

#include "apr_shmem.h"
#include "apr_lock.h"
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

typedef struct mbox {
    char msg[1024]; 
    int msgavail; 
} mbox;
apr_pool_t *context;
mbox *boxes;

#define SIZE       256
#define CYCLES     40
#define TESTSIZE   (apr_size_t)(4096 * SIZE)
#define TEST2SIZE  (apr_size_t)(CYCLES * SIZE)

static void msgwait(int boxnum)
{
    volatile int test = 0;
    while (test == 0) {
        apr_sleep(0);
        test = boxes[boxnum].msgavail;
    }    
    fprintf(stdout, "\nreceived a message in box %d, message was: %s\n", 
            boxnum, boxes[boxnum].msg); 
}

static void msgput(int boxnum, char *msg)
{
    fprintf(stdout, "Sending message to box %d\n", boxnum);
    apr_cpystrn(boxes[boxnum].msg, msg, strlen(msg));
    boxes[boxnum].msgavail = 1;
}

int main(void)
{
#if APR_HAS_SHARED_MEMORY
    apr_shmem_t *shm;
    pid_t pid;
    int cntr;
    apr_size_t size;
    char *ptrs[CYCLES];
    apr_size_t psize[CYCLES];
    apr_status_t rv;
    apr_size_t cksize;    
    apr_initialize();
    

    for (size = 0;size < CYCLES;size++){
        ptrs[size] = NULL;
        psize[size] = sizeof(mbox) * (size + 1);
    }
    
    printf("APR Shared Memory Test\n");
    printf("======================\n\n");
    printf("Initializing the context............................"); 
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        printf("could not initialize\n");
        exit(-1);
    }
    printf("OK\n");

    printf("Creating shared memory block (%" APR_SIZE_T_FMT " bytes)........", 
           TESTSIZE); 
    if (apr_shm_init(&shm, TESTSIZE, NULL, context) != APR_SUCCESS) { 
        fprintf(stderr, "Error allocating shared memory block\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    printf("Allocating shared mbox memory......................."); 
    size = sizeof(mbox) * 2;
    boxes = apr_shm_calloc(shm, size);
    if (boxes == NULL) { 
        fprintf(stderr, "Error creating message boxes.\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    printf("\nAbout to stress the alloc/free cycle.\n");
    printf("Smallest allocation will be %" APR_SIZE_T_FMT " bytes\n", 
           psize[0]);
    printf("Largest allocation will be  %" APR_SIZE_T_FMT " bytes\n", 
           psize[CYCLES -1]);
    printf("I will be doing it in %d steps\n", CYCLES);
    
    printf("\tAllocating via apr_shm_malloc...............");
    for (cntr = 0;cntr < CYCLES;cntr++){
        ptrs[cntr] = apr_shm_malloc(shm, psize[cntr]);
        if (ptrs[cntr] == NULL){
            printf("Failed at step %d, %" APR_SIZE_T_FMT " bytes\n", 
                   cntr, psize[cntr]);
            exit (-1);
        }
    }
    printf("OK\n\tFreeing.....................................");
    for (cntr = 0;cntr < CYCLES;cntr++){
        if (apr_shm_free(shm, ptrs[cntr]) != APR_SUCCESS){
            printf("Failed at step %d, %" APR_SIZE_T_FMT " bytes\n", 
                   cntr, psize[cntr]);
            exit (-1);
        }
    }
    printf("OK\n");

    printf("\tAllocating via apr_shm_calloc...............");
    for (cntr = CYCLES-1;cntr > -1;cntr--){
        ptrs[cntr] = apr_shm_malloc(shm, psize[cntr]);
        if (ptrs[cntr] == NULL){
            printf("Failed at %" APR_SIZE_T_FMT" bytes\n", 
                   psize[cntr]);
            exit (-1);
        }
    }
    printf("OK\n\tFreeing.....................................");
    for (cntr = 0;cntr < CYCLES;cntr++){
        if (apr_shm_free(shm, ptrs[cntr]) != APR_SUCCESS){
            printf("Failed at step %d, %" APR_SIZE_T_FMT 
                   " bytes\n", cntr, psize[cntr]);
            exit (-1);
        }
    }
    printf("OK\n");

    printf("Checking we have all we should have remaining.......");
    rv = apr_shm_avail(shm, &cksize);
    if (rv == APR_ENOTIMPL){
        printf("Not Impl.\n");
    } else {
        if (rv != APR_SUCCESS){
            printf("Failed!\n");
            exit (-1);
        }
        if (cksize == (TESTSIZE - size)){
            printf ("OK\n");
        } else {
            printf ("Failed.\nShould have had %" APR_SIZE_T_FMT 
                    " bytes, instead there are %"
                    APR_SIZE_T_FMT " bytes :(\n",
                    TESTSIZE - size, cksize);
            exit(-1);
        }
    }
    printf("%d cycles of malloc and calloc passed.\n\n", CYCLES);

    printf("Block test.\n");
    printf("\tI am about to allocate %" APR_SIZE_T_FMT 
           " bytes..........", TEST2SIZE);
    if ((ptrs[0] = apr_shm_malloc(shm, TEST2SIZE)) == NULL){
        printf("Failed.\n");
        exit (-1);
    }
    printf ("OK\n");
    printf("\tFreeing the block of %" APR_SIZE_T_FMT 
           " bytes............", TEST2SIZE);
    if ((rv = apr_shm_free(shm, ptrs[0])) != APR_SUCCESS){
        printf("Failed!\n");
        exit(-1);
    }
    printf ("OK\n");
    
    printf ("\tAbout to allocate %d blocks of %d bytes....", CYCLES, SIZE);
    for (cntr = 0;cntr < CYCLES;cntr++){
        if ((ptrs[cntr] = apr_shm_malloc(shm, SIZE)) == NULL){
            printf("Failed.\n");
            printf("Couldn't allocate block %d\n", cntr + 1);
            exit (-1);
        }
    }
    printf("Complete.\n");

    printf ("\tAbout to free %d blocks of %d bytes........", CYCLES, SIZE);
    for (cntr = 0;cntr < CYCLES;cntr++){
        if ((rv = apr_shm_free(shm, ptrs[cntr])) != APR_SUCCESS){
            printf("Failed\n");
            printf("Counldn't free block %d\n", cntr + 1);
            exit (-1);
        }
    }
    printf("Complete.\n");

    printf("Checking we have all we should have remaining.......");
    rv = apr_shm_avail(shm, &cksize);
    if (rv == APR_ENOTIMPL){
        printf("Not Impl.\n");
    } else {
        if (rv != APR_SUCCESS){
            printf("Failed!\n");
            exit (-1);
        }
        if (cksize == (TESTSIZE - size)){
            printf ("OK\n");
        } else {
            printf ("Failed.\nShould have had %"
                    APR_SIZE_T_FMT " bytes, instead there are %"
                    APR_SIZE_T_FMT " bytes :(\n",
                    TESTSIZE - size, cksize);
            exit(-1);
        }
    }

    printf("Block test complete.\n\n");
             
    printf("Creating a child process\n");
    pid = fork();
    if (pid == 0) {
        apr_sleep(1);
        if (apr_shm_open(shm) == APR_SUCCESS) {
            msgwait(1);
            msgput(0, "Msg received\n");
        } else {
            puts( "Child: unable to get access to shared memory" );
        }
        exit(1);
    }
    else if (pid > 0) {
        msgput(1, "Sending a message\n");
        apr_sleep(1);
        msgwait(0);
        exit(1);
    }
    else {
        printf("Error creating a child process\n");
        exit(1);
    }
#else
    printf("APR SHMEM test not run!\n");
    printf("shmem is not supported on this platform\n"); 
    return (-1);
#endif
}
