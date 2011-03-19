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
#include "testlfsabi.h"
#include "apr_file_info.h"

/*
 * Check that various types don't change size depending on
 * -D_FILE_OFFSET_BITS=64
 */

extern void get_type_sizes32(int *res);
extern void get_type_sizes64(int *res);

static void get_type_sizes(int *res)
{
#include "testlfsabi_include.c"
}

static void check_type_sizes(abts_case *tc, void *data)
{
    int normal[IDX_MAX], bits32[IDX_MAX], bits64[IDX_MAX];
    get_type_sizes(normal);
    get_type_sizes32(bits32);
    get_type_sizes64(bits64);
    CHECKSIZE(tc, normal, bits32, apr_dev_t);
    CHECKSIZE(tc, normal, bits64, apr_dev_t);
    CHECKSIZE(tc, normal, bits32, apr_ino_t);
    CHECKSIZE(tc, normal, bits64, apr_ino_t);
    CHECKSIZE(tc, normal, bits32, apr_off_t);
    CHECKSIZE(tc, normal, bits64, apr_off_t);
    CHECKSIZE(tc, normal, bits32, apr_socklen_t);
    CHECKSIZE(tc, normal, bits64, apr_socklen_t);
    CHECKSIZE(tc, normal, bits32, apr_size_t);
    CHECKSIZE(tc, normal, bits64, apr_size_t);
}

abts_suite *testlfsabi(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, check_type_sizes, NULL);

    return suite;
}

