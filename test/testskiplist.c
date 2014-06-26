/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
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
#include "apr_skiplist.h"
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif

static void add_int_to_skiplist(apr_skiplist *list, int n){
    int* a = apr_skiplist_alloc(list, sizeof(int));
    *a = n;
    apr_skiplist_insert(list, a);
}

static int comp(void *a, void *b){
    return *((int*) a) - *((int*) b);
}


static int compk(void *a, void *b){
    return comp(a, b);
}

static void skiplist_test(abts_case *tc, void *data) {
    int test_elems = 10;
    int i = 0, j = 0;
    int *val = NULL;
    apr_skiplist * list = NULL;
    apr_pool_t *p;

    apr_pool_create(&p, NULL);
    apr_skiplist_init(&list, p);
    apr_skiplist_set_compare(list, comp, compk);
    
    /* insert 10 objects */
    for (i = 0; i < test_elems; ++i){
        add_int_to_skiplist(list, i);
    }

    /* remove all objects */
    while (val = apr_skiplist_pop(list, NULL)){
        ABTS_INT_EQUAL(tc, *val, j++);
    }

    /* insert 10 objects again */
    for (i = test_elems; i < test_elems+test_elems; ++i){
        add_int_to_skiplist(list, i);
    }

    j = test_elems;
    while (val = apr_skiplist_pop(list, NULL)){
        ABTS_INT_EQUAL(tc, *val, j++);
    }

    /* empty */
    val = apr_skiplist_pop(list, NULL);
    ABTS_PTR_EQUAL(tc, val, NULL);

    add_int_to_skiplist(list, 42);
    val = apr_skiplist_pop(list, NULL);
    ABTS_INT_EQUAL(tc, *val, 42); 

    /* empty */
    val = apr_skiplist_pop(list, NULL);
    ABTS_PTR_EQUAL(tc, val, NULL);
}

abts_suite *testskiplist(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, skiplist_test, NULL);

    return suite;
}

