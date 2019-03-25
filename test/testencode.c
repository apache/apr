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

#include "apr_encode.h"
#include "apr_strings.h"

#include "abts.h"
#include "testutil.h"

static const unsigned char ufoobar[] = { 'f', 'o', 'o', 'b', 'a', 'r' };

static void test_encode_base64(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *src, *target;
    const char *dest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    /*
     * Test vectors from https://tools.ietf.org/html/rfc4648#section-10
     */
    src = "";
    target = "";
    dest = apr_pencode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "f";
    target = "Zg==";
    dest = apr_pencode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "f";
    target = "Zg";
    dest = apr_pencode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fo";
    target = "Zm8=";
    dest = apr_pencode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fo";
    target = "Zm8";
    dest = apr_pencode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foo";
    target = "Zm9v";
    dest = apr_pencode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foo";
    target = "Zm9v";
    dest = apr_pencode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    apr_pool_destroy(pool);
}

static void test_encode_base64_binary(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *target;
    const char *dest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    /*
     * Test vectors from https://tools.ietf.org/html/rfc4648#section-10
     */
    target = "";
    dest = apr_pencode_base64_binary(pool, ufoobar, 0, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "Zg==";
    dest = apr_pencode_base64_binary(pool, ufoobar, 1, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "Zg";
    dest = apr_pencode_base64_binary(pool, ufoobar, 1, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "Zm8=";
    dest = apr_pencode_base64_binary(pool, ufoobar, 2, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "Zm8";
    dest = apr_pencode_base64_binary(pool, ufoobar, 2, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "Zm9v";
    dest = apr_pencode_base64_binary(pool, ufoobar, 3, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "Zm9v";
    dest = apr_pencode_base64_binary(pool, ufoobar, 3, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    apr_pool_destroy(pool);
}

static void test_decode_base64(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *target, *src;
    const char *dest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    /*
     * Test vectors from https://tools.ietf.org/html/rfc4648#section-10
     */
    src = "";
    target = "";
    dest = apr_pdecode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, dest, target);

    src = "Zg==";
    target = "f";
    dest = apr_pdecode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, dest, target);

    src = "Zg";
    target = "f";
    dest = apr_pdecode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, dest, target);

    src = "Zm8=";
    target = "fo";
    dest = apr_pdecode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, dest, target);

    src = "Zm8";
    target = "fo";
    dest = apr_pdecode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, dest, target);

    src = "Zm9v";
    target = "foo";
    dest = apr_pdecode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, dest, target);

    src = "Zm9v";
    target = "foo";
    dest = apr_pdecode_base64(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, dest, target);

    apr_pool_destroy(pool);
}

static void test_decode_base64_binary(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *src;
    const unsigned char *udest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    /*
     * Test vectors from https://tools.ietf.org/html/rfc4648#section-10
     */
    src = "";
    udest = apr_pdecode_base64_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base64_binary target!=dest", memcmp(ufoobar, udest, 0) == 0);
    ABTS_INT_EQUAL(tc, len, 0);

    src = "Zg==";
    udest = apr_pdecode_base64_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base64_binary target!=dest", memcmp(ufoobar, udest, 1) == 0);
    ABTS_INT_EQUAL(tc, len, 1);

    src = "Zg";
    udest = apr_pdecode_base64_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base64_binary target!=dest", memcmp(ufoobar, udest, 1) == 0);
    ABTS_INT_EQUAL(tc, len, 1);

    src = "Zm8=";
    udest = apr_pdecode_base64_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base64_binary target!=dest", memcmp(ufoobar, udest, 2) == 0);
    ABTS_INT_EQUAL(tc, len, 2);

    src = "Zm8";
    udest = apr_pdecode_base64_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base64_binary target!=dest", memcmp(ufoobar, udest, 2) == 0);
    ABTS_INT_EQUAL(tc, len, 2);

    src = "Zm9v";
    udest = apr_pdecode_base64_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base64_binary target!=dest", memcmp(ufoobar, udest, 3) == 0);
    ABTS_INT_EQUAL(tc, len, 3);

    src = "Zm9v";
    udest = apr_pdecode_base64_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base64_binary target!=dest", memcmp(ufoobar, udest, 3) == 0);
    ABTS_INT_EQUAL(tc, len, 3);

    apr_pool_destroy(pool);
}

static void test_encode_base32(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *src, *target;
    const char *dest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    /*
     * Test vectors from https://tools.ietf.org/html/rfc4648#section-10
     */
    src = "";
    target = "";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "f";
    target = "MY======";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "f";
    target = "MY";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "f";
    target = "CO======";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "f";
    target = "CO";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fo";
    target = "MZXQ====";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fo";
    target = "MZXQ";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fo";
    target = "CPNG====";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fo";
    target = "CPNG";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foo";
    target = "MZXW6===";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foo";
    target = "MZXW6";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foo";
    target = "CPNMU===";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foo";
    target = "CPNMU";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foob";
    target = "MZXW6YQ=";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foob";
    target = "MZXW6YQ";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foob";
    target = "CPNMUOG=";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foob";
    target = "CPNMUOG";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fooba";
    target = "MZXW6YTB";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fooba";
    target = "MZXW6YTB";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fooba";
    target = "CPNMUOJ1";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "fooba";
    target = "CPNMUOJ1";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foobar";
    target = "MZXW6YTBOI======";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foobar";
    target = "MZXW6YTBOI";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foobar";
    target = "CPNMUOJ1E8======";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "foobar";
    target = "CPNMUOJ1E8";
    dest = apr_pencode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    apr_pool_destroy(pool);
}

static void test_encode_base32_binary(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *target;
    const char *dest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    /*
     * Test vectors from https://tools.ietf.org/html/rfc4648#section-10
     */
    target = "";
    dest = apr_pencode_base32_binary(pool, ufoobar, 0, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MY======";
    dest = apr_pencode_base32_binary(pool, ufoobar, 1, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MY";
    dest = apr_pencode_base32_binary(pool, ufoobar, 1, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CO======";
    dest = apr_pencode_base32_binary(pool, ufoobar, 1, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CO";
    dest = apr_pencode_base32_binary(pool, ufoobar, 1, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXQ====";
    dest = apr_pencode_base32_binary(pool, ufoobar, 2, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXQ";
    dest = apr_pencode_base32_binary(pool, ufoobar, 2, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNG====";
    dest = apr_pencode_base32_binary(pool, ufoobar, 2, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNG";
    dest = apr_pencode_base32_binary(pool, ufoobar, 2, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXW6===";
    dest = apr_pencode_base32_binary(pool, ufoobar, 3, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXW6";
    dest = apr_pencode_base32_binary(pool, ufoobar, 3, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNMU===";
    dest = apr_pencode_base32_binary(pool, ufoobar, 3, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNMU";
    dest = apr_pencode_base32_binary(pool, ufoobar, 3, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXW6YQ=";
    dest = apr_pencode_base32_binary(pool, ufoobar, 4, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXW6YQ";
    dest = apr_pencode_base32_binary(pool, ufoobar, 4, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNMUOG=";
    dest = apr_pencode_base32_binary(pool, ufoobar, 4, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNMUOG";
    dest = apr_pencode_base32_binary(pool, ufoobar, 4, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXW6YTB";
    dest = apr_pencode_base32_binary(pool, ufoobar, 5, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXW6YTB";
    dest = apr_pencode_base32_binary(pool, ufoobar, 5, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNMUOJ1";
    dest = apr_pencode_base32_binary(pool, ufoobar, 5, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNMUOJ1";
    dest = apr_pencode_base32_binary(pool, ufoobar, 5, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXW6YTBOI======";
    dest = apr_pencode_base32_binary(pool, ufoobar, 6, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "MZXW6YTBOI";
    dest = apr_pencode_base32_binary(pool, ufoobar, 6, APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNMUOJ1E8======";
    dest = apr_pencode_base32_binary(pool, ufoobar, 6, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    target = "CPNMUOJ1E8";
    dest = apr_pencode_base32_binary(pool, ufoobar, 6, APR_ENCODE_BASE32HEX | APR_ENCODE_NOPADDING, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    apr_pool_destroy(pool);
}

static void test_decode_base32(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *target, *src;
    const char *dest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    /*
     * Test vectors from https://tools.ietf.org/html/rfc4648#section-10
     */
    src = "";
    target = "";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MY======";
    target = "f";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MY";
    target = "f";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CO======";
    target = "f";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CO";
    target = "f";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXQ====";
    target = "fo";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXQ";
    target = "fo";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNG====";
    target = "fo";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNG";
    target = "fo";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXW6===";
    target = "foo";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXW6";
    target = "foo";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNMU===";
    target = "foo";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNMU";
    target = "foo";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXW6YQ=";
    target = "foob";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXW6YQ=";
    target = "foob";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNMUOG=";
    target = "foob";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNMUOG";
    target = "foob";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXW6YTB";
    target = "fooba";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXW6YTB";
    target = "fooba";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNMUOJ1";
    target = "fooba";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNMUOJ1";
    target = "fooba";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXW6YTBOI======";
    target = "foobar";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "MZXW6YTBOI";
    target = "foobar";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNMUOJ1E8======";
    target = "foobar";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    src = "CPNMUOJ1E8";
    target = "foobar";
    dest = apr_pdecode_base32(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_STR_EQUAL(tc, target, dest);

    apr_pool_destroy(pool);
}

static void test_decode_base32_binary(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *src;
    const unsigned char *udest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    /*
     * Test vectors from https://tools.ietf.org/html/rfc4648#section-10
     */
    src = "";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 0) == 0);
    ABTS_INT_EQUAL(tc, 0, len);

    src = "MY======";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 1) == 0);
    ABTS_INT_EQUAL(tc, 1, len);

    src = "MY";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 1) == 0);
    ABTS_INT_EQUAL(tc, 1, len);

    src = "CO======";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 1) == 0);
    ABTS_INT_EQUAL(tc, 1, len);

    src = "CO";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 1) == 0);
    ABTS_INT_EQUAL(tc, 1, len);

    src = "MZXQ====";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 2) == 0);
    ABTS_INT_EQUAL(tc, 2, len);

    src = "MZXQ";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 2) == 0);
    ABTS_INT_EQUAL(tc, 2, len);

    src = "CPNG====";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 2) == 0);
    ABTS_INT_EQUAL(tc, 2, len);

    src = "CPNG";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 2) == 0);
    ABTS_INT_EQUAL(tc, 2, len);

    src = "MZXW6===";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 3) == 0);
    ABTS_INT_EQUAL(tc, 3, len);

    src = "MZXW6";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 3) == 0);
    ABTS_INT_EQUAL(tc, 3, len);

    src = "CPNMU===";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 3) == 0);
    ABTS_INT_EQUAL(tc, 3, len);

    src = "CPNMU";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 3) == 0);
    ABTS_INT_EQUAL(tc, 3, len);

    src = "MZXW6YQ=";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 4) == 0);
    ABTS_INT_EQUAL(tc, 4, len);

    src = "MZXW6YQ=";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 4) == 0);
    ABTS_INT_EQUAL(tc, 4, len);

    src = "CPNMUOG=";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 4) == 0);
    ABTS_INT_EQUAL(tc, 4, len);

    src = "CPNMUOG";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 4) == 0);
    ABTS_INT_EQUAL(tc, 4, len);

    src = "MZXW6YTB";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 5) == 0);
    ABTS_INT_EQUAL(tc, 5, len);

    src = "MZXW6YTB";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 5) == 0);
    ABTS_INT_EQUAL(tc, 5, len);

    src = "CPNMUOJ1";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 5) == 0);
    ABTS_INT_EQUAL(tc, 5, len);

    src = "CPNMUOJ1";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 5) == 0);
    ABTS_INT_EQUAL(tc, 5, len);

    src = "MZXW6YTBOI======";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 6) == 0);
    ABTS_INT_EQUAL(tc, 6, len);

    src = "MZXW6YTBOI";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 6) == 0);
    ABTS_INT_EQUAL(tc, 6, len);

    src = "CPNMUOJ1E8======";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 6) == 0);
    ABTS_INT_EQUAL(tc, 6, len);

    src = "CPNMUOJ1E8";
    udest = apr_pdecode_base32_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_BASE32HEX, &len);
    ABTS_ASSERT(tc, "apr_pdecode_base32_binary target!=dest", memcmp(ufoobar, udest, 6) == 0);
    ABTS_INT_EQUAL(tc, 6, len);

    apr_pool_destroy(pool);
}

