/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include "apr_time.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "test_apr.h"
#include "apr_strings.h"
#include <time.h>

#define STR_SIZE 45

/* The time value is used throughout the tests, so just make this a global.
 * Also, we need a single value that we can test for the positive tests, so
 * I chose the number below, it corresponds to:
 *           2002-08-14 12:05:36.186711 -25200 [257 Sat].
 * Which happens to be when I wrote the new tests.
 */
static apr_time_t now = APR_INT64_C(1032030336186711);

static char* print_time (apr_pool_t *pool, const apr_time_exp_t *xt)
{
    return apr_psprintf (pool,
                         "%04d-%02d-%02d %02d:%02d:%02d.%06d %+05d [%d %s]%s",
                         xt->tm_year + 1900,
                         xt->tm_mon,
                         xt->tm_mday,
                         xt->tm_hour,
                         xt->tm_min,
                         xt->tm_sec,
                         xt->tm_usec,
                         xt->tm_gmtoff,
                         xt->tm_yday + 1,
                         apr_day_snames[xt->tm_wday],
                         (xt->tm_isdst ? " DST" : ""));
}


static void test_now(CuTest *tc)
{
    apr_time_t timediff;
    apr_time_t current;
    time_t os_now;

    current = apr_time_now();
    time(&os_now);

    timediff = os_now - (current / APR_USEC_PER_SEC); 
    /* Even though these are called so close together, there is the chance
     * that the time will be slightly off, so accept anything between -1 and
     * 1 second.
     */
    CuAssert(tc, "apr_time and OS time do not agree", 
             (timediff > -2) && (timediff < 2));
}

static void test_gmtstr(CuTest *tc)
{
    apr_status_t rv;
    apr_time_exp_t xt;

    rv = apr_time_exp_gmt(&xt, now);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_time_exp_gmt");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
    CuAssertStrEquals(tc, "2002-08-14 19:05:36.186711 +0000 [257 Sat]", 
                      print_time(p, &xt));
}

static void test_localstr(CuTest *tc)
{
    apr_status_t rv;
    apr_time_exp_t xt;

    rv = apr_time_exp_lt(&xt, now);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_time_exp_lt");
    }
    /* XXX: This test is bogus, and a good example of why one never
     * runs tests in their own timezone.  Of course changing the delta
     * has no impact on the resulting time, only the tm_gmtoff.
     */
    CuAssertTrue(tc, rv == APR_SUCCESS);
    CuAssertStrEquals(tc, "2002-08-14 12:05:36.186711 -25200 [257 Sat] DST",
                      print_time(p, &xt));
}

static void test_exp_get_gmt(CuTest *tc)
{
    apr_status_t rv;
    apr_time_exp_t xt;
    apr_time_t imp;
    apr_int64_t hr_off_64;

    rv = apr_time_exp_gmt(&xt, now);
    CuAssertTrue(tc, rv == APR_SUCCESS);
    rv = apr_time_exp_get(&imp, &xt);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_time_exp_get");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
    hr_off_64 = (apr_int64_t) xt.tm_gmtoff * APR_USEC_PER_SEC;
    CuAssertTrue(tc, now + hr_off_64 == imp);
}

static void test_exp_get_lt(CuTest *tc)
{
    apr_status_t rv;
    apr_time_exp_t xt;
    apr_time_t imp;
    apr_int64_t hr_off_64;

    rv = apr_time_exp_lt(&xt, now);
    CuAssertTrue(tc, rv == APR_SUCCESS);
    rv = apr_time_exp_get(&imp, &xt);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_time_exp_get");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
    hr_off_64 = (apr_int64_t) xt.tm_gmtoff * APR_USEC_PER_SEC;
    CuAssertTrue(tc, now + hr_off_64 == imp);
}

static void test_imp_gmt(CuTest *tc)
{
    apr_status_t rv;
    apr_time_exp_t xt;
    apr_time_t imp;

    rv = apr_time_exp_gmt(&xt, now);
    CuAssertTrue(tc, rv == APR_SUCCESS);
    rv = apr_time_exp_gmt_get(&imp, &xt);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_time_exp_gmt_get");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
    CuAssertTrue(tc, now == imp);
}

