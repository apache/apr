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

#include "apr_jose.h"
#include "apr_encode.h"

static apr_status_t apr_jose_encode_base64_json(apr_bucket_brigade *brigade,
        apr_brigade_flush flush, void *ctx, apr_json_value_t *json,
        apr_pool_t *pool)
{
    apr_status_t status = APR_SUCCESS;

    if (json) {

        apr_bucket_brigade *bb;

        bb = apr_brigade_create(pool, brigade->bucket_alloc);

        status = apr_json_encode(bb, flush, ctx, json,
                APR_JSON_FLAGS_WHITESPACE, pool);
        if (APR_SUCCESS == status) {

            char *buf;
            apr_size_t buflen;
            const char *buf64;
            apr_size_t buf64len;

            apr_brigade_pflatten(bb, &buf, &buflen, pool);
            buf64 = apr_pencode_base64(pool, buf, buflen, APR_ENCODE_BASE64URL,
                    &buf64len);
            status = apr_brigade_write(brigade, flush, ctx, buf64, buf64len);

        }

    }

    return status;
}

static apr_status_t apr_jose_encode_compact_jwe(apr_bucket_brigade *brigade,
        apr_brigade_flush flush, void *ctx, apr_jose_t *jose, apr_jose_cb_t *cb,
        apr_pool_t *p)
{
    apr_bucket_brigade *bb = apr_brigade_create(p,
            brigade->bucket_alloc);

    apr_jose_jwe_t *jwe = jose->jose.jwe;

    apr_status_t status = APR_SUCCESS;

    /*
     * 7.1.  JWE Compact Serialization
     *
     * The JWE Compact Serialization represents encrypted content as a
     * compact, URL-safe string.  This string is:
     *
     *      BASE64URL(UTF8(JWE Protected Header)) || '.' ||
     */

    status = apr_jose_encode_base64_json(brigade, flush, ctx,
            jwe->encryption->protected, p);
    if (APR_SUCCESS != status) {
        return status;
    }

    status = apr_brigade_write(brigade, flush, ctx, ".", 1);
    if (APR_SUCCESS != status) {
        return status;
    }

    if (cb && cb->encrypt) {
        status = apr_jose_encode(bb, flush, ctx, jwe->payload, cb, p);
        if (APR_SUCCESS != status) {
            jose->result = jwe->payload->result;
            return status;
        }
        status = cb->encrypt(bb, jose, jwe->recipient, jwe->encryption,
                cb->ctx, p);
        if (APR_SUCCESS != status) {
            return status;
        }
    }

    if (APR_SUCCESS == status) {

        struct iovec vec[7];

        /*
         *    7.   Compute the encoded key value BASE64URL(JWE Encrypted Key).
         *
         *    10.  Compute the encoded Initialization Vector value BASE64URL(JWE
         *         Initialization Vector).
         *
         *    16.  Compute the encoded ciphertext value BASE64URL(JWE Ciphertext).
         *
         *    17.  Compute the encoded Authentication Tag value BASE64URL(JWE
          *         Authentication Tag).
          *
          *    18.  If a JWE AAD value is present, compute the encoded AAD value
          *         BASE64URL(JWE AAD).
          *
         *    19.  Create the desired serialized output.  The Compact Serialization
         *         of this result is the string BASE64URL(UTF8(JWE Protected
         *         Header)) || '.' || BASE64URL(JWE Encrypted Key) || '.' ||
         *         BASE64URL(JWE Initialization Vector) || '.' || BASE64URL(JWE
         *         Ciphertext) || '.' || BASE64URL(JWE Authentication Tag).  The
         *         JWE JSON Serialization is described in Section 7.2.
         */

        /*
         *      BASE64URL(JWE Encrypted Key) || '.' ||
         */

        vec[0].iov_base = (void *) apr_pencode_base64_binary(p,
                jwe->recipient->ekey.data, jwe->recipient->ekey.len,
                APR_ENCODE_BASE64URL, &vec[0].iov_len);
        vec[1].iov_base = ".";
        vec[1].iov_len = 1;

        /*
         *      BASE64URL(JWE Initialization Vector) || '.' ||
         */

        vec[2].iov_base = (void *) apr_pencode_base64_binary(p,
                jwe->encryption->iv.data, jwe->encryption->iv.len,
                APR_ENCODE_BASE64URL, &vec[2].iov_len);
        vec[3].iov_base = ".";
        vec[3].iov_len = 1;

        /*
         *      BASE64URL(JWE Ciphertext) || '.' ||
         */

        vec[4].iov_base = (void *) apr_pencode_base64_binary(p,
                jwe->encryption->cipher.data, jwe->encryption->cipher.len,
                APR_ENCODE_BASE64URL, &vec[4].iov_len);
        vec[5].iov_base = ".";
        vec[5].iov_len = 1;

        /*
         *      BASE64URL(JWE Authentication Tag)
         */

        vec[6].iov_base = (void *)apr_pencode_base64_binary(p, jwe->encryption->tag.data, jwe->encryption->tag.len,
                APR_ENCODE_BASE64URL, &vec[6].iov_len);

        status = apr_brigade_writev(brigade, flush, ctx, vec, 7);

    }

    return status;
}

