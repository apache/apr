/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "apr_shm.h"
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

#if APR_HAS_SHARED_MEMORY

typedef struct mbox {
    char msg[1024]; 
    int msgavail; 
} mbox;
mbox *boxes;

#define N_BOXES 10
#define N_MESSAGES 100
#define SHARED_SIZE (apr_size_t)(N_BOXES * sizeof(mbox))
#define SHARED_FILENAME "/tmp/apr.testshm.shm"

static void msgwait(int sleep_sec, int first_box, int last_box)
{
    int i;
    apr_time_t start = apr_time_now();
    apr_interval_time_t sleep_duration = apr_time_from_sec(sleep_sec);
    while (apr_time_now() - start < sleep_duration) {
        for (i = first_box; i < last_box; i++) {
            if (boxes[i].msgavail) {
                fprintf(stdout, "received a message in box %d, message was: %s\n", 
                        i, boxes[i].msg); 
                boxes[i].msgavail = 0; /* reset back to 0 */
            }
        }
        apr_sleep(apr_time_make(0, 10000)); /* 10ms */
    }
    fprintf(stdout, "done waiting on mailboxes...\n");
}

static void msgput(int boxnum, char *msg)
{
    fprintf(stdout, "Sending message to box %d\n", boxnum);
    apr_cpystrn(boxes[boxnum].msg, msg, strlen(msg));
    boxes[boxnum].msgavail = 1;
}

static apr_status_t test_anon(apr_pool_t *parpool)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_shm_t *shm;
    apr_size_t retsize;
    pid_t pid;
    int cnt, i, exit_int;

    rv = apr_pool_create(&pool, parpool);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Error creating child pool\n");
        return rv;
    }

    printf("Creating anonymous shared memory block (%"
           APR_SIZE_T_FMT " bytes)........", SHARED_SIZE); 
    rv = apr_shm_create(&shm, SHARED_SIZE, NULL, pool);
    if (rv != APR_SUCCESS) { 
        fprintf(stderr, "Error allocating shared memory block\n");
        return rv;
    }
    fprintf(stdout, "OK\n");

    printf("Checking size...%" APR_SIZE_T_FMT " bytes...",
            retsize = apr_shm_size_get(shm));
    if (retsize != SHARED_SIZE) {
        fprintf(stderr, "Error allocating shared memory block\n");
        return rv;
    }
    fprintf(stdout, "OK\n");

    printf("Allocating shared mbox memory for %d boxes ..............",
           N_BOXES); 
    boxes = apr_shm_baseaddr_get(shm);
    if (boxes == NULL) { 
        fprintf(stderr, "Error creating message boxes.\n");
        return rv;
    }
    fprintf(stdout, "OK\n");

    printf("Shared Process Test (child/parent)\n");
    pid = fork();
    if (pid == 0) { /* child */
        msgwait(5, 0, N_BOXES);
        exit(0);
    }
    else if (pid > 0) { /* parent */
        i = N_BOXES;
        cnt = N_MESSAGES;
        while (--cnt > 0) {
            if ((i-=3) < 0) {
                i += N_BOXES; /* start over at the top */
            }
            msgput(i, "Sending a message\n");
            apr_sleep(apr_time_make(0, 10000));
        }
    }
    else {
        printf("Error creating a child process\n");
        return errno;
    }
    /* wait for the child */
    printf("Waiting for child to exit.\n");
    if (waitpid(pid, &exit_int, 0) < 0) {
        return errno;
    }

    printf("Destroying shared memory segment...");
    rv = apr_shm_destroy(shm);
    if (rv != APR_SUCCESS) {
        printf("FAILED\n");
        return rv;
    }
    printf("OK\n");

    apr_pool_destroy(pool);

    return APR_SUCCESS;
}

