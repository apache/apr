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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "apr_file_info.h"
#include "apr_fnmatch.h"
#include "apr_tables.h"

static void test_glob(CuTest *tc)
{
    int i;
    char **list;
    apr_array_header_t *result;
    apr_status_t rv = apr_match_glob("data\\*.txt", &result, p);

    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    /* XXX If we ever add a file that matches *.txt to data, then we need
     * to increase this.
     */
    CuAssertIntEquals(tc, 2, result->nelts);

    list = (char **)result->elts;
    for (i = 0; i < result->nelts; i++) {
        char *dot = strrchr(list[i], '.');
        CuAssertStrEquals(tc, dot, ".txt");
    }
}

static void test_glob_currdir(CuTest *tc)
{
    int i;
    char **list;
    apr_array_header_t *result;
    apr_status_t rv;
    apr_filepath_set("data", p);
    rv = apr_match_glob("*.txt", &result, p);

    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    /* XXX If we ever add a file that matches *.txt to data, then we need
     * to increase this.
     */
    CuAssertIntEquals(tc, 2, result->nelts);

    list = (char **)result->elts;
    for (i = 0; i < result->nelts; i++) {
        char *dot = strrchr(list[i], '.');
        CuAssertStrEquals(tc, dot, ".txt");
    }
}

CuSuite *testfnmatch(void)
{
    CuSuite *suite = CuSuiteNew("Fnmatch");

    SUITE_ADD_TEST(suite, test_glob);
    SUITE_ADD_TEST(suite, test_glob_currdir);

    return suite;
}

