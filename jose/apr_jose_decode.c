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
 * levelations under the License.
 */

#include "apr_jose.h"
#include "apr_lib.h"
#include "apr_encode.h"

static
apr_status_t apr_jose_flatten(apr_bucket_brigade *bb, apr_jose_text_t *in,
        apr_pool_t *pool)
{
    apr_bucket *e;

    /* most common case - one pool bucket, avoid unnecessary duplication */
    e = APR_BRIGADE_FIRST(bb);
    if (e != APR_BRIGADE_SENTINEL(bb)) {
        if (!APR_BUCKET_NEXT(e) && APR_BUCKET_IS_POOL(e)) {
            apr_bucket_pool *p = e->data;
            if (pool == p->pool) {
                return apr_bucket_read(e, &in->text, &in->len, APR_BLOCK_READ);
            }
        }
    }

    return apr_brigade_pflatten(bb, (char **)&in->text, &in->len, pool);
}

static
apr_status_t apr_jose_decode_jwk(apr_jose_t **jose,
        const char *typ, apr_bucket_brigade *bb, apr_jose_cb_t *cb,
        int level, int flags, apr_pool_t *pool)
{
    apr_json_value_t *key;
    apr_jose_text_t in;
    apr_off_t offset;
    apr_status_t status;

    status = apr_jose_flatten(bb, &in, pool);
    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_json_decode(&key, in.text, in.len, &offset,
            APR_JSON_FLAGS_WHITESPACE, level, pool);

    *jose = apr_jose_jwk_make(NULL, key, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }

    if (APR_SUCCESS != status) {
        char buf[1024];
        apr_strerror(status, buf, sizeof(buf));
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWK decoding failed at offset %" APR_OFF_T_FMT ": %s",
                                        offset, buf);

        return status;
    }

    return APR_SUCCESS;
}

static
apr_status_t apr_jose_decode_jwks(apr_jose_t **jose,
        const char *typ, apr_bucket_brigade *bb, apr_jose_cb_t *cb,
        int level, int flags, apr_pool_t *pool)
{
    apr_json_value_t *keys;
    apr_jose_text_t in;
    apr_off_t offset;
    apr_status_t status;

    status = apr_jose_flatten(bb, &in, pool);
    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_json_decode(&keys, in.text, in.len,
            &offset, APR_JSON_FLAGS_WHITESPACE, level, pool);

    *jose = apr_jose_jwks_make(NULL, keys, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }

    if (APR_SUCCESS != status) {
        char buf[1024];
        apr_strerror(status, buf, sizeof(buf));
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWKS decoding failed at offset %" APR_OFF_T_FMT ": %s",
                offset, buf);

        return status;
    }

    if (keys->type != APR_JSON_ARRAY) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWKS 'keys' is not an array");
        return APR_EINVAL;
    }

    return APR_SUCCESS;
}

static
apr_status_t apr_jose_decode_jwt(apr_jose_t **jose,
        const char *typ, apr_bucket_brigade *bb, apr_jose_cb_t *cb,
        int level, int flags, apr_pool_t *pool)
{
    apr_json_value_t *claims;
    apr_jose_text_t in;
    apr_off_t offset;
    apr_status_t status;

    status = apr_jose_flatten(bb, &in, pool);
    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_json_decode(&claims, in.text, in.len, &offset,
            APR_JSON_FLAGS_WHITESPACE, level, pool);

    *jose = apr_jose_jwt_make(NULL, claims, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }

    if (APR_SUCCESS != status) {
        char buf[1024];
        apr_strerror(status, buf, sizeof(buf));
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWT decoding failed at offset %" APR_OFF_T_FMT ": %s",
                offset, buf);

        return status;
    }

    return APR_SUCCESS;
}

static
apr_status_t apr_jose_decode_data(apr_jose_t **jose, const char *typ,
        apr_bucket_brigade *brigade, apr_jose_cb_t *cb, int level, int flags,
        apr_pool_t *pool)
{
    apr_jose_text_t in;
    apr_status_t status;

    status = apr_jose_flatten(brigade, &in, pool);
    if (APR_SUCCESS != status) {
        return status;
    }

    *jose = apr_jose_data_make(NULL, typ, (const unsigned char *) in.text,
            in.len, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }

    return status;
}

static
apr_status_t apr_jose_decode_jws_signature(apr_jose_t **jose,
        apr_jose_signature_t *signature, const char *typ, const char *cty,
        apr_jose_text_t *ph64, apr_jose_text_t *sig64, apr_jose_text_t *pl64,
        apr_json_value_t *uh, apr_jose_cb_t *cb, int level, int *flags,
        apr_pool_t *pool, apr_bucket_brigade *bb)
{
    const char *phs;
    apr_size_t phlen;
    apr_off_t offset;
    apr_status_t status = APR_SUCCESS;

    /*
     * Base64url-decode the encoded representation of the JWS Protected
     * Header, following the restriction that no line breaks,
     * whitespace, or other additional characters have been used.
     */

    phs = apr_pdecode_base64(pool, ph64->text, ph64->len, APR_ENCODE_BASE64URL,
            &phlen);

    if (!phs) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS 'protected' is not valid base64url");
        return APR_EINVAL;
    }

    /*
     * Verify that the resulting octet sequence is a UTF-8-encoded
     * representation of a completely valid JSON object conforming to
     * RFC 7159 [RFC7159]; let the JWS Protected Header be this JSON
     * object.
     */

    status = apr_json_decode(&signature->protected_header, phs, phlen, &offset,
    APR_JSON_FLAGS_WHITESPACE, level, pool);
    if (APR_SUCCESS != status) {
        char buf[1024];
        apr_strerror(status, buf, sizeof(buf));
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS 'protected' decoding failed at %" APR_OFF_T_FMT ": %s",
                offset, buf);

        return status;
    }

    /*
     * If using the JWS Compact Serialization, let the JOSE Header be
     * the JWS Protected Header.  Otherwise, when using the JWS JSON
     * Serialization, let the JOSE Header be the union of the members of
     * the corresponding JWS Protected Header and JWS Unprotected
     * Header, all of which must be completely valid JSON objects.
     * During this step, verify that the resulting JOSE Header does not
     * contain duplicate Header Parameter names.  When using the JWS
     * JSON Serialization, this restriction includes that the same
     * Header Parameter name also MUST NOT occur in distinct JSON object
     * values that together comprise the JOSE Header.
     */

    if (uh) {
        signature->header = apr_json_overlay(pool, signature->protected_header,
                uh, APR_JSON_FLAGS_STRICT);
    } else {
        signature->header = signature->protected_header;
    }

    /*
     * Verify that the implementation understands and can process all
     * fields that it is required to support, whether required by this
     * specification, by the algorithm being used, or by the "crit"
     * Header Parameter value, and that the values of those parameters
     * are also understood and supported.
     */

    /*
     * Base64url-decode the encoded representation of the JWS Signature,
     * following the restriction that no line breaks, whitespace, or
     * other additional characters have been used.
     */

    signature->sig.data = apr_pdecode_base64_binary(pool, sig64->text,
            sig64->len,
            APR_ENCODE_BASE64URL, &signature->sig.len);
    if (!signature->sig.data) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS signature decoding failed: bad character");
        return APR_BADCH;
    }

    /*
     * The verify function is expected to perform some or all of the
     * following steps:
     *
     * FIXME fill in from RFC
     */

    status = cb->verify(bb, *jose, signature, cb->ctx, flags, pool);

    return status;
}