static void test_encode_base16(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *src, *target;
    const char *dest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    src = "foobar";
    target = "666f6f626172";
    dest = apr_pencode_base16(pool, src, APR_ENCODE_STRING, APR_ENCODE_LOWER, &len);
    ABTS_STR_EQUAL(tc, target, dest);
    apr_encode_base16(NULL, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, strlen(dest) + 1),
                (len == strlen(dest) + 1));

    src = "foobar";
    target = "666F6F626172";
    dest = apr_pencode_base16(pool, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);
    apr_encode_base16(NULL, src, APR_ENCODE_STRING, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, strlen(dest) + 1),
                (len == strlen(dest) + 1));

    src = "foobar";
    target = "66:6f:6f:62:61:72";
    dest = apr_pencode_base16(pool, src, APR_ENCODE_STRING, APR_ENCODE_COLON | APR_ENCODE_LOWER, &len);
    ABTS_STR_EQUAL(tc, target, dest);
    apr_encode_base16(NULL, src, APR_ENCODE_STRING, APR_ENCODE_COLON, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, strlen(dest) + 1),
                (len == strlen(dest) + 1));

    src = "foobar";
    target = "66:6F:6F:62:61:72";
    dest = apr_pencode_base16(pool, src, APR_ENCODE_STRING, APR_ENCODE_COLON, &len);
    ABTS_STR_EQUAL(tc, target, dest);
    apr_encode_base16(NULL, src, APR_ENCODE_STRING, APR_ENCODE_COLON, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, strlen(dest) + 1),
                (len == strlen(dest) + 1));

    apr_pool_destroy(pool);
}

