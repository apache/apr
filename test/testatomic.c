/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "test_apr.h"
#include "apr_strings.h"
#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_atomic.h"
#include "apr_time.h"

/* Use pthread_setconcurrency where it is available and not a nullop,
 * i.e. platforms using M:N or M:1 thread models: */
#if APR_HAS_THREADS && \
   ((defined(SOLARIS2) && SOLARIS2 > 26) || defined(_AIX))
/* also HP-UX, IRIX? ... */
#define HAVE_PTHREAD_SETCONCURRENCY
#endif

#ifdef HAVE_PTHREAD_SETCONCURRENCY
#include <pthread.h>
#endif

static void test_init(CuTest *tc)
{
    apr_assert_success(tc, "Could not initliaze atomics", apr_atomic_init(p));
}

static void test_set32(CuTest *tc)
{
    apr_uint32_t y32;
    apr_atomic_set32(&y32, 2);
    CuAssertIntEquals(tc, 2, y32);
}

static void test_read32(CuTest *tc)
{
    apr_uint32_t y32;
    apr_atomic_set32(&y32, 2);
    CuAssertIntEquals(tc, 2, apr_atomic_read32(&y32));
}

static void test_dec32(CuTest *tc)
{
    apr_uint32_t y32;
    int rv;

    apr_atomic_set32(&y32, 2);

    rv = apr_atomic_dec32(&y32);
    CuAssertIntEquals(tc, 1, y32);
    CuAssert(tc, "atomic_dec returned zero when it shouldn't", rv != 0);

    rv = apr_atomic_dec32(&y32);
    CuAssertIntEquals(tc, 0, y32);
    CuAssert(tc, "atomic_dec didn't returned zero when it should", rv == 0);
}

static void test_xchg32(CuTest *tc)
{
    apr_uint32_t oldval;
    apr_uint32_t y32;

    apr_atomic_set32(&y32, 100);
    oldval = apr_atomic_xchg32(&y32, 50);

    CuAssertIntEquals(tc, 100, oldval);
    CuAssertIntEquals(tc, 50, y32);
}

static void test_cas_equal(CuTest *tc)
{
    apr_uint32_t casval = 0;
    apr_uint32_t oldval;

    oldval = apr_atomic_cas32(&casval, 12, 0);
    CuAssertIntEquals(tc, 0, oldval);
    CuAssertIntEquals(tc, 12, casval);
}

static void test_cas_equal_nonnull(CuTest *tc)
{
    apr_uint32_t casval = 12;
    apr_uint32_t oldval;

    oldval = apr_atomic_cas32(&casval, 23, 12);
    CuAssertIntEquals(tc, 12, oldval);
    CuAssertIntEquals(tc, 23, casval);
}

static void test_cas_notequal(CuTest *tc)
{
    apr_uint32_t casval = 12;
    apr_uint32_t oldval;

    oldval = apr_atomic_cas32(&casval, 23, 2);
    CuAssertIntEquals(tc, 12, oldval);
    CuAssertIntEquals(tc, 12, casval);
}

static void test_add32(CuTest *tc)
{
    apr_uint32_t oldval;
    apr_uint32_t y32;

    apr_atomic_set32(&y32, 23);
    oldval = apr_atomic_add32(&y32, 4);
    CuAssertIntEquals(tc, 23, oldval);
    CuAssertIntEquals(tc, 27, y32);
}

static void test_inc32(CuTest *tc)
{
    apr_uint32_t oldval;
    apr_uint32_t y32;

    apr_atomic_set32(&y32, 23);
    oldval = apr_atomic_inc32(&y32);
    CuAssertIntEquals(tc, 23, oldval);
    CuAssertIntEquals(tc, 24, y32);
}

static void test_set_add_inc_sub(CuTest *tc)
{
    apr_uint32_t y32;

    apr_atomic_set32(&y32, 0);
    apr_atomic_add32(&y32, 20);
    apr_atomic_inc32(&y32);
    apr_atomic_sub32(&y32, 10);

    CuAssertIntEquals(tc, 11, y32);
}

static void test_wrap_zero(CuTest *tc)
{
    apr_uint32_t y32;
    apr_uint32_t rv;
    apr_uint32_t minus1 = -1;
    char *str;

    apr_atomic_set32(&y32, 0);
    rv = apr_atomic_dec32(&y32);

    CuAssert(tc, "apr_atomic_dec32 on zero returned zero.", rv != 0);
    str = apr_psprintf(p, "zero wrap failed: 0 - 1 = %d", y32);
    CuAssert(tc, str, y32 == minus1);
}

static void test_inc_neg1(CuTest *tc)
{
    apr_uint32_t y32 = -1;
    apr_uint32_t minus1 = -1;
    apr_uint32_t rv;
    char *str;

    rv = apr_atomic_inc32(&y32);

    CuAssert(tc, "apr_atomic_dec32 on zero returned zero.", rv == minus1);
    str = apr_psprintf(p, "zero wrap failed: -1 + 1 = %d", y32);
    CuAssert(tc, str, y32 == 0);
}


