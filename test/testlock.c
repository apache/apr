/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_thread_mutex.h"
#include "apr_thread_rwlock.h"
#include "apr_thread_cond.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "apr_atomic.h"
#include "testutil.h"

#if APR_HAS_THREADS

#define MAX_ITER 40000
#define MAX_COUNTER 100000
#define MAX_RETRY 5

static void *APR_THREAD_FUNC thread_rwlock_func(apr_thread_t *thd, void *data);
static void *APR_THREAD_FUNC thread_mutex_function(apr_thread_t *thd, void *data);
static void *APR_THREAD_FUNC thread_mutex_sleep_function(apr_thread_t *thd, void *data);
static void *APR_THREAD_FUNC thread_cond_producer(apr_thread_t *thd, void *data);
static void *APR_THREAD_FUNC thread_cond_consumer(apr_thread_t *thd, void *data);

static apr_thread_mutex_t *thread_mutex;
static apr_thread_rwlock_t *rwlock;
static int i = 0, x = 0;

static int buff[MAX_COUNTER];

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

static apr_thread_mutex_t *timeout_mutex;
static apr_thread_cond_t *timeout_cond;

static void *APR_THREAD_FUNC thread_rwlock_func(apr_thread_t *thd, void *data)
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

static void *APR_THREAD_FUNC thread_mutex_function(apr_thread_t *thd, void *data)
{
    int exitLoop = 1;

    /* slight delay to allow things to settle */
    apr_sleep (1);

    while (1) {
        apr_status_t rv;

        if (data) {
            rv = apr_thread_mutex_timedlock(thread_mutex, *(apr_interval_time_t *)data);
        }
        else {
            rv = apr_thread_mutex_lock(thread_mutex);
        }

        if (rv != APR_SUCCESS) {
            fprintf(stderr, "thread_mutex_function: failed locking mutex\n");
            apr_thread_exit(thd, rv);
            break;
        }

        if (i == MAX_ITER)
            exitLoop = 0;
        else
        {
            i++;
            x++;
        }

        rv = apr_thread_mutex_unlock(thread_mutex);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "thread_mutex_function: failed unlocking mutex\n");
            apr_thread_exit(thd, rv);
            break;
        }

        if (!exitLoop)
            break;
    }

    apr_thread_exit(thd, APR_SUCCESS);
    return NULL;
}

/* Sleepy-loop until f_ value matches val: */
#define wait_for_flag(f_, val) while (apr_atomic_read32(&(f_)) != val) apr_sleep(100000)

/* Helper function.  Passed (apr_uint32_t *) flag as data, sets flag
 * to one, locks the timeout_mutex, waits for *flag to be set to zero
 * and terminates.  The co-ordination could also be done via mutexes
 * but since we're timedlocking timeout_mutex it would look like a
 * deadlock to a mutex implementation which detects deadlocks. */
static void *APR_THREAD_FUNC thread_mutex_sleep_function(apr_thread_t *thd, void *data)
{
    apr_uint32_t *flag = data;
    apr_status_t rv;

    rv = apr_thread_mutex_lock(timeout_mutex);
    if (rv) {
        fprintf(stderr, "testlock: failed to lock timeout mutex, errno %d\n", rv);
        apr_thread_exit(thd, rv);
    }

    apr_atomic_set32(flag, 1);

    wait_for_flag(*flag, 0);

    rv = apr_thread_mutex_unlock(timeout_mutex);

    apr_thread_exit(thd, APR_SUCCESS);
    return NULL;
}

static void *APR_THREAD_FUNC thread_cond_producer(apr_thread_t *thd, void *data)
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

static void *APR_THREAD_FUNC thread_cond_consumer(apr_thread_t *thd, void *data)
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

#define JOIN_WITH_SUCCESS(tc_, thd_) do { apr_status_t rv__thd;         \
        APR_ASSERT_SUCCESS(tc_, "join thread", apr_thread_join(&rv__thd, thd_)); \
        APR_ASSERT_SUCCESS(tc_, "spawned thread terminated successfully", rv__thd); \
    } while (0)

