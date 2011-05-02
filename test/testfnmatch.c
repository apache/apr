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

#include "testutil.h"
#include "apr_file_info.h"
#include "apr_fnmatch.h"
#include "apr_tables.h"

/* XXX NUM_FILES must be equal to the nummber of expected files with a
 * .txt extension in the data directory at the time testfnmatch
 * happens to be run (!?!). */

#define NUM_FILES (5)

#define APR_FNM_ANY  0
#define APR_FNM_NONE 128

/* A string is expected to match pattern only if a success_flags bit is true,
 * and is expected to fail for all other cases (so APR_FNM_NONE is bit never tested).
 */
static struct pattern_s {
    const char *pattern;
    const char *string;
    int         success_flags;
} patterns[] = {

    {"test", "test",           APR_FNM_ANY},
    {"test/this", "test/this", APR_FNM_ANY},
    {"teST", "TEst",           APR_FNM_CASE_BLIND},
    {"*", "",                  APR_FNM_ANY},
    {"*", "test",              APR_FNM_ANY},
    {".*", ".test",            APR_FNM_PERIOD},
    {"*", ".test",             APR_FNM_NONE},
    {"", "test",               APR_FNM_NONE},
    {"", "*",                  APR_FNM_NONE},
    {"test", "*",              APR_FNM_NONE},
    {"test/this", "test/",     APR_FNM_NONE},
    {"test/", "test/this",     APR_FNM_NONE},

    {NULL, NULL, 0}
};

#define APR_FNM_MAX 15 /* all bits */

static void test_fnmatch(abts_case *tc, void *data)
{
    struct pattern_s *test = patterns;
    int i;
    int res;

    for (test = patterns; test->pattern; ++test)
        for (i = 0; i <= APR_FNM_MAX; ++i)
        {
            res = apr_fnmatch(test->pattern, test->string, i);
            if ((i & test->success_flags) == test->success_flags) {
                if (res == APR_FNM_NOMATCH)
                    break;
            }
            else {
                if (res != APR_FNM_NOMATCH)
                    break;
            }
        }

    ABTS_INT_EQUAL(tc, APR_FNM_MAX + 1, i);
}

static void test_glob(abts_case *tc, void *data)
{
    int i;
    char **list;
    apr_array_header_t *result;
    
    APR_ASSERT_SUCCESS(tc, "glob match against data/*.txt",
                       apr_match_glob("data\\*.txt", &result, p));

    ABTS_INT_EQUAL(tc, NUM_FILES, result->nelts);

    list = (char **)result->elts;
    for (i = 0; i < result->nelts; i++) {
        char *dot = strrchr(list[i], '.');
        ABTS_STR_EQUAL(tc, ".txt", dot);
    }
}

static void test_glob_currdir(abts_case *tc, void *data)
{
    int i;
    char **list;
    apr_array_header_t *result;
    apr_filepath_set("data", p);
    
    APR_ASSERT_SUCCESS(tc, "glob match against *.txt with data as current",
                       apr_match_glob("*.txt", &result, p));


    ABTS_INT_EQUAL(tc, NUM_FILES, result->nelts);

    list = (char **)result->elts;
    for (i = 0; i < result->nelts; i++) {
        char *dot = strrchr(list[i], '.');
        ABTS_STR_EQUAL(tc, ".txt", dot);
    }
    apr_filepath_set("..", p);
}

abts_suite *testfnmatch(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, test_fnmatch, NULL);
    abts_run_test(suite, test_glob, NULL);
    abts_run_test(suite, test_glob_currdir, NULL);

    return suite;
}

