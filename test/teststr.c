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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "apr_general.h"
#include "apr_strings.h"
#include "apr_errno.h"

/* I haven't bothered to check for APR_ENOTIMPL here, AFAIK, all string
 * functions exist on all platforms.
 */

static void test_strtok(CuTest *tc)
{
    struct {
        char *input;
        char *sep;
    }
    cases[] = {
        {
            "",
            "Z"
        },
        {
            "      asdf jkl; 77889909            \r\n\1\2\3Z",
            " \r\n\3\2\1"
        },
        {
            NULL,  /* but who cares if apr_strtok() segfaults? */
            " \t"
        },
#if 0     /* don't do this... you deserve to segfault */
        {
            "a b c              ",
            NULL
        },
#endif
        {
            "   a       b        c   ",
            ""
        },
        {
            "a              b c         ",
            " "
        }
    };
    int curtc;

    for (curtc = 0; curtc < sizeof cases / sizeof cases[0]; curtc++) {
        char *retval1, *retval2;
        char *str1, *str2;
        char *state;

        str1 = apr_pstrdup(p, cases[curtc].input);
        str2 = apr_pstrdup(p, cases[curtc].input);

        do {
            retval1 = apr_strtok(str1, cases[curtc].sep, &state);
            retval2 = strtok(str2, cases[curtc].sep);

            if (!retval1) {
                CuAssertTrue(tc, retval2 == NULL);
            }
            else {
                CuAssertTrue(tc, retval2 != NULL);
                CuAssertStrEquals(tc, retval2, retval1);
            }

            str1 = str2 = NULL; /* make sure we pass NULL on subsequent calls */
        } while (retval1);
    }
}

static void snprintf_noNULL(CuTest *tc)
{
    char buff[100];
    char *testing = apr_palloc(p, 10);

    testing[0] = 't';
    testing[1] = 'e';
    testing[2] = 's';
    testing[3] = 't';
    testing[4] = 'i';
    testing[5] = 'n';
    testing[6] = 'g';
    
    /* If this test fails, we are going to seg fault. */
    apr_snprintf(buff, sizeof(buff), "%.*s", 7, testing);
    CuAssertStrNEquals(tc, buff, testing, 7);
}

static void snprintf_0NULL(CuTest *tc)
{
    int rv;

    rv = apr_snprintf(NULL, 0, "%sBAR", "FOO");
    CuAssertIntEquals(tc, 6, rv);
}

static void snprintf_0nonNULL(CuTest *tc)
{
    int rv;
    char *buff = "testing";

    rv = apr_snprintf(buff, 0, "%sBAR", "FOO");
    CuAssertIntEquals(tc, 6, rv);
    CuAssert(tc, "buff unmangled", strcmp(buff, "FOOBAR") != 0);
}

static void snprintf_int64(CuTest *tc)
{
    char buf[100];
    apr_int64_t i = APR_INT64_C(-42);
    apr_uint64_t ui = APR_INT64_C(42); /* no APR_UINT64_C */
    apr_uint64_t big = APR_INT64_C(3141592653589793238);

    apr_snprintf(buf, sizeof buf, "%" APR_INT64_T_FMT, i);
    CuAssertStrEquals(tc, buf, "-42");

    apr_snprintf(buf, sizeof buf, "%" APR_UINT64_T_FMT, ui);
    CuAssertStrEquals(tc, buf, "42");

    apr_snprintf(buf, sizeof buf, "%" APR_UINT64_T_FMT, big);
    CuAssertStrEquals(tc, buf, "3141592653589793238");
}

static void snprintf_underflow(CuTest *tc)
{
    char buf[20];
    int rv;

    rv = apr_snprintf(buf, sizeof buf, "%.2f", (double)0.0001);
    CuAssertIntEquals(tc, 4, rv);
    CuAssertStrEquals(tc, "0.00", buf);
    
    rv = apr_snprintf(buf, sizeof buf, "%.2f", (double)0.001);
    CuAssertIntEquals(tc, 4, rv);
    CuAssertStrEquals(tc, "0.00", buf);
    
    rv = apr_snprintf(buf, sizeof buf, "%.2f", (double)0.01);
    CuAssertIntEquals(tc, 4, rv);
    CuAssertStrEquals(tc, "0.01", buf);
}

static void string_error(CuTest *tc)
{
     char buf[128], *rv;
     apr_status_t n;

     buf[0] = '\0';
     rv = apr_strerror(APR_ENOENT, buf, sizeof buf);
     CuAssertPtrEquals(tc, buf, rv);
     CuAssertTrue(tc, strlen(buf) > 0);

     rv = apr_strerror(APR_TIMEUP, buf, sizeof buf);
     CuAssertPtrEquals(tc, buf, rv);
     CuAssertStrEquals(tc, "The timeout specified has expired", buf);
     
     /* throw some randomish numbers at it to check for robustness */
     for (n = 1; n < 1000000; n *= 2) {
         apr_strerror(n, buf, sizeof buf);
     }
}

#define SIZE 180000
static void string_long(CuTest *tc)
{
    char s[SIZE + 1];

    memset(s, 'A', SIZE);
    s[SIZE] = '\0';

    apr_psprintf(p, "%s", s);
}

