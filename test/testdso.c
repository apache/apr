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
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_dso.h"
#include "apr_strings.h"
#include "apr.h"
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef NETWARE
# define MOD_NAME "mod_test.nlm"
#elif defined(BEOS) || defined(WIN32)
# define MOD_NAME "mod_test.so"
#elif defined(DARWIN)
# define MOD_NAME ".libs/mod_test.so" 
# define LIB_NAME ".libs/libmod_test.dylib" 
#elif defined(__hpux__)
# define MOD_NAME ".libs/mod_test.sl"
# define LIB_NAME ".libs/libmod_test.sl"
#elif defined(_AIX) || defined(__bsdi__)
# define MOD_NAME ".libs/libmod_test.so"
# define LIB_NAME ".libs/libmod_test.so"
#else /* Every other Unix */
# define MOD_NAME ".libs/mod_test.so"
# define LIB_NAME ".libs/libmod_test.so"
#endif

static char *modname;

static void test_load_module(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;
    char errstr[256];

    status = apr_dso_load(&h, modname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    apr_dso_unload(h);
}

static void test_dso_sym(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_dso_handle_sym_t func1 = NULL;
    apr_status_t status;
    void (*function)(char str[256]);
    char teststr[256];
    char errstr[256];

    status = apr_dso_load(&h, modname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_sym(&func1, h, "print_hello");
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, func1);

    function = (void (*)(char *))func1;
    (*function)(teststr);
    CuAssertStrEquals(tc, "Hello - I'm a DSO!\n", teststr);

    apr_dso_unload(h);
}

static void test_dso_sym_return_value(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_dso_handle_sym_t func1 = NULL;
    apr_status_t status;
    int (*function)(int);
    char errstr[256];

    status = apr_dso_load(&h, modname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_sym(&func1, h, "count_reps");
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, func1);

    function = (int (*)(int))func1;
    status = (*function)(5);
    CuAssertIntEquals(tc, 5, status);

    apr_dso_unload(h);
}

static void test_unload_module(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;
    char errstr[256];
    apr_dso_handle_sym_t func1 = NULL;

    status = apr_dso_load(&h, modname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_unload(h);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);

    status = apr_dso_sym(&func1, h, "print_hello");
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ESYMNOTFOUND(status));
}


#ifdef LIB_NAME
static char *libname;

static void test_load_library(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;
    char errstr[256];

    status = apr_dso_load(&h, libname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    apr_dso_unload(h);
}

static void test_dso_sym_library(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_dso_handle_sym_t func1 = NULL;
    apr_status_t status;
    void (*function)(char str[256]);
    char teststr[256];
    char errstr[256];

    status = apr_dso_load(&h, libname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_sym(&func1, h, "print_hello");
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, func1);

    function = (void (*)(char *))func1;
    (*function)(teststr);
    CuAssertStrEquals(tc, "Hello - I'm a DSO!\n", teststr);

    apr_dso_unload(h);
}

static void test_dso_sym_return_value_library(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_dso_handle_sym_t func1 = NULL;
    apr_status_t status;
    int (*function)(int);
    char errstr[256];

    status = apr_dso_load(&h, libname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_sym(&func1, h, "count_reps");
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, func1);

    function = (int (*)(int))func1;
    status = (*function)(5);
    CuAssertIntEquals(tc, 5, status);

    apr_dso_unload(h);
}

static void test_unload_library(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;
    char errstr[256];
    apr_dso_handle_sym_t func1 = NULL;

    status = apr_dso_load(&h, libname, p);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);
    CuAssertPtrNotNull(tc, h);

    status = apr_dso_unload(h);
    CuAssert(tc, apr_dso_error(h, errstr, 256), APR_SUCCESS == status);

    status = apr_dso_sym(&func1, h, "print_hello");
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_ESYMNOTFOUND(status));
}

#endif /* def(LIB_NAME) */

static void test_load_notthere(CuTest *tc)
{
    apr_dso_handle_t *h = NULL;
    apr_status_t status;

    status = apr_dso_load(&h, "No_File.so", p);

    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EDSOOPEN(status));
    CuAssertPtrNotNull(tc, h);
}    

CuSuite *testdso(void)
{
    CuSuite *suite = CuSuiteNew("DSO");

    modname = apr_pcalloc(p, 256);
    getcwd(modname, 256);
    modname = apr_pstrcat(p, modname, "/", MOD_NAME, NULL);

    SUITE_ADD_TEST(suite, test_load_module);
    SUITE_ADD_TEST(suite, test_dso_sym);
    SUITE_ADD_TEST(suite, test_dso_sym_return_value);
    SUITE_ADD_TEST(suite, test_unload_module);

#ifdef LIB_NAME
    libname = apr_pcalloc(p, 256);
    getcwd(libname, 256);
    libname = apr_pstrcat(p, libname, "/", LIB_NAME, NULL);

    SUITE_ADD_TEST(suite, test_load_library);
    SUITE_ADD_TEST(suite, test_dso_sym_library);
    SUITE_ADD_TEST(suite, test_dso_sym_return_value_library);
    SUITE_ADD_TEST(suite, test_unload_library);
#endif

    SUITE_ADD_TEST(suite, test_load_notthere);

    return suite;
}

