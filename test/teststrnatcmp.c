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

#include "apr_file_io.h"
#include "apr_errno.h"
#include "apr_strings.h"
#include "testutil.h"

static void less0(abts_case *tc, void *data)
{
    int rv = apr_strnatcmp("a", "b");
    abts_assert(tc, "didn't compare simple strings properly", rv < 0);
}

static void str_equal(abts_case *tc, void *data)
{
    int rv = apr_strnatcmp("a", "a");
    abts_assert(tc, "didn't compare simple strings properly", rv == 0);
}

static void more0(abts_case *tc, void *data)
{
    int rv = apr_strnatcmp("b", "a");
    abts_assert(tc, "didn't compare simple strings properly", rv > 0);
}

static void less_ignore_case(abts_case *tc, void *data)
{
    int rv = apr_strnatcasecmp("a", "B");
    abts_assert(tc, "didn't compare simple strings properly", rv < 0);
}

static void str_equal_ignore_case(abts_case *tc, void *data)
{
    int rv = apr_strnatcasecmp("a", "A");
    abts_assert(tc, "didn't compare simple strings properly", rv == 0);
}

static void more_ignore_case(abts_case *tc, void *data)
{
    int rv = apr_strnatcasecmp("b", "A");
    abts_assert(tc, "didn't compare simple strings properly", rv > 0);
}

static void natcmp(abts_case *tc, void *data)
{
    int rv = apr_strnatcasecmp("a2", "a10");
    abts_assert(tc, "didn't compare simple strings properly", rv < 0);
}

abts_suite *teststrnatcmp(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, less0, NULL);
    abts_run_test(suite, str_equal, NULL);
    abts_run_test(suite, more0, NULL);
    abts_run_test(suite, less_ignore_case, NULL);
    abts_run_test(suite, str_equal_ignore_case, NULL);
    abts_run_test(suite, more_ignore_case, NULL);
    abts_run_test(suite, natcmp, NULL);

    return suite;
}

