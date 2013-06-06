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
 * The apr_vsnprintf/apr_snprintf functions are based on, and used with the
 * permission of, the  SIO stdio-replacement strx_* functions by Panos
 * Tsirigotis <panos@alumni.cs.colorado.edu> for xinetd.
 */

/**
 * @file apr_base64.h
 * @brief APR-UTIL Base64 Encoding
 */
#ifndef APR_BASE64_H
#define APR_BASE64_H

#include "apu.h"
#include "apr_general.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup APR_Util_Base64 Base64 Encoding
 * @ingroup APR
 * @{
 */

/* Simple BASE64 encode/decode functions.
 * 
 * As we might encode binary strings, hence we require the length of
 * the incoming plain source. And return the length of what we decoded.
 *
 * The decoding function takes any non valid char (i.e. whitespace, \0
 * or anything non A-Z,0-9 etc) as terminal.
 * 
 * The handling of terminating \0 characters differs from function to
 * function.
 *
 */

/**
 * Given the length of an un-encoded string, get the length of the
 * encoded string.
 * @param len the length of an unencoded string.
 * @return the length of the string after it is encoded, including the
 * trailing \0
 */ 
APR_DECLARE(int) apr_base64_encode_len(int len) __attribute__((pure));

/**
 * Encode a text string using base64encoding. On EBCDIC machines, the input
 * is first converted to ASCII.
 * @param coded_dst The destination string for the encoded string. A \0 is
 * appended.
 * @param plain_src The original string in plain text
 * @param len_plain_src The length of the plain text string
 * @return the length of the encoded string, including the trailing \0
 */ 
APR_DECLARE(int) apr_base64_encode(char * coded_dst, const char *plain_src, 
                                   int len_plain_src)
                 __attribute__((nonnull(1,2)));

/**
 * Encode an text string using base64encoding. This is the same as
 * apr_base64_encode() except on EBCDIC machines, where the conversion of the
 * input to ASCII is left out.
 * @param coded_dst The destination string for the encoded string. A \0 is
 * appended.
 * @param plain_src The original string in plain text
 * @param len_plain_src The length of the plain text string
 * @return the length of the encoded string, including the trailing \0
 */ 
APR_DECLARE(int) apr_base64_encode_binary(char * coded_dst,
                                          const unsigned char *plain_src,
                                          int len_plain_src)
                 __attribute__((nonnull(1,2)));

/**
 * Encode a string into memory allocated from a pool in base 64 format
 * @param p The pool to allocate from
 * @param string The plaintext string
 * @return The encoded string
 */
APR_DECLARE(char *) apr_pbase64_encode(apr_pool_t *p, const char *string)
        __attribute__((nonnull(1,2)));

/**
 * Determine the maximum buffer length required to decode the plain text
 * string given the encoded string.
 * @param coded_src The encoded string
 * @return the maximum required buffer length for the plain text string
 */ 
APR_DECLARE(int) apr_base64_decode_len(const char * coded_src)
                 __attribute__((nonnull(1))) __attribute__((pure));

/**
 * Decode a string to plain text. On EBCDIC machines, the result is then
 * converted to EBCDIC.
 * @param plain_dst The destination string for the plain text. A \0 is
 * appended.
 * @param coded_src The encoded string 
 * @return the length of the plain text string (excluding the trailing \0)
 */ 
APR_DECLARE(int) apr_base64_decode(char * plain_dst, const char *coded_src)
                 __attribute__((nonnull(1,2)));

/**
 * Decode an string to plain text. This is the same as apr_base64_decode()
 * except no \0 is appended and on EBCDIC machines, the conversion of the
 * output to EBCDIC is left out.
 * @param plain_dst The destination string for the plain text. The string is
 * not \0-terminated.
 * @param coded_src The encoded string 
 * @return the length of the plain text string
 */ 
APR_DECLARE(int) apr_base64_decode_binary(unsigned char * plain_dst, 
                                          const char *coded_src)
                 __attribute__((nonnull(1,2)));

/**
 * Decode a base64 encoded string into memory allocated from a pool
 * @param p The pool to allocate from
 * @param bufcoded The encoded string
 * @return The decoded string
 */
APR_DECLARE(char *) apr_pbase64_decode(apr_pool_t *p, const char *bufcoded)
        __attribute__((nonnull(1,2)));

/** @} */
#ifdef __cplusplus
}
#endif

#endif	/* !APR_BASE64_H */
