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
#include "apr_errno.h"
#include "apr_general.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include "apr_time.h"
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#if !APR_HAS_THREADS
int main(void)
{
    fprintf(stderr,
            "This program won't work on this platform because there is no "
            "support for threads.\n");
    return 0;
}
#else /* !APR_HAS_THREADS */

void * APR_THREAD_FUNC thread_func1(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func2(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func3(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func4(apr_thread_t *thd, void *data);

apr_thread_mutex_t *thread_lock;
apr_pool_t *context;
apr_thread_once_t *control = NULL;
int x = 0;
int value = 0;
apr_status_t exit_ret_val = 123; /* just some made up number to check on later */

static void init_func(void)
{
    value++;
}

void * APR_THREAD_FUNC thread_func1(apr_thread_t *thd, void *data)
{
    int i;

    apr_thread_once(control, init_func);

    for (i = 0; i < 10000; i++) {
        apr_thread_mutex_lock(thread_lock);
        x++;
        apr_thread_mutex_unlock(thread_lock);
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
} 

int main(void)
{
    apr_thread_t *t1;
    apr_thread_t *t2;
    apr_thread_t *t3;
    apr_thread_t *t4;
    apr_status_t r1, r2, r3, r4;
    apr_status_t s1, s2, s3, s4;
    apr_initialize();

    printf("APR Simple Thread Test\n======================\n\n");
    
    printf("%-60s", "Initializing the context"); 
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "Failed.\nCould not initialize\n");
        exit(-1);
    }
    printf("OK\n");

    apr_thread_once_init(&control, context);

    printf("%-60s", "Initializing the lock"); 
    r1 = apr_thread_mutex_create(&thread_lock, APR_THREAD_MUTEX_DEFAULT,
                                 context); 
    if (r1 != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "Failed\nCould not create lock\n");
        exit(-1);
    }
    printf("OK\n");

    printf("%-60s", "Starting all the threads"); 
    r1 = apr_thread_create(&t1, NULL, thread_func1, NULL, context);
    r2 = apr_thread_create(&t2, NULL, thread_func1, NULL, context);
    r3 = apr_thread_create(&t3, NULL, thread_func1, NULL, context);
    r4 = apr_thread_create(&t4, NULL, thread_func1, NULL, context);
    if (r1 != APR_SUCCESS || r2 != APR_SUCCESS || 
        r3 != APR_SUCCESS || r4 != APR_SUCCESS) {
        fflush(stdout);
        fprintf(stderr, "Failed\nError starting thread\n");
        exit(-1);
    }
    printf("OK\n");

    printf("%-60s", "Waiting for threads to exit");
    fflush(stdout);
    apr_thread_join(&s1, t1);
    apr_thread_join(&s2, t2);
    apr_thread_join(&s3, t3);
    apr_thread_join(&s4, t4);
    printf("OK\n");

    printf("%-60s", "Checking thread's returned value");
    if (s1 != exit_ret_val || s2 != exit_ret_val ||
        s3 != exit_ret_val || s4 != exit_ret_val) {
        fflush(stdout);
        fprintf(stderr, 
                "Invalid return value\nGot %d/%d/%d/%d, but expected %d for all 4\n",
                s1, s2, s3, s4, exit_ret_val);
        exit(-1);
    }
    printf("OK\n");

    printf("%-60s", "Checking if locks worked"); 
    if (x != 40000) {
        fflush(stdout);
        fprintf(stderr, "No!\nThe locks didn't work????  x = %d instead of 40,000\n", x);
        exit(-1);
    }
    printf("OK\n");

    printf("%-60s", "Checking if apr_thread_once worked");
    if (value != 1) {
        fflush(stdout);
        fprintf(stderr, "Failed!\napr_thread_once must not have worked, "
                "value is %d instead of 1\n", value);
        exit(-1);
    }
    printf("OK\n");

    apr_terminate();

    return 0;
}

#endif /* !APR_HAS_THREADS */