static void test_thread_mutex(abts_case *tc, void *data)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    s1 = apr_thread_mutex_create(&thread_mutex, APR_THREAD_MUTEX_DEFAULT, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s1);
    ABTS_PTR_NOTNULL(tc, thread_mutex);

    i = 0;
    x = 0;

    s1 = apr_thread_create(&t1, NULL, thread_mutex_function, NULL, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s1);
    s2 = apr_thread_create(&t2, NULL, thread_mutex_function, NULL, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s2);
    s3 = apr_thread_create(&t3, NULL, thread_mutex_function, NULL, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s3);
    s4 = apr_thread_create(&t4, NULL, thread_mutex_function, NULL, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s4);

    JOIN_WITH_SUCCESS(tc, t1);
    JOIN_WITH_SUCCESS(tc, t2);
    JOIN_WITH_SUCCESS(tc, t3);
    JOIN_WITH_SUCCESS(tc, t4);

    ABTS_INT_EQUAL(tc, MAX_ITER, x);
}

static void test_thread_timedmutex(abts_case *tc, void *data)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;
    apr_interval_time_t timeout;

    s1 = apr_thread_mutex_create(&thread_mutex, APR_THREAD_MUTEX_TIMED, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s1);
    ABTS_PTR_NOTNULL(tc, thread_mutex);

    i = 0;
    x = 0;

    timeout = apr_time_from_sec(5);

    s1 = apr_thread_create(&t1, NULL, thread_mutex_function, &timeout, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s1);
    s2 = apr_thread_create(&t2, NULL, thread_mutex_function, &timeout, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s2);
    s3 = apr_thread_create(&t3, NULL, thread_mutex_function, &timeout, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s3);
    s4 = apr_thread_create(&t4, NULL, thread_mutex_function, &timeout, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s4);

    JOIN_WITH_SUCCESS(tc, t1);
    JOIN_WITH_SUCCESS(tc, t2);
    JOIN_WITH_SUCCESS(tc, t3);
    JOIN_WITH_SUCCESS(tc, t4);

    ABTS_INT_EQUAL(tc, MAX_ITER, x);
}

static void test_thread_rwlock(abts_case *tc, void *data)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    s1 = apr_thread_rwlock_create(&rwlock, p);
    if (s1 == APR_ENOTIMPL) {
        ABTS_NOT_IMPL(tc, "rwlocks not implemented");
        return;
    }
    APR_ASSERT_SUCCESS(tc, "rwlock_create", s1);
    ABTS_PTR_NOTNULL(tc, rwlock);

    i = 0;
    x = 0;

    s1 = apr_thread_create(&t1, NULL, thread_rwlock_func, NULL, p);
    APR_ASSERT_SUCCESS(tc, "create thread 1", s1);
    s2 = apr_thread_create(&t2, NULL, thread_rwlock_func, NULL, p);
    APR_ASSERT_SUCCESS(tc, "create thread 2", s2);
    s3 = apr_thread_create(&t3, NULL, thread_rwlock_func, NULL, p);
    APR_ASSERT_SUCCESS(tc, "create thread 3", s3);
    s4 = apr_thread_create(&t4, NULL, thread_rwlock_func, NULL, p);
    APR_ASSERT_SUCCESS(tc, "create thread 4", s4);

    apr_thread_join(&s1, t1);
    apr_thread_join(&s2, t2);
    apr_thread_join(&s3, t3);
    apr_thread_join(&s4, t4);

    ABTS_INT_EQUAL(tc, MAX_ITER, x);

    apr_thread_rwlock_destroy(rwlock);
}

