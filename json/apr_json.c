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

#include <ctype.h>
#include <stdlib.h>

#include "apr_json.h"

#define APR_JSON_OBJECT_INSERT_TAIL(o, e) do {                              \
        apr_json_kv_t *ap__b = (e);                                        \
        APR_RING_INSERT_TAIL(&(o)->list, ap__b, apr_json_kv_t, link);      \
        APR_RING_CHECK_CONSISTENCY(&(o)->list, apr_json_kv_t, link);                             \
    } while (0)

#define APR_JSON_ARRAY_INSERT_TAIL(o, e) do {                              \
        apr_json_value_t *ap__b = (e);                                        \
        APR_RING_INSERT_TAIL(&(o)->list, ap__b, apr_json_value_t, link);      \
        APR_RING_CHECK_CONSISTENCY(&(o)->list, apr_json_value_t, link);                             \
    } while (0)

apr_json_value_t *apr_json_value_create(apr_pool_t *pool)
{
    return apr_pcalloc(pool, sizeof(apr_json_value_t));
}

apr_json_value_t *apr_json_object_create(apr_pool_t *pool)
{
    apr_json_object_t *object;

    apr_json_value_t *json = apr_json_value_create(pool);

    json->type = APR_JSON_OBJECT;
    json->value.object = object = apr_pcalloc(pool, sizeof(apr_json_object_t));
    APR_RING_INIT(&object->list, apr_json_kv_t, link);
    object->hash = apr_hash_make(pool);

    return json;
}

apr_json_value_t *apr_json_string_create(apr_pool_t *pool, const char *val,
        apr_ssize_t len) {
    apr_json_value_t *json = apr_json_value_create(pool);

    if (json) {
        if (val) {
            json->type = APR_JSON_STRING;
            json->value.string.p = val;
            json->value.string.len = len;
        } else {
            json->type = APR_JSON_NULL;
        }
    }

    return json;
}

apr_json_value_t *apr_json_array_create(apr_pool_t *pool, int nelts)
{
    apr_json_value_t *json = apr_json_value_create(pool);

    if (json) {
        json->type = APR_JSON_ARRAY;
        json->value.array = apr_pcalloc(pool, sizeof(apr_json_array_t));
        APR_RING_INIT(&json->value.array->list, apr_json_value_t, link);
        json->value.array->array = apr_array_make(pool, nelts,
                sizeof(apr_json_value_t *));
    }

    return json;
}

apr_json_value_t *apr_json_long_create(apr_pool_t *pool, apr_int64_t lnumber)
{
    apr_json_value_t *json = apr_json_value_create(pool);

    if (json) {
        json->type = APR_JSON_LONG;
        json->value.lnumber = lnumber;
    }

    return json;
}

apr_json_value_t *apr_json_double_create(apr_pool_t *pool, double dnumber)
{
    apr_json_value_t *json = apr_json_value_create(pool);

    if (json) {
        json->type = APR_JSON_DOUBLE;
        json->value.dnumber = dnumber;
    }

    return json;
}

apr_json_value_t *apr_json_boolean_create(apr_pool_t *pool, int boolean)
{
    apr_json_value_t *json = apr_json_value_create(pool);

    if (json) {
        json->type = APR_JSON_BOOLEAN;
        json->value.boolean = boolean;
    }

    return json;
}

apr_json_value_t *apr_json_null_create(apr_pool_t *pool)
{
    apr_json_value_t *json = apr_json_value_create(pool);

    if (json) {
        json->type = APR_JSON_NULL;
    }

    return json;
}

apr_status_t apr_json_object_set(apr_json_value_t *object,
        const char *key, apr_ssize_t klen, apr_json_value_t *val,
        apr_pool_t *pool)
{
    apr_json_kv_t *kv;
    apr_hash_t *hash;

    if (object->type != APR_JSON_OBJECT) {
        return APR_EINVAL;
    }

    if (klen == APR_JSON_VALUE_STRING) {
        klen = strlen(key);
    }

    hash = object->value.object->hash;

    kv = apr_hash_get(hash, key, klen);

    if (!val) {
        if (kv) {
            apr_hash_set(hash, key, klen, NULL);
            APR_RING_REMOVE((kv), link);
        }
        return APR_SUCCESS;
    }

    if (!kv) {
        kv = apr_palloc(pool, sizeof(apr_json_kv_t));
        APR_RING_ELEM_INIT(kv, link);
        APR_JSON_OBJECT_INSERT_TAIL(object->value.object, kv);
        apr_hash_set(hash, key, klen,
                kv);
    }

    kv->k = apr_json_string_create(pool, key, klen);
    kv->v = val;

    return APR_SUCCESS;
}

