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
#include "apr_file_io.h"

static void test_temp_dir(CuTest *tc)
{
    const char *tempdir = NULL;
    apr_status_t rv;

    rv = apr_temp_dir_get(&tempdir, p);
    apr_assert_success(tc, "Error finding Temporary Directory", rv);
    CuAssertPtrNotNull(tc, tempdir);
}

CuSuite *testtemp(void)
{
    CuSuite *suite = CuSuiteNew("Temp Dir");

    SUITE_ADD_TEST(suite, test_temp_dir);

    return suite;
}

