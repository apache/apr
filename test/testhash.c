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
#include "apr.h"
#include "apr_strings.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_hash.h"

static void dump_hash(apr_pool_t *p, apr_hash_t *h, char *str) 
{
    apr_hash_index_t *hi;
    char *val, *key;
    apr_ssize_t len;
    int i = 0;

    str[0] = '\0';

    for (hi = apr_hash_first(p, h); hi; hi = apr_hash_next(hi)) {
        apr_hash_this(hi,(void*) &key, &len, (void*) &val);
        apr_snprintf(str, 8196, "%sKey %s (%" APR_SSIZE_T_FMT ") Value %s\n", 
                     str, key, len, val);
        i++;
    }
    apr_snprintf(str, 8196, "%s#entries %d\n", str, i);
}

static void sum_hash(apr_pool_t *p, apr_hash_t *h, int *pcount, int *keySum, int *valSum) 
{
    apr_hash_index_t *hi;
    void *val, *key;
    int count = 0;

    *keySum = 0;
    *valSum = 0;
    *pcount = 0;
    for (hi = apr_hash_first(p, h); hi; hi = apr_hash_next(hi)) {
        apr_hash_this(hi, (void*)&key, NULL, &val);
        *valSum += *(int *)val;
        *keySum += *(int *)key;
        count++;
    }
    *pcount=count;
}

static void hash_make(CuTest *tc)
{
    apr_hash_t *h = NULL;

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);
}

static void hash_set(CuTest *tc)
{
    apr_hash_t *h = NULL;
    char *result = NULL;

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    apr_hash_set(h, "key", APR_HASH_KEY_STRING, "value");
    result = apr_hash_get(h, "key", APR_HASH_KEY_STRING);
    CuAssertStrEquals(tc, "value", result);
}

static void hash_reset(CuTest *tc)
{
    apr_hash_t *h = NULL;
    char *result = NULL;

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    apr_hash_set(h, "key", APR_HASH_KEY_STRING, "value");
    result = apr_hash_get(h, "key", APR_HASH_KEY_STRING);
    CuAssertStrEquals(tc, "value", result);

    apr_hash_set(h, "key", APR_HASH_KEY_STRING, "new");
    result = apr_hash_get(h, "key", APR_HASH_KEY_STRING);
    CuAssertStrEquals(tc, "new", result);
}

static void same_value(CuTest *tc)
{
    apr_hash_t *h = NULL;
    char *result = NULL;

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    apr_hash_set(h, "same1", APR_HASH_KEY_STRING, "same");
    result = apr_hash_get(h, "same1", APR_HASH_KEY_STRING);
    CuAssertStrEquals(tc, "same", result);

    apr_hash_set(h, "same2", APR_HASH_KEY_STRING, "same");
    result = apr_hash_get(h, "same2", APR_HASH_KEY_STRING);
    CuAssertStrEquals(tc, "same", result);
}

static void key_space(CuTest *tc)
{
    apr_hash_t *h = NULL;
    char *result = NULL;

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    apr_hash_set(h, "key with space", APR_HASH_KEY_STRING, "value");
    result = apr_hash_get(h, "key with space", APR_HASH_KEY_STRING);
    CuAssertStrEquals(tc, "value", result);
}

/* This is kind of a hack, but I am just keeping an existing test.  This is
 * really testing apr_hash_first, apr_hash_next, and apr_hash_this which 
 * should be tested in three separate tests, but this will do for now.
 */
static void hash_traverse(CuTest *tc)
{
    apr_hash_t *h;
    char str[8196];

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    apr_hash_set(h, "OVERWRITE", APR_HASH_KEY_STRING, "should not see this");
    apr_hash_set(h, "FOO3", APR_HASH_KEY_STRING, "bar3");
    apr_hash_set(h, "FOO3", APR_HASH_KEY_STRING, "bar3");
    apr_hash_set(h, "FOO1", APR_HASH_KEY_STRING, "bar1");
    apr_hash_set(h, "FOO2", APR_HASH_KEY_STRING, "bar2");
    apr_hash_set(h, "FOO4", APR_HASH_KEY_STRING, "bar4");
    apr_hash_set(h, "SAME1", APR_HASH_KEY_STRING, "same");
    apr_hash_set(h, "SAME2", APR_HASH_KEY_STRING, "same");
    apr_hash_set(h, "OVERWRITE", APR_HASH_KEY_STRING, "Overwrite key");

    dump_hash(p, h, str);
    CuAssertStrEquals(tc, "Key FOO1 (4) Value bar1\n"
                          "Key FOO2 (4) Value bar2\n"
                          "Key OVERWRITE (9) Value Overwrite key\n"
                          "Key FOO3 (4) Value bar3\n"
                          "Key SAME1 (5) Value same\n"
                          "Key FOO4 (4) Value bar4\n"
                          "Key SAME2 (5) Value same\n"
                          "#entries 7\n", str);
}

