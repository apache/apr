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

#include "apr_thread_proc.h"
#include "apr_lock.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_atomic.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include "apr_time.h"
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#if !(defined WIN32) && !(defined NETWARE)
#include <pthread.h>
#endif

apr_pool_t *context;
apr_atomic_t y;      /* atomic locks */

static apr_status_t check_basic_atomics(volatile apr_atomic_t*p)
{
    apr_uint32_t oldval;
    apr_uint32_t casval=0;
    apr_atomic_set(&y,0);
    printf("%-60s", "testing CAS");
    oldval = apr_atomic_cas(&casval,12,0);
    if (oldval != 0) {
        fprintf(stderr, "Failed\noldval =%d should be zero\n",oldval);
        return APR_EGENERAL;
    }
    printf("OK\n");
    printf("%-60s", "testing CAS - match non-null");
    oldval = apr_atomic_cas(&casval,23,12);
    if (oldval != 12) {
        fprintf(stderr, "Failed\noldval =%d should be 12 y=%d\n",
                oldval,
                casval);
        return APR_EGENERAL;
    }
    printf("OK\n");
    printf("%-60s", "testing CAS - no match");
    oldval = apr_atomic_cas(&casval,23,12);
    if (oldval != 23 ) {
        fprintf(stderr, "Failed\noldval =%d should be 23 y=%d\n",
                oldval, 
                casval);
        return APR_EGENERAL;
    }
    printf("OK\n");

    printf("%-60s", "testing add");
    apr_atomic_set(&y,23);
    apr_atomic_add(&y,4);
    if (apr_atomic_read(&y) != 27) {
        fprintf(stderr, "Failed\nAtomic Add doesn't add up ;( expected 27 got %d\n",
            oldval);
        exit(-1);
    }
 
    printf("OK\n");
    printf("%-60s", "testing add/inc");
    apr_atomic_set(&y,0);
    apr_atomic_add(&y,20);
    apr_atomic_inc(&y);
    if (apr_atomic_read(&y) != 21) {
        fprintf(stderr, "Failed.\natomics do not add up\n");
        return APR_EGENERAL;
    }
    fprintf(stdout, "OK\n");

    return APR_SUCCESS;
}

#if !APR_HAS_THREADS
int main(void)
{
    apr_status_t rv;

    fprintf(stderr,
            "This program won't work fully on this platform because there is no "
            "support for threads.\n");
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "Failed.\nCould not initialize\n");
        exit(-1);
    }
    rv = apr_atomic_init(context);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Failed.\nCould not initialize atomics\n");
        exit(-1);
    }
    rv = check_basic_atomics(&y);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Failed.\n");
        exit(-1);
    }
    return 0;
}
#else /* !APR_HAS_THREADS */

