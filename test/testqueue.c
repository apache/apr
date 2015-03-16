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

#include "apu.h"
#include "apr_queue.h"
#include "apr_thread_pool.h"
#include "apr_time.h"
#include "abts.h"
#include "testutil.h"

#if APR_HAS_THREADS

#define NUMBER_CONSUMERS    3
#define CONSUMER_ACTIVITY   4
#define NUMBER_PRODUCERS    4
#define PRODUCER_ACTIVITY   5
#define QUEUE_SIZE          100

static apr_queue_t *queue;

static void * APR_THREAD_FUNC consumer(apr_thread_t *thd, void *data)
{
    long sleeprate;
    abts_case *tc = data;
    apr_status_t rv;
    void *v;

    sleeprate = 1000000/CONSUMER_ACTIVITY;
    apr_sleep((rand() % 4) * 1000000); /* sleep random seconds */

    while (1)
    {
        rv = apr_queue_pop(queue, &v);

        if (rv == APR_EINTR)
            continue;

        if (rv == APR_EOF)
            break;

        ABTS_TRUE(tc, v == NULL);
        ABTS_TRUE(tc, rv == APR_SUCCESS);

        apr_sleep(sleeprate); /* sleep this long to acheive our rate */
    }

    return NULL;
}

static void * APR_THREAD_FUNC producer(apr_thread_t *thd, void *data)
{
    long sleeprate;
    abts_case *tc = data;
    apr_status_t rv;

    sleeprate = 1000000/PRODUCER_ACTIVITY;
    apr_sleep((rand() % 4) * 1000000); /* sleep random seconds */

    while (1)
    {
        rv = apr_queue_push(queue, NULL);

        if (rv == APR_EINTR)
            continue;

        if (rv == APR_EOF)
            break;

        ABTS_TRUE(tc, rv == APR_SUCCESS);

        apr_sleep(sleeprate); /* sleep this long to acheive our rate */
    }

    return NULL;
}

static void test_queue_producer_consumer(abts_case *tc, void *data)
{
    unsigned int i;
    apr_status_t rv;
    apr_thread_pool_t *thrp;

    /* XXX: non-portable */
    srand((unsigned int)apr_time_now());

    rv = apr_queue_create(&queue, QUEUE_SIZE, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_thread_pool_create(&thrp, 0, NUMBER_CONSUMERS + NUMBER_PRODUCERS, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    for (i = 0; i < NUMBER_CONSUMERS; i++) {
        rv = apr_thread_pool_push(thrp, consumer, tc, 0, NULL);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }

    for (i = 0; i < NUMBER_PRODUCERS; i++) {
        rv = apr_thread_pool_push(thrp, producer, tc, 0, NULL);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }

    apr_sleep(5000000); /* sleep 5 seconds */

    rv = apr_queue_term(queue);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    rv = apr_thread_pool_destroy(thrp);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
}

static void test_queue_timeout(abts_case *tc, void *data)
{
    apr_queue_t *q;
    apr_status_t rv;
    apr_time_t start;
    unsigned int i;
    void *value;

    rv = apr_queue_create(&q, 5, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    for (i = 0; i < 2; ++i) {
        rv = apr_queue_timedpush(q, NULL, apr_time_from_msec(1));
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }
    for (i = 0; i < 3; ++i) {
        rv = apr_queue_trypush(q, NULL);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }

    start = apr_time_now();
    rv = apr_queue_timedpush(q, NULL, apr_time_from_msec(1));
    ABTS_TRUE(tc, APR_STATUS_IS_TIMEUP(rv));
    ABTS_TRUE(tc, apr_time_now() - start >= apr_time_from_msec(1));

    rv = apr_queue_trypush(q, NULL);
    ABTS_TRUE(tc, APR_STATUS_IS_EAGAIN(rv));

    for (i = 0; i < 2; ++i) {
        rv = apr_queue_timedpop(q, &value, apr_time_from_msec(1));
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }
    for (i = 0; i < 3; ++i) {
        rv = apr_queue_trypop(q, &value);
        ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    }

    start = apr_time_now();
    rv = apr_queue_timedpop(q, &value, apr_time_from_sec(1));
    ABTS_TRUE(tc, APR_STATUS_IS_TIMEUP(rv));
    ABTS_TRUE(tc, apr_time_now() - start >= apr_time_from_msec(1));

    rv = apr_queue_trypop(q, &value);
    ABTS_TRUE(tc, APR_STATUS_IS_EAGAIN(rv));

    rv = apr_queue_term(q);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
}

#endif /* APR_HAS_THREADS */

abts_suite *testqueue(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

#if APR_HAS_THREADS
    abts_run_test(suite, test_queue_producer_consumer, NULL);
    abts_run_test(suite, test_queue_timeout, NULL);
#endif /* APR_HAS_THREADS */

    return suite;
}
