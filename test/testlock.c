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

#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_lock.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include "test_apr.h"

#if !APR_HAS_THREADS
int main(void)
{
    printf("This program won't work on this platform because there is no "
           "support for threads.\n");
    return 0;
}
#else /* !APR_HAS_THREADS */

#define MAX_ITER 40000

void * APR_THREAD_FUNC thread_rw_func(void *data);
void * APR_THREAD_FUNC thread_function(void *data);
apr_status_t test_exclusive(void);
apr_status_t test_rw(void);
apr_status_t test_multiple_locking(void);


apr_file_t *in, *out, *err;
apr_lock_t *thread_rw_lock, *thread_lock;
apr_pool_t *pool;
int i = 0, x = 0;

void * APR_THREAD_FUNC thread_rw_func(void *data)
{
    int exitLoop = 1;

    while (1)
    {
        apr_lock_acquire_rw(thread_rw_lock, APR_READER);
        if (i == MAX_ITER)
            exitLoop = 0;
        apr_lock_release(thread_rw_lock);

        if (!exitLoop)
            break;

        apr_lock_acquire_rw(thread_rw_lock, APR_WRITER);
        if (i != MAX_ITER)
        {
            i++;
            x++;
        }
        apr_lock_release(thread_rw_lock);
    }
    return NULL;
} 

void * APR_THREAD_FUNC thread_function(void *data)
{
    int exitLoop = 1;

    /* slight delay to allow things to settle */
    apr_sleep (1);
    
    while (1)
    {
        apr_lock_acquire(thread_lock);
        if (i == MAX_ITER)
            exitLoop = 0;
        else 
        {
            i++;
            x++;
        }
        apr_lock_release(thread_lock);

        if (!exitLoop)
            break;
    }
    return NULL;
} 

int test_rw(void)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    printf("RW Lock Tests\n");
    printf("%-60s", "    Initializing the RW lock");
    s1 = apr_lock_create(&thread_rw_lock, APR_READWRITE, APR_INTRAPROCESS,
                         "lock.file", pool);
    if (s1 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    i = 0;
    x = 0;

    printf("%-60s","    Starting all the threads"); 
    s1 = apr_thread_create(&t1, NULL, thread_rw_func, NULL, pool);
    s2 = apr_thread_create(&t2, NULL, thread_rw_func, NULL, pool);
    s3 = apr_thread_create(&t3, NULL, thread_rw_func, NULL, pool);
    s4 = apr_thread_create(&t4, NULL, thread_rw_func, NULL, pool);
    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS || 
        s3 != APR_SUCCESS || s4 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    printf("%-60s", "    Waiting for threads to exit");
    apr_thread_join(&s1, t1);
    apr_thread_join(&s2, t2);
    apr_thread_join(&s3, t3);
    apr_thread_join(&s4, t4);
    printf("OK\n");

    if (x != MAX_ITER) {
        fprintf(stderr, "RW locks didn't work as expected. x = %d instead of %d\n",
                x, MAX_ITER);
    }
    else {
        printf("Test passed\n");
    }
    
    return APR_SUCCESS;
}

apr_status_t test_exclusive(void)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    printf("Exclusive lock test\n");
    printf("%-60s", "    Initializing the lock");
    s1 = apr_lock_create(&thread_lock, APR_MUTEX, APR_INTRAPROCESS, 
                         "lock.file", pool); 

    if (s1 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    i = 0;
    x = 0;

    printf("%-60s", "    Starting all the threads"); 
    s1 = apr_thread_create(&t1, NULL, thread_function, NULL, pool);
    s2 = apr_thread_create(&t2, NULL, thread_function, NULL, pool);
    s3 = apr_thread_create(&t3, NULL, thread_function, NULL, pool);
    s4 = apr_thread_create(&t4, NULL, thread_function, NULL, pool);
    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS || 
        s3 != APR_SUCCESS || s4 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    printf("%-60s", "    Waiting for threads to exit");
    apr_thread_join(&s1, t1);
    apr_thread_join(&s2, t2);
    apr_thread_join(&s3, t3);
    apr_thread_join(&s4, t4);
    printf("OK\n");

    if (x != MAX_ITER) {
        fprintf(stderr, "Locks don't appear to work!  x = %d instead of %d\n",
                x, MAX_ITER);
    }
    else {
        printf("Test passed\n");
    }
    return APR_SUCCESS;
}

apr_status_t test_multiple_locking(void)
{
    apr_lock_t *multi;
    int try = 0;
    apr_status_t rv;
 
    printf("Testing multiple locking\n");
    printf("%-60s","    Creating the lock we'll use");
    if ((rv = apr_lock_create(&multi, APR_MUTEX, APR_LOCKALL,"multi.lock",
                        pool)) != APR_SUCCESS) {
        printf("Failed!\n");
        return rv;
    }
    printf("OK\n");

    printf("%-60s", "    Trying to lock 10 times");
    for (try = 0; try < 10; try++) {
        if ((rv = apr_lock_acquire(multi)) != APR_SUCCESS) {
            printf("Failed!\n");
            printf("Error at try %d\n", try);
            return rv;
        }
    }
    printf("OK\n");

    printf("%-60s", "    Trying to unlock 10 times");
    for (try = 0; try < 10; try++) {
        if ((rv = apr_lock_release(multi)) != APR_SUCCESS) {
            printf("Failed!\n");
            printf("Error at try %d\n", try);
            return rv;
        }
    }
    printf("OK\n");

    printf("%-60s", "    Destroying the lock we've been using");
    if ((rv = apr_lock_destroy(multi)) != APR_SUCCESS) {
        printf("Failed!\n");
        return rv;
    }
    printf("OK\n");

    printf("Test passed\n");
    return APR_SUCCESS;
}

int main(void)
{
    apr_status_t rv;
    char errmsg[200];

    printf("APR Locks Test\n==============\n\n");
        
    apr_initialize();
    atexit(apr_terminate);

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS)
        exit(-1);

    if ((rv = test_exclusive()) != APR_SUCCESS) {
        fprintf(stderr,"Exclusive Lock test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-2);
    }
    
    if ((rv = test_multiple_locking()) != APR_SUCCESS) {
        fprintf(stderr,"Multiple Locking test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-3);
    }
    
    if ((rv = test_rw()) != APR_SUCCESS) {
        fprintf(stderr,"RW Lock test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-4);
    }

    return 1;
}

#endif /* !APR_HAS_THREADS */