/* ### FIXME: apr.h/apr_strings.h should provide these! */
#define MY_LLONG_MAX (APR_INT64_C(9223372036854775807))
#define MY_LLONG_MIN (-MY_LLONG_MAX - APR_INT64_C(1))

static void string_strtoi64(CuTest *tc)
{
    static const struct {
        int errnum, base;
        const char *in, *end;
        apr_int64_t result;
    } ts[] = {
        
        /* base 10 tests */
        { 0, 10, "123545", NULL, APR_INT64_C(123545) },
        { 0, 10, "   123545", NULL, APR_INT64_C(123545) },
        { 0, 10, "   +123545", NULL, APR_INT64_C(123545) },
        { 0, 10, "-123545", NULL, APR_INT64_C(-123545) },
        { 0, 10, "   00000123545", NULL, APR_INT64_C(123545) },
        { 0, 10, "123545ZZZ", "ZZZ", APR_INT64_C(123545) },
        { 0, 10, "   123545   ", "   ", APR_INT64_C(123545) },

        /* base 16 tests */
        { 0, 16, "1E299", NULL, APR_INT64_C(123545) },
        { 0, 16, "1e299", NULL, APR_INT64_C(123545) },
        { 0, 16, "0x1e299", NULL, APR_INT64_C(123545) },
        { 0, 16, "0X1E299", NULL, APR_INT64_C(123545) },
        { 0, 16, "+1e299", NULL, APR_INT64_C(123545) },
        { 0, 16, "-1e299", NULL, APR_INT64_C(-123545) },
        { 0, 16, "   -1e299", NULL, APR_INT64_C(-123545) },

        /* automatic base detection tests */
        { 0, 0, "123545", NULL, APR_INT64_C(123545) },
        { 0, 0, "0x1e299", NULL, APR_INT64_C(123545) },
        { 0, 0, "  0x1e299", NULL, APR_INT64_C(123545) },
        { 0, 0, "+0x1e299", NULL, APR_INT64_C(123545) },
        { 0, 0, "-0x1e299", NULL, APR_INT64_C(-123545) },

        /* large number tests */
        { 0, 10, "8589934605", NULL, APR_INT64_C(8589934605) },
        { 0, 10, "-8589934605", NULL, APR_INT64_C(-8589934605) },
        { 0, 16, "0x20000000D", NULL, APR_INT64_C(8589934605) },
        { 0, 16, "-0x20000000D", NULL, APR_INT64_C(-8589934605) },
        { 0, 16, "   0x20000000D", NULL, APR_INT64_C(8589934605) },
        { 0, 16, "   0x20000000D", NULL, APR_INT64_C(8589934605) },

        /* error cases */
        { ERANGE, 10, "999999999999999999999999999999999", "", MY_LLONG_MAX },
        { ERANGE, 10, "-999999999999999999999999999999999", "", MY_LLONG_MIN },

#if 0
        /* C99 doesn't require EINVAL for an invalid range. */
        { EINVAL, 99, "", (void *)-1 /* don't care */, 0 },
#endif

        /* some strtoll implementations give EINVAL when no conversion
         * is performed. */
        { -1 /* don't care */, 10, "zzz", "zzz", APR_INT64_C(0) },
        { -1 /* don't care */, 10, "", NULL, APR_INT64_C(0) }

    };
    int n;

    for (n = 0; n < sizeof(ts)/sizeof(ts[0]); n++) {
        char *end = "end ptr not changed";
        apr_int64_t result;
        int errnum;
        
        errno = 0;
        result = apr_strtoi64(ts[n].in, &end, ts[n].base);
        errnum = errno;

        CuAssert(tc,
                 apr_psprintf(p, "for '%s': result was %" APR_INT64_T_FMT 
                              " not %" APR_INT64_T_FMT, ts[n].in,
                              result, ts[n].result),
                 result == ts[n].result);
        
        if (ts[n].errnum != -1) {
            CuAssert(tc,
                     apr_psprintf(p, "for '%s': errno was %d not %d", ts[n].in,
                                  errnum, ts[n].errnum),
                     ts[n].errnum == errnum);
        }

        if (ts[n].end == NULL) {
            /* end must point to NUL terminator of .in */
            CuAssertPtrEquals(tc, ts[n].in + strlen(ts[n].in), end);
        } else if (ts[n].end != (void *)-1) {
            CuAssert(tc,
                     apr_psprintf(p, "for '%s', end was '%s' not '%s'",
                                  ts[n].in, end, ts[n].end),
                     strcmp(ts[n].end, end) == 0);
        }
    }
}

CuSuite *teststr(void)
{
    CuSuite *suite = CuSuiteNew("Strings");

    SUITE_ADD_TEST(suite, snprintf_0NULL);
    SUITE_ADD_TEST(suite, snprintf_0nonNULL);
    SUITE_ADD_TEST(suite, snprintf_noNULL);
    SUITE_ADD_TEST(suite, snprintf_int64);
    SUITE_ADD_TEST(suite, snprintf_underflow);
    SUITE_ADD_TEST(suite, test_strtok);
    SUITE_ADD_TEST(suite, string_error);
    SUITE_ADD_TEST(suite, string_long);
    SUITE_ADD_TEST(suite, string_strtoi64);

    return suite;
}