static
apr_status_t apr_jose_decode_jwe_recipient(apr_jose_t **jose,
        apr_bucket_brigade *bb, apr_jose_recipient_t *recipient,
        apr_jose_encryption_t *encryption, const char *typ, const char *cty,
        apr_jose_text_t *ph64, apr_jose_text_t *aad64, apr_jose_cb_t *cb,
        int level, int *dflags, apr_pool_t *pool)
{
    apr_json_value_t *header;
    apr_status_t status;

    /*
     * If using the JWE Compact Serialization, let the JOSE Header be
     * the JWE Protected Header.  Otherwise, when using the JWE JSON
     * Serialization, let the JOSE Header be the union of the members
     * of the JWE Protected Header, the JWE Shared Unprotected Header
     * and the corresponding JWE Per-Recipient Unprotected Header, all
     * of which must be completely valid JSON objects.  During this
     * step, verify that the resulting JOSE Header does not contain
     * duplicate Header Parameter names.  When using the JWE JSON
     * Serialization, this restriction includes that the same Header
     * Parameter name also MUST NOT occur in distinct JSON object
     * values that together comprise the JOSE Header.
     */

    header = apr_json_overlay(pool, recipient->header,
            apr_json_overlay(pool, encryption->protected,
                    encryption->unprotected, APR_JSON_FLAGS_STRICT),
            APR_JSON_FLAGS_STRICT);

    if (!header) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "JWE decryption failed: protected, unprotected and per "
                "recipient headers had an overlapping element, or were all missing");
        return APR_EINVAL;
    }

    /*
     * Verify that the implementation understands and can process all
     * fields that it is required to support, whether required by this
     * specification, by the algorithms being used, or by the "crit"
     * Header Parameter value, and that the values of those parameters
     * are also understood and supported.
     */

    /*
     * The decrypt function is expected to perform some or all of the
     * following steps:
     *
     * 6.   Determine the Key Management Mode employed by the algorithm
     * specified by the "alg" (algorithm) Header Parameter.
     *
     * 7.   Verify that the JWE uses a key known to the recipient.
     *
     * 8.   When Direct Key Agreement or Key Agreement with Key Wrapping are
     *      employed, use the key agreement algorithm to compute the value
     *      of the agreed upon key.  When Direct Key Agreement is employed,
     *      let the CEK be the agreed upon key.  When Key Agreement with Key
     *      Wrapping is employed, the agreed upon key will be used to
     *      decrypt the JWE Encrypted Key.
     *
     * 9.   When Key Wrapping, Key Encryption, or Key Agreement with Key
     *      Wrapping are employed, decrypt the JWE Encrypted Key to produce
     *      the CEK.  The CEK MUST have a length equal to that required for
     *      the content encryption algorithm.  Note that when there are
     *       multiple recipients, each recipient will only be able to decrypt
     *       JWE Encrypted Key values that were encrypted to a key in that
     *       recipient's possession.  It is therefore normal to only be able
     *       to decrypt one of the per-recipient JWE Encrypted Key values to
     *       obtain the CEK value.  Also, see Section 11.5 for security
     *       considerations on mitigating timing attacks.
     *
     *  10.  When Direct Key Agreement or Direct Encryption are employed,
     *       verify that the JWE Encrypted Key value is an empty octet
     *       sequence.
     *
     *  11.  When Direct Encryption is employed, let the CEK be the shared
     *       symmetric key.
     *
     *  12.  Record whether the CEK could be successfully determined for this
     *       recipient or not.
     *
     *  13.  If the JWE JSON Serialization is being used, repeat this process
     *       (steps 4-12) for each recipient contained in the representation.
     *
     *  14.  Compute the Encoded Protected Header value BASE64URL(UTF8(JWE
     *       Protected Header)).  If the JWE Protected Header is not present
     *       (which can only happen when using the JWE JSON Serialization and
     *       no "protected" member is present), let this value be the empty
     *       string.
     *
     *  15.  Let the Additional Authenticated Data encryption parameter be
     *       ASCII(Encoded Protected Header).  However, if a JWE AAD value is
     *       present (which can only be the case when using the JWE JSON
     *       Serialization), instead let the Additional Authenticated Data
     *       encryption parameter be ASCII(Encoded Protected Header || '.' ||
     *       BASE64URL(JWE AAD)).
     *
     *  16.  Decrypt the JWE Ciphertext using the CEK, the JWE Initialization
     *       Vector, the Additional Authenticated Data value, and the JWE
     *       Authentication Tag (which is the Authentication Tag input to the
     *       calculation) using the specified content encryption algorithm,
     *       returning the decrypted plaintext and validating the JWE
     *       Authentication Tag in the manner specified for the algorithm,
     *       rejecting the input without emitting any decrypted output if the
     *       JWE Authentication Tag is incorrect.
     *
     *  17.  If a "zip" parameter was included, uncompress the decrypted
     *       plaintext using the specified compression algorithm.
     */

    status = cb->decrypt(bb, *jose, recipient, encryption, header, ph64, aad64,
            cb->ctx, dflags, pool);

    recipient->status = status;

    return status;
}