static void test_encode_base16_binary(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *target;
    const unsigned char usrc[] = {
        0xFF, 0x00, 0xFF, 0x00
    };
    const char *dest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    target = "ff00ff00";
    dest = apr_pencode_base16_binary(pool, usrc, 4, APR_ENCODE_LOWER, &len);
    ABTS_STR_EQUAL(tc, target, dest);
    apr_encode_base16_binary(NULL, usrc, 4, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, strlen(dest) + 1),
                (len == strlen(dest) + 1));

    target = "FF00FF00";
    dest = apr_pencode_base16_binary(pool, usrc, 4, APR_ENCODE_NONE, &len);
    ABTS_STR_EQUAL(tc, target, dest);
    apr_encode_base16_binary(NULL, usrc, 4, APR_ENCODE_NONE, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, strlen(dest) + 1),
                (len == strlen(dest) + 1));

    target = "ff:00:ff:00";
    dest = apr_pencode_base16_binary(pool, usrc, 4, APR_ENCODE_COLON | APR_ENCODE_LOWER, &len);
    ABTS_STR_EQUAL(tc, target, dest);
    apr_encode_base16_binary(NULL, usrc, 4, APR_ENCODE_COLON, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, strlen(dest) + 1),
                (len == strlen(dest) + 1));

    target = "FF:00:FF:00";
    dest = apr_pencode_base16_binary(pool, usrc, 4, APR_ENCODE_COLON, &len);
    ABTS_STR_EQUAL(tc, target, dest);
    apr_encode_base16_binary(NULL, usrc, 4, APR_ENCODE_COLON, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, strlen(dest) + 1),
                (len == strlen(dest) + 1));

    apr_pool_destroy(pool);
}