static void test_cond(abts_case *tc, void *data)
{
    apr_thread_t *p1, *p2, *p3, *p4, *c1;
    apr_status_t s0, s1, s2, s3, s4;
    int count1, count2, count3, count4;
    int sum;

    APR_ASSERT_SUCCESS(tc, "create put mutex",
                       apr_thread_mutex_create(&put.mutex,
                                               APR_THREAD_MUTEX_DEFAULT, p));
    ABTS_PTR_NOTNULL(tc, put.mutex);

    APR_ASSERT_SUCCESS(tc, "create nready mutex",
                       apr_thread_mutex_create(&nready.mutex,
                                               APR_THREAD_MUTEX_DEFAULT, p));
    ABTS_PTR_NOTNULL(tc, nready.mutex);

    APR_ASSERT_SUCCESS(tc, "create condvar",
                       apr_thread_cond_create(&nready.cond, p));
    ABTS_PTR_NOTNULL(tc, nready.cond);

    count1 = count2 = count3 = count4 = 0;
    put.nput = put.nval = 0;
    nready.nready = 0;
    i = 0;
    x = 0;

    s0 = apr_thread_create(&p1, NULL, thread_cond_producer, &count1, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s0);
    s1 = apr_thread_create(&p2, NULL, thread_cond_producer, &count2, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s1);
    s2 = apr_thread_create(&p3, NULL, thread_cond_producer, &count3, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s2);
    s3 = apr_thread_create(&p4, NULL, thread_cond_producer, &count4, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s3);
    s4 = apr_thread_create(&c1, NULL, thread_cond_consumer, NULL, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s4);

    apr_thread_join(&s0, p1);
    apr_thread_join(&s1, p2);
    apr_thread_join(&s2, p3);
    apr_thread_join(&s3, p4);
    apr_thread_join(&s4, c1);

    APR_ASSERT_SUCCESS(tc, "destroy condvar",
                       apr_thread_cond_destroy(nready.cond));

    sum = count1 + count2 + count3 + count4;
    /*
    printf("count1 = %d count2 = %d count3 = %d count4 = %d\n",
            count1, count2, count3, count4);
    */
    ABTS_INT_EQUAL(tc, MAX_COUNTER, sum);
}

static void test_timeoutcond(abts_case *tc, void *data)
{
    apr_status_t s;
    apr_interval_time_t timeout;
    apr_time_t begin, end;
    int i;

    s = apr_thread_mutex_create(&timeout_mutex, APR_THREAD_MUTEX_DEFAULT, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s);
    ABTS_PTR_NOTNULL(tc, timeout_mutex);

    s = apr_thread_cond_create(&timeout_cond, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s);
    ABTS_PTR_NOTNULL(tc, timeout_cond);

    timeout = apr_time_from_sec(5);

    for (i = 0; i < MAX_RETRY; i++) {
        apr_thread_mutex_lock(timeout_mutex);

        begin = apr_time_now();
        s = apr_thread_cond_timedwait(timeout_cond, timeout_mutex, timeout);
        end = apr_time_now();
        apr_thread_mutex_unlock(timeout_mutex);

        if (s != APR_SUCCESS && !APR_STATUS_IS_TIMEUP(s)) {
            continue;
        }
        ABTS_INT_EQUAL(tc, 1, APR_STATUS_IS_TIMEUP(s));
        ABTS_ASSERT(tc, "Timer returned too late", end - begin - timeout < 500000);
        break;
    }
    ABTS_ASSERT(tc, "Too many retries", i < MAX_RETRY);
    APR_ASSERT_SUCCESS(tc, "Unable to destroy the conditional",
                       apr_thread_cond_destroy(timeout_cond));
}

/* Test whether _timedlock times out appropriately.  Since
 * double-locking a non-recursive mutex has undefined behaviour, and
 * double-locking a recursive mutex succeeds immediately, a thread is
 * spawned to hold the lock while this thread tests whether _timedlock
 * times out. */
static void test_timeoutmutex(abts_case *tc, void *data)
{
    apr_status_t s;
    apr_interval_time_t timeout;
    apr_time_t begin, end;
    apr_thread_t *th;
    apr_uint32_t flag = 0;
    int i;

    s = apr_thread_mutex_create(&timeout_mutex,
                                APR_THREAD_MUTEX_TIMED |
                                APR_THREAD_MUTEX_UNNESTED, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s);
    ABTS_PTR_NOTNULL(tc, timeout_mutex);

    s = apr_thread_create(&th, NULL, thread_mutex_sleep_function, &flag, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, s);

    wait_for_flag(flag, 1); /* the thread will set flag to 1 once the
                             * timeout_mutex is locked. */

    timeout = apr_time_from_sec(5);

    for (i = 0; i < MAX_RETRY; i++) {
        begin = apr_time_now();
        s = apr_thread_mutex_timedlock(timeout_mutex, timeout);
        end = apr_time_now();

        if (s != APR_SUCCESS && !APR_STATUS_IS_TIMEUP(s)) {
            continue;
        }
        ABTS_INT_EQUAL(tc, 1, APR_STATUS_IS_TIMEUP(s));
        ABTS_ASSERT(tc, "Timer returned too late", end - begin - timeout < 1000000);
        break;
    }

    apr_atomic_set32(&flag, 0); /* tell the thread to exit.  */

    JOIN_WITH_SUCCESS(tc, th);

    ABTS_ASSERT(tc, "Too many retries", i < MAX_RETRY);
    APR_ASSERT_SUCCESS(tc, "Unable to destroy the timeout mutex",
                       apr_thread_mutex_destroy(timeout_mutex));
}