static apr_status_t apr_jose_encode_compact_jws(apr_bucket_brigade *brigade,
        apr_brigade_flush flush, void *ctx, apr_jose_t *jose, apr_jose_cb_t *cb,
        apr_pool_t *p)
{
    apr_bucket *e;
    apr_bucket_brigade *bb = apr_brigade_create(p,
            brigade->bucket_alloc);
    apr_jose_text_t payload;
    apr_jose_text_t payload64;

    apr_jose_jws_t *jws = jose->jose.jws;

    apr_status_t status;

    status = apr_jose_encode(bb, flush, ctx, jws->payload, cb, p);
    if (APR_SUCCESS != status) {
        jose->result = jws->payload->result;
        return status;
    }

    status = apr_brigade_pflatten(bb, (char **)&payload.text, &payload.len, p);
    if (APR_SUCCESS != status) {
        return status;
    }

    payload64.text = apr_pencode_base64(p, payload.text, payload.len,
            APR_ENCODE_BASE64URL, &payload64.len);

    apr_brigade_cleanup(bb);

    /*
     * 7.1.  JWS Compact Serialization
     *
     *    The JWS Compact Serialization represents digitally signed or MACed
     *    content as a compact, URL-safe string.  This string is:
     *
     *    BASE64URL(UTF8(JWS Protected Header)) || '.' ||
     */

    if (jws->signature) {
        status = apr_jose_encode_base64_json(bb, flush, ctx,
                jws->signature->protected_header, p);
        if (APR_SUCCESS != status) {
            return status;
        }
    }

    status = apr_brigade_write(bb, flush, ctx, ".", 1);
    if (APR_SUCCESS != status) {
        return status;
    }

    /*
     *   BASE64URL(JWS Payload) ||
     */

    e = apr_bucket_pool_create(payload64.text, payload64.len, p,
            bb->bucket_alloc);
    APR_BRIGADE_INSERT_TAIL(bb, e);

    /*
     *    '.' || BASE64URL(JWS Signature)
     */

    if (cb && cb->sign && jws->signature) {
        status = cb->sign(bb, jose, jws->signature, cb->ctx, p);
        if (APR_SUCCESS != status) {
            return status;
        }
    }

    APR_BRIGADE_CONCAT(brigade, bb);

    status = apr_brigade_write(brigade, flush, ctx, ".", 1);
    if (APR_SUCCESS != status) {
        return status;
    }

    if (jws->signature && jws->signature->sig.data && APR_SUCCESS == status) {

        const char *buf64;
        apr_size_t buf64len;

        buf64 = apr_pencode_base64_binary(p, jws->signature->sig.data,
                jws->signature->sig.len,
                APR_ENCODE_BASE64URL, &buf64len);
        status = apr_brigade_write(brigade, flush, ctx, buf64,
                buf64len);

    }

    return status;
}

