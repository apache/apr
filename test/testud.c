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

#include <stdio.h>
#include <stdlib.h>
#include "apr_file_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "test_apr.h"

static apr_pool_t *pool;
static char *testdata;
static int cleanup_called = 0;

static apr_status_t string_cleanup(void *data)
{
    cleanup_called = 1;
    return APR_SUCCESS;
}

static void set_userdata(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_pool_userdata_set(testdata, "TEST", string_cleanup, pool);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
}

static void get_userdata(CuTest *tc)
{
    apr_status_t rv;
    char *retdata;

    rv = apr_pool_userdata_get((void **)&retdata, "TEST", pool);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertStrEquals(tc, retdata, testdata);
}

static void get_nonexistkey(CuTest *tc)
{
    apr_status_t rv;
    char *retdata;

    rv = apr_pool_userdata_get((void **)&retdata, "DOESNTEXIST", pool);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrEquals(tc, retdata, NULL);
}

static void post_pool_clear(CuTest *tc)
{
    apr_status_t rv;
    char *retdata;

    rv = apr_pool_userdata_get((void **)&retdata, "DOESNTEXIST", pool);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrEquals(tc, retdata, NULL);
}

CuSuite *testud(void)
{
    CuSuite *suite = CuSuiteNew("User Data");

    apr_pool_create(&pool, p);
    testdata = apr_pstrdup(pool, "This is a test\n");

    SUITE_ADD_TEST(suite, set_userdata);
    SUITE_ADD_TEST(suite, get_userdata);
    SUITE_ADD_TEST(suite, get_nonexistkey);

    apr_pool_clear(pool);

    SUITE_ADD_TEST(suite, post_pool_clear);

    return suite;
}

