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
#include "test_apr.h"

static void less0(CuTest *tc)
{
    int rv = apr_strnatcmp("a", "b");
    CuAssert(tc, "didn't compare simple strings properly", rv < 0);
}

static void str_equal(CuTest *tc)
{
    int rv = apr_strnatcmp("a", "a");
    CuAssert(tc, "didn't compare simple strings properly", rv == 0);
}

static void more0(CuTest *tc)
{
    int rv = apr_strnatcmp("b", "a");
    CuAssert(tc, "didn't compare simple strings properly", rv > 0);
}

static void less_ignore_case(CuTest *tc)
{
    int rv = apr_strnatcasecmp("a", "B");
    CuAssert(tc, "didn't compare simple strings properly", rv < 0);
}

static void str_equal_ignore_case(CuTest *tc)
{
    int rv = apr_strnatcasecmp("a", "A");
    CuAssert(tc, "didn't compare simple strings properly", rv == 0);
}

static void more_ignore_case(CuTest *tc)
{
    int rv = apr_strnatcasecmp("b", "A");
    CuAssert(tc, "didn't compare simple strings properly", rv > 0);
}

static void natcmp(CuTest *tc)
{
    int rv = apr_strnatcasecmp("a2", "a10");
    CuAssert(tc, "didn't compare simple strings properly", rv < 0);
}

CuSuite *teststrnatcmp(void)
{
    CuSuite *suite = CuSuiteNew("Natural String Cmp");

    SUITE_ADD_TEST(suite, less0);
    SUITE_ADD_TEST(suite, str_equal);
    SUITE_ADD_TEST(suite, more0);
    SUITE_ADD_TEST(suite, less_ignore_case);
    SUITE_ADD_TEST(suite, str_equal_ignore_case);
    SUITE_ADD_TEST(suite, more_ignore_case);
    SUITE_ADD_TEST(suite, natcmp);

    return suite;
}

