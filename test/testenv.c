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

#include "apr_env.h"
#include "apr_errno.h"
#include "test_apr.h"

#define TEST_ENVVAR_NAME "apr_test_envvar"
#define TEST_ENVVAR_VALUE "Just a value that we'll check"

static int have_env_set;
static int have_env_get;

static void test_setenv(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_env_set(TEST_ENVVAR_NAME, TEST_ENVVAR_VALUE, p);
    have_env_set = (rv != APR_ENOTIMPL);
    if (!have_env_set) {
        CuNotImpl(tc, "apr_env_set");
    }
    apr_assert_success(tc, "set environment variable", rv);
}

static void test_getenv(CuTest *tc)
{
    char *value;
    apr_status_t rv;

    if (!have_env_set) {
        CuNotImpl(tc, "apr_env_set (skip test for apr_env_get)");
    }

    rv = apr_env_get(&value, TEST_ENVVAR_NAME, p);
    have_env_get = (rv != APR_ENOTIMPL);
    if (!have_env_get) {
        CuNotImpl(tc, "apr_env_get");
    }
    apr_assert_success(tc, "get environment variable", rv);
    CuAssertStrEquals(tc, TEST_ENVVAR_VALUE, value);
}

static void test_delenv(CuTest *tc)
{
    char *value;
    apr_status_t rv;

    if (!have_env_set) {
        CuNotImpl(tc, "apr_env_set (skip test for apr_env_delete)");
    }

    rv = apr_env_delete(TEST_ENVVAR_NAME, p);
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, "apr_env_delete");
    }
    apr_assert_success(tc, "delete environment variable", rv);

    if (!have_env_get) {
        CuNotImpl(tc, "apr_env_get (skip sanity check for apr_env_delete)");
    }
    rv = apr_env_get(&value, TEST_ENVVAR_NAME, p);
    CuAssertIntEquals(tc, APR_ENOENT, rv);
}

CuSuite *testenv(void)
{
    CuSuite *suite = CuSuiteNew("Environment");

    SUITE_ADD_TEST(suite, test_setenv);
    SUITE_ADD_TEST(suite, test_getenv);
    SUITE_ADD_TEST(suite, test_delenv);

    return suite;
}

