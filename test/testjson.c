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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "apr_json.h"

#include "abts.h"
#include "testutil.h"

static void test_json_identity(abts_case * tc, void *data)
{
    apr_json_value_t *json = NULL;
    apr_json_kv_t *image, *width, *ids, *title,
            *animated, *thumbnail, *height;
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    const char *src;
    char buf[1024];
    apr_size_t len = sizeof(buf);
    apr_off_t offset = 0;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    src = "{"
        "  \"Image\" : {"
        "    \"Width\" : 800 ,"
        "    \"IDs\" : [116, 943, 234, 38793],"
        "    \"Title\" : \"View from 15th Floor\","
        "    \"Animated\" : false,"
        "    \"Thumbnail\" : {"
        "      \"Height\" : 125,"
        "      \"Width\" : 100,"
        "      \"Url\" : \"http://www.example.com/image/481989943\""
        "    },"
        "    \"Height\" : 600 "
        "  }"
        "}";

    apr_json_decode(&json, src, APR_JSON_VALUE_STRING, &offset, APR_JSON_FLAGS_WHITESPACE,
            10, p);
    apr_json_encode(bb, NULL, NULL, json, APR_JSON_FLAGS_WHITESPACE, p);
    apr_brigade_flatten(bb, buf, &len);
    apr_json_decode(&json, buf, len, &offset, APR_JSON_FLAGS_WHITESPACE, 10, p);

    ABTS_STR_NEQUAL(tc, src, buf, len);

    ABTS_INT_EQUAL(tc, len, offset);
    ABTS_INT_EQUAL(tc, APR_JSON_OBJECT, json->type);
    image = apr_hash_get(json->value.object->hash, "Image", 5);
    ABTS_PTR_NOTNULL(tc, image);
    width = apr_hash_get(image->v->value.object->hash, "Width", 5);
    ABTS_PTR_NOTNULL(tc, width);
    ABTS_INT_EQUAL(tc, APR_JSON_LONG, width->v->type);
    ABTS_INT_EQUAL(tc, 800, width->v->value.lnumber);
    ids = apr_hash_get(image->v->value.object->hash, "IDs", 3);
    ABTS_PTR_NOTNULL(tc, ids);
    ABTS_INT_EQUAL(tc, APR_JSON_ARRAY, ids->v->type);
    title = apr_hash_get(image->v->value.object->hash, "Title", 5);
    ABTS_PTR_NOTNULL(tc, title);
    ABTS_INT_EQUAL(tc, APR_JSON_STRING, title->v->type);
    animated = apr_hash_get(image->v->value.object->hash, "Animated", 8);
    ABTS_PTR_NOTNULL(tc, animated);
    ABTS_INT_EQUAL(tc, APR_JSON_BOOLEAN, animated->v->type);
    ABTS_TRUE(tc, !animated->v->value.boolean);
    thumbnail = apr_hash_get(image->v->value.object->hash, "Thumbnail", 9);
    ABTS_PTR_NOTNULL(tc, thumbnail);
    ABTS_INT_EQUAL(tc, APR_JSON_OBJECT, thumbnail->v->type);
    height = apr_hash_get(image->v->value.object->hash, "Height", 6);
    ABTS_PTR_NOTNULL(tc, height);
    ABTS_INT_EQUAL(tc, APR_JSON_LONG, height->v->type);
    ABTS_INT_EQUAL(tc, 600, height->v->value.lnumber);

}

static void test_json_level(abts_case * tc, void *data)
{
    apr_json_value_t *json = NULL;
    apr_status_t status;
    const char *src;
    apr_off_t offset = 0;

    src = "{"
        "\"One\":{"
        "\"Two\":{"
        "\"Three\":{";

    status = apr_json_decode(&json, src, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 2, p);

    ABTS_INT_EQUAL(tc, APR_EINVAL, status);

}