void * APR_THREAD_FUNC thread_func_mutex(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func_atomic(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func_none(apr_thread_t *thd, void *data);

apr_lock_t *thread_lock;
apr_thread_once_t *control = NULL;
volatile long x = 0; /* mutex locks */
volatile long z = 0; /* no locks */
int value = 0;
apr_status_t exit_ret_val = 123; /* just some made up number to check on later */

#define NUM_THREADS 50
#define NUM_ITERATIONS 20000
static void init_func(void)
{
    value++;
}

void * APR_THREAD_FUNC thread_func_mutex(apr_thread_t *thd, void *data)
{
    int i;

    apr_thread_once(control, init_func);

    for (i = 0; i < NUM_ITERATIONS; i++) {
        apr_lock_acquire(thread_lock);
        x++;
        apr_lock_release(thread_lock);
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
} 

void * APR_THREAD_FUNC thread_func_atomic(apr_thread_t *thd, void *data)
{
    int i;

    apr_thread_once(control, init_func);

    for (i = 0; i < NUM_ITERATIONS ; i++) {
        apr_atomic_inc( &y );
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
}
void * APR_THREAD_FUNC thread_func_none(apr_thread_t *thd, void *data)
{
    int i;

    apr_thread_once(control, init_func);

    for (i = 0; i < NUM_ITERATIONS ; i++) {
        z++;
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
}

int main(int argc, char**argv)
{
    apr_thread_t *t1[NUM_THREADS];
    apr_thread_t *t2[NUM_THREADS];
    apr_status_t r1[NUM_THREADS]; 
    apr_status_t r2[NUM_THREADS]; 
    apr_status_t s1[NUM_THREADS]; 
    apr_status_t s2[NUM_THREADS];
    apr_status_t rv;
    int i;
    int mutex=0;

    apr_initialize();

    if (argc==2 && argv[1][0]=='m') 
        mutex=1;

    printf("APR Simple Thread Test\n======================\n\n");
    
#if !(defined WIN32) && !(defined NETWARE)
    pthread_setconcurrency(8);
#endif
    printf("%-60s", "Initializing the context"); 
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "Failed.\nCould not initialize\n");
        exit(-1);
    }
    printf("OK\n");

    apr_thread_once_init(&control, context);

    if (mutex==1) {
        printf("%-60s", "Initializing the lock"); 
        rv = apr_lock_create(&thread_lock, APR_MUTEX, APR_INTRAPROCESS, 
                             APR_LOCK_DEFAULT, "lock.file", context); 
        if (rv != APR_SUCCESS) {
            fflush(stdout);
            fprintf(stderr, "Failed\nCould not create lock\n");
            exit(-1);
        }
        printf("OK\n");
    }
    rv = apr_atomic_init( context);

    rv = check_basic_atomics(&y);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "Failed.\n");
        exit(-1);
    }
    apr_atomic_set(&y,0);

    printf("%-60s", "Starting all the threads"); 
    for (i=0;i<NUM_THREADS;i++) {
        if (mutex ==1) 
            r1[i] = apr_thread_create(&t1[i], NULL, thread_func_mutex, NULL, context);
        else
            r1[i] = apr_thread_create(&t1[i], NULL, thread_func_atomic, NULL, context);
        r2[i] = apr_thread_create(&t2[i], NULL, thread_func_none, NULL, context);
        if (r1[i] != APR_SUCCESS || r2[i] != APR_SUCCESS ) {
            fflush(stdout);
            fprintf(stderr, "Failed\nError starting thread in group %d\n",i);
            exit(-1);
        }
    }
    printf("OK\n");

    printf("%-60s", "Waiting for threads to exit");
    fflush(stdout);
    for (i=0;i<NUM_THREADS;i++) {
        apr_thread_join(&s1[i], t1[i]);
        apr_thread_join(&s2[i], t2[i]);
        if (s1[i] != exit_ret_val || s2[i] != exit_ret_val ) {
                fprintf(stderr, 
                        "Invalid return value\nGot %d/%d, but expected %d for all \n",
                        s1[i], s2[i], exit_ret_val);
        }
    }
    printf("OK\n");

    if (mutex ==1) {
        printf("%-60s", "Checking if mutex locks worked"); 
        if (x != NUM_THREADS*NUM_ITERATIONS) {
            fflush(stdout);
            fprintf(stderr, 
                    "No!\nThe locks didn't work?? x = %ld instead of %ld\n",
                    x,
                    (long)NUM_THREADS*NUM_ITERATIONS);
        }
        else
            printf("OK\n");
    }
    else {
        printf("%-60s", "Checking if atomic worked"); 
        if (apr_atomic_read(&y) != NUM_THREADS*NUM_ITERATIONS) {
            fflush(stdout);
            fprintf(stderr, 
                    "No!\nThe atomics didn't work?? y = %ld instead of %ld\n",
                    (long)apr_atomic_read(&y),
                    (long)NUM_THREADS*NUM_ITERATIONS);
        }
        else
            printf("OK\n");
    }
    printf("%-60s", "Checking if nolock worked"); 
    if ( z != NUM_THREADS*NUM_ITERATIONS) {
        fflush(stdout);
        fprintf(stderr, 
                "no suprise\nThe no-locks didn't work. z = %ld instead of %ld\n", 
                z, 
                (long)NUM_THREADS*NUM_ITERATIONS);
    }
    else
        printf("OK\n");

    printf("%-60s", "Checking if apr_thread_once worked");
    if (value != 1) {
        fflush(stdout);
        fprintf(stderr, "Failed!\napr_thread_once must not have worked, "
                "value is %d instead of 1\n", value);
        exit(-1);
    }
    printf("OK\n");

    return 0;
}

#endif /* !APR_HAS_THREADS */
