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
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_lib.h"

#if WIN32
#define ABS_ROOT "C:/"
#elif defined(NETWARE)
#define ABS_ROOT "SYS:/"
#else
#define ABS_ROOT "/"
#endif

static void merge_aboveroot(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;
    char errmsg[256];

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo", ABS_ROOT"bar", APR_FILEPATH_NOTABOVEROOT,
                            p);
    apr_strerror(rv, errmsg, sizeof(errmsg));
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EABOVEROOT(rv));
    CuAssertPtrEquals(tc, NULL, dstpath);
    CuAssertStrEquals(tc, "The given path was above the root path", errmsg);
}

static void merge_belowroot(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo", ABS_ROOT"foo/bar", 
                            APR_FILEPATH_NOTABOVEROOT, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/bar", dstpath);
}

static void merge_noflag(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo", ABS_ROOT"foo/bar", 0, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/bar", dstpath);
}

static void merge_dotdot(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo/bar", "../baz", 0, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/baz", dstpath);
}

static void merge_secure(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo/bar", "../bar/baz", 0, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/bar/baz", dstpath);
}

static void merge_notrel(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo/bar", "../baz",
                            APR_FILEPATH_NOTRELATIVE, p);
    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT"foo/baz", dstpath);
}

static void merge_notrelfail(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;
    char errmsg[256];

    rv = apr_filepath_merge(&dstpath, "foo/bar", "../baz", 
                            APR_FILEPATH_NOTRELATIVE, p);
    apr_strerror(rv, errmsg, sizeof(errmsg));

    CuAssertPtrEquals(tc, NULL, dstpath);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ERELATIVE(rv));
    CuAssertStrEquals(tc, "The given path is relative", errmsg);
}

static void merge_notabsfail(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;
    char errmsg[256];

    rv = apr_filepath_merge(&dstpath, ABS_ROOT"foo/bar", "../baz", 
                            APR_FILEPATH_NOTABSOLUTE, p);
    apr_strerror(rv, errmsg, sizeof(errmsg));

    CuAssertPtrEquals(tc, NULL, dstpath);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EABSOLUTE(rv));
    CuAssertStrEquals(tc, "The given path is absolute", errmsg);
}

static void merge_notabs(CuTest *tc)
{
    apr_status_t rv;
    char *dstpath = NULL;

    rv = apr_filepath_merge(&dstpath, "foo/bar", "../baz", 
                            APR_FILEPATH_NOTABSOLUTE, p);

    CuAssertPtrNotNull(tc, dstpath);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, "foo/baz", dstpath);
}

static void root_absolute(CuTest *tc)
{
    apr_status_t rv;
    const char *root = NULL;
    const char *path = ABS_ROOT"foo/bar";

    rv = apr_filepath_root(&root, &path, 0, p);

    CuAssertPtrNotNull(tc, root);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, ABS_ROOT, root);
}

static void root_relative(CuTest *tc)
{
    apr_status_t rv;
    const char *root = NULL;
    const char *path = "foo/bar";
    char errmsg[256];

    rv = apr_filepath_root(&root, &path, 0, p);
    apr_strerror(rv, errmsg, sizeof(errmsg));

    CuAssertPtrEquals(tc, NULL, root);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ERELATIVE(rv));
    CuAssertStrEquals(tc, "The given path is relative", errmsg);
}


#if 0
    root_result(rootpath);
    root_result(addpath);
}
#endif

CuSuite *testnames(void)
{
    CuSuite *suite = CuSuiteNew("Path names");

    SUITE_ADD_TEST(suite, merge_aboveroot);
    SUITE_ADD_TEST(suite, merge_belowroot);
    SUITE_ADD_TEST(suite, merge_noflag);
    SUITE_ADD_TEST(suite, merge_dotdot);
    SUITE_ADD_TEST(suite, merge_secure);
    SUITE_ADD_TEST(suite, merge_notrel);
    SUITE_ADD_TEST(suite, merge_notrelfail);
    SUITE_ADD_TEST(suite, merge_notabs);
    SUITE_ADD_TEST(suite, merge_notabsfail);

    SUITE_ADD_TEST(suite, root_absolute);
    SUITE_ADD_TEST(suite, root_relative);

    return suite;
}