static apr_status_t apr_jose_encode_json_jwe(apr_bucket_brigade *brigade,
        apr_brigade_flush flush, void *ctx, apr_jose_t *jose, apr_jose_cb_t *cb,
        apr_pool_t *p)
{

    apr_json_value_t *json;
    char *buf;
    const char *buf64;
    apr_size_t len;
    apr_size_t len64;

    apr_bucket_brigade *bb = apr_brigade_create(p,
            brigade->bucket_alloc);

    apr_jose_jwe_t *jwe = jose->jose.jwe;

    apr_status_t status = APR_SUCCESS;

    /* create our json */

    json = apr_json_object_create(p);

    /* create protected header */

    if (jwe->encryption) {
        apr_jose_encryption_t *e = jwe->encryption;

        if (e->protected) {

            status = apr_jose_encode_base64_json(bb, flush, ctx,
                    e->protected, p);
            if (APR_SUCCESS != status) {
                return status;
            }

            status = apr_brigade_pflatten(bb, &buf, &len, p);
            if (APR_SUCCESS != status) {
                return status;
            }

            apr_brigade_cleanup(bb);

            apr_json_object_set(json, "protected",
                    APR_JSON_VALUE_STRING,
                    apr_json_string_create(p, buf, len), p);

        }

        /* create unprotected header */

        if (e->unprotected) {

            apr_json_object_set(json, "unprotected",
                    APR_JSON_VALUE_STRING, e->unprotected, p);

        }

        /* create recipient */
        if (jwe->recipient) {

            apr_jose_recipient_t *recip = jwe->recipient;

            /* create the payload */

            status = apr_jose_encode(bb, flush, ctx, jwe->payload, cb, p);
            if (APR_SUCCESS != status) {
                jose->result = jwe->payload->result;
                return status;
            }

            if (cb && cb->encrypt && recip) {
                status = cb->encrypt(bb, jose, recip, e, cb->ctx, p);
                if (APR_SUCCESS != status) {
                    return status;
                }
            }

            apr_brigade_cleanup(bb);

            /* create header */

            apr_json_object_set(json, "header",
                    APR_JSON_VALUE_STRING, recip->header, p);

            apr_brigade_cleanup(bb);

            /* create encrypted key */

            buf64 = apr_pencode_base64_binary(p, recip->ekey.data,
                    recip->ekey.len,
                    APR_ENCODE_BASE64URL, &len64);

            apr_json_object_set(json, "encrypted_key",
                    APR_JSON_VALUE_STRING,
                    apr_json_string_create(p, buf64, len64), p);

        }

        /* create recipients */
        if (jwe->recipients) {

            apr_json_value_t *recips;
            int i;

            /* create recipients element */

            recips = apr_json_array_create(p, jwe->recipients->nelts);
            apr_json_object_set(json, "recipients",
                    APR_JSON_VALUE_STRING, recips, p);

            /* populate each recipient */

            for (i = 0; i < jwe->recipients->nelts; i++) {
                apr_json_value_t *r = apr_json_object_create(p);
                apr_jose_recipient_t *recip = APR_ARRAY_IDX(
                        jwe->recipients, i, apr_jose_recipient_t *);

                if (!recip) {
                    continue;
                }

                apr_json_array_add(recips, r);

                /* create the payload */

                status = apr_jose_encode(bb, flush, ctx, jwe->payload, cb, p);
                if (APR_SUCCESS != status) {
                    jose->result = jwe->payload->result;
                    return status;
                }

                if (cb && cb->encrypt && recip) {
                    status = cb->encrypt(bb, jose, recip, e, cb->ctx, p);
                    if (APR_SUCCESS != status) {
                        return status;
                    }
                }

                apr_brigade_cleanup(bb);

                /* create header */

                apr_json_object_set(r, "header",
                        APR_JSON_VALUE_STRING, recip->header, p);

                apr_brigade_cleanup(bb);

                /* create encrypted key */

                buf64 = apr_pencode_base64_binary(p, recip->ekey.data,
                        recip->ekey.len,
                        APR_ENCODE_BASE64URL, &len64);

                apr_json_object_set(r, "encrypted_key",
                        APR_JSON_VALUE_STRING,
                        apr_json_string_create(p, buf64, len64), p);

            }
            if (APR_SUCCESS != status) {
                return status;
            }

        }

        /* create iv */

        if (e->iv.len) {

            buf64 = apr_pencode_base64_binary(p, e->iv.data, e->iv.len,
                    APR_ENCODE_BASE64URL, &len64);

            apr_json_object_set(json, "iv", APR_JSON_VALUE_STRING,
                    apr_json_string_create(p, buf64, len64), p);
        }

        /* create aad */

        if (e->aad.len) {

            buf64 = apr_pencode_base64_binary(p, e->aad.data, e->aad.len,
                    APR_ENCODE_BASE64URL, &len64);

            apr_json_object_set(json, "aad", APR_JSON_VALUE_STRING,
                    apr_json_string_create(p, buf64, len64), p);
        }

        /* create ciphertext */

        if (e->cipher.len) {

            buf64 = apr_pencode_base64_binary(p, e->cipher.data, e->cipher.len,
                    APR_ENCODE_BASE64URL, &len64);

            apr_json_object_set(json, "ciphertext", APR_JSON_VALUE_STRING,
                    apr_json_string_create(p, buf64, len64), p);
        }

        /* create tag */

        if (e->tag.len) {

            buf64 = apr_pencode_base64_binary(p, e->tag.data, e->tag.len,
                    APR_ENCODE_BASE64URL, &len64);

            apr_json_object_set(json, "tag", APR_JSON_VALUE_STRING,
                    apr_json_string_create(p, buf64, len64), p);
        }

    }

    /* write out our final result */

    if (json) {
        status = apr_json_encode(brigade, flush, ctx, json,
                APR_JSON_FLAGS_WHITESPACE, p);
    }

    return status;
}