static
apr_status_t apr_jose_decode_compact_jws(apr_jose_t **jose,
        const char *left, const char *right,
        apr_json_value_t *ph, const char *typ, const char *cty,
        apr_jose_text_t *in, apr_jose_text_t *ph64, apr_jose_cb_t *cb,
        int level, int flags, apr_pool_t *pool, apr_bucket_brigade *bb)
{
    apr_jose_jws_t *jws;
    apr_jose_text_t sig64;
    apr_jose_text_t pl64;
    apr_jose_text_t pls;
    const char *dot;
    apr_bucket *e;
    apr_status_t status = APR_EINVAL;
    int vflags = APR_JOSE_FLAG_NONE;

    if (!cb || !cb->verify) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Verification failed: no verify callback provided");
        return APR_EINIT;
    }

    *jose = apr_jose_jws_make(*jose, NULL, NULL, NULL, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }
    jws = (*jose)->jose.jws;

    /*
     * If using the JWS Compact Serialization, let the JOSE Header be
     * the JWS Protected Header.
     */

    jws->signature = apr_jose_signature_make(NULL, NULL, ph, pool);
    if (!jws->signature) {
        return APR_ENOMEM;
    }

    dot = memchr(left, '.', right - left);
    if (!dot) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS compact decoding failed: one lonely dot");
        return APR_BADCH;
    }

    pl64.text = left;
    pl64.len = dot - left;

    left = dot + 1;

    sig64.text = left;
    sig64.len = right - left;

    /*
     * Validate the JWS Signature against the JWS Signing Input
     * ASCII(BASE64URL(UTF8(JWS Protected Header)) || '.' ||
     * BASE64URL(JWS Payload)) in the manner defined for the algorithm
     * being used, which MUST be accurately represented by the value of
     * the "alg" (algorithm) Header Parameter, which MUST be present.
     * See Section 10.6 for security considerations on algorithm
     * validation.  Record whether the validation succeeded or not.
     */

    status = apr_brigade_write(bb, NULL, NULL, in->text,
            sig64.text - in->text - 1);
    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_jose_decode_jws_signature(jose, jws->signature,
            typ, cty, ph64, &sig64, &pl64, NULL, cb, level, &vflags, pool, bb);

    if (APR_SUCCESS != status) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "JWS verification failed: signature rejected");
        return status;
    }

    /*
     * Base64url-decode the encoded representation of the JWS Payload,
     * following the restriction that no line breaks, whitespace, or
     * other additional characters have been used.
     */

    pls.text = apr_pdecode_base64(pool, pl64.text,
            pl64.len, APR_ENCODE_BASE64URL, &pls.len);
    if (!pls.text) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS 'payload' is not valid base64url");
        return APR_BADCH;
    }


    apr_brigade_cleanup(bb);
    e = apr_bucket_pool_create(pls.text, pls.len, pool,
            bb->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bb, e);

    return APR_SUCCESS;
}

static
apr_status_t apr_jose_decode_compact_jwe(apr_jose_t **jose, const char *left,
        const char *right, apr_json_value_t *ph, apr_json_value_t *enc,
        const char *typ, const char *cty, apr_jose_text_t *ph64,
        apr_jose_cb_t *cb, int level, int flags, apr_pool_t *pool,
        apr_bucket_brigade *bb)
{
    const char *dot;
    apr_jose_jwe_t *jwe;
    apr_jose_text_t aad64;
    apr_status_t status;
    int dflags = APR_JOSE_FLAG_NONE;

    if (!cb || !cb->decrypt) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Decryption failed: no decrypt callback provided");
        return APR_EINIT;
    }

    *jose = apr_jose_jwe_make(*jose, NULL, NULL, NULL, NULL, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }
    jwe = (*jose)->jose.jwe;

    jwe->encryption = apr_jose_encryption_make(NULL, NULL,
            NULL, pool);
    if (!jwe->encryption) {
        return APR_ENOMEM;
    }

    jwe->recipient = apr_jose_recipient_make(NULL, NULL, pool);
    if (!jwe->recipient) {
        return APR_ENOMEM;
    }

    /*
     * Parse the JWE representation to extract the serialized values
     * for the components of the JWE.  When using the JWE Compact
     * Serialization, these components are the base64url-encoded
     * representations of the JWE Protected Header, the JWE Encrypted
     * Key, the JWE Initialization Vector, the JWE Ciphertext, and the
     * JWE Authentication Tag, and when using the JWE JSON
     * Serialization, these components also include the base64url-
     * encoded representation of the JWE AAD and the unencoded JWE
     * Shared Unprotected Header and JWE Per-Recipient Unprotected
     * Header values.  When using the JWE Compact Serialization, the
     * JWE Protected Header, the JWE Encrypted Key, the JWE
     * Initialization Vector, the JWE Ciphertext, and the JWE
     * Authentication Tag are represented as base64url-encoded values
     * in that order, with each value being separated from the next by
     * a single period ('.') character, resulting in exactly four
     * delimiting period characters being used.  The JWE JSON
     * Serialization is described in Section 7.2.
     */

    /* protected header */
    if (ph) {
        jwe->encryption->protected = ph;
    }

    /* encrypted key */
    dot = memchr(left, '.', right - left);
    if (!dot) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: compact JWE decoding failed: one lonely dot");
        return APR_BADCH;
    }

    jwe->recipient->ekey.data = apr_pdecode_base64_binary(pool, left,
            dot - left, APR_ENCODE_BASE64URL, &jwe->recipient->ekey.len);
    if (!jwe->recipient->ekey.data) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWE ekey base64url decoding failed at %" APR_SIZE_T_FMT "",
                jwe->recipient->ekey.len);
        return APR_BADCH;
    }

    left = dot + 1;

    /* iv */
    dot = memchr(left, '.', right - left);
    if (!dot) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWE compact decoding failed: only two dots");
        return APR_BADCH;
    }

    jwe->encryption->iv.data = apr_pdecode_base64_binary(pool, left,
            dot - left, APR_ENCODE_BASE64URL, &jwe->encryption->iv.len);
    if (!jwe->encryption->iv.data) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWE iv base64url decoding failed at %" APR_SIZE_T_FMT "",
                                        jwe->encryption->iv.len);
        return APR_BADCH;
    }

    left = dot + 1;

    /* ciphertext */
    dot = memchr(left, '.', right - left);
    if (!dot) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JOSE compact JWE decoding failed: only three dots");

        return APR_BADCH;
    }

    jwe->encryption->cipher.data = apr_pdecode_base64_binary(pool, left,
            dot - left, APR_ENCODE_BASE64URL, &jwe->encryption->cipher.len);
    if (!jwe->encryption->cipher.data) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWE ciphertext base64url decoding failed at %" APR_SIZE_T_FMT "",
                jwe->encryption->cipher.len);

        return APR_BADCH;
    }

    left = dot + 1;

    /* tag */
    jwe->encryption->tag.data = apr_pdecode_base64_binary(pool, left,
            dot - left, APR_ENCODE_BASE64URL, &jwe->encryption->tag.len);
    if (!jwe->encryption->tag.data) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWE tag base64url decoding failed at %" APR_SIZE_T_FMT "",
                jwe->encryption->tag.len);

        return APR_BADCH;
    }

    /* aad is the empty string in compact serialisation */
    memset(&aad64, 0, sizeof(apr_jose_text_t));

    status = apr_jose_decode_jwe_recipient(jose,
            bb, jwe->recipient, jwe->encryption, typ, cty, ph64, &aad64, cb,
            level, &dflags, pool);

    if (APR_SUCCESS != status) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Decryption failed: JWE decryption failed");

        return status;
    }

    return APR_SUCCESS;
}

