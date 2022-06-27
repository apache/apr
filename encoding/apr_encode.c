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

/* encode/decode functions.
 *
 * These functions perform various encoding operations, and are provided in
 * pairs, a function to query the length of and encode existing buffers, as
 * well as companion functions to perform the same process to memory
 * allocated from a pool.
 *
 * The API is designed to have the smallest possible RAM footprint, and so
 * will only allocate the exact amount of RAM needed for each conversion.
 */

#include "apr_encode.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_encode_private.h"

/* lookup table: fast and const should make it shared text page. */
static const unsigned char pr2six[256] =
{
#if !APR_CHARSET_EBCDIC
    /* ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 62, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 128, 64, 64,
    64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 63,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
#else                           /* APR_CHARSET_EBCDIC */
    /* EBCDIC table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    62, 63, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 63, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 128, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 64, 64, 64, 64, 64, 64,
    64, 35, 36, 37, 38, 39, 40, 41, 42, 43, 64, 64, 64, 64, 64, 64,
    64, 64, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 0, 1, 2, 3, 4, 5, 6, 7, 8, 64, 64, 64, 64, 64, 64,
    64, 9, 10, 11, 12, 13, 14, 15, 16, 17, 64, 64, 64, 64, 64, 64,
    64, 64, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64, 64,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64
#endif                          /* APR_CHARSET_EBCDIC */
};

static const unsigned char pr2five[256] =
{
#if !APR_CHARSET_EBCDIC
    /* ASCII table */
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 26, 27, 28, 29, 30, 31, 32, 32, 32, 32, 32, 128, 32, 32,
    32, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
#else                           /* APR_CHARSET_EBCDIC */
    /* EBCDIC table */
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 128, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 0, 1, 2, 3, 4, 5, 6, 7, 8, 32, 32, 32, 32, 32, 32,
    32, 9, 10, 11, 12, 13, 14, 15, 16, 17, 32, 32, 32, 32, 32, 32,
    32, 32, 18, 19, 20, 21, 22, 23, 24, 25, 32, 32, 32, 32, 32, 32,
    32, 32, 26, 27, 28, 29, 30, 31, 32, 32, 32, 32, 32, 32, 32, 32
#endif                          /* APR_CHARSET_EBCDIC */
};

static const unsigned char pr2fivehex[256] =
{
#if !APR_CHARSET_EBCDIC
    /* ASCII table */
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 32, 32, 32, 128, 32, 32,
    32, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    25, 26, 27, 28, 29, 30, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32
#else                           /* APR_CHARSET_EBCDIC */
    /* EBCDIC table */
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 128, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    32, 10, 11, 12, 13, 14, 15, 16, 17, 18, 32, 32, 32, 32, 32, 32,
    32, 19, 20, 21, 22, 23, 24, 25, 26, 27, 32, 32, 32, 32, 32, 32,
    32, 32, 28, 29, 30, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 32, 32, 32, 32, 32, 32
#endif                          /* APR_CHARSET_EBCDIC */
};

static const unsigned char pr2two[256] =
{
#if !APR_CHARSET_EBCDIC
    /* ASCII table */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 32, 16, 16, 16, 16, 16,
    16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
#else                           /* APR_CHARSET_EBCDIC */
    /* EBCDIC table */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 32, 16, 16, 16, 16, 16,
    16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 16, 16, 16, 16, 16, 16
#endif                          /* APR_CHARSET_EBCDIC */
};

