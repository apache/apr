/* Copyright 2002-2004 The Apache Software Foundation
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

#include "testutil.h"
#include "apr.h"
#include "apr_strings.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_tables.h"
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif

static apr_table_t *t1 = NULL;

static void table_make(abts_case *tc, void *data)
{
    t1 = apr_table_make(p, 5);
    abts_ptr_notnull(tc, t1);
}

static void table_get(abts_case *tc, void *data)
{
    const char *val;

    apr_table_set(t1, "foo", "bar");
    val = apr_table_get(t1, "foo");
    abts_str_equal(tc, val, "bar");
}

static void table_set(abts_case *tc, void *data)
{
    const char *val;

    apr_table_set(t1, "setkey", "bar");
    apr_table_set(t1, "setkey", "2ndtry");
    val = apr_table_get(t1, "setkey");
    abts_str_equal(tc, val, "2ndtry");
}

static void table_getnotthere(abts_case *tc, void *data)
{
    const char *val;

    val = apr_table_get(t1, "keynotthere");
    abts_ptr_equal(tc, NULL, (void *)val);
}

static void table_add(abts_case *tc, void *data)
{
    const char *val;

    apr_table_add(t1, "addkey", "bar");
    apr_table_add(t1, "addkey", "foo");
    val = apr_table_get(t1, "addkey");
    abts_str_equal(tc, val, "bar");

}

static void table_nelts(abts_case *tc, void *data)
{
    const char *val;
    apr_table_t *t = apr_table_make(p, 1);

    apr_table_set(t, "abc", "def");
    apr_table_set(t, "def", "abc");
    apr_table_set(t, "foo", "zzz");
    val = apr_table_get(t, "foo");
    abts_str_equal(tc, val, "zzz");
    val = apr_table_get(t, "abc");
    abts_str_equal(tc, val, "def");
    val = apr_table_get(t, "def");
    abts_str_equal(tc, val, "abc");
    abts_int_equal(tc, 3, apr_table_elts(t)->nelts);
}

static void table_clear(abts_case *tc, void *data)
{
    apr_table_clear(t1);
    abts_int_equal(tc, 0, apr_table_elts(t1)->nelts);
}

static void table_unset(abts_case *tc, void *data)
{
    const char *val;
    apr_table_t *t = apr_table_make(p, 1);

    apr_table_set(t, "a", "1");
    apr_table_set(t, "b", "2");
    apr_table_unset(t, "b");
    abts_int_equal(tc, 1, apr_table_elts(t)->nelts);
    val = apr_table_get(t, "a");
    abts_str_equal(tc, val, "1");
    val = apr_table_get(t, "b");
    abts_ptr_equal(tc, (void *)val, (void *)NULL);
}

static void table_overlap(abts_case *tc, void *data)
{
    const char *val;
    apr_table_t *t1 = apr_table_make(p, 1);
    apr_table_t *t2 = apr_table_make(p, 1);

    apr_table_addn(t1, "a", "0");
    apr_table_addn(t1, "g", "7");
    apr_table_addn(t2, "a", "1");
    apr_table_addn(t2, "b", "2");
    apr_table_addn(t2, "c", "3");
    apr_table_addn(t2, "b", "2.0");
    apr_table_addn(t2, "d", "4");
    apr_table_addn(t2, "e", "5");
    apr_table_addn(t2, "b", "2.");
    apr_table_addn(t2, "f", "6");
    apr_table_overlap(t1, t2, APR_OVERLAP_TABLES_SET);
    
    abts_int_equal(tc, apr_table_elts(t1)->nelts, 7);
    val = apr_table_get(t1, "a");
    abts_str_equal(tc, val, "1");
    val = apr_table_get(t1, "b");
    abts_str_equal(tc, val, "2.");
    val = apr_table_get(t1, "c");
    abts_str_equal(tc, val, "3");
    val = apr_table_get(t1, "d");
    abts_str_equal(tc, val, "4");
    val = apr_table_get(t1, "e");
    abts_str_equal(tc, val, "5");
    val = apr_table_get(t1, "f");
    abts_str_equal(tc, val, "6");
    val = apr_table_get(t1, "g");
    abts_str_equal(tc, val, "7");
}

abts_suite *testtable(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, table_make, NULL);
    abts_run_test(suite, table_get, NULL);
    abts_run_test(suite, table_set, NULL);
    abts_run_test(suite, table_getnotthere, NULL);
    abts_run_test(suite, table_add, NULL);
    abts_run_test(suite, table_nelts, NULL);
    abts_run_test(suite, table_clear, NULL);
    abts_run_test(suite, table_unset, NULL);
    abts_run_test(suite, table_overlap, NULL);

    return suite;
}