/* This is kind of a hack, but I am just keeping an existing test.  This is
 * really testing apr_hash_first, apr_hash_next, and apr_hash_this which 
 * should be tested in three separate tests, but this will do for now.
 */
static void summation_test(CuTest *tc)
{
    apr_hash_t *h;
    int sumKeys, sumVal, trySumKey, trySumVal;
    int i, j, *val, *key;

    h =apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    sumKeys = 0;
    sumVal = 0;
    trySumKey = 0;
    trySumVal = 0;

    for (i = 0; i < 100; i++) {
        j = i * 10 + 1;
        sumKeys += j;
        sumVal += i;
        key = apr_palloc(p, sizeof(int));
        *key = j;
        val = apr_palloc(p, sizeof(int));
        *val = i;
        apr_hash_set(h, key, sizeof(int), val);
    }

    sum_hash(p, h, &i, &trySumKey, &trySumVal);
    CuAssertIntEquals(tc, 100, i);
    CuAssertIntEquals(tc, sumVal, trySumVal);
    CuAssertIntEquals(tc, sumKeys, trySumKey);
}

static void delete_key(CuTest *tc)
{
    apr_hash_t *h = NULL;
    char *result = NULL;

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    apr_hash_set(h, "key", APR_HASH_KEY_STRING, "value");
    apr_hash_set(h, "key2", APR_HASH_KEY_STRING, "value2");

    result = apr_hash_get(h, "key", APR_HASH_KEY_STRING);
    CuAssertStrEquals(tc, "value", result);

    result = apr_hash_get(h, "key2", APR_HASH_KEY_STRING);
    CuAssertStrEquals(tc, "value2", result);

    apr_hash_set(h, "key", APR_HASH_KEY_STRING, NULL);

    result = apr_hash_get(h, "key", APR_HASH_KEY_STRING);
    CuAssertPtrEquals(tc, NULL, result);

    result = apr_hash_get(h, "key2", APR_HASH_KEY_STRING);
    CuAssertStrEquals(tc, "value2", result);
}

static void hash_count_0(CuTest *tc)
{
    apr_hash_t *h = NULL;
    int count;

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    count = apr_hash_count(h);
    CuAssertIntEquals(tc, 0, count);
}

static void hash_count_1(CuTest *tc)
{
    apr_hash_t *h = NULL;
    int count;

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    apr_hash_set(h, "key", APR_HASH_KEY_STRING, "value");

    count = apr_hash_count(h);
    CuAssertIntEquals(tc, 1, count);
}

static void hash_count_5(CuTest *tc)
{
    apr_hash_t *h = NULL;
    int count;

    h = apr_hash_make(p);
    CuAssertPtrNotNull(tc, h);

    apr_hash_set(h, "key1", APR_HASH_KEY_STRING, "value1");
    apr_hash_set(h, "key2", APR_HASH_KEY_STRING, "value2");
    apr_hash_set(h, "key3", APR_HASH_KEY_STRING, "value3");
    apr_hash_set(h, "key4", APR_HASH_KEY_STRING, "value4");
    apr_hash_set(h, "key5", APR_HASH_KEY_STRING, "value5");

    count = apr_hash_count(h);
    CuAssertIntEquals(tc, 5, count);
}

static void overlay_empty(CuTest *tc)
{
    apr_hash_t *base = NULL;
    apr_hash_t *overlay = NULL;
    apr_hash_t *result = NULL;
    int count;
    char str[8196];

    base = apr_hash_make(p);
    overlay = apr_hash_make(p);
    CuAssertPtrNotNull(tc, base);
    CuAssertPtrNotNull(tc, overlay);

    apr_hash_set(base, "key1", APR_HASH_KEY_STRING, "value1");
    apr_hash_set(base, "key2", APR_HASH_KEY_STRING, "value2");
    apr_hash_set(base, "key3", APR_HASH_KEY_STRING, "value3");
    apr_hash_set(base, "key4", APR_HASH_KEY_STRING, "value4");
    apr_hash_set(base, "key5", APR_HASH_KEY_STRING, "value5");

    result = apr_hash_overlay(p, overlay, base);

    count = apr_hash_count(result);
    CuAssertIntEquals(tc, 5, count);

    dump_hash(p, result, str);
    CuAssertStrEquals(tc, "Key key1 (4) Value value1\n"
                          "Key key2 (4) Value value2\n"
                          "Key key3 (4) Value value3\n"
                          "Key key4 (4) Value value4\n"
                          "Key key5 (4) Value value5\n"
                          "#entries 5\n", str);
}