static void test_rfcstr(CuTest *tc)
{
    apr_status_t rv;
    char str[STR_SIZE];

    rv = apr_rfc822_date(str, now);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_rfc822_date");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
    CuAssertStrEquals(tc, "Sat, 14 Sep 2002 19:05:36 GMT", str);
}

static void test_ctime(CuTest *tc)
{
    apr_status_t rv;
    char str[STR_SIZE];

    /* XXX: This test is bogus, and a good example of why one never
     * runs tests in their own timezone.  Of course changing the delta
     * has no impact on the resulting time, only the tm_gmtoff.
     */
    rv = apr_ctime(str, now);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_ctime");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
    CuAssertStrEquals(tc, "Sat Sep 14 12:05:36 2002", str);
}

static void test_strftime(CuTest *tc)
{
    apr_status_t rv;
    apr_time_exp_t xt;
    char *str = NULL;
    apr_size_t sz;

    rv = apr_time_exp_gmt(&xt, now);
    str = apr_palloc(p, STR_SIZE + 1);
    rv = apr_strftime(str, &sz, STR_SIZE, "%R %A %d %B %Y", &xt);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_strftime");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
    CuAssertStrEquals(tc, "19:05 Saturday 14 September 2002", str);
}

static void test_strftimesmall(CuTest *tc)
{
    apr_status_t rv;
    apr_time_exp_t xt;
    char str[STR_SIZE];
    apr_size_t sz;

    rv = apr_time_exp_gmt(&xt, now);
    rv = apr_strftime(str, &sz, STR_SIZE, "%T", &xt);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_strftime");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
    CuAssertStrEquals(tc, "19:05:36", str);
}

static void test_exp_tz(CuTest *tc)
{
    apr_status_t rv;
    apr_time_exp_t xt;
    apr_int32_t hr_off = -5 * 3600; /* 5 hours in seconds */

    rv = apr_time_exp_tz(&xt, now, hr_off);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_time_exp_tz");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
    CuAssertTrue(tc, (xt.tm_usec == 186711) && 
                     (xt.tm_sec == 36) &&
                     (xt.tm_min == 5) && 
                     (xt.tm_hour == 14) &&
                     (xt.tm_mday == 14) &&
                     (xt.tm_mon == 8) &&
                     (xt.tm_year == 102) &&
                     (xt.tm_wday == 6) &&
                     (xt.tm_yday == 256));
}

static void test_strftimeoffset(CuTest *tc)
{
    apr_status_t rv;
    apr_time_exp_t xt;
    char str[STR_SIZE];
    apr_size_t sz;
    apr_int32_t hr_off = -5 * 3600; /* 5 hours in seconds */

    apr_time_exp_tz(&xt, now, hr_off);
    rv = apr_strftime(str, &sz, STR_SIZE, "%T", &xt);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_strftime");
    }
    CuAssertTrue(tc, rv == APR_SUCCESS);
}

CuSuite *testtime(void)
{
    CuSuite *suite = CuSuiteNew("Time");

    SUITE_ADD_TEST(suite, test_now);
    SUITE_ADD_TEST(suite, test_gmtstr);
    SUITE_ADD_TEST(suite, test_localstr);
    SUITE_ADD_TEST(suite, test_exp_get_gmt);
    SUITE_ADD_TEST(suite, test_exp_get_lt);
    SUITE_ADD_TEST(suite, test_imp_gmt);
    SUITE_ADD_TEST(suite, test_rfcstr);
    SUITE_ADD_TEST(suite, test_ctime);
    SUITE_ADD_TEST(suite, test_strftime);
    SUITE_ADD_TEST(suite, test_strftimesmall);
    SUITE_ADD_TEST(suite, test_exp_tz);
    SUITE_ADD_TEST(suite, test_strftimeoffset);

    return suite;
}