static
apr_status_t apr_jose_decode_compact(apr_jose_t **jose, const char *typ,
        apr_bucket_brigade *brigade, apr_jose_cb_t *cb, int level, int flags,
        apr_pool_t *pool)
{
    apr_bucket_brigade *bb;
    apr_jose_text_t in;
    apr_jose_text_t ph64;
    apr_jose_text_t phs;
    apr_json_kv_t *kv;
    apr_json_value_t *header;
    const char *left;
    const char *right;
    const char *dot;
    const char *cty = NULL;
    apr_off_t offset;
    apr_status_t status = APR_ENOTIMPL;

    status = apr_jose_flatten(brigade, &in, pool);
    if (APR_SUCCESS != status) {
        return status;
    }

    left = in.text;
    right = in.text + in.len;

    *jose = apr_jose_make(NULL, APR_JOSE_TYPE_NONE, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }

    bb = apr_brigade_create(pool, brigade->bucket_alloc);
    if (!bb) {
        return APR_ENOMEM;
    }

    /*
     * Use a heuristic to see whether this is a JWT, JWE or JWS.
     *
     * This is described in https://tools.ietf.org/html/rfc7519#section-7.2
     */

    /* Verify that the JWT contains at least one period ('.')
     * character.
     */
    dot = memchr(left, '.', in.len);
    if (!dot) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JOSE compact decoding failed: no dots found");

        return APR_BADCH;
    }

    ph64.text = in.text;
    ph64.len = dot - in.text;

    left = dot + 1;

    /*
     * Let the Encoded JOSE Header be the portion of the JWT before the
     * first period ('.') character.
     *
     * Base64url decode the Encoded JOSE Header following the
     * restriction that no line breaks, whitespace, or other additional
     * characters have been used.
     */

    phs.text = apr_pdecode_base64(pool, ph64.text, ph64.len, APR_ENCODE_BASE64URL,
            &phs.len);
    if (!phs.text) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JOSE header base64url decoding failed at %" APR_SIZE_T_FMT "",
                phs.len);

        return APR_BADCH;
    }

    /*
     * Verify that the resulting octet sequence is a UTF-8-encoded
     * representation of a completely valid JSON object conforming to
     * RFC 7159 [RFC7159]; let the JOSE Header be this JSON object.
     */

    status = apr_json_decode(&header, phs.text, phs.len, &offset,
            APR_JSON_FLAGS_WHITESPACE, level, pool);

    if (APR_SUCCESS != status) {
        char buf[1024];
        apr_strerror(status, buf, sizeof(buf));
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JOSE header decoding failed at %" APR_OFF_T_FMT ": %s",
                offset, buf);

        return status;
    }

    kv = apr_json_object_get(header, APR_JOSE_JWSE_CONTENT_TYPE,
            APR_JSON_VALUE_STRING);
    if (kv) {
        if (kv->v->type == APR_JSON_STRING) {
            cty = apr_pstrndup(pool, kv->v->value.string.p,
                    kv->v->value.string.len);
        }
    }

    if (cty) {
        if (!strcasecmp(cty, "JWT") || !strcasecmp(cty, "application/jwt")) {
            typ = "JWT";
        }
    }

    kv = apr_json_object_get(header, APR_JOSE_JWSE_TYPE, APR_JSON_VALUE_STRING);
    if (kv) {
        if (kv->v->type == APR_JSON_STRING) {
            typ = apr_pstrndup(pool, kv->v->value.string.p,
                    kv->v->value.string.len);
        }
    }

    /*
     * Determine whether the JWT is a JWS or a JWE using any of the
     * methods described in Section 9 of [JWE].
     *
     * The JOSE Header for a JWS can also be distinguished from the JOSE
     * Header for a JWE by determining whether an "enc" (encryption
     * algorithm) member exists.  If the "enc" member exists, it is a
     * JWE; otherwise, it is a JWS.
     */

    kv = apr_json_object_get(header, APR_JOSE_JWE_ENCRYPTION,
            APR_JSON_VALUE_STRING);
    if (kv) {
        status = apr_jose_decode_compact_jwe(jose, left, right, header, kv->v,
                typ, cty, &ph64, cb, level, flags, pool, bb);
    } else {
        status = apr_jose_decode_compact_jws(jose, left, right, header, typ, cty, &in, &ph64,
                cb, level, flags, pool, bb);
    }

    if (APR_SUCCESS == status) {

        /*
         * JWT is an anomaly.
         *
         * If we have stripped off one level of JOSE, and the content-type
         * is present and set to JWT, our payload is a next level JOSE.
         *
         * If we have stripped off one level of JOSE, and the content-type
         * is not present but the type is present and set to JWT, our payload
         * is a JSON object containing claims.
         */

        if (!cty && typ
                && (!strcasecmp(typ, "JWT")
                        || !strcasecmp(typ, "application/jwt"))) {

            status = apr_jose_decode_jwt(
                    flags & APR_JOSE_FLAG_DECODE_ALL ?
                            &(*jose)->jose.jws->payload : jose, typ, bb, cb,
                    level, flags, pool);

        }
        else {

            if (level <= 0) {
                apr_errprintf(&(*jose)->result, pool, NULL, 0,
                        "Syntax error: too many nested JOSE payloads");
                return APR_EINVAL;
            }
            level--;

            status = apr_jose_decode(
                    flags & APR_JOSE_FLAG_DECODE_ALL ?
                            &(*jose)->jose.jws->payload : jose, typ, bb, cb,
                    level, flags, pool);
        }

    }

    return status;
}

