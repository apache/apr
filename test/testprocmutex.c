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
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_proc_mutex.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include "test_apr.h"

#if APR_HAS_FORK

#define MAX_ITER 200
#define CHILDREN 6
#define MAX_COUNTER (MAX_ITER * CHILDREN)

static apr_proc_mutex_t *proc_lock;
static volatile int *x;

/* a slower more racy way to implement (*x)++ */
static int increment(int n)
{
    apr_sleep(1);
    return n+1;
}

static void make_child(CuTest *tc, apr_proc_t **proc, apr_pool_t *p)
{
    apr_status_t rv;

    *proc = apr_pcalloc(p, sizeof(**proc));

    /* slight delay to allow things to settle */
    apr_sleep (1);

    rv = apr_proc_fork(*proc, p);
    if (rv == APR_INCHILD) {
        int i = 0;
        /* The parent process has setup all processes to call apr_terminate
         * at exit.  But, that means that all processes must also call
         * apr_initialize at startup.  You cannot have an unequal number
         * of apr_terminate and apr_initialize calls.  If you do, bad things
         * will happen.  In this case, the bad thing is that if the mutex
         * is a semaphore, it will be destroyed before all of the processes
         * die.  That means that the test will most likely fail.
         */
        apr_initialize();

        if (apr_proc_mutex_child_init(&proc_lock, NULL, p))
            exit(1);

        do {
            if (apr_proc_mutex_lock(proc_lock))
                exit(1);
            i++;
            *x = increment(*x);
            if (apr_proc_mutex_unlock(proc_lock))
                exit(1);
        } while (i < MAX_ITER);
        exit(0);
    } 

    CuAssert(tc, "fork failed", rv == APR_INPARENT);
}

/* Wait for a child process and check it terminated with success. */
static void await_child(CuTest *tc, apr_proc_t *proc)
{
    int code;
    apr_exit_why_e why;
    apr_status_t rv;

    rv = apr_proc_wait(proc, &code, &why, APR_WAIT);
    CuAssert(tc, "child did not terminate with success",
             rv == APR_CHILD_DONE && why == APR_PROC_EXIT && code == 0);
}

static void test_exclusive(CuTest *tc, const char *lockname)
{
    apr_proc_t *child[CHILDREN];
    apr_status_t rv;
    int n;
 
    rv = apr_proc_mutex_create(&proc_lock, lockname, APR_LOCK_DEFAULT, p);
    apr_assert_success(tc, "create the mutex", rv);
 
    for (n = 0; n < CHILDREN; n++)
        make_child(tc, &child[n], p);

    for (n = 0; n < CHILDREN; n++)
        await_child(tc, child[n]);
    
    CuAssert(tc, "Locks don't appear to work", *x == MAX_COUNTER);
}
#endif

static void proc_mutex(CuTest *tc)
{
#if APR_HAS_FORK
    apr_status_t rv;
    const char *shmname = "tpm.shm";
    apr_shm_t *shm;

    /* Use anonymous shm if available. */
    rv = apr_shm_create(&shm, sizeof(int), NULL, p);
    if (rv == APR_ENOTIMPL) {
        apr_file_remove(shmname, p);
        rv = apr_shm_create(&shm, sizeof(int), shmname, p);
    }

    apr_assert_success(tc, "create shm segment", rv);

    x = apr_shm_baseaddr_get(shm);
    test_exclusive(tc, NULL);
#else
    CuNotImpl(tc, "APR lacks fork() support");
#endif
}


CuSuite *testprocmutex(void)
{
    CuSuite *suite = CuSuiteNew("Cross-Process Mutexes");

    SUITE_ADD_TEST(suite, proc_mutex);

    return suite;
}

