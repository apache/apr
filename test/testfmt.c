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
#include "apr.h"
#include "apr_portable.h"
#include "apr_strings.h"

static void ssize_t_fmt(CuTest *tc)
{
    char buf[100];
    apr_ssize_t var = 0;

    sprintf(buf, "%" APR_SSIZE_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
    apr_snprintf(buf, sizeof(buf), "%" APR_SSIZE_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
}

static void size_t_fmt(CuTest *tc)
{
    char buf[100];
    apr_size_t var = 0;

    sprintf(buf, "%" APR_SIZE_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
    apr_snprintf(buf, sizeof(buf), "%" APR_SIZE_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
}

static void off_t_fmt(CuTest *tc)
{
    char buf[100];
    apr_off_t var = 0;

    sprintf(buf, "%" APR_OFF_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
    apr_snprintf(buf, sizeof(buf), "%" APR_OFF_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
}

static void pid_t_fmt(CuTest *tc)
{
    char buf[100];
    pid_t var = 0;

    sprintf(buf, "%" APR_PID_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
    apr_snprintf(buf, sizeof(buf), "%" APR_PID_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
}

static void int64_t_fmt(CuTest *tc)
{
    char buf[100];
    apr_int64_t var = 0;

    sprintf(buf, "%" APR_INT64_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
    apr_snprintf(buf, sizeof(buf), "%" APR_INT64_T_FMT, var);
    CuAssertStrEquals(tc, "0", buf);
}

static void uint64_t_fmt(CuTest *tc)
{
    char buf[100];
    apr_uint64_t var = APR_UINT64_C(14000000);

    sprintf(buf, "%" APR_UINT64_T_FMT, var);
    CuAssertStrEquals(tc, "14000000", buf);
    apr_snprintf(buf, sizeof(buf), "%" APR_UINT64_T_FMT, var);
    CuAssertStrEquals(tc, "14000000", buf);
}

static void uint64_t_hex_fmt(CuTest *tc)
{
    char buf[100];
    apr_uint64_t var = APR_UINT64_C(14000000);

    sprintf(buf, "%" APR_UINT64_T_HEX_FMT, var);
    CuAssertStrEquals(tc, "d59f80", buf);
    apr_snprintf(buf, sizeof(buf), "%" APR_UINT64_T_HEX_FMT, var);
    CuAssertStrEquals(tc, "d59f80", buf);
}

static void more_int64_fmts(CuTest *tc)
{
    char buf[100];
    apr_int64_t i = APR_INT64_C(-42);
    apr_uint64_t ui = APR_UINT64_C(42);
    apr_uint64_t big = APR_UINT64_C(3141592653589793238);

    apr_snprintf(buf, sizeof buf, "%" APR_INT64_T_FMT, i);
    CuAssertStrEquals(tc, buf, "-42");

    apr_snprintf(buf, sizeof buf, "%" APR_UINT64_T_FMT, ui);
    CuAssertStrEquals(tc, buf, "42");

    apr_snprintf(buf, sizeof buf, "%" APR_UINT64_T_FMT, big);
    CuAssertStrEquals(tc, buf, "3141592653589793238");
}

CuSuite *testfmt(void)
{
    CuSuite *suite = CuSuiteNew("Formats");

    SUITE_ADD_TEST(suite, ssize_t_fmt);
    SUITE_ADD_TEST(suite, size_t_fmt);
    SUITE_ADD_TEST(suite, off_t_fmt);
    SUITE_ADD_TEST(suite, pid_t_fmt);
    SUITE_ADD_TEST(suite, int64_t_fmt);
    SUITE_ADD_TEST(suite, uint64_t_fmt);
    SUITE_ADD_TEST(suite, uint64_t_hex_fmt);
    SUITE_ADD_TEST(suite, more_int64_fmts);

    return suite;
}

