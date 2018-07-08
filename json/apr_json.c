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

apr_json_value_t *apr_json_value_create(apr_pool_t *pool)
{
    return apr_pcalloc(pool, sizeof(apr_json_value_t));
}

apr_json_object_t *apr_json_object_create(apr_pool_t *pool)
{
    apr_json_object_t *object = apr_pcalloc(pool,
            sizeof(apr_json_object_t));
    APR_RING_INIT(&object->list, apr_json_kv_t, link);
    object->hash = apr_hash_make(pool);

    return object;
}

void apr_json_object_set(apr_json_object_t *object, apr_json_value_t *key,
        apr_json_value_t *val, apr_pool_t *pool)
{
    apr_json_kv_t *kv;

    kv = apr_hash_get(object->hash, key->value.string.p, key->value.string.len);

    if (!val) {
        if (kv) {
            apr_hash_set(object->hash, key->value.string.p, key->value.string.len,
                    NULL);
            APR_RING_REMOVE((kv), link);
        }
        return;
    }

    if (!kv) {
        kv = apr_palloc(pool, sizeof(apr_json_kv_t));
        APR_RING_ELEM_INIT(kv, link);
        APR_JSON_OBJECT_INSERT_TAIL(object, kv);
        apr_hash_set(object->hash, key->value.string.p, key->value.string.len,
                kv);
    }

    kv->k = key;
    kv->v = val;
}

apr_json_kv_t *apr_json_object_get(apr_json_object_t *object, const char *key)
{
    return apr_hash_get(object->hash, key, APR_HASH_KEY_STRING);
}