#if APR_HAS_THREADS

void * APR_THREAD_FUNC thread_func_mutex(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func_atomic(apr_thread_t *thd, void *data);
void * APR_THREAD_FUNC thread_func_none(apr_thread_t *thd, void *data);

apr_thread_mutex_t *thread_lock;
volatile apr_uint32_t x = 0; /* mutex locks */
volatile apr_uint32_t y = 0; /* atomic operations */
volatile apr_uint32_t z = 0; /* no locks */
apr_status_t exit_ret_val = 123; /* just some made up number to check on later */

#define NUM_THREADS 40
#define NUM_ITERATIONS 20000
void * APR_THREAD_FUNC thread_func_mutex(apr_thread_t *thd, void *data)
{
    int i;

    for (i = 0; i < NUM_ITERATIONS; i++) {
        apr_thread_mutex_lock(thread_lock);
        x++;
        apr_thread_mutex_unlock(thread_lock);
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
} 

void * APR_THREAD_FUNC thread_func_atomic(apr_thread_t *thd, void *data)
{
    int i;

    for (i = 0; i < NUM_ITERATIONS ; i++) {
        apr_atomic_inc32(&y);
        apr_atomic_add32(&y, 2);
        apr_atomic_dec32(&y);
        apr_atomic_dec32(&y);
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
}

void * APR_THREAD_FUNC thread_func_none(apr_thread_t *thd, void *data)
{
    int i;

    for (i = 0; i < NUM_ITERATIONS ; i++) {
        z++;
    }
    apr_thread_exit(thd, exit_ret_val);
    return NULL;
}

static void test_atomics_threaded(CuTest *tc)
{
    apr_thread_t *t1[NUM_THREADS];
    apr_thread_t *t2[NUM_THREADS];
    apr_thread_t *t3[NUM_THREADS];
    apr_status_t s1[NUM_THREADS]; 
    apr_status_t s2[NUM_THREADS];
    apr_status_t s3[NUM_THREADS];
    apr_status_t rv;
    int i;

#ifdef HAVE_PTHREAD_SETCONCURRENCY
    pthread_setconcurrency(8);
#endif

    rv = apr_thread_mutex_create(&thread_lock, APR_THREAD_MUTEX_DEFAULT, p);
    apr_assert_success(tc, "Could not create lock", rv);

    for (i = 0; i < NUM_THREADS; i++) {
        apr_status_t r1, r2, r3;
        r1 = apr_thread_create(&t1[i], NULL, thread_func_mutex, NULL, p);
        r2 = apr_thread_create(&t2[i], NULL, thread_func_atomic, NULL, p);
        r3 = apr_thread_create(&t3[i], NULL, thread_func_none, NULL, p);
        CuAssert(tc, "Failed creating threads",
                 r1 == APR_SUCCESS && r2 == APR_SUCCESS && 
                 r3 == APR_SUCCESS);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        apr_thread_join(&s1[i], t1[i]);
        apr_thread_join(&s2[i], t2[i]);
        apr_thread_join(&s3[i], t3[i]);
                     
        CuAssert(tc, "Invalid return value from thread_join",
                 s1[i] == exit_ret_val && s2[i] == exit_ret_val && 
                 s3[i] == exit_ret_val);
    }

    CuAssertIntEquals(tc, x, NUM_THREADS * NUM_ITERATIONS);
    CuAssertIntEquals(tc, apr_atomic_read32(&y), NUM_THREADS * NUM_ITERATIONS);
    /* Comment out this test, because I have no clue what this test is
     * actually telling us.  We are checking something that may or may not
     * be true, and it isn't really testing APR at all.
    CuAssert(tc, "We expect this to fail, because we tried to update "
                 "an integer in a non-thread-safe manner.",
             z != NUM_THREADS * NUM_ITERATIONS);
     */
}

#endif /* !APR_HAS_THREADS */

CuSuite *testatomic(void)
{
    CuSuite *suite = CuSuiteNew("Atomic");

    SUITE_ADD_TEST(suite, test_init);
    SUITE_ADD_TEST(suite, test_set32);
    SUITE_ADD_TEST(suite, test_read32);
    SUITE_ADD_TEST(suite, test_dec32);
    SUITE_ADD_TEST(suite, test_xchg32);
    SUITE_ADD_TEST(suite, test_cas_equal);
    SUITE_ADD_TEST(suite, test_cas_equal_nonnull);
    SUITE_ADD_TEST(suite, test_cas_notequal);
    SUITE_ADD_TEST(suite, test_add32);
    SUITE_ADD_TEST(suite, test_inc32);
    SUITE_ADD_TEST(suite, test_set_add_inc_sub);
    SUITE_ADD_TEST(suite, test_wrap_zero);
    SUITE_ADD_TEST(suite, test_inc_neg1);

#if APR_HAS_THREADS
    SUITE_ADD_TEST(suite, test_atomics_threaded);
#endif

    return suite;
}