static apr_status_t apr_jose_encode_json_jws(apr_bucket_brigade *brigade,
        apr_brigade_flush flush, void *ctx, apr_jose_t *jose, apr_jose_cb_t *cb,
        apr_pool_t *p)
{
    apr_json_value_t *json;
    char *buf;
    const char *buf64;
    apr_size_t len;
    apr_size_t len64;

    apr_bucket_brigade *bb = apr_brigade_create(p,
            brigade->bucket_alloc);

    apr_jose_jws_t *jws = jose->jose.jws;

    apr_status_t status = APR_SUCCESS;

    /* create our json */

    json = apr_json_object_create(p);

    /* calculate BASE64URL(JWS Payload) */

    status = apr_jose_encode(bb, flush, ctx, jws->payload, cb, p);
    if (APR_SUCCESS != status) {
        jose->result = jws->payload->result;
        return status;
    }

    status = apr_brigade_pflatten(bb, &buf, &len, p);
    if (APR_SUCCESS != status) {
        return status;
    }

    buf64 = apr_pencode_base64(p, buf, len,
            APR_ENCODE_BASE64URL, &len64);

    apr_brigade_cleanup(bb);

    /* add the payload to our json */

    apr_json_object_set(json, "payload", APR_JSON_VALUE_STRING,
            apr_json_string_create(p, buf64, len64), p);

    /* calculate the flattened signature */

    if (jws->signature) {

        /* create protected header */

        status = apr_jose_encode_base64_json(bb, flush, ctx,
                jws->signature->protected_header, p);
        if (APR_SUCCESS != status) {
            return status;
        }

        status = apr_brigade_pflatten(bb, &buf, &len, p);
        if (APR_SUCCESS != status) {
            return status;
        }

        apr_json_object_set(json, "protected", APR_JSON_VALUE_STRING,
                apr_json_string_create(p, buf, len), p);

        status = apr_brigade_write(bb, flush, ctx, ".", 1);
        if (APR_SUCCESS != status) {
            return status;
        }

        status = apr_brigade_write(bb, flush, ctx, buf64, len64);
        if (APR_SUCCESS != status) {
            return status;
        }

        if (cb && cb->sign && jws->signature) {
            status = cb->sign(bb, jose, jws->signature, cb->ctx, p);
            if (APR_SUCCESS != status) {
                return status;
            }
        }

        apr_brigade_cleanup(bb);

        /* create header */

        apr_json_object_set(json, "header", APR_JSON_VALUE_STRING,
                jws->signature->header, p);

        apr_brigade_cleanup(bb);

        /* create signature */

        buf64 = apr_pencode_base64_binary(p,
                jws->signature->sig.data, jws->signature->sig.len,
                APR_ENCODE_BASE64URL, &len64);

        apr_json_object_set(json, "signature", APR_JSON_VALUE_STRING,
                apr_json_string_create(p, buf64, len64), p);

    }

    /* otherwise calculate the general signatures */

    else if (jws->signatures) {

        apr_json_value_t *sigs;
        int i;

        /* create signatures element */

        sigs = apr_json_array_create(p, jws->signatures->nelts);
        apr_json_object_set(json, "signatures", APR_JSON_VALUE_STRING,
                sigs, p);

        /* populate each signature */

        for (i = 0; i < jws->signatures->nelts; i++) {
            apr_json_value_t *s = apr_json_object_create(p);
            apr_jose_signature_t *sig = APR_ARRAY_IDX(
                    jws->signatures, i, apr_jose_signature_t *);

            apr_json_array_add(sigs, s);

            /* create protected header */

            status = apr_jose_encode_base64_json(bb, flush, ctx,
                    sig->protected_header, p);
            if (APR_SUCCESS != status) {
                return status;
            }

            status = apr_brigade_pflatten(bb, &buf, &len, p);
            if (APR_SUCCESS != status) {
                return status;
            }

            /* add protected header to array */

            apr_json_object_set(s, "protected", APR_JSON_VALUE_STRING,
                    apr_json_string_create(p, buf, len), p);

            status = apr_brigade_write(bb, flush, ctx, ".", 1);
            if (APR_SUCCESS != status) {
                return status;
            }

            status = apr_brigade_write(bb, flush, ctx, buf64, len64);
            if (APR_SUCCESS != status) {
                return status;
            }

            if (cb && cb->sign && sig) {
                status = cb->sign(bb, jose, sig, cb->ctx, p);
                if (APR_SUCCESS != status) {
                    return status;
                }
            }

            apr_brigade_cleanup(bb);

            /* create header */

            apr_json_object_set(s, "header", APR_JSON_VALUE_STRING,
                    sig->header, p);

            apr_brigade_cleanup(bb);

            /* create signature */

            buf64 = apr_pencode_base64_binary(p, sig->sig.data,
                    sig->sig.len,
                    APR_ENCODE_BASE64URL, &len64);

            apr_json_object_set(s, "signature", APR_JSON_VALUE_STRING,
                    apr_json_string_create(p, buf64, len64), p);

        }
        if (APR_SUCCESS != status) {
            return status;
        }

    }

    /* write out our final result */

    if (json) {
        status = apr_json_encode(brigade, flush, ctx, json,
                APR_JSON_FLAGS_WHITESPACE, p);
    }

    return status;
}

