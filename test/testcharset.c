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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "apr_portable.h"

#include "abts.h"
#include "testutil.h"

static void test_os_default_encoding(abts_case *tc, void *data)
{
    apr_pool_t *pool;
    const char *encoding;

    apr_pool_create(&pool, NULL);

    encoding = apr_os_default_encoding(pool);

    /* We cannot assert for actual value, so just log it. */
    abts_log_message("apr_os_default_encoding() = %s", encoding);
}

static void test_os_locale_encoding(abts_case *tc, void *data)
{
    apr_pool_t *pool;
    const char *encoding;

    apr_pool_create(&pool, NULL);

    encoding = apr_os_locale_encoding(pool);

    /* We cannot assert for actual value, so just log it. */
    abts_log_message("apr_os_locale_encoding() = %s", encoding);
}

abts_suite *testcharset(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_os_default_encoding, NULL);
    abts_run_test(suite, test_os_locale_encoding, NULL);

    return suite;
}
