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
/**
 * @file apr_json.h
 * @brief APR-UTIL JSON Library
 */
#ifndef APR_JSON_H
#define APR_JSON_H

/**
 * @defgroup APR_Util_JSON JSON Encoding and Decoding
 * @ingroup APR_Util
 * @{
 */
#include "apr.h"
#include "apr_pools.h"
#include "apr_tables.h"
#include "apr_hash.h"
#include "apr_strings.h"
#include "apr_buckets.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @package Apache JSON library
 *
 * RFC8259 compliant JSON encoding and decoding library.
 *
 * https://tools.ietf.org/html/rfc8259
 *
 * This API generates UTF-8 encoded JSON, and writes it to the
 * bucket brigade specified. All strings are verified as valid UTF-8
 * before processing, with invalid UTF-8 characters replaced.
 *
 * This API parses UTF-8 encoded JSON, and returns the result as
 * a set of structures. All JSON strings are unescaped. Any bad
 * characters or formatting will cause parsing to be terminated
 * and an error returned, along with the offset of the error.
 *
 * Whitespace may be optionally preserved or ignored as required
 * during generation and parsing.
 *
 * The ordering of object keys is preserved, allowing the decode and
 * encode process to reproduce an identical result. This maintains
 * stable behaviour during unit tests.
 */

/**
 * When passing a string to one of the encode functions, this value can be
 * passed to indicate a string-valued key, and have the length computed
 * automatically.
 */
#define APR_JSON_VALUE_STRING      (-1)

/**
 * Flag indicating no special processing.
 */
#define APR_JSON_FLAGS_NONE 0

/**
 * Flag indicating include whitespace.
 */
#define APR_JSON_FLAGS_WHITESPACE 1

/**
 * A structure to hold a JSON object.
 */
typedef struct apr_json_object_t apr_json_object_t;

/**
 * Enum that represents the type of the given JSON value.
 */
typedef enum apr_json_type_e {
    APR_JSON_OBJECT,
    APR_JSON_ARRAY,
    APR_JSON_STRING,
    APR_JSON_LONG,
    APR_JSON_DOUBLE,
    APR_JSON_BOOLEAN,
    APR_JSON_NULL
} apr_json_type_e;

/**
 * A structure to hold a UTF-8 encoded JSON string.
 */
typedef struct apr_json_string_t {
    /** pointer to the string */
    const char *p;
    /** string length, or APR_JSON_VALUE_STRING to compute length automatically */
    apr_ssize_t len;
} apr_json_string_t;

/**
 * A structure that holds a JSON value.
 *
 * Use apr_json_value_create() to allocate.
 */
typedef struct apr_json_value_t {
    /** preceding whitespace, if any */
    const char *pre;
    /** trailing whitespace, if any */
    const char *post;
    /** type of the value */
    apr_json_type_e type;
    /** actual value. which member is valid depends on type. */
    union {
        /** JSON object */
        apr_json_object_t *object;
        /** JSON array */
        apr_array_header_t *array;
        /** JSON floating point value */
        double dnumber;
        /** JSON long integer value */
        apr_int64_t lnumber;
        /** JSON UTF-8 encoded string value */
        apr_json_string_t string;
        /** JSON boolean value */
        int boolean;
    } value;
} apr_json_value_t;

/**
 * A structure to hold a JSON object key value pair.
 *
 * Use apr_json_object_set() to allocate.
 */
typedef struct apr_json_kv_t {
    /** Links to the rest of the kv pairs */
    APR_RING_ENTRY(apr_json_kv_t) link;
    /** the key */
    apr_json_value_t *k;
    /** the value */
    apr_json_value_t *v;
} apr_json_kv_t;

/**
 * A structure to hold a JSON object.
 *
 * Use apr_json_object_create() to allocate.
 */
struct apr_json_object_t {
    /** The key value pairs in the object are in this list */
    APR_RING_HEAD(apr_json_object_list_t, apr_json_kv_t) list;
    /** JSON object */
    apr_hash_t *hash;
};

/**
 * Allocate and return a apr_json_value_t structure.
 *
 * @param pool The pool to allocate from.
 * @return The apr_json_value_t structure.
 */
APR_DECLARE(apr_json_value_t *) apr_json_value_create(apr_pool_t *pool)
        __attribute__((nonnull(1)));

/**
 * Allocate and return a JSON string with the given value.
 *
 * @param pool The pool to allocate from.
 * @param val The UTF-8 encoded string value.
 * @param len The length of the string, or APR_JSON_VALUE_STRING.
 * @return The apr_json_value_t structure.
 */
APR_DECLARE(apr_json_value_t *)
        apr_json_string_create(apr_pool_t *pool, const char *val,
                apr_ssize_t len) __attribute__((nonnull(1)));

/**
 * Allocate and return a JSON array.
 *
 * @param pool The pool to allocate from.
 * @param nelts the number of elements in the initial array
 * @return The apr_json_value_t structure.
 */