apr_status_t apr_jose_encode(apr_bucket_brigade *brigade,
        apr_brigade_flush flush, void *ctx, apr_jose_t *jose, apr_jose_cb_t *cb,
        apr_pool_t *pool)
{
    apr_pool_t *p;
    apr_status_t status = APR_EINVAL;

    apr_pool_create(&p, pool);
    if (p == NULL) {
        return APR_ENOMEM;
    }

    /* first, generic data types */
    switch (jose->type) {
    case APR_JOSE_TYPE_JWK: {

        /* do nothing for now */

        break;
    }

    case APR_JOSE_TYPE_JWKS: {

        /* do nothing for now */

        break;
    }

    case APR_JOSE_TYPE_DATA: {

        apr_jose_data_t *data = jose->jose.data;

        if (data) {

            struct iovec vec[1];

            vec[0].iov_base = (void *)data->data;
            vec[0].iov_len = data->len;

            status = apr_brigade_writev(brigade, flush, ctx, vec, 1);
            if (APR_SUCCESS != status) {
                break;
            }
        }

        break;
    }

    case APR_JOSE_TYPE_TEXT: {

        apr_jose_text_t *text = jose->jose.text;

        if (text) {
            status = apr_brigade_write(brigade, flush, ctx, text->text,
                    text->len);
            if (APR_SUCCESS != status) {
                break;
            }
        }

        break;
    }

    case APR_JOSE_TYPE_JSON: {

        apr_json_value_t *json = jose->jose.json->json;

        if (json) {
            status = apr_json_encode(brigade, flush, ctx, json,
                    APR_JSON_FLAGS_WHITESPACE, p);
        }

        break;
    }

    case APR_JOSE_TYPE_JWE: {

        status = apr_jose_encode_compact_jwe(brigade, flush, ctx, jose, cb,
                p);

        break;
    }

    case APR_JOSE_TYPE_JWE_JSON: {

        status = apr_jose_encode_json_jwe(brigade, flush, ctx, jose, cb, p);

        break;
    }

    case APR_JOSE_TYPE_JWS: {

        status = apr_jose_encode_compact_jws(brigade, flush, ctx, jose, cb,
                p);

        break;
    }

    case APR_JOSE_TYPE_JWS_JSON: {

        status = apr_jose_encode_json_jws(brigade, flush, ctx, jose, cb, p);

        break;
    }

    case APR_JOSE_TYPE_JWT: {

        apr_json_value_t *claims = jose->jose.jwt->claims;

        if (claims) {
            status = apr_json_encode(brigade, flush, ctx, claims,
                    APR_JSON_FLAGS_WHITESPACE, p);
        }

        break;
    }

    default: {
        apr_errprintf(&jose->result, pool, NULL, 0,
                "JOSE type '%d' not recognised", jose->type);

        status = APR_ENOTIMPL;

        break;
    }
    }

    apr_pool_destroy(p);

    return status;
}