static
apr_status_t apr_jose_decode_json_jws(apr_jose_t **jose, apr_json_value_t *val,
        const char *typ, const char *cty, apr_json_value_t *pl,
        apr_jose_cb_t *cb, int level, int flags, apr_pool_t *pool,
        apr_bucket_brigade *bb)
{
    apr_jose_text_t ph64;
    apr_jose_text_t sig64;
    apr_jose_text_t pl64;
    apr_jose_text_t pls;
    apr_jose_jws_t *jws;
    apr_json_kv_t *kv;
    apr_json_value_t *uh;
    apr_bucket *e;
    apr_status_t status = APR_EINVAL;
    int vflags = APR_JOSE_FLAG_NONE;

    if (!cb || !cb->verify) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Verification failed: no verify callback provided");

        return APR_EINIT;
    }

    if (pl->type != APR_JSON_STRING) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS 'payload' is not a string");

        return APR_EINVAL;
    }

    pl64.text = pl->value.string.p;
    pl64.len = pl->value.string.len;

    /*
     * Base64url-decode the encoded representation of the JWS Payload,
     * following the restriction that no line breaks, whitespace, or
     * other additional characters have been used.
     */

    pls.text = apr_pdecode_base64(pool, pl64.text,
            pl64.len, APR_ENCODE_BASE64URL, &pls.len);
    if (!pls.text) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS 'payload' is not valid base64url");

        return APR_BADCH;
    }

    *jose = apr_jose_jws_json_make(*jose, NULL, NULL, NULL, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }
    jws = (*jose)->jose.jws;

    /* for each signature in signatures... */
    kv = apr_json_object_get(val, APR_JOSE_JWS_SIGNATURES,
            APR_JSON_VALUE_STRING);
    if (kv) {
        apr_json_value_t *sigs = kv->v;
        int i;
        int verified = 0;

        if (sigs->type != APR_JSON_ARRAY) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWS 'signatures' is not an array");

            return APR_EINVAL;
        }

        jws->signatures = apr_array_make(pool, sigs->value.array->array->nelts,
                sizeof(apr_jose_signature_t *));
        if (!jws->signatures) {
            return APR_ENOMEM;
        }

        /*
         * If the JWS JSON Serialization is being used, repeat this process
         * (steps 4-8) for each digital signature or MAC value contained in
         * the representation.
         */

        for (i = 0; i < sigs->value.array->array->nelts; i++) {
            apr_json_value_t *sig = apr_json_array_get(sigs, i);

            if (sig) {
                apr_jose_signature_t **sp;

                if (sig->type != APR_JSON_OBJECT) {
                    apr_errprintf(&(*jose)->result, pool, NULL, 0,
                            "Syntax error: JWS 'signatures' array contains a non-object");

                    return APR_EINVAL;
                }

                sp = apr_array_push(jws->signatures);
                *sp = apr_pcalloc(pool, sizeof(apr_jose_signature_t));
                if (!*sp) {
                    return APR_ENOMEM;
                }

                kv = apr_json_object_get(sig, APR_JOSE_JWSE_PROTECTED,
                        APR_JSON_VALUE_STRING);
                if (!kv) {
                    apr_errprintf(&(*jose)->result, pool, NULL, 0,
                            "Syntax error: JWS 'protected' header is missing");

                    return APR_EINVAL;
                }

                if (kv->v->type != APR_JSON_STRING) {
                    apr_errprintf(&(*jose)->result, pool, NULL, 0,
                            "Syntax error: JWS 'protected' is not a string");

                    return APR_EINVAL;
                }

                ph64.text = kv->v->value.string.p;
                ph64.len = kv->v->value.string.len;

                kv = apr_json_object_get(sig, APR_JOSE_JWSE_HEADER,
                        APR_JSON_VALUE_STRING);
                if (kv) {

                    if (kv->v->type != APR_JSON_OBJECT) {
                        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                                "Syntax error: JWS 'header' is not an object");

                        return APR_EINVAL;
                    }

                    uh = kv->v;
                }
                else {
                    uh = NULL;
                }

                kv = apr_json_object_get(sig, APR_JOSE_JWS_SIGNATURE,
                        APR_JSON_VALUE_STRING);
                if (!kv) {
                    apr_errprintf(&(*jose)->result, pool, NULL, 0,
                            "Syntax error: JWS 'signature' header is missing");

                    return APR_EINVAL;
                }

                if (kv->v->type != APR_JSON_STRING) {
                    apr_errprintf(&(*jose)->result, pool, NULL, 0,
                            "Syntax error: JWS 'signature' is not a string");

                    return APR_EINVAL;
                }

                sig64.text = kv->v->value.string.p;
                sig64.len = kv->v->value.string.len;

                /*
                 * Validate the JWS Signature against the JWS Signing Input
                 * ASCII(BASE64URL(UTF8(JWS Protected Header)) || '.' ||
                 * BASE64URL(JWS Payload)) in the manner defined for the algorithm
                 * being used, which MUST be accurately represented by the value of
                 * the "alg" (algorithm) Header Parameter, which MUST be present.
                 * See Section 10.6 for security considerations on algorithm
                 * validation.  Record whether the validation succeeded or not.
                 */

                apr_brigade_cleanup(bb);

                status = apr_brigade_write(bb, NULL, NULL, ph64.text,
                        ph64.len);
                if (APR_SUCCESS != status) {
                    return status;
                }

                status = apr_brigade_putc(bb, NULL, NULL, '.');
                if (APR_SUCCESS != status) {
                    return status;
                }

                status = apr_brigade_write(bb, NULL, NULL, pl64.text,
                        pl64.len);
                if (APR_SUCCESS != status) {
                    return status;
                }

                status = apr_jose_decode_jws_signature(jose, *sp, typ, cty,
                        &ph64, &sig64, &pl64, uh, cb, level, &vflags, pool, bb);

                if (APR_SUCCESS == status) {

                    verified++;

                    if (verified == 1) {

                        apr_brigade_cleanup(bb);
                        e = apr_bucket_pool_create(pls.text, pls.len, pool,
                                bb->bucket_alloc);
                        APR_BRIGADE_INSERT_TAIL(bb, e);

                        if (level <= 0) {
                            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                                    "Syntax error: too many nested JOSE payloads");
                            return APR_EINVAL;
                        }
                        level--;

                        status = apr_jose_decode(
                                flags & APR_JOSE_FLAG_DECODE_ALL ?
                                        &(*jose)->jose.jwe->payload : jose, typ,
                                bb, cb, level, flags, pool);

                        if (APR_SUCCESS != status) {
                            return status;
                        }

                    }

                }

                if (!(vflags & APR_JOSE_FLAG_BREAK)) {
                    break;
                }

            }

        }

        if (!verified) {
            apr_jose_t *j = *jose;

            if (!j->result.msg) {
                apr_errprintf(&(*jose)->result, pool, NULL, 0,
                        "JWS verification failed: no signatures matched");
            }

            return APR_ENOVERIFY;
        }

        return APR_SUCCESS;
    }

    jws->signature = apr_jose_signature_make(NULL, NULL, NULL,
            pool);
    if (!jws->signature) {
        return APR_ENOMEM;
    }

    kv = apr_json_object_get(val, APR_JOSE_JWSE_PROTECTED,
            APR_JSON_VALUE_STRING);
    if (!kv) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS 'protected' header is missing");

        return APR_EINVAL;
    }

    if (kv->v->type != APR_JSON_STRING) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS 'protected' is not a string");

        return APR_EINVAL;
    }

    ph64.text = kv->v->value.string.p;
    ph64.len = kv->v->value.string.len;

    kv = apr_json_object_get(val, APR_JOSE_JWSE_HEADER, APR_JSON_VALUE_STRING);
    if (kv) {

        if (kv->v->type != APR_JSON_OBJECT) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWS 'header' is not an object");

            return APR_EINVAL;
        }

        uh = kv->v;
    }
    else {
        uh = NULL;
    }

    kv = apr_json_object_get(val, APR_JOSE_JWS_SIGNATURE,
            APR_JSON_VALUE_STRING);
    if (!kv) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS 'signature' header is missing");

        return APR_EINVAL;
    }

    if (kv->v->type != APR_JSON_STRING) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWS 'signature' is not a string");

        return APR_EINVAL;
    }

    sig64.text = kv->v->value.string.p;
    sig64.len = kv->v->value.string.len;

    /*
     * Validate the JWS Signature against the JWS Signing Input
     * ASCII(BASE64URL(UTF8(JWS Protected Header)) || '.' ||
     * BASE64URL(JWS Payload)) in the manner defined for the algorithm
     * being used, which MUST be accurately represented by the value of
     * the "alg" (algorithm) Header Parameter, which MUST be present.
     * See Section 10.6 for security considerations on algorithm
     * validation.  Record whether the validation succeeded or not.
     */

    apr_brigade_cleanup(bb);

    status = apr_brigade_write(bb, NULL, NULL, ph64.text,
            ph64.len);
    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_brigade_putc(bb, NULL, NULL, '.');
    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_brigade_write(bb, NULL, NULL, pl64.text,
            pl64.len);
    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_jose_decode_jws_signature(jose, jws->signature, typ, cty,
            &ph64, &sig64, &pl64, uh, cb, level, &vflags, pool, bb);

    if (APR_SUCCESS != status) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "JWS verification failed: signature rejected");

        return status;
    }

    apr_brigade_cleanup(bb);
    e = apr_bucket_pool_create(pls.text, pls.len, pool,
            bb->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bb, e);

    if (level <= 0) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: too many nested JOSE payloads");
        return APR_EINVAL;
    }
    level--;

    return apr_jose_decode(
            flags & APR_JOSE_FLAG_DECODE_ALL ?
                    &(*jose)->jose.jws->payload : jose, typ, bb, cb,
            level, flags, pool);
}

