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
#include "apr_thread_mutex.h"
#include "apr_thread_rwlock.h"
#include "apr_thread_cond.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"
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
#define MAX_COUNTER 100000

void * APR_THREAD_FUNC thread_rw_func(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_rwlock_func(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_function(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_mutex_function(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_cond_producer(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_cond_consumer(apr_thread_t *thd, void *data);
apr_status_t test_exclusive(void);
apr_status_t test_rw(void);
apr_status_t test_multiple_locking(const char *);
apr_status_t test_cond(void);


apr_file_t *in, *out, *err;
apr_lock_t *thread_rw_lock, *thread_lock;
apr_thread_mutex_t *thread_mutex;
apr_thread_rwlock_t *rwlock;
apr_pool_t *pool;
int i = 0, x = 0;

int buff[MAX_COUNTER];
struct {
    apr_thread_mutex_t *mutex;
    int                nput;
    int                nval;
} put;

struct {
    apr_thread_mutex_t *mutex;
    apr_thread_cond_t  *cond;
    int                nready;
} nready;

void * APR_THREAD_FUNC thread_rw_func(apr_thread_t *thd, void *data)
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

void * APR_THREAD_FUNC thread_rwlock_func(apr_thread_t *thd, void *data)
{
    int exitLoop = 1;

    while (1)
    {
        apr_thread_rwlock_rdlock(rwlock);
        if (i == MAX_ITER)
            exitLoop = 0;
        apr_thread_rwlock_unlock(rwlock);

        if (!exitLoop)
            break;

        apr_thread_rwlock_wrlock(rwlock);
        if (i != MAX_ITER)
        {
            i++;
            x++;
        }
        apr_thread_rwlock_unlock(rwlock);
    }
    return NULL;
} 

void * APR_THREAD_FUNC thread_function(apr_thread_t *thd, void *data)
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

void * APR_THREAD_FUNC thread_mutex_function(apr_thread_t *thd, void *data)
{
    int exitLoop = 1;

    /* slight delay to allow things to settle */
    apr_sleep (1);
    
    while (1)
    {
        apr_thread_mutex_lock(thread_mutex);
        if (i == MAX_ITER)
            exitLoop = 0;
        else 
        {
            i++;
            x++;
        }
        apr_thread_mutex_unlock(thread_mutex);

        if (!exitLoop)
            break;
    }
    return NULL;
} 

void * APR_THREAD_FUNC thread_cond_producer(apr_thread_t *thd, void *data)
{
    for (;;) {
        apr_thread_mutex_lock(put.mutex);
        if (put.nput >= MAX_COUNTER) {
            apr_thread_mutex_unlock(put.mutex);
            return NULL;
        }
        buff[put.nput] = put.nval;
        put.nput++;
        put.nval++;
        apr_thread_mutex_unlock(put.mutex);

        apr_thread_mutex_lock(nready.mutex);
        if (nready.nready == 0)
            apr_thread_cond_signal(nready.cond);
        nready.nready++;
        apr_thread_mutex_unlock(nready.mutex);

        *((int *) data) += 1;
    }

    return NULL;
}

void * APR_THREAD_FUNC thread_cond_consumer(apr_thread_t *thd, void *data)
{
    int i;

    for (i = 0; i < MAX_COUNTER; i++) {
        apr_thread_mutex_lock(nready.mutex);
        while (nready.nready == 0)
            apr_thread_cond_wait(nready.cond, nready.mutex);
        nready.nready--;
        apr_thread_mutex_unlock(nready.mutex);

        if (buff[i] != i)
            printf("buff[%d] = %d\n", i, buff[i]);
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

apr_status_t test_multiple_locking(const char *lockfile)
{
    apr_lock_t *multi;
    int try = 0;
    apr_status_t rv;
 
    printf("Testing multiple locking\n");
    printf("%-60s","    Creating the lock we'll use");
    if ((rv = apr_lock_create(&multi, APR_MUTEX, APR_LOCKALL, lockfile,
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

static apr_status_t test_thread_mutex(void)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    printf("thread_mutex test\n");
    printf("%-60s", "    Initializing the mutex");
    s1 = apr_thread_mutex_create(&thread_mutex, pool);

    if (s1 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    i = 0;
    x = 0;

    printf("%-60s", "    Starting all the threads"); 
    s1 = apr_thread_create(&t1, NULL, thread_mutex_function, NULL, pool);
    s2 = apr_thread_create(&t2, NULL, thread_mutex_function, NULL, pool);
    s3 = apr_thread_create(&t3, NULL, thread_mutex_function, NULL, pool);
    s4 = apr_thread_create(&t4, NULL, thread_mutex_function, NULL, pool);
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
        fprintf(stderr, "pthread_mutex don't appear to work!"
                        "  x = %d instead of %d\n",
                x, MAX_ITER);
    }
    else {
        printf("Test passed\n");
    }
    return APR_SUCCESS;
}

static int test_thread_rwlock(void)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    printf("thread_rwlock Tests\n");
    printf("%-60s", "    Initializing the apr_thread_rwlock_t");
    s1 = apr_thread_rwlock_create(&rwlock, pool);
    if (s1 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    i = 0;
    x = 0;

    printf("%-60s","    Starting all the threads"); 
    s1 = apr_thread_create(&t1, NULL, thread_rwlock_func, NULL, pool);
    s2 = apr_thread_create(&t2, NULL, thread_rwlock_func, NULL, pool);
    s3 = apr_thread_create(&t3, NULL, thread_rwlock_func, NULL, pool);
    s4 = apr_thread_create(&t4, NULL, thread_rwlock_func, NULL, pool);
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
        fprintf(stderr, "thread_rwlock didn't work as expected. x = %d instead of %d\n",
                x, MAX_ITER);
    }
    else {
        printf("Test passed\n");
    }
    
    return APR_SUCCESS;
}

apr_status_t test_cond(void)
{
    apr_thread_t *p1, *p2, *p3, *p4, *c1;
    apr_status_t s0, s1, s2, s3, s4;
    int count1, count2, count3, count4;
    int sum;

    printf("thread_cond Tests\n");
    printf("%-60s", "    Initializing the first apr_thread_mutex_t");
    s1 = apr_thread_mutex_create(&put.mutex, pool);
    if (s1 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    printf("%-60s", "    Initializing the second apr_thread_mutex_t");
    s1 = apr_thread_mutex_create(&nready.mutex, pool);
    if (s1 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    printf("%-60s", "    Initializing the apr_thread_cond_t");
    s1 = apr_thread_cond_create(&nready.cond, pool);
    if (s1 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    count1 = count2 = count3 = count4 = 0;
    put.nput = put.nval = 0;
    nready.nready = 0;
    i = 0;
    x = 0;

    printf("%-60s","    Starting all the threads"); 
    s0 = apr_thread_create(&p1, NULL, thread_cond_producer, &count1, pool);
    s1 = apr_thread_create(&p2, NULL, thread_cond_producer, &count2, pool);
    s2 = apr_thread_create(&p3, NULL, thread_cond_producer, &count3, pool);
    s3 = apr_thread_create(&p4, NULL, thread_cond_producer, &count4, pool);
    s4 = apr_thread_create(&c1, NULL, thread_cond_consumer, NULL, pool);
    if (s0 != APR_SUCCESS || s1 != APR_SUCCESS || s2 != APR_SUCCESS || 
        s3 != APR_SUCCESS || s4 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");

    printf("%-60s", "    Waiting for threads to exit");
    apr_thread_join(&s0, p1);
    apr_thread_join(&s1, p2);
    apr_thread_join(&s2, p3);
    apr_thread_join(&s3, p4);
    apr_thread_join(&s4, c1);
    printf("OK\n");

    sum = count1 + count2 + count3 + count4;
    /*
    printf("count1 = %d count2 = %d count3 = %d count4 = %d\n",
            count1, count2, count3, count4);
    */
    if (sum != MAX_COUNTER) {
        fprintf(stderr, "thread_cond didn't work as expected. sum = %d, instead of %d\n", sum, MAX_COUNTER);
    }
    else {
        printf("Test passed\n");
    }
    
    return APR_SUCCESS;
}

int main(int argc, const char * const *argv)
{
    apr_status_t rv;
    char errmsg[200];
    const char *lockname = "multi.lock";
    apr_getopt_t *opt;
    char optchar;
    const char *optarg;

    printf("APR Locks Test\n==============\n\n");
        
    apr_initialize();
    atexit(apr_terminate);

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS)
        exit(-1);

    if ((rv = apr_getopt_init(&opt, pool, argc, argv)) != APR_SUCCESS) {
        fprintf(stderr, "Could not set up to parse options: [%d] %s\n",
                rv, apr_strerror(rv, errmsg, sizeof errmsg));
        exit(-1);
    }
        
    while ((rv = apr_getopt(opt, "f:", &optchar, &optarg)) == APR_SUCCESS) {
        if (optchar == 'f') {
            lockname = optarg;
        }
    }

    if (rv != APR_SUCCESS && rv != APR_EOF) {
        fprintf(stderr, "Could not parse options: [%d] %s\n",
                rv, apr_strerror(rv, errmsg, sizeof errmsg));
        exit(-1);
    }

    if ((rv = test_exclusive()) != APR_SUCCESS) {
        fprintf(stderr,"Exclusive Lock test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-2);
    }
    
    if ((rv = test_multiple_locking(lockname)) != APR_SUCCESS) {
        fprintf(stderr,"Multiple Locking test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-3);
    }
    
    if ((rv = test_rw()) != APR_SUCCESS) {
        fprintf(stderr,"RW Lock test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-4);
    }

    if ((rv = test_thread_mutex()) != APR_SUCCESS) {
        fprintf(stderr,"thread_mutex test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-5);
    }

    if ((rv = test_thread_rwlock()) != APR_SUCCESS) {
        fprintf(stderr,"thread_rwlock test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-6);
    }

    if ((rv = test_cond()) != APR_SUCCESS) {
        fprintf(stderr,"thread_cond test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-7);
    }

    return 0;
}

#endif /* !APR_HAS_THREADS */