static const char base64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char base64url[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static const char base32[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static const char base32hex[] =
"0123456789ABCDEFGHIJKLMNOPQRSTUV";

static const char base16[] = "0123456789ABCDEF";
static const char base16lower[] = "0123456789abcdef";

APR_DECLARE(apr_status_t) apr_encode_base64(char *dest, const char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;
    const char *base;

    if (src && slen == APR_ENCODE_STRING) {
        count = strlen(src);
    }
    else if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (dest) {
        char *bufout = dest;
        apr_size_t i = 0;

        if (0 == ((flags & APR_ENCODE_BASE64URL))) {
            base = base64;
        }
        else {
            base = base64url;
        }

        if (count > 2) {
            for (; i < count - 2; i += 3) {
                *bufout++ = base[(TO_ASCII(src[i]) >> 2) & 0x3F];
                *bufout++ = base[((TO_ASCII(src[i]) & 0x3) << 4 |
                                  (TO_ASCII(src[i + 1]) & 0xF0) >> 4)];
                *bufout++ = base[((TO_ASCII(src[i + 1]) & 0xF) << 2 |
                                  (TO_ASCII(src[i + 2]) & 0xC0) >> 6)];
                *bufout++ = base[TO_ASCII(src[i + 2]) & 0x3F];
            }
        }
        if (i < count) {
            *bufout++ = base[(TO_ASCII(src[i]) >> 2) & 0x3F];
            if (i == (count - 1)) {
                *bufout++ = base[(TO_ASCII(src[i]) & 0x3) << 4];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                }
            }
            else {
                *bufout++ = base[((TO_ASCII(src[i]) & 0x3) << 4 |
                                  (TO_ASCII(src[i + 1]) & 0xF0) >> 4)];
                *bufout++ = base[(TO_ASCII(src[i + 1]) & 0xF) << 2];
            }
            if (!(flags & APR_ENCODE_NOPADDING)) {
                *bufout++ = '=';
            }
        }

        dlen = bufout - dest;
        dest[dlen] = '\0';
    }
    else {
        dlen = ((count + 2u) / 3u) * 4u + 1u;
        if (dlen <= count) {
            status = APR_ENOSPC;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(apr_status_t) apr_encode_base64_binary(char *dest, const unsigned char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;
    const char *base;

    if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (dest) {
        char *bufout = dest;
        apr_size_t i = 0;

        if (0 == ((flags & APR_ENCODE_BASE64URL))) {
            base = base64;
        }
        else {
            base = base64url;
        }

        if (count > 2) {
            for (; i < count - 2; i += 3) {
                *bufout++ = base[(src[i] >> 2) & 0x3F];
                *bufout++ = base[((src[i] & 0x3) << 4 |
                                  (src[i + 1] & 0xF0) >> 4)];
                *bufout++ = base[((src[i + 1] & 0xF) << 2 |
                                  (src[i + 2] & 0xC0) >> 6)];
                *bufout++ = base[src[i + 2] & 0x3F];
            }
        }
        if (i < count) {
            *bufout++ = base[(src[i] >> 2) & 0x3F];
            if (i == (count - 1)) {
                *bufout++ = base[((src[i] & 0x3) << 4)];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                }
            }
            else {
                *bufout++ = base[((src[i] & 0x3) << 4 |
                                  (src[i + 1] & 0xF0) >> 4)];
                *bufout++ = base[(src[i + 1] & 0xF) << 2];
            }
            if (!(flags & APR_ENCODE_NOPADDING)) {
                *bufout++ = '=';
            }
        }

        dlen = bufout - dest;
        dest[dlen] = '\0';
    }
    else {
        dlen = ((count + 2u) / 3u) * 4u + 1u;
        if (dlen <= count) {
            status = APR_ENOSPC;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(const char *)apr_pencode_base64(apr_pool_t * p, const char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!src) {
        return NULL;
    }

    switch (apr_encode_base64(NULL, src, slen, flags, &size)) {
    case APR_SUCCESS:{
            char *cmd = apr_palloc(p, size);
            if (cmd) {
                apr_encode_base64(cmd, src, slen, flags, len);
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(const char *)apr_pencode_base64_binary(apr_pool_t * p, const unsigned char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!src) {
        return NULL;
    }

    switch (apr_encode_base64_binary(NULL, src, slen, flags, &size)) {
    case APR_SUCCESS:{
            char *cmd = apr_palloc(p, size);
            if (cmd) {
                apr_encode_base64_binary(cmd, src, slen, flags, len);
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(apr_status_t) apr_decode_base64(char *dest, const char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;

    if (src && slen == APR_ENCODE_STRING) {
        count = strlen(src);
    }
    else if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (src) {
        const unsigned char *bufin;

        bufin = (const unsigned char *)src;
        while (count) {
            if (pr2six[*bufin] >= 64) {
                if (!(flags & APR_ENCODE_RELAXED)) {
                    if (count <= 2) {
                        do {
                            if (pr2six[bufin[count - 1]] <= 64)
                                break;
                        } while (--count);
                    }
                    if (count) {
                        status = APR_BADCH;
                    }
                }
                break;
            }
            count--;
            bufin++;
        }
        count = bufin - (const unsigned char *)src;

        if (dest) {
            unsigned char *bufout;

            bufout = (unsigned char *)dest;
            bufin = (const unsigned char *)src;

            while (count >= 4) {
                *(bufout++) = TO_NATIVE(pr2six[bufin[0]] << 2 |
                                        pr2six[bufin[1]] >> 4);
                *(bufout++) = TO_NATIVE(pr2six[bufin[1]] << 4 |
                                        pr2six[bufin[2]] >> 2);
                *(bufout++) = TO_NATIVE(pr2six[bufin[2]] << 6 |
                                        pr2six[bufin[3]]);
                bufin += 4;
                count -= 4;
            }

            if (count == 1) {
                status = APR_EINCOMPLETE;
            }
            if (count > 1) {
                *(bufout++) = TO_NATIVE(pr2six[bufin[0]] << 2 |
                                        pr2six[bufin[1]] >> 4);
            }
            if (count > 2) {
                *(bufout++) = TO_NATIVE(pr2six[bufin[1]] << 4 |
                                        pr2six[bufin[2]] >> 2);
            }

            dlen = bufout - (unsigned char *)dest;
            dest[dlen] = '\0';
        }
    }

    if (!src || !dest) {
        dlen = (count / 4u) * 3u + 1u;
        switch (count % 4) {
        case 3:
            dlen += 2;
            break;
        case 2:
            dlen++;
            break;
        case 1:
            status = APR_EINCOMPLETE;
            break;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(apr_status_t) apr_decode_base64_binary(unsigned char *dest,
             const char *src, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;

    if (src && slen == APR_ENCODE_STRING) {
        count = strlen(src);
    }
    else if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (src) {
        const unsigned char *bufin;

        bufin = (const unsigned char *)src;
        while (count) {
            if (pr2six[*bufin] >= 64) {
                if (!(flags & APR_ENCODE_RELAXED)) {
                    if (count <= 2) {
                        do {
                            if (pr2six[bufin[count - 1]] <= 64)
                                break;
                        } while (--count);
                    }
                    if (count) {
                        status = APR_BADCH;
                    }
                }
                break;
            }
            count--;
            bufin++;
        }
        count = bufin - (const unsigned char *)src;

        if (dest) {
            unsigned char *bufout;

            bufout = (unsigned char *)dest;
            bufin = (const unsigned char *)src;

            while (count >= 4) {
                *(bufout++) = (pr2six[bufin[0]] << 2 |
                               pr2six[bufin[1]] >> 4);
                *(bufout++) = (pr2six[bufin[1]] << 4 |
                               pr2six[bufin[2]] >> 2);
                *(bufout++) = (pr2six[bufin[2]] << 6 |
                               pr2six[bufin[3]]);
                bufin += 4;
                count -= 4;
            }

            if (count == 1) {
                status = APR_EINCOMPLETE;
            }
            if (count > 1) {
                *(bufout++) = (pr2six[bufin[0]] << 2 |
                               pr2six[bufin[1]] >> 4);
            }
            if (count > 2) {
                *(bufout++) = (pr2six[bufin[1]] << 4 |
                               pr2six[bufin[2]] >> 2);
            }

            dlen = bufout - dest;
        }
    }

    if (!src || !dest) {
        dlen = (count / 4u) * 3u;
        switch (count % 4) {
        case 3:
            dlen += 2;
            break;
        case 2:
            dlen++;
            break;
        case 1:
            status = APR_EINCOMPLETE;
            break;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(const char *)apr_pdecode_base64(apr_pool_t * p, const char *str,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!str) {
        return NULL;
    }

    switch (apr_decode_base64(NULL, str, slen, flags, &size)) {
    case APR_SUCCESS:{
            void *cmd = apr_palloc(p, size);
            if (cmd) {
                apr_decode_base64(cmd, str, slen, flags, len);
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(const unsigned char *)apr_pdecode_base64_binary(apr_pool_t * p,
             const char *str, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!str) {
        return NULL;
    }

    switch (apr_decode_base64_binary(NULL, str, slen, flags, &size)) {
    case APR_SUCCESS:{
            unsigned char *cmd = apr_palloc(p, size + 1);
            if (cmd) {
                apr_decode_base64_binary(cmd, str, slen, flags, len);
                cmd[size] = 0;
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(apr_status_t) apr_encode_base32(char *dest, const char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;
    const char *base;

    if (src && slen == APR_ENCODE_STRING) {
        count = strlen(src);
    }
    else if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (dest) {
        char *bufout = dest;
        apr_size_t i = 0;

        if (!((flags & APR_ENCODE_BASE32HEX))) {
            base = base32;
        }
        else {
            base = base32hex;
        }

        if (count > 4) {
            for (; i < count - 4; i += 5) {
                *bufout++ = base[(TO_ASCII(src[i]) >> 3) & 0x1F];
                *bufout++ = base[(((TO_ASCII(src[i]) << 2) & 0x1C) |
                                  ((TO_ASCII(src[i + 1]) >> 6) & 0x3))];
                *bufout++ = base[(TO_ASCII(src[i + 1]) >> 1) & 0x1F];
                *bufout++ = base[(((TO_ASCII(src[i + 1]) << 4) & 0x10) |
                                  ((TO_ASCII(src[i + 2]) >> 4) & 0xF))];
                *bufout++ = base[(((TO_ASCII(src[i + 2]) << 1) & 0x1E) |
                                  ((TO_ASCII(src[i + 3]) >> 7) & 0x1))];
                *bufout++ = base[(TO_ASCII(src[i + 3]) >> 2) & 0x1F];
                *bufout++ = base[(((TO_ASCII(src[i + 3]) << 3) & 0x18) |
                                  ((TO_ASCII(src[i + 4]) >> 5) & 0x7))];
                *bufout++ = base[TO_ASCII(src[i + 4]) & 0x1F];
            }
        }
        if (i < count) {
            *bufout++ = base[(TO_ASCII(src[i]) >> 3) & 0x1F];
            if (i == (count - 1)) {
                *bufout++ = base[(TO_ASCII(src[i]) << 2) & 0x1C];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                }
            }
            else if (i == (count - 2)) {
                *bufout++ = base[(((TO_ASCII(src[i]) << 2) & 0x1C) |
                                  ((TO_ASCII(src[i + 1]) >> 6) & 0x3))];
                *bufout++ = base[(TO_ASCII(src[i + 1]) >> 1) & 0x1F];
                *bufout++ = base[(TO_ASCII(src[i + 1]) << 4) & 0x10];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                }
            }
            else if (i == (count - 3)) {
                *bufout++ = base[(((TO_ASCII(src[i]) << 2) & 0x1C) |
                                  ((TO_ASCII(src[i + 1]) >> 6) & 0x3))];
                *bufout++ = base[(TO_ASCII(src[i + 1]) >> 1) & 0x1F];
                *bufout++ = base[(((TO_ASCII(src[i + 1]) << 4) & 0x10) |
                                  ((TO_ASCII(src[i + 2]) >> 4) & 0xF))];
                *bufout++ = base[(TO_ASCII(src[i + 2]) << 1) & 0x1E];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                }
            }
            else {
                *bufout++ = base[(((TO_ASCII(src[i]) << 2) & 0x1C) |
                                  ((TO_ASCII(src[i + 1]) >> 6) & 0x3))];
                *bufout++ = base[(TO_ASCII(src[i + 1]) >> 1) & 0x1F];
                *bufout++ = base[(((TO_ASCII(src[i + 1]) << 4) & 0x10) |
                                  ((TO_ASCII(src[i + 2]) >> 4) & 0xF))];
                *bufout++ = base[(((TO_ASCII(src[i + 2]) << 1) & 0x1E) |
                                  ((TO_ASCII(src[i + 3]) >> 7) & 0x1))];
                *bufout++ = base[(TO_ASCII(src[i + 3]) >> 2) & 0x1F];
                *bufout++ = base[(TO_ASCII(src[i + 3]) << 3) & 0x18];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                }
            }
        }

        dlen = bufout - dest;
        dest[dlen] = '\0';
    }
    else {
        dlen = ((count + 4u) / 5u) * 8u + 1u;
        if (dlen <= count) {
            status = APR_ENOSPC;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(apr_status_t) apr_encode_base32_binary(char *dest, const unsigned char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;
    const char *base;

    if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (dest) {
        char *bufout = dest;
        apr_size_t i = 0;

        if (!((flags & APR_ENCODE_BASE32HEX))) {
            base = base32;
        }
        else {
            base = base32hex;
        }

        if (count > 4) {
            for (; i < count - 4; i += 5) {
                *bufout++ = base[((src[i] >> 3) & 0x1F)];
                *bufout++ = base[(((src[i] << 2) & 0x1C) |
                                  ((src[i + 1] >> 6) & 0x3))];
                *bufout++ = base[((src[i + 1] >> 1) & 0x1F)];
                *bufout++ = base[(((src[i + 1] << 4) & 0x10) |
                                  ((src[i + 2] >> 4) & 0xF))];
                *bufout++ = base[(((src[i + 2] << 1) & 0x1E) |
                                  ((src[i + 3] >> 7) & 0x1))];
                *bufout++ = base[((src[i + 3] >> 2) & 0x1F)];
                *bufout++ = base[(((src[i + 3] << 3) & 0x18) |
                                  ((src[i + 4] >> 5) & 0x7))];
                *bufout++ = base[(src[i + 4] & 0x1F)];
            }
        }
        if (i < count) {
            *bufout++ = base[(src[i] >> 3) & 0x1F];
            if (i == (count - 1)) {
                *bufout++ = base[((src[i] << 2) & 0x1C)];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                }
            }
            else if (i == (count - 2)) {
                *bufout++ = base[(((src[i] << 2) & 0x1C) |
                                  ((src[i + 1] >> 6) & 0x3))];
                *bufout++ = base[((src[i + 1] >> 1) & 0x1F)];
                *bufout++ = base[((src[i + 1] << 4) & 0x10)];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                }
            }
            else if (i == (count - 3)) {
                *bufout++ = base[(((src[i] << 2) & 0x1C) |
                                  ((src[i + 1] >> 6) & 0x3))];
                *bufout++ = base[((src[i + 1] >> 1) & 0x1F)];
                *bufout++ = base[(((src[i + 1] << 4) & 0x10) |
                                  ((src[i + 2] >> 4) & 0xF))];
                *bufout++ = base[((src[i + 2] << 1) & 0x1E)];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                    *bufout++ = '=';
                    *bufout++ = '=';
                }
            }
            else {
                *bufout++ = base[(((src[i] << 2) & 0x1C) |
                                  ((src[i + 1] >> 6) & 0x3))];
                *bufout++ = base[((src[i + 1] >> 1) & 0x1F)];
                *bufout++ = base[(((src[i + 1] << 4) & 0x10) |
                                  ((src[i + 2] >> 4) & 0xF))];
                *bufout++ = base[(((src[i + 2] << 1) & 0x1E) |
                                  ((src[i + 3] >> 7) & 0x1))];
                *bufout++ = base[((src[i + 3] >> 2) & 0x1F)];
                *bufout++ = base[((src[i + 3] << 3) & 0x18)];
                if (!(flags & APR_ENCODE_NOPADDING)) {
                    *bufout++ = '=';
                }
            }
        }

        dlen = bufout - dest;
        dest[dlen] = '\0';
    }
    else {
        dlen = ((count + 4u) / 5u) * 8u + 1u;
        if (dlen <= count) {
            status = APR_ENOSPC;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(const char *)apr_pencode_base32(apr_pool_t * p, const char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!src) {
        return NULL;
    }

    switch (apr_encode_base32(NULL, src, slen, flags, &size)) {
    case APR_SUCCESS:{
            char *cmd = apr_palloc(p, size);
            if (cmd) {
                apr_encode_base32(cmd, src, slen, flags, len);
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(const char *)apr_pencode_base32_binary(apr_pool_t * p, const unsigned char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!src) {
        return NULL;
    }

    switch (apr_encode_base32_binary(NULL, src, slen, flags, &size)) {
    case APR_SUCCESS:{
            char *cmd = apr_palloc(p, size);
            if (cmd) {
                apr_encode_base32_binary(cmd, src, slen, flags, len);
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(apr_status_t) apr_decode_base32(char *dest, const char *src,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;

    if (src && slen == APR_ENCODE_STRING) {
        count = strlen(src);
    }
    else if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (src) {
        const unsigned char *bufin;
        const unsigned char *pr2;

        if ((flags & APR_ENCODE_BASE32HEX)) {
            pr2 = pr2fivehex;
        }
        else {
            pr2 = pr2five;
        }

        bufin = (const unsigned char *)src;
        while (count) {
            if (pr2[*bufin] >= 32) {
                if (!(flags & APR_ENCODE_RELAXED)) {
                    if (count <= 6) {
                        do {
                            if (pr2[bufin[count - 1]] <= 32)
                                break;
                        } while (--count);
                    }
                    if (count) {
                        status = APR_BADCH;
                    }
                }
                break;
            }
            count--;
            bufin++;
        }
        count = bufin - (const unsigned char *)src;

        if (dest) {
            unsigned char *bufout;

            bufout = (unsigned char *)dest;
            bufin = (const unsigned char *)src;

            while (count >= 8) {
                *(bufout++) = TO_NATIVE(pr2[bufin[0]] << 3 |
                                        pr2[bufin[1]] >> 2);
                *(bufout++) = TO_NATIVE(pr2[bufin[1]] << 6 |
                                        pr2[bufin[2]] << 1 |
                                        pr2[bufin[3]] >> 4);
                *(bufout++) = TO_NATIVE(pr2[bufin[3]] << 4 |
                                        pr2[bufin[4]] >> 1);
                *(bufout++) = TO_NATIVE(pr2[bufin[4]] << 7 |
                                        pr2[bufin[5]] << 2 |
                                        pr2[bufin[6]] >> 3);
                *(bufout++) = TO_NATIVE(pr2[bufin[6]] << 5 |
                                        pr2[bufin[7]]);
                bufin += 8;
                count -= 8;
            }

            if (count == 1) {
                status = APR_EINCOMPLETE;
            }
            if (count >= 2) {
                *(bufout++) = TO_NATIVE(pr2[bufin[0]] << 3 |
                                        pr2[bufin[1]] >> 2);
            }
            if (count == 3) {
                status = APR_EINCOMPLETE;
            }
            if (count >= 4) {
                *(bufout++) = TO_NATIVE(pr2[bufin[1]] << 6 |
                                        pr2[bufin[2]] << 1 |
                                        pr2[bufin[3]] >> 4);
            }
            if (count >= 5) {
                *(bufout++) = TO_NATIVE(pr2[bufin[3]] << 4 |
                                        pr2[bufin[4]] >> 1);
            }
            if (count == 6) {
                status = APR_EINCOMPLETE;
            }
            if (count >= 7) {
                *(bufout++) = TO_NATIVE(pr2[bufin[4]] << 7 |
                                        pr2[bufin[5]] << 2 |
                                        pr2[bufin[6]] >> 3);
            }

            dlen = bufout - (unsigned char *)dest;
            dest[dlen] = '\0';
        }
    }

    if (!src || !dest) {
        dlen = (count / 8u) * 5u + 1u;
        switch (count % 8) {
        case 7:
            dlen += 4;
            break;
        case 6:
            status = APR_EINCOMPLETE;
        case 5:
            dlen += 3;
            break;
        case 4:
            dlen += 2;
            break;
        case 3:
            status = APR_EINCOMPLETE;
        case 2:
            dlen++;
            break;
        case 1:
            status = APR_EINCOMPLETE;
            break;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(apr_status_t) apr_decode_base32_binary(unsigned char *dest,
             const char *src, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;

    if (src && slen == APR_ENCODE_STRING) {
        count = strlen(src);
    }
    else if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (src) {
        const unsigned char *bufin;
        const unsigned char *pr2;

        if ((flags & APR_ENCODE_BASE32HEX)) {
            pr2 = pr2fivehex;
        }
        else {
            pr2 = pr2five;
        }

        bufin = (const unsigned char *)src;
        while (count) {
            if (pr2[*bufin] >= 32) {
                if (!(flags & APR_ENCODE_RELAXED)) {
                    if (count <= 6) {
                        do {
                            if (pr2[bufin[count - 1]] <= 32)
                                break;
                        } while (--count);
                    }
                    if (count) {
                        status = APR_BADCH;
                    }
                }
                break;
            }
            count--;
            bufin++;
        }
        count = bufin - (const unsigned char *)src;

        if (dest) {
            unsigned char *bufout;

            bufout = (unsigned char *)dest;
            bufin = (const unsigned char *)src;

            while (count >= 8) {
                *(bufout++) = (pr2[bufin[0]] << 3 |
                               pr2[bufin[1]] >> 2);
                *(bufout++) = (pr2[bufin[1]] << 6 |
                               pr2[bufin[2]] << 1 |
                               pr2[bufin[3]] >> 4);
                *(bufout++) = (pr2[bufin[3]] << 4 |
                               pr2[bufin[4]] >> 1);
                *(bufout++) = (pr2[bufin[4]] << 7 |
                               pr2[bufin[5]] << 2 |
                               pr2[bufin[6]] >> 3);
                *(bufout++) = (pr2[bufin[6]] << 5 |
                               pr2[bufin[7]]);
                bufin += 8;
                count -= 8;
            }

            if (count == 1) {
                status = APR_EINCOMPLETE;
            }
            if (count >= 2) {
                *(bufout++) = (pr2[bufin[0]] << 3 |
                               pr2[bufin[1]] >> 2);
            }
            if (count == 3) {
                status = APR_EINCOMPLETE;
            }
            if (count >= 4) {
                *(bufout++) = (pr2[bufin[1]] << 6 |
                               pr2[bufin[2]] << 1 |
                               pr2[bufin[3]] >> 4);
            }
            if (count >= 5) {
                *(bufout++) = (pr2[bufin[3]] << 4 |
                               pr2[bufin[4]] >> 1);
            }
            if (count == 6) {
                status = APR_EINCOMPLETE;
            }
            if (count >= 7) {
                *(bufout++) = (pr2[bufin[4]] << 7 |
                               pr2[bufin[5]] << 2 |
                               pr2[bufin[6]] >> 3);
            }

            dlen = bufout - dest;
        }
    }

    if (!src || !dest) {
        dlen = (count / 8u) * 5u;
        switch (count % 8) {
        case 7:
            dlen += 4;
            break;
        case 6:
            status = APR_EINCOMPLETE;
        case 5:
            dlen += 3;
            break;
        case 4:
            dlen += 2;
            break;
        case 3:
            status = APR_EINCOMPLETE;
        case 2:
            dlen++;
            break;
        case 1:
            status = APR_EINCOMPLETE;
            break;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(const char *)apr_pdecode_base32(apr_pool_t * p, const char *str,
                              apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!str) {
        return NULL;
    }

    switch (apr_decode_base32(NULL, str, slen, flags, &size)) {
    case APR_SUCCESS:{
            void *cmd = apr_palloc(p, size);
            if (cmd) {
                apr_decode_base32(cmd, str, slen, flags, len);
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(const unsigned char *)apr_pdecode_base32_binary(apr_pool_t * p,
             const char *str, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!str) {
        return NULL;
    }

    switch (apr_decode_base32_binary(NULL, str, slen, flags, &size)) {
    case APR_SUCCESS:{
            unsigned char *cmd = apr_palloc(p, size + 1);
            if (cmd) {
                apr_decode_base32_binary(cmd, str, slen, flags, len);
                cmd[size] = 0;
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(apr_status_t) apr_encode_base16(char *dest,
             const char *src, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;

    if (src && slen == APR_ENCODE_STRING) {
        count = strlen(src);
    }
    else if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (dest) {
        char *bufout = dest;
        const char *base;
        apr_size_t i;

        if ((flags & APR_ENCODE_LOWER)) {
            base = base16lower;
        }
        else {
            base = base16;
        }

        for (i = 0; i < count; i++) {
            if ((flags & APR_ENCODE_COLON) && i) {
                *(bufout++) = ':';
            }
            *(bufout++) = base[TO_ASCII(src[i]) >> 4];
            *(bufout++) = base[TO_ASCII(src[i]) & 0xf];
        }

        dlen = bufout - dest;
        dest[dlen] = '\0';
    }
    else {
        dlen = count * 2u + 1u;
        if (dlen <= count) {
            status = APR_ENOSPC;
        }
        if ((flags & APR_ENCODE_COLON) && count > 1) {
            apr_size_t more = dlen + count - 1;
            if (more <= dlen) {
                status = APR_ENOSPC;
            }
            dlen = more;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(apr_status_t) apr_encode_base16_binary(char *dest,
    const unsigned char *src, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;

    if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (dest) {
        char *bufout = dest;
        const char *base;
        apr_size_t i;

        if ((flags & APR_ENCODE_LOWER)) {
            base = base16lower;
        }
        else {
            base = base16;
        }

        for (i = 0; i < count; i++) {
            if ((flags & APR_ENCODE_COLON) && i) {
                *(bufout++) = ':';
            }
            *(bufout++) = base[src[i] >> 4];
            *(bufout++) = base[src[i] & 0xf];
        }

        dlen = bufout - dest;
        dest[dlen] = '\0';
    }
    else {
        dlen = count * 2u + 1u;
        if (dlen <= count) {
            status = APR_ENOSPC;
        }
        if ((flags & APR_ENCODE_COLON) && count > 1) {
            apr_size_t more = dlen + count - 1;
            if (more <= dlen) {
                status = APR_ENOSPC;
            }
            dlen = more;
        }
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(const char *)apr_pencode_base16(apr_pool_t * p,
             const char *src, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!src) {
        return NULL;
    }

    switch (apr_encode_base16(NULL, src, slen, flags, &size)) {
    case APR_SUCCESS:{
            char *cmd = apr_palloc(p, size);
            if (cmd) {
                apr_encode_base16(cmd, src, slen, flags, len);
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(const char *)apr_pencode_base16_binary(apr_pool_t * p,
                      const unsigned char *src, apr_ssize_t slen, int flags,
                                                   apr_size_t * len)
{
    apr_size_t size;

    if (!src) {
        return NULL;
    }

    switch (apr_encode_base16_binary(NULL, src, slen, flags, &size)) {
    case APR_SUCCESS:{
            char *cmd = apr_palloc(p, size);
            if (cmd) {
                apr_encode_base16_binary(cmd, src, slen, flags, len);
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(apr_status_t) apr_decode_base16(char *dest,
             const char *src, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;

    if (src && slen == APR_ENCODE_STRING) {
        count = strlen(src);
    }
    else if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (src) {
        const unsigned char *bufin;

        bufin = (const unsigned char *)src;
        while (count) {
            if (pr2two[*bufin] >= 16
                && (!(flags & APR_ENCODE_COLON)
                    || pr2two[*bufin] != 32 /* ':' */)) {
                if (!(flags & APR_ENCODE_RELAXED)) {
                    status = APR_BADCH;
                }
                break;
            }
            count--;
            bufin++;
        }
        count = bufin - (const unsigned char *)src;

        if (dest) {
            unsigned char *bufout;

            bufout = (unsigned char *)dest;
            bufin = (const unsigned char *)src;

            while (count >= 2) {
                if (pr2two[bufin[0]] == 32 /* ':' */) {
                    bufin += 1;
                    count -= 1;
                }
                else {
                    *(bufout++) = TO_NATIVE(pr2two[bufin[0]] << 4 |
                                            pr2two[bufin[1]]);
                    bufin += 2;
                    count -= 2;
                }
            }

            if (count == 1) {
                status = APR_EINCOMPLETE;
            }

            dlen = bufout - (unsigned char *)dest;
            dest[dlen] = '\0';
        }
    }

    if (!src || !dest) {
        if (flags & APR_ENCODE_COLON) {
            if (count && (count + 1u) % 3u) {
                status = APR_EINCOMPLETE;
            }
            count -= count / 3u;
        }
        if (count % 2u) {
            status = APR_EINCOMPLETE;
        }
        dlen = count / 2u + 1u;
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(apr_status_t) apr_decode_base16_binary(unsigned char *dest,
             const char *src, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = slen, dlen = 0;

    if (src && slen == APR_ENCODE_STRING) {
        count = strlen(src);
    }
    else if (slen < 0 || (dest && !src)) {
        return (src) ? APR_EINVAL : APR_NOTFOUND;
    }

    if (src) {
        const unsigned char *bufin;

        bufin = (const unsigned char *)src;
        while (count) {
            if (pr2two[*bufin] >= 16
                && (!(flags & APR_ENCODE_COLON)
                    || pr2two[*bufin] != 32 /* ':' */)) {
                if (!(flags & APR_ENCODE_RELAXED)) {
                    status = APR_BADCH;
                }
                break;
            }
            count--;
            bufin++;
        }
        count = bufin - (const unsigned char *)src;

        if (dest) {
            unsigned char *bufout;

            bufout = (unsigned char *)dest;
            bufin = (const unsigned char *)src;

            while (count >= 2) {
                if (pr2two[bufin[0]] == 32 /* ':' */) {
                    bufin += 1;
                    count -= 1;
                }
                else {
                    *(bufout++) = (pr2two[bufin[0]] << 4 |
                                   pr2two[bufin[1]]);
                    bufin += 2;
                    count -= 2;
                }
            }

            if (count == 1) {
                status = APR_EINCOMPLETE;
            }

            dlen = bufout - (unsigned char *)dest;
        }
    }

    if (!src || !dest) {
        if (flags & APR_ENCODE_COLON) {
            if (count && (count + 1u) % 3u) {
                status = APR_EINCOMPLETE;
            }
            count -= count / 3u;
        }
        if (count % 2u) {
            status = APR_EINCOMPLETE;
        }
        dlen = count / 2u;
    }

    if (len) {
        *len = dlen;
    }
    return status;
}

APR_DECLARE(const char *)apr_pdecode_base16(apr_pool_t * p,
             const char *str, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!str) {
        return NULL;
    }

    switch (apr_decode_base16(NULL, str, slen, flags, &size)) {
    case APR_SUCCESS:{
            void *cmd = apr_palloc(p, size);
            if (cmd) {
                apr_decode_base16(cmd, str, slen, flags, len);
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}

APR_DECLARE(const unsigned char *)apr_pdecode_base16_binary(apr_pool_t * p,
             const char *str, apr_ssize_t slen, int flags, apr_size_t * len)
{
    apr_size_t size;

    if (!str) {
        return NULL;
    }

    switch (apr_decode_base16_binary(NULL, str, slen, flags, &size)) {
    case APR_SUCCESS:{
            unsigned char *cmd = apr_palloc(p, size + 1);
            if (cmd) {
                apr_decode_base16_binary(cmd, str, slen, flags, len);
                cmd[size] = 0;
            }
            return cmd;
        }
    default:{
            break;
        }
    }

    return NULL;
}
