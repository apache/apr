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

#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "apr_strings.h"
#include "test_apr.h"

static void format_arg(char *str, char option, const char *arg)
{
    if (arg) {
        apr_snprintf(str, 8196, "%soption: %c with %s\n", str, option, arg);
    }
    else {
        apr_snprintf(str, 8196, "%soption: %c\n", str, option);
    }
}

static void unknown_arg(void *str, const char *err, ...)
{
    va_list va;

    va_start(va, err);
    apr_vsnprintf(str, 8196, err, va);
    va_end(va);
}

static void no_options_found(CuTest *tc)
{
    int largc = 5;
    const char * const largv[] = {"testprog", "-a", "-b", "-c", "-d"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
   
    while (apr_getopt(opt, "abcd", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            default:
                format_arg(str, data, optarg);
        }
    }
    CuAssertStrEquals(tc, "option: a\n"
                          "option: b\n"
                          "option: c\n"
                          "option: d\n", str);
}

static void no_options(CuTest *tc)
{
    int largc = 5;
    const char * const largv[] = {"testprog", "-a", "-b", "-c", "-d"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "efgh", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
            case 'b':
            case 'c':
            case 'd':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
    CuAssertStrEquals(tc, "testprog: illegal option -- a\n", str);
}

static void required_option(CuTest *tc)
{
    int largc = 3;
    const char * const largv[] = {"testprog", "-a", "foo"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "a:", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
    CuAssertStrEquals(tc, "option: a with foo\n", str);
}

static void required_option_notgiven(CuTest *tc)
{
    int largc = 2;
    const char * const largv[] = {"testprog", "-a"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "a:", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
    CuAssertStrEquals(tc, "testprog: option requires an argument -- a\n", str);
}

static void optional_option(CuTest *tc)
{
    int largc = 3;
    const char * const largv[] = {"testprog", "-a", "foo"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "a::", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
    CuAssertStrEquals(tc, "option: a with foo\n", str);
}

static void optional_option_notgiven(CuTest *tc)
{
    int largc = 2;
    const char * const largv[] = {"testprog", "-a"};
    apr_getopt_t *opt;
    apr_status_t rv;
    char data;
    const char *optarg;
    char str[8196];

    str[0] = '\0';
    rv = apr_getopt_init(&opt, p, largc, largv);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    opt->errfn = unknown_arg;
    opt->errarg = str;
   
    while (apr_getopt(opt, "a::", &data, &optarg) == APR_SUCCESS) {
        switch (data) {
            case 'a':
                format_arg(str, data, optarg);
                break;
            default:
                break;
        }
    }
#if 0
/*  Our version of getopt doesn't allow for optional arguments.  */
    CuAssertStrEquals(tc, "option: a\n", str);
#endif
    CuAssertStrEquals(tc, "testprog: option requires an argument -- a\n", str);
}

CuSuite *testgetopt(void)
{
    CuSuite *suite = CuSuiteNew("Getopt");

    SUITE_ADD_TEST(suite, no_options);
    SUITE_ADD_TEST(suite, no_options_found);
    SUITE_ADD_TEST(suite, required_option);
    SUITE_ADD_TEST(suite, required_option_notgiven);
    SUITE_ADD_TEST(suite, optional_option);
    SUITE_ADD_TEST(suite, optional_option_notgiven);

    return suite;
}