static void test_json_eof(abts_case * tc, void *data)
{
    apr_json_value_t *json = NULL;
    apr_status_t status;
    const char *src;
    apr_off_t offset = 0;

    src = "{"
        "\"One\":{"
        "\"Two\":{"
        "\"Three\":{";

    status = apr_json_decode(&json, src, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    ABTS_INT_EQUAL(tc, APR_EOF, status);

}

static void test_json_string(abts_case * tc, void *data)
{
    apr_json_value_t *json = NULL;
    apr_status_t status;
    const char *src;
    apr_off_t offset = 0;

    /* "턞\"\t/\b\f\n\r\t"; */
    const unsigned char expected[] = {237, 132, 158, 34, 9, 47, 8, 12, 10, 13, 9, 0};

    src = "\"\\uD834\\uDD1E\\\"\\t\\/\\b\\f\\n\\r\\t\"";

    status = apr_json_decode(&json, src, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
    ABTS_INT_EQUAL(tc, APR_JSON_STRING, json->type);

    ABTS_ASSERT(tc, "check for string unescape match",
            (memcmp(expected, json->value.string.p, json->value.string.len) == 0));
}

static void test_json_overlay(abts_case * tc, void *data)
{
    const char *o = "{\"o1\":\"foo\",\"common\":\"bar\",\"o2\":\"baz\"}";
    const char *b = "{\"b1\":\"foo\",\"common\":\"bar\",\"b2\":\"baz\"}";

    apr_json_value_t *res;
    apr_json_value_t *base;
    apr_json_value_t *overlay;

    apr_off_t offset;
    apr_status_t status;

    status = apr_json_decode(&base, b, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    status = apr_json_decode(&overlay, o, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    res = apr_json_overlay(p, overlay, base, APR_JSON_FLAGS_NONE);
    ABTS_INT_EQUAL(tc, 5, apr_hash_count(res->value.object->hash));

    res = apr_json_overlay(p, overlay, base, APR_JSON_FLAGS_STRICT);
    ABTS_ASSERT(tc, "overlay strict should return NULL",
            (res == NULL));

}

static void test_json_object_iterate(abts_case * tc, void *data)
{
    const char *o = "{\"o1\":\"foo\",\"o2\":\"bar\"}";

    apr_json_value_t *val;
    apr_json_kv_t *kv;

    apr_off_t offset;
    apr_status_t status;

    status = apr_json_decode(&val, o, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    kv = apr_json_object_first(val);
    ABTS_PTR_NOTNULL(tc, kv);

    kv = apr_json_object_next(val, kv);
    ABTS_PTR_NOTNULL(tc, kv);

    kv = apr_json_object_next(val, kv);
    ABTS_ASSERT(tc, "object next should return NULL",
            (kv == NULL));

}

static void test_json_array_iterate(abts_case * tc, void *data)
{
    const char *o = "[\"a1\",\"a2\"]";

    apr_json_value_t *arr;
    apr_json_value_t *val;

    apr_off_t offset;
    apr_status_t status;

    status = apr_json_decode(&arr, o, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    val = apr_json_array_first(arr);
    ABTS_PTR_NOTNULL(tc, val);

    val = apr_json_array_next(arr, val);
    ABTS_PTR_NOTNULL(tc, val);

    val = apr_json_array_next(arr, val);
    ABTS_ASSERT(tc, "array next should return NULL",
            (val == NULL));

}

static void test_json_create(abts_case* tc, void* data)
{
    apr_json_value_t *json;
    apr_status_t status;
    apr_bucket_brigade *bb;
    apr_bucket_alloc_t *ba;
    char buf[1024];
    apr_size_t len;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    json = apr_json_object_create(p);
    apr_json_object_set(json, "null", APR_JSON_VALUE_STRING,
                        apr_json_null_create(p), p);

    apr_json_object_set(json, "bool", APR_JSON_VALUE_STRING,
                        apr_json_boolean_create(p, 1), p);

    apr_json_object_set(json, "double", APR_JSON_VALUE_STRING,
                        apr_json_double_create(p, 12.34), p);

    apr_json_object_set(json, "long", APR_JSON_VALUE_STRING,
                        apr_json_long_create(p, 1234), p);

    apr_json_object_set(json, "string", APR_JSON_VALUE_STRING,
                        apr_json_string_create(p, "str",
                                               APR_JSON_VALUE_STRING),
                        p);

    status = apr_json_encode(bb, NULL, NULL, json,
                             APR_JSON_FLAGS_WHITESPACE, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    len = sizeof(buf);
    status = apr_brigade_flatten(bb, buf, &len);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
    buf[len] = 0;

    ABTS_STR_EQUAL(tc,
                   "{\"null\":null,\"bool\":true,\"double\":12.340000,"
                   "\"long\":1234,\"string\":\"str\"}",
                   buf);
}

abts_suite *testjson(abts_suite * suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_json_identity, NULL);
    abts_run_test(suite, test_json_level, NULL);
    abts_run_test(suite, test_json_eof, NULL);
    abts_run_test(suite, test_json_string, NULL);
    abts_run_test(suite, test_json_overlay, NULL);
    abts_run_test(suite, test_json_object_iterate, NULL);
    abts_run_test(suite, test_json_array_iterate, NULL);
    abts_run_test(suite, test_json_create, NULL);

    return suite;
}