static void overlay_2unique(CuTest *tc)
{
    apr_hash_t *base = NULL;
    apr_hash_t *overlay = NULL;
    apr_hash_t *result = NULL;
    int count;
    char str[8196];

    base = apr_hash_make(p);
    overlay = apr_hash_make(p);
    CuAssertPtrNotNull(tc, base);
    CuAssertPtrNotNull(tc, overlay);

    apr_hash_set(base, "base1", APR_HASH_KEY_STRING, "value1");
    apr_hash_set(base, "base2", APR_HASH_KEY_STRING, "value2");
    apr_hash_set(base, "base3", APR_HASH_KEY_STRING, "value3");
    apr_hash_set(base, "base4", APR_HASH_KEY_STRING, "value4");
    apr_hash_set(base, "base5", APR_HASH_KEY_STRING, "value5");

    apr_hash_set(overlay, "overlay1", APR_HASH_KEY_STRING, "value1");
    apr_hash_set(overlay, "overlay2", APR_HASH_KEY_STRING, "value2");
    apr_hash_set(overlay, "overlay3", APR_HASH_KEY_STRING, "value3");
    apr_hash_set(overlay, "overlay4", APR_HASH_KEY_STRING, "value4");
    apr_hash_set(overlay, "overlay5", APR_HASH_KEY_STRING, "value5");

    result = apr_hash_overlay(p, overlay, base);

    count = apr_hash_count(result);
    CuAssertIntEquals(tc, 10, count);

    dump_hash(p, result, str);
    /* I don't know why these are out of order, but they are.  I would probably
     * consider this a bug, but others should comment.
     */
    CuAssertStrEquals(tc, "Key base5 (5) Value value5\n"
                          "Key overlay1 (8) Value value1\n"
                          "Key overlay2 (8) Value value2\n"
                          "Key overlay3 (8) Value value3\n"
                          "Key overlay4 (8) Value value4\n"
                          "Key overlay5 (8) Value value5\n"
                          "Key base1 (5) Value value1\n"
                          "Key base2 (5) Value value2\n"
                          "Key base3 (5) Value value3\n"
                          "Key base4 (5) Value value4\n"
                          "#entries 10\n", str);
}

static void overlay_same(CuTest *tc)
{
    apr_hash_t *base = NULL;
    apr_hash_t *result = NULL;
    int count;
    char str[8196];

    base = apr_hash_make(p);
    CuAssertPtrNotNull(tc, base);

    apr_hash_set(base, "base1", APR_HASH_KEY_STRING, "value1");
    apr_hash_set(base, "base2", APR_HASH_KEY_STRING, "value2");
    apr_hash_set(base, "base3", APR_HASH_KEY_STRING, "value3");
    apr_hash_set(base, "base4", APR_HASH_KEY_STRING, "value4");
    apr_hash_set(base, "base5", APR_HASH_KEY_STRING, "value5");

    result = apr_hash_overlay(p, base, base);

    count = apr_hash_count(result);
    CuAssertIntEquals(tc, 5, count);

    dump_hash(p, result, str);
    /* I don't know why these are out of order, but they are.  I would probably
     * consider this a bug, but others should comment.
     */
    CuAssertStrEquals(tc, "Key base5 (5) Value value5\n"
                          "Key base1 (5) Value value1\n"
                          "Key base2 (5) Value value2\n"
                          "Key base3 (5) Value value3\n"
                          "Key base4 (5) Value value4\n"
                          "#entries 5\n", str);
}
        
CuSuite *testhash(void)
{
    CuSuite *suite = CuSuiteNew("Hash");

    SUITE_ADD_TEST(suite, hash_make);
    SUITE_ADD_TEST(suite, hash_set);
    SUITE_ADD_TEST(suite, hash_reset);
    SUITE_ADD_TEST(suite, same_value);
    SUITE_ADD_TEST(suite, key_space);
    SUITE_ADD_TEST(suite, delete_key);

    SUITE_ADD_TEST(suite, hash_count_0);
    SUITE_ADD_TEST(suite, hash_count_1);
    SUITE_ADD_TEST(suite, hash_count_5);

    SUITE_ADD_TEST(suite, hash_traverse);
    SUITE_ADD_TEST(suite, summation_test);

    SUITE_ADD_TEST(suite, overlay_empty);
    SUITE_ADD_TEST(suite, overlay_2unique);
    SUITE_ADD_TEST(suite, overlay_same);

    return suite;
}