static
apr_status_t apr_jose_decode_json_jwe(apr_jose_t **jose, apr_json_value_t *val,
        const char *typ, const char *cty, apr_json_value_t *ct,
        apr_jose_cb_t *cb, int level, int flags, apr_pool_t *pool,
        apr_bucket_brigade *bb)
{
    apr_jose_text_t ph64;
    apr_jose_text_t aad64;
    apr_jose_jwe_t *jwe;
    apr_json_kv_t *kv;
    apr_status_t status = APR_EGENERAL;
    int dflags = APR_JOSE_FLAG_NONE;

    if (!cb || !cb->decrypt) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Decryption failed: no decrypt callback provided");

        return APR_EINIT;
    }

    if (ct->type != APR_JSON_STRING) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWE 'ciphertext' is not a string");

        return APR_EINVAL;
    }

    *jose = apr_jose_jwe_json_make(*jose, NULL, NULL, NULL, NULL, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }
    jwe = (*jose)->jose.jwe;

    jwe->encryption = apr_jose_encryption_make(NULL, NULL,
            NULL, pool);
    if (!jwe->encryption) {
        return APR_ENOMEM;
    }

    /*
     * Base64url decode the encoded representations of the JWE
     * Protected Header, the JWE Encrypted Key, the JWE Initialization
     * Vector, the JWE Ciphertext, the JWE Authentication Tag, and the
     * JWE AAD, following the restriction that no line breaks,
     * whitespace, or other additional characters have been used.
     */

    kv = apr_json_object_get(val, APR_JOSE_JWSE_PROTECTED,
            APR_JSON_VALUE_STRING);
    if (kv) {
        const char *phs;
        apr_size_t phlen;
        apr_off_t offset;

        if (kv->v->type != APR_JSON_STRING) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'protected' is not a string");

            return APR_EINVAL;
        }

        /*
         * Verify that the octet sequence resulting from decoding the
         * encoded JWE Protected Header is a UTF-8-encoded representation
         * of a completely valid JSON object conforming to RFC 7159
         * [RFC7159]; let the JWE Protected Header be this JSON object.
         */

        ph64.text = kv->v->value.string.p;
        ph64.len = kv->v->value.string.len;

        phs = apr_pdecode_base64(pool, ph64.text,
                ph64.len, APR_ENCODE_BASE64URL, &phlen);

        if (!phs) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'protected' is not valid base64url");

            return APR_EINVAL;
        }

        status = apr_json_decode(&jwe->encryption->protected, phs, phlen, &offset,
                APR_JSON_FLAGS_WHITESPACE, level, pool);
        if (APR_SUCCESS != status) {
            char buf[1024];
            apr_strerror(status, buf, sizeof(buf));
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'protected' decoding failed at %" APR_OFF_T_FMT ": %s",
                    offset, buf);

            return status;
        }

    }
    else {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWE 'protected' header is missing");

        return APR_EINVAL;
    }

    /* unprotected */
    kv = apr_json_object_get(val, APR_JOSE_JWE_UNPROTECTED,
            APR_JSON_VALUE_STRING);
    if (kv) {

        if (kv->v->type != APR_JSON_OBJECT) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'unprotected' is not an object");

            return APR_EINVAL;
        }

        jwe->encryption->unprotected = kv->v;
    }

    /* ciphertext */
    jwe->encryption->cipher.data = apr_pdecode_base64_binary(pool,
            ct->value.string.p, ct->value.string.len, APR_ENCODE_BASE64URL,
            &jwe->encryption->cipher.len);
    if (!jwe->encryption->cipher.data) {
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JWE 'ciphertext' is not valid base64url");

        return APR_BADCH;
    }

    /* iv */
    kv = apr_json_object_get(val, APR_JOSE_JWE_IV, APR_JSON_VALUE_STRING);
    if (kv) {

        if (kv->v->type != APR_JSON_STRING) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'iv' is not a string");

            return APR_EINVAL;
        }

        jwe->encryption->iv.data = apr_pdecode_base64_binary(pool,
                kv->v->value.string.p, kv->v->value.string.len, APR_ENCODE_BASE64URL,
                &jwe->encryption->iv.len);
        if (!jwe->encryption->iv.data) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'iv' is not valid base64url");

            return APR_BADCH;
        }

    }

    /* tag */
    kv = apr_json_object_get(val, APR_JOSE_JWE_TAG, APR_JSON_VALUE_STRING);
    if (kv) {

        if (kv->v->type != APR_JSON_STRING) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'tag' is not a string");

            return APR_EINVAL;
        }

        jwe->encryption->tag.data = apr_pdecode_base64_binary(pool,
                kv->v->value.string.p, kv->v->value.string.len, APR_ENCODE_BASE64URL,
                &jwe->encryption->tag.len);
        if (!jwe->encryption->tag.data) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'tag' is not valid base64url");

            return APR_BADCH;
        }

    }

    /* aad */
    kv = apr_json_object_get(val, APR_JOSE_JWE_AAD, APR_JSON_VALUE_STRING);
    if (kv) {

        if (kv->v->type != APR_JSON_STRING) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'aad' is not a string");

            return APR_EINVAL;
        }

        aad64.text = kv->v->value.string.p;
        aad64.len = kv->v->value.string.len;

        jwe->encryption->aad.data = apr_pdecode_base64_binary(pool,
                aad64.text, aad64.len, APR_ENCODE_BASE64URL,
                &jwe->encryption->aad.len);
        if (!jwe->encryption->aad.data) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'add' is not valid base64url");

            return APR_BADCH;
        }

    }
    else {
        memset(&aad64, 0, sizeof(apr_jose_text_t));
    }

    /* for each recipient in recipients... */
    kv = apr_json_object_get(val, APR_JOSE_JWE_RECIPIENTS,
            APR_JSON_VALUE_STRING);
    if (kv) {
        apr_json_value_t *recips = kv->v;
        int i;
        int decrypt = 0;

        if (recips->type != APR_JSON_ARRAY) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'recipients' is not an array");

            return APR_EINVAL;
        }

        (*jose)->jose.jwe->recipients = apr_array_make(pool,
                recips->value.array->array->nelts, sizeof(apr_jose_recipient_t *));
        if (!(*jose)->jose.jwe->recipients) {
            return APR_ENOMEM;
        }

        for (i = 0; i < recips->value.array->array->nelts; i++) {
            apr_json_value_t *recip = apr_json_array_get(recips, i);

            if (recip) {
                apr_jose_recipient_t **rp;
                apr_jose_recipient_t *recipient;

                if (recip->type != APR_JSON_OBJECT) {
                    apr_errprintf(&(*jose)->result, pool, NULL, 0,
                            "Syntax error: JWE 'recipients' array contains a non-object");

                    return APR_EINVAL;
                }

                rp = apr_array_push((*jose)->jose.jwe->recipients);
                *rp = recipient = apr_pcalloc(pool, sizeof(apr_jose_recipient_t));
                if (!recipient) {
                    return APR_ENOMEM;
                }

                /* unprotected */
                kv = apr_json_object_get(recip, APR_JOSE_JWSE_HEADER,
                        APR_JSON_VALUE_STRING);
                if (kv) {

                    if (kv->v->type != APR_JSON_OBJECT) {
                        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                                "Syntax error: JWE 'header' is not an object");

                        return APR_EINVAL;
                    }

                    recipient->header = kv->v;
                }

                kv = apr_json_object_get(recip, APR_JOSE_JWE_EKEY,
                        APR_JSON_VALUE_STRING);
                if (kv) {

                    if (kv->v->type != APR_JSON_STRING) {
                        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                                "Syntax error: JWE 'encrypted_key' element must be a string");

                        return APR_EINVAL;
                    }

                    recipient->ekey.data = apr_pdecode_base64_binary(pool,
                            kv->v->value.string.p, kv->v->value.string.len, APR_ENCODE_BASE64URL,
                            &recipient->ekey.len);
                    if (!recipient->ekey.data) {
                        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                                "Syntax error: JWE 'encrypted_key' is not valid base64url");

                        return APR_BADCH;
                    }

                }

                apr_brigade_cleanup(bb);

                status = apr_jose_decode_jwe_recipient(jose, bb, recipient,
                        jwe->encryption, typ, cty, &ph64, &aad64, cb, level,
                        &dflags, pool);

                if (APR_SUCCESS == status) {

                    decrypt++;

                    if (decrypt == 1) {

                        if (level <= 0) {
                            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                                    "Syntax error: too many nested JOSE payloads");
                            return APR_EINVAL;
                        }
                        level--;

                        status = apr_jose_decode(
                                flags & APR_JOSE_FLAG_DECODE_ALL ?
                                        &(*jose)->jose.jwe->payload : jose, typ,
                                        bb, cb, level, flags, pool);

                        if (APR_SUCCESS != status) {
                            return status;
                        }

                    }

                }

                if (!(dflags & APR_JOSE_FLAG_BREAK)) {
                    break;
                }

            }

        }

        if (!decrypt) {
            apr_jose_t *j = *jose;

            if (!j->result.msg) {
                apr_errprintf(&(*jose)->result, pool, NULL, 0,
                        "JWE decryption failed: no recipients matched");
            }

            return APR_ECRYPT;
        }

        return APR_SUCCESS;
    }

    /* ok, just one recipient */
    kv = apr_json_object_get(val, APR_JOSE_JWE_EKEY, APR_JSON_VALUE_STRING);
    if (kv) {
        apr_json_value_t *ekey = kv->v;
        apr_jose_recipient_t *recipient;

        if (ekey->type != APR_JSON_STRING) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Syntax error: JWE 'encrypted_key' element must be a string");

            return APR_EINVAL;
        }

        recipient = apr_pcalloc(pool, sizeof(apr_jose_recipient_t));
        if (!recipient) {
            return APR_ENOMEM;
        }

        /* unprotected */
        kv = apr_json_object_get(val, APR_JOSE_JWSE_HEADER,
                APR_JSON_VALUE_STRING);
        if (kv) {

            if (kv->v->type != APR_JSON_OBJECT) {
                apr_errprintf(&(*jose)->result, pool, NULL, 0,
                        "Syntax error: JWE 'header' is not an object");

                return APR_EINVAL;
            }

            recipient->header = kv->v;
        }

        apr_brigade_cleanup(bb);

        status = apr_jose_decode_jwe_recipient(jose, bb, recipient,
                jwe->encryption, typ, cty, &ph64, &aad64, cb, level, &dflags,
                pool);

        if (APR_SUCCESS == status) {

            if (level <= 0) {
                apr_errprintf(&(*jose)->result, pool, NULL, 0,
                        "Syntax error: too many nested JOSE payloads");
                return APR_EINVAL;
            }
            level--;

            return apr_jose_decode(
                    flags & APR_JOSE_FLAG_DECODE_ALL ?
                            &(*jose)->jose.jwe->payload : jose, typ, bb,
                    cb, level, flags, pool);

        }

        if (APR_SUCCESS != status) {
            apr_errprintf(&(*jose)->result, pool, NULL, 0,
                    "Decryption failed: JWE decryption failed");
        }

    }

    /* no recipient at all */
    apr_errprintf(&(*jose)->result, pool, NULL, 0,
            "Syntax error: No 'recipients' or 'encrypted_key' present");

    return APR_EINVAL;

}

