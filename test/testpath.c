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
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_tables.h"

#if defined(WIN32) || defined(NETWARE) || defined(OS2)
#define PSEP ";"
#define DSEP "\\"
#else
#define PSEP ":"
#define DSEP "/"
#endif

#define PX ""
#define P1 "first path"
#define P2 "second" DSEP "path"
#define P3 "th ird" DSEP "path"
#define P4 "fourth" DSEP "pa th"
#define P5 "fifthpath"

static const char *parts_in[] = { P1, P2, P3, PX, P4, P5 };
static const char *path_in = P1 PSEP P2 PSEP P3 PSEP PX PSEP P4 PSEP P5;
static const int  parts_in_count = sizeof(parts_in)/sizeof(*parts_in);

static const char *parts_out[] = { P1, P2, P3, P4, P5 };
static const char *path_out = P1 PSEP P2 PSEP P3 PSEP P4 PSEP P5;
static const int  parts_out_count = sizeof(parts_out)/sizeof(*parts_out);

static void list_split_multi(CuTest *tc)
{
    int i;
    apr_status_t rv;
    apr_array_header_t *pathelts;

    pathelts = NULL;
    rv = apr_filepath_list_split(&pathelts, path_in, p);
    CuAssertPtrNotNull(tc, pathelts);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, parts_out_count, pathelts->nelts);
    for (i = 0; i < pathelts->nelts; ++i)
        CuAssertStrEquals(tc, parts_out[i], ((char**)pathelts->elts)[i]);
}

static void list_split_single(CuTest *tc)
{
    int i;
    apr_status_t rv;
    apr_array_header_t *pathelts;

    for (i = 0; i < parts_in_count; ++i)
    {
        pathelts = NULL;
        rv = apr_filepath_list_split(&pathelts, parts_in[i], p);
        CuAssertPtrNotNull(tc, pathelts);
        CuAssertIntEquals(tc, APR_SUCCESS, rv);
        if (parts_in[i][0] == '\0')
            CuAssertIntEquals(tc, 0, pathelts->nelts);
        else
        {
            CuAssertIntEquals(tc, 1, pathelts->nelts);
            CuAssertStrEquals(tc, parts_in[i], *(char**)pathelts->elts);
        }
    }
}

static void list_merge_multi(CuTest *tc)
{
    int i;
    char *liststr;
    apr_status_t rv;
    apr_array_header_t *pathelts;

    pathelts = apr_array_make(p, parts_in_count, sizeof(const char*));
    for (i = 0; i < parts_in_count; ++i)
        *(const char**)apr_array_push(pathelts) = parts_in[i];

    liststr = NULL;
    rv = apr_filepath_list_merge(&liststr, pathelts, p);
    CuAssertPtrNotNull(tc, liststr);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertStrEquals(tc, liststr, path_out);
}

static void list_merge_single(CuTest *tc)
{
    int i;
    char *liststr;
    apr_status_t rv;
    apr_array_header_t *pathelts;

    pathelts = apr_array_make(p, 1, sizeof(const char*));
    apr_array_push(pathelts);
    for (i = 0; i < parts_in_count; ++i)
    {
        *(const char**)pathelts->elts = parts_in[i];
        liststr = NULL;
        rv = apr_filepath_list_merge(&liststr, pathelts, p);
        if (parts_in[i][0] == '\0')
            CuAssertPtrEquals(tc, NULL, liststr);
        else
        {
            CuAssertPtrNotNull(tc, liststr);
            CuAssertIntEquals(tc, APR_SUCCESS, rv);
            CuAssertStrEquals(tc, liststr, parts_in[i]);
        }
    }
}


CuSuite *testpath(void)
{
    CuSuite *suite = CuSuiteNew("Path lists");

    SUITE_ADD_TEST(suite, list_split_multi);
    SUITE_ADD_TEST(suite, list_split_single);
    SUITE_ADD_TEST(suite, list_merge_multi);
    SUITE_ADD_TEST(suite, list_merge_single);

    return suite;
}

