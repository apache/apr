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

#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "errno.h"
#include "apr_time.h"
#include "test_apr.h"

#if APR_HAS_THREADS

static apr_thread_mutex_t *thread_lock;
static apr_thread_once_t *control = NULL;
static int x = 0;
static int value = 0;

static apr_thread_t *t1;
static apr_thread_t *t2;
static apr_thread_t *t3;
static apr_thread_t *t4;

/* just some made up number to check on later */
static apr_status_t exit_ret_val = 123;

static void init_func(void)
{
    value++;
}

static void * APR_THREAD_FUNC thread_func1(apr_thread_t *thd, void *data)
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

static void thread_init(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_thread_once_init(&control, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_thread_mutex_create(&thread_lock, APR_THREAD_MUTEX_DEFAULT, p); 
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void create_threads(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_thread_create(&t1, NULL, thread_func1, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_thread_create(&t2, NULL, thread_func1, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_thread_create(&t3, NULL, thread_func1, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_thread_create(&t4, NULL, thread_func1, NULL, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void join_threads(CuTest *tc)
{
    apr_status_t s;

    apr_thread_join(&s, t1);
    CuAssertIntEquals(tc, exit_ret_val, s);
    apr_thread_join(&s, t2);
    CuAssertIntEquals(tc, exit_ret_val, s);
    apr_thread_join(&s, t3);
    CuAssertIntEquals(tc, exit_ret_val, s);
    apr_thread_join(&s, t4);
    CuAssertIntEquals(tc, exit_ret_val, s);
}

static void check_locks(CuTest *tc)
{
    CuAssertIntEquals(tc, 40000, x);
}

static void check_thread_once(CuTest *tc)
{
    CuAssertIntEquals(tc, 1, value);
}

#else

static void threads_not_impl(CuTest *tc)
{
    CuNotImpl(tc, "Threads not implemented on this platform");
}

#endif

CuSuite *testthread(void)
{
    CuSuite *suite = CuSuiteNew("Threads");

#if !APR_HAS_THREADS
    SUITE_ADD_TEST(suite, threads_not_impl);
#else
    SUITE_ADD_TEST(suite, thread_init);
    SUITE_ADD_TEST(suite, create_threads);
    SUITE_ADD_TEST(suite, join_threads);
    SUITE_ADD_TEST(suite, check_locks);
    SUITE_ADD_TEST(suite, check_thread_once);
#endif

    return suite;
}