static
apr_status_t apr_jose_decode_json(apr_jose_t **jose, const char *typ,
        apr_bucket_brigade *brigade, apr_jose_cb_t *cb, int level,
        int flags, apr_pool_t *pool)
{
    apr_json_value_t *val;
    apr_bucket_brigade *bb;
    apr_jose_text_t in;
    apr_off_t offset;
    apr_status_t status;

    *jose = apr_jose_make(NULL, APR_JOSE_TYPE_NONE, pool);
    if (!*jose) {
        return APR_ENOMEM;
    }

    status = apr_jose_flatten(brigade, &in, pool);
    if (APR_SUCCESS != status) {
        return status;
    }

    bb = apr_brigade_create(pool, brigade->bucket_alloc);
    if (!bb) {
        return APR_ENOMEM;
    }

    /*
     * Parse the JWS representation to extract the serialized values for
     * the components of the JWS.  When using the JWS Compact
     * Serialization, these components are the base64url-encoded
     * representations of the JWS Protected Header, the JWS Payload, and
     * the JWS Signature, and when using the JWS JSON Serialization,
     * these components also include the unencoded JWS Unprotected
     * Header value.  When using the JWS Compact Serialization, the JWS
     * Protected Header, the JWS Payload, and the JWS Signature are
     * represented as base64url-encoded values in that order, with each
     * value being separated from the next by a single period ('.')
     * character, resulting in exactly two delimiting period characters
     * being used.  The JWS JSON Serialization is described in
     * Section 7.2.
     */

    status = apr_json_decode(&val, in.text, in.len, &offset,
            APR_JSON_FLAGS_WHITESPACE, level, pool);
    if (APR_SUCCESS == status) {
        apr_json_kv_t *kv;
        const char *cty = NULL;

        /*
         * 9.  Distinguishing between JWS and JWE Objects
         *
         * If the object is using the JWS JSON Serialization or the JWE JSON
         * Serialization, the members used will be different.  JWSs have a
         * "payload" member and JWEs do not.  JWEs have a "ciphertext" member
         * and JWSs do not.
         */

        /* are we JWS? */
        kv = apr_json_object_get(val, APR_JOSE_JWS_PAYLOAD,
                APR_JSON_VALUE_STRING);
        if (kv) {

            return apr_jose_decode_json_jws(jose, val, typ, cty,
                    kv->v, cb, level, flags, pool, bb);

        }

        /* are we JWE? */
        kv = apr_json_object_get(val, APR_JOSE_JWE_CIPHERTEXT,
                APR_JSON_VALUE_STRING);
        if (kv) {

            return apr_jose_decode_json_jwe(jose, val, typ, cty,
                    kv->v, cb, level, flags, pool, bb);

        }

        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JOSE JSON contained neither a 'payload' nor a 'ciphertext'");

        return APR_EINVAL;

    }
    else {
        char buf[1024];
        apr_strerror(status, buf, sizeof(buf));
        apr_errprintf(&(*jose)->result, pool, NULL, 0,
                "Syntax error: JOSE JSON decoding failed at character %" APR_OFF_T_FMT ": %s",
                offset, buf);
    }

    return status;
}

