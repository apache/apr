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

#include "abts.h"
#include "testutil.h"
#include "apr_hooks.h"

#define TEST_DECLARE(type) type

APR_DECLARE_EXTERNAL_HOOK(test,TEST,int, toyhook, (char *x, apr_size_t s))

APR_HOOK_STRUCT(
    APR_HOOK_LINK(toyhook)
)

APR_IMPLEMENT_EXTERNAL_HOOK_RUN_ALL(test,TEST,int, toyhook,
                                    (char *x, apr_size_t s), (x, s), 0, -1)

static void safe_concat(char *buf, apr_size_t buf_size, const char *append)
{
    if (strlen(buf) + strlen(append) + 1 <= buf_size) {
        strcat(buf, append);
    }
}

static int toyhook_1(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "1");
    return 0;
}

static int toyhook_2(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "2");
    return 0;
}

static int toyhook_3(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "3");
    return 0;
}

static int toyhook_4(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "4");
    return 0;
}

static int toyhook_5(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "5");
    return 0;
}

static void test_basic_ordering(abts_case *tc, void *data)
{
    char buf[6] = {0};

    apr_hook_global_pool = p;
    apr_hook_deregister_all();

    apr_hook_debug_current = "foo";
    test_hook_toyhook(toyhook_5, NULL, NULL, APR_HOOK_MIDDLE + 2);
    test_hook_toyhook(toyhook_1, NULL, NULL, APR_HOOK_MIDDLE - 2);
    test_hook_toyhook(toyhook_3, NULL, NULL, APR_HOOK_MIDDLE);
    test_hook_toyhook(toyhook_2, NULL, NULL, APR_HOOK_MIDDLE - 1);
    test_hook_toyhook(toyhook_4, NULL, NULL, APR_HOOK_MIDDLE + 1);

    apr_hook_sort_all();

    test_run_toyhook(buf, sizeof buf);
    
    ABTS_STR_EQUAL(tc, "12345", buf);
}

static void test_pred_ordering(abts_case *tc, void *data)
{
    char buf[10] = {0};
    static const char *hook2_predecessors[] = {"1", NULL};
    static const char *hook3_predecessors[] = {"2", NULL};

    apr_hook_global_pool = p;
    apr_hook_deregister_all();

    apr_hook_debug_current = "1";
    test_hook_toyhook(toyhook_1, NULL, NULL, APR_HOOK_MIDDLE);
    apr_hook_debug_current = "3";
    test_hook_toyhook(toyhook_3, hook3_predecessors, NULL, APR_HOOK_MIDDLE);
    apr_hook_debug_current = "2";
    test_hook_toyhook(toyhook_2, hook2_predecessors, NULL, APR_HOOK_MIDDLE);
    apr_hook_debug_current = "2";
    test_hook_toyhook(toyhook_2, hook2_predecessors, NULL, APR_HOOK_MIDDLE);

    apr_hook_sort_all();

    test_run_toyhook(buf, sizeof buf);
    
    /* FAILS ABTS_STR_EQUAL(tc, "1223", buf); */
}

abts_suite *testhooks(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_basic_ordering, NULL);
    abts_run_test(suite, test_pred_ordering, NULL);

    return suite;
}