apr_status_t apr_json_object_set_ex(apr_json_value_t *object,
        apr_json_value_t *key, apr_json_value_t *val,
        apr_pool_t *pool)
{
    apr_json_kv_t *kv;
    apr_hash_t *hash;

    if (object->type != APR_JSON_OBJECT
            || key->type != APR_JSON_STRING) {
        return APR_EINVAL;
    }

    hash = object->value.object->hash;

    kv = apr_hash_get(hash, key->value.string.p, key->value.string.len);

    if (!val) {
        if (kv) {
            apr_hash_set(hash, key->value.string.p, key->value.string.len,
                    NULL);
            APR_RING_REMOVE((kv), link);
        }
        return APR_SUCCESS;
    }

    if (!kv) {
        kv = apr_palloc(pool, sizeof(apr_json_kv_t));
        APR_RING_ELEM_INIT(kv, link);
        APR_JSON_OBJECT_INSERT_TAIL(object->value.object, kv);
        apr_hash_set(hash, key->value.string.p, key->value.string.len,
                kv);
    }

    kv->k = key;
    kv->v = val;

    return APR_SUCCESS;
}

apr_json_kv_t *apr_json_object_get(apr_json_value_t *object, const char *key, apr_ssize_t klen)
{
    if (object->type != APR_JSON_OBJECT) {
        return NULL;
    }

    return apr_hash_get(object->value.object->hash, key, klen);
}

apr_json_kv_t *apr_json_object_first(apr_json_value_t *obj)
{
    apr_json_kv_t *kv;

    if (obj->type != APR_JSON_OBJECT) {
        return NULL;
    }

    kv = APR_RING_FIRST(&(obj->value.object)->list);

    if (kv != APR_RING_SENTINEL(&(obj->value.object)->list, apr_json_kv_t, link)) {
        return kv;
    }
    else {
        return NULL;
    }
}

apr_json_kv_t *apr_json_object_next(apr_json_value_t *obj, apr_json_kv_t *kv)
{
    apr_json_kv_t *next;

    if (obj->type != APR_JSON_OBJECT) {
        return NULL;
    }

    next = APR_RING_NEXT((kv), link);

    if (next != APR_RING_SENTINEL(&(obj->value.object)->list, apr_json_kv_t, link)) {
        return next;
    }
    else {
        return NULL;
    }
}

apr_status_t apr_json_array_add(apr_json_value_t *arr,
        apr_json_value_t *val)
{
    apr_array_header_t *array;

    if (arr->type != APR_JSON_ARRAY) {
        return APR_EINVAL;
    }

    APR_RING_ELEM_INIT(val, link);
    APR_JSON_ARRAY_INSERT_TAIL(arr->value.array, val);

    array = arr->value.array->array;
    if (array) {
        *((apr_json_value_t **) (apr_array_push(array))) = val;
    }

    return APR_SUCCESS;
}

apr_json_value_t *apr_json_array_get(apr_json_value_t *arr, int index)
{
    if (arr->type != APR_JSON_ARRAY) {
        return NULL;
    }

    return APR_ARRAY_IDX(arr->value.array->array, index, apr_json_value_t *);
}

apr_json_value_t *apr_json_array_first(const apr_json_value_t *arr)
{
    apr_json_value_t *val;

    if (arr->type != APR_JSON_ARRAY) {
        return NULL;
    }

    val = APR_RING_FIRST(&(arr->value.array)->list);

    if (val
            != APR_RING_SENTINEL(&(arr->value.object)->list, apr_json_value_t,
                    link)) {
        return val;
    } else {
        return NULL;
    }
}

apr_json_value_t *apr_json_array_next(const apr_json_value_t *arr,
        const apr_json_value_t *val)
{
    apr_json_value_t *next;

    if (arr->type != APR_JSON_ARRAY) {
        return NULL;
    }

    next = APR_RING_NEXT((val), link);

    if (next
            != APR_RING_SENTINEL(&(arr->value.array)->list, apr_json_value_t,
                    link)) {
        return next;
    } else {
        return NULL;
    }
}

apr_json_value_t *apr_json_overlay(apr_pool_t *p,
        apr_json_value_t *overlay, apr_json_value_t *base,
        int flags)
{
    apr_json_value_t *res;
    apr_json_kv_t *kv;
    int oc, bc;

    if (!base || base->type != APR_JSON_OBJECT) {
        return overlay;
    }
    if (!overlay) {
        return base;
    }
    if (overlay->type != APR_JSON_OBJECT) {
        return overlay;
    }

    oc = apr_hash_count(overlay->value.object->hash);
    if (!oc) {
        return base;
    }
    bc = apr_hash_count(base->value.object->hash);
    if (!bc) {
        return overlay;
    }

    res = apr_json_object_create(p);

    for (kv = APR_RING_FIRST(&(base->value.object)->list);
         kv != APR_RING_SENTINEL(&(base->value.object)->list, apr_json_kv_t, link);
         kv = APR_RING_NEXT((kv), link)) {

        if (!apr_hash_get(overlay->value.object->hash, kv->k->value.string.p,
                kv->k->value.string.len)) {

            apr_json_object_set_ex(res, kv->k, kv->v, p);

        }
        else if (APR_JSON_FLAGS_STRICT & flags) {
            return NULL;
        }

    }

    for (kv = APR_RING_FIRST(&(overlay->value.object)->list);
         kv != APR_RING_SENTINEL(&(overlay->value.object)->list, apr_json_kv_t, link);
         kv = APR_RING_NEXT((kv), link)) {

        apr_json_object_set_ex(res, kv->k, kv->v, p);

    }

    return res;
}