static apr_status_t test_named(apr_pool_t *parpool)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_shm_t *shm;
    apr_size_t retsize;
    pid_t pidproducer, pidconsumer;
    int exit_int;

    rv = apr_pool_create(&pool, parpool);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Error creating child pool\n");
        return rv;
    }

    printf("Creating named shared memory block (%"
           APR_SIZE_T_FMT " bytes)........", SHARED_SIZE); 
    rv = apr_shm_create(&shm, SHARED_SIZE, SHARED_FILENAME, pool);
    if (rv != APR_SUCCESS) { 
        fprintf(stderr, "Error allocating shared memory block\n");
        return rv;
    }
    fprintf(stdout, "OK\n");

    printf("Checking size...%" APR_SIZE_T_FMT " bytes...",
            retsize = apr_shm_size_get(shm));
    if (retsize != SHARED_SIZE) {
        fprintf(stderr, "Error allocating shared memory block\n");
        return rv;
    }
    fprintf(stdout, "OK\n");

    printf("Allocating shared mbox memory for %d boxes ..............",
           N_BOXES); 
    boxes = apr_shm_baseaddr_get(shm);
    if (boxes == NULL) { 
        fprintf(stderr, "Error creating message boxes.\n");
        return rv;
    }
    fprintf(stdout, "OK\n");

    printf("fork()ing and exec()ing children:\n");
    pidproducer = fork();
    if (pidproducer == 0) { /* child */
        /* FIXME: exec a producer */
        printf("starting consumer.....\n");
        if (execlp("testshmconsumer", "testshmconsumer", (char*)0) < 0) {
            return errno;
        }
    }
    else if (pidproducer > 0) { /* parent */
        /* fork another child */
        pidconsumer = fork();
        if (pidconsumer == 0) { /* child */
            /* FIXME: exec a producer */
            printf("starting producer.....\n");
            if (execlp("testshmproducer", "testshmproducer", (char*)0) < 0) {
                return errno;
            }
        }
        else if (pidconsumer < 0) { /* parent */
            printf("Error creating a child process\n");
            return errno;
        }
    }
    else {
        printf("Error creating a child process\n");
        return errno;
    }
    /* wait for the child */
    printf("Waiting for producer to exit.\n");
    if (waitpid(pidconsumer, &exit_int, 0) < 0) {
        return errno;
    }
    if (!WIFEXITED(exit_int)) {
        printf("Producer was unsuccessful.\n");
        return APR_EGENERAL;
    }
    printf("Waiting for consumer to exit.\n");
    if (waitpid(pidproducer, &exit_int, 0) < 0) {
        return errno;
    }
    if (!WIFEXITED(exit_int)) {
        printf("Consumer was unsuccessful.\n");
        return APR_EGENERAL;
    }

    printf("Destroying shared memory segment...");
    rv = apr_shm_destroy(shm);
    if (rv != APR_SUCCESS) {
        printf("FAILED\n");
        return rv;
    }
    printf("OK\n");

    apr_pool_destroy(pool);

    return APR_SUCCESS;
}

int main(void)
{
    apr_status_t rv;
    apr_pool_t *pool;
    char errmsg[200];

    apr_initialize();
    
    printf("APR Shared Memory Test\n");
    printf("======================\n\n");

    printf("Initializing the pool............................"); 
    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        printf("could not initialize pool\n");
        exit(-1);
    }
    printf("OK\n");

    rv = test_anon(pool);
    if (rv != APR_SUCCESS) {
        if (rv == APR_ENOTIMPL) {
            printf("Anonymous shared memory unavailable on this platform.\n");
        }
        else {
            printf("Anonymous shared memory test FAILED: [%d] %s\n",
                   rv, apr_strerror(rv, errmsg, sizeof(errmsg)));
            exit(-2);
        }
    }
    printf("Anonymous shared memory test passed!\n");

    if ((rv = test_named(pool)) != APR_SUCCESS) {
        printf("Name-based shared memory test FAILED: [%d] %s \n",
               rv, apr_strerror(rv, errmsg, sizeof(errmsg)));
        exit(-3);
    }
    printf("Named shared memory test passed!\n");

    return 0;
}

#else /* APR_HAS_SHARED_MEMORY */
#error shmem is not supported on this platform
#endif /* APR_HAS_SHARED_MEMORY */

