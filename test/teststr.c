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

static void string_error(CuTest *tc)
{
     char buf[128], *rv;

     buf[0] = '\0';
     rv = apr_strerror(APR_ENOENT, buf, sizeof buf);
     CuAssertPtrEquals(tc, buf, rv);
     CuAssertTrue(tc, strlen(buf) > 0);

     rv = apr_strerror(APR_TIMEUP, buf, sizeof buf);
     CuAssertPtrEquals(tc, buf, rv);
     CuAssertStrEquals(tc, "The timeout specified has expired", buf);
}

#define SIZE 180000
static void string_long(CuTest *tc)
{
    char s[SIZE + 1];

    memset(s, 'A', SIZE);
    s[SIZE] = '\0';

    apr_psprintf(p, "%s", s);
}

CuSuite *teststr(void)
{
    CuSuite *suite = CuSuiteNew("Strings");

    SUITE_ADD_TEST(suite, snprintf_0NULL);
    SUITE_ADD_TEST(suite, snprintf_0nonNULL);
    SUITE_ADD_TEST(suite, snprintf_noNULL);
    SUITE_ADD_TEST(suite, snprintf_int64);
    SUITE_ADD_TEST(suite, test_strtok);
    SUITE_ADD_TEST(suite, string_error);
    SUITE_ADD_TEST(suite, string_long);

    return suite;
}