APR_DECLARE(apr_json_value_t *)
        apr_json_array_create(apr_pool_t *pool, int nelts)
        __attribute__((nonnull(1)));

/**
 * Allocate and return a JSON object.
 *
 * @param pool The pool to allocate from.
 * @return The apr_json_value_t structure.
 */
APR_DECLARE(apr_json_value_t *) apr_json_object_create(apr_pool_t *pool)
        __attribute__((nonnull(1)));

/**
 * Allocate and return a JSON long.
 *
 * @param pool The pool to allocate from.
 * @param lnumber The long value.
 * @return The apr_json_value_t structure.
 */
APR_DECLARE(apr_json_value_t *)
        apr_json_long_create(apr_pool_t *pool, apr_int64_t lnumber)
        __attribute__((nonnull(1)));

/**
 * Allocate and return a JSON double.
 *
 * @param pool The pool to allocate from.
 * @param dnumber The double value.
 * @return The apr_json_value_t structure.
 */
APR_DECLARE(apr_json_value_t *)
        apr_json_double_create(apr_pool_t *pool, double dnumber)
        __attribute__((nonnull(1)));

/**
 * Allocate and return a JSON boolean.
 *
 * @param pool The pool to allocate from.
 * @param boolean The boolean value.
 * @return The apr_json_value_t structure.
 */
APR_DECLARE(apr_json_value_t *)
        apr_json_boolean_create(apr_pool_t *pool, int boolean)
        __attribute__((nonnull(1)));

/**
 * Allocate and return a JSON null.
 *
 * @param pool The pool to allocate from.
 * @return The apr_json_value_t structure.
 */
APR_DECLARE(apr_json_value_t *)
        apr_json_null_create(apr_pool_t *pool)
        __attribute__((nonnull(1)));

/**
 * Associate a value with a key in a JSON object.
 * @param obj The JSON object.
 * @param key Pointer to the key string.
 * @param val Value to associate with the key.
 * @param pool Pool to use.
 * @return APR_SUCCESS on success, APR_EINVAL if the key is
 *   NULL or not a string, or the object is not an APR_JSON_OBJECT.
 * @remark If the value is NULL the key value pair is deleted.
 */
APR_DECLARE(apr_status_t) apr_json_object_set(apr_json_value_t *obj,
        apr_json_value_t *key, apr_json_value_t *val,
        apr_pool_t *pool) __attribute__((nonnull(1, 4)));

/**
 * Look up the value associated with a key in a JSON object.
 * @param obj The JSON object.
 * @param key Pointer to the key.
 * @return Returns NULL if the key is not present.
 */
APR_DECLARE(apr_json_kv_t *)
        apr_json_object_get(apr_json_object_t *obj, const char *key)
        __attribute__((nonnull(1, 2)));

/**
 * Decode utf8-encoded JSON string into apr_json_value_t.
 * @param retval the result
 * @param injson utf8-encoded JSON string.
 * @param size length of the input string.
 * @param offset number of characters processed.
 * @param flags set to APR_JSON_FLAGS_WHITESPACE to preserve whitespace,
 *   or APR_JSON_FLAGS_NONE to filter whitespace.
 * @param level maximum nesting level we are prepared to decode.
 * @param pool pool used to allocate the result from.
 * @return APR_SUCCESS on success, APR_EOF if the JSON text is truncated.
 *   APR_BADCH when a decoding error has occurred (the location of the error
 *   is at offset), APR_EINVAL if the level has been exceeded, or
 *   APR_ENOTIMPL on platforms where not implemented.
 */
APR_DECLARE(apr_status_t) apr_json_decode(apr_json_value_t ** retval,
        const char *injson, apr_ssize_t size, apr_off_t * offset,
        int flags, int level, apr_pool_t * pool)
        __attribute__((nonnull(1, 2, 7)));

/**
 * Encode data represented as apr_json_value_t to utf8-encoded JSON string
 * and append it to the specified brigade.
 *
 * All JSON strings are checked for invalid UTF-8 character sequences,
 * and if found invalid sequences are replaced with the replacement
 * character "�" (U+FFFD).
 *
 * @param brigade brigade the result will be appended to.
 * @param flush optional flush function for the brigade. Can be NULL.
 * @param ctx optional contaxt for the flush function. Can be NULL.
 * @param json the JSON data.
 * @param flags set to APR_JSON_FLAGS_WHITESPACE to preserve whitespace,
 *   or APR_JSON_FLAGS_NONE to filter whitespace.
 * @param pool pool used to allocate the buckets from.
 * @return APR_SUCCESS on success, or APR_ENOTIMPL on platforms where not
 *   implemented.
 */
APR_DECLARE(apr_status_t) apr_json_encode(apr_bucket_brigade * brigade,
        apr_brigade_flush flush, void *ctx, const apr_json_value_t * json,
        int flags, apr_pool_t * pool) __attribute__((nonnull(1, 4, 6)));

#ifdef __cplusplus
}
#endif
/** @} */
#endif /* APR_JSON_H */