apr_status_t apr_jose_decode(apr_jose_t **jose, const char *typ,
        apr_bucket_brigade *brigade, apr_jose_cb_t *cb, int level, int flags,
        apr_pool_t *pool)
{

    /* handle JOSE and JOSE+JSON */
    if (typ) {
        switch (typ[0]) {
        case 'a':
        case 'A': {

            if (!strncasecmp(typ, "application/", 12)) {
                const char *sub = typ + 12;

                if (!strcasecmp(sub, "jwt")) {
                    return apr_jose_decode_compact(jose, typ, brigade, cb,
                            level, flags, pool);
                } else if (!strcasecmp(sub, "jose")) {
                    return apr_jose_decode_compact(jose, NULL, brigade, cb,
                            level, flags, pool);
                } else if (!strcasecmp(sub, "jose+json")) {
                    return apr_jose_decode_json(jose, NULL, brigade, cb, level,
                            flags, pool);
                } else if (!strcasecmp(sub, "jwk+json")) {
                    return apr_jose_decode_jwk(jose, typ, brigade, cb, level,
                            flags, pool);
                } else if (!strcasecmp(sub, "jwk-set+json")) {
                    return apr_jose_decode_jwks(jose, typ, brigade, cb, level,
                            flags, pool);
                }

            }

            break;
        }
        case 'J':
        case 'j': {

            if (!strcasecmp(typ, "JWT")) {
                return apr_jose_decode_compact(jose, typ, brigade, cb, level, flags,
                        pool);
            } else if (!strcasecmp(typ, "JOSE")) {
                return apr_jose_decode_compact(jose, NULL, brigade, cb, level,
                        flags, pool);
            } else if (!strcasecmp(typ, "JOSE+JSON")) {
                return apr_jose_decode_json(jose, NULL, brigade, cb, level, flags,
                        pool);
            } else if (!strcasecmp(typ, "JWK+JSON")) {
                return apr_jose_decode_jwk(jose, typ, brigade, cb, level, flags,
                        pool);
            } else if (!strcasecmp(typ, "JWK-SET+JSON")) {
                return apr_jose_decode_jwks(jose, typ, brigade, cb, level, flags,
                        pool);
            }

            break;
        }
        }
    }

    return apr_jose_decode_data(jose, typ, brigade, cb, level, flags, pool);
}