static void test_decode_base16(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *src, *target;
    const char *dest;
    apr_size_t len;

    apr_pool_create(&pool, NULL);

    src = "3A:3B:3C:3D";
    target = ":;<=";
    dest = apr_pdecode_base16(pool, src, APR_ENCODE_STRING, APR_ENCODE_COLON, &len);
    ABTS_STR_EQUAL(tc, target, dest);
    ABTS_INT_EQUAL(tc, 4, (int)len);
    apr_decode_base16(NULL, src, APR_ENCODE_STRING, APR_ENCODE_COLON, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, (apr_size_t) 5),
                (len == 5));

    apr_pool_destroy(pool);
}

static void test_decode_base16_binary(abts_case * tc, void *data)
{
    apr_pool_t *pool;
    const char *src;
    const unsigned char utarget[] = {
        0xFF, 0x00, 0xFF, 0x00
    };
    const unsigned char *udest;
    apr_size_t len, vlen;

    apr_pool_create(&pool, NULL);

    src = "ff:00:ff:00";
    udest = apr_pdecode_base16_binary(pool, src, APR_ENCODE_STRING, APR_ENCODE_COLON, &vlen);
    ABTS_ASSERT(tc, "apr_pdecode_base16_binary target!=dest", memcmp(utarget, udest, 4) == 0);
    ABTS_INT_EQUAL(tc, (int)vlen, 4);
    apr_decode_base16_binary(NULL, src, APR_ENCODE_STRING, APR_ENCODE_COLON, &len);
    ABTS_ASSERT(tc,
                apr_psprintf(pool, "size mismatch (%" APR_SIZE_T_FMT "!=%" APR_SIZE_T_FMT ")", len, (apr_size_t) 4),
                (len == 4));

    apr_pool_destroy(pool);
}

abts_suite *testencode(abts_suite * suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_encode_base64, NULL);
    abts_run_test(suite, test_encode_base64_binary, NULL);
    abts_run_test(suite, test_decode_base64, NULL);
    abts_run_test(suite, test_decode_base64_binary, NULL);
    abts_run_test(suite, test_encode_base32, NULL);
    abts_run_test(suite, test_encode_base32_binary, NULL);
    abts_run_test(suite, test_decode_base32, NULL);
    abts_run_test(suite, test_decode_base32_binary, NULL);
    abts_run_test(suite, test_encode_base16, NULL);
    abts_run_test(suite, test_encode_base16_binary, NULL);
    abts_run_test(suite, test_decode_base16, NULL);
    abts_run_test(suite, test_decode_base16_binary, NULL);

    return suite;
}