static void test_thread_nestedmutex(abts_case *tc, void *data)
{
    apr_thread_mutex_t *m;
    apr_status_t rv;

    rv = apr_thread_mutex_create(&m, APR_THREAD_MUTEX_NESTED, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, m);

    rv = apr_thread_mutex_lock(m);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_thread_mutex_trylock(m);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    if (rv == APR_SUCCESS)
    {
        rv = apr_thread_mutex_unlock(m);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }

    rv = apr_thread_mutex_unlock(m);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
}

static void test_thread_unnestedmutex(abts_case *tc, void *data)
{
    apr_thread_mutex_t *m;
    apr_status_t rv;

    rv = apr_thread_mutex_create(&m, APR_THREAD_MUTEX_UNNESTED, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, m);

    rv = apr_thread_mutex_lock(m);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_thread_mutex_trylock(m);
    ABTS_INT_EQUAL(tc, APR_EBUSY, rv);
    if (rv == APR_SUCCESS)
    {
        rv = apr_thread_mutex_unlock(m);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }

    rv = apr_thread_mutex_unlock(m);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
}

#ifdef WIN32
static void *APR_THREAD_FUNC
thread_win32_abandoned_mutex_function(apr_thread_t *thd, void *data)
{
    apr_thread_mutex_t *mutex = data;
    apr_status_t rv;

    rv = apr_thread_mutex_lock(mutex);

    /* exit from thread without unlocking mutex. */
    apr_thread_exit(thd, rv);

    return NULL;
}

static void test_win32_abandoned_mutex(abts_case *tc, void *data)
{
    apr_status_t rv;
    apr_thread_t *thread;
    apr_thread_mutex_t *mutex;

    /* Create timed mutex: APR will create Win32 mutex object in this case. */
    rv = apr_thread_mutex_create(&mutex, APR_THREAD_MUTEX_TIMED, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_thread_create(&thread, NULL, thread_win32_abandoned_mutex_function,
                           mutex, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    apr_thread_join(&rv, thread);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_thread_mutex_trylock(mutex);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_thread_mutex_unlock (mutex);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
}

#endif

#endif /* !APR_HAS_THREADS */

#if !APR_HAS_THREADS
static void threads_not_impl(abts_case *tc, void *data)
{
    ABTS_NOT_IMPL(tc, "Threads not implemented on this platform");
}
#endif


abts_suite *testlock(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

#if !APR_HAS_THREADS
    abts_run_test(suite, threads_not_impl, NULL);
#else
    abts_run_test(suite, test_thread_mutex, NULL);
#if APR_HAS_TIME_DEPENDANT_TESTS
    abts_run_test(suite, test_thread_timedmutex, NULL);
#endif
    abts_run_test(suite, test_thread_nestedmutex, NULL);
    abts_run_test(suite, test_thread_unnestedmutex, NULL);
    abts_run_test(suite, test_thread_rwlock, NULL);
    abts_run_test(suite, test_cond, NULL);
    abts_run_test(suite, test_timeoutcond, NULL);
#if APR_HAS_TIME_DEPENDANT_TESTS
    abts_run_test(suite, test_timeoutmutex, NULL);
#endif
#ifdef WIN32
    abts_run_test(suite, test_win32_abandoned_mutex, NULL);
#endif
#endif

    return suite;
}

