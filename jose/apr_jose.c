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

APR_DECLARE(apu_err_t *) apr_jose_error(apr_jose_t *jose)
{
    return &jose->result;
}

APR_DECLARE(apr_jose_t *) apr_jose_make(apr_jose_t *jose, apr_jose_type_e type,
        apr_pool_t *pool)
{

    if (!jose) {
        jose = apr_pcalloc(pool, sizeof(apr_jose_t));
        if (!jose) {
            return NULL;
        }
    }

    jose->pool = pool;
    jose->type = type;

    return jose;
}

APR_DECLARE(apr_jose_t *) apr_jose_data_make(apr_jose_t *jose, const char *typ,
        const unsigned char *in, apr_size_t inlen, apr_pool_t *pool)
{

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_DATA, pool);
        if (!jose) {
            return NULL;
        }
    }

    jose->typ = typ;
    jose->jose.data = apr_palloc(pool, sizeof(apr_jose_data_t));
    if (!jose->jose.data) {
        return NULL;
    }
    jose->jose.data->data = in;
    jose->jose.data->len = inlen;

    return jose;
}

APR_DECLARE(apr_jose_t *) apr_jose_json_make(apr_jose_t *jose, const char *cty,
        apr_json_value_t *json, apr_pool_t *pool)
{

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_JSON, pool);
        if (!jose) {
            return NULL;
        }
    }

    jose->cty = cty;
    jose->jose.json = apr_palloc(pool, sizeof(apr_jose_json_t));
    if (!jose->jose.json) {
        return NULL;
    }
    jose->jose.json->json = json;

    return jose;
}

APR_DECLARE(apr_jose_signature_t *) apr_jose_signature_make(
        apr_jose_signature_t *signature, apr_json_value_t *header,
        apr_json_value_t *protected, apr_pool_t *pool)
{

    if (!signature) {
        signature = apr_pcalloc(pool, sizeof(apr_jose_signature_t));
        if (!signature) {
            return NULL;
        }
    }

    signature->header = header;
    signature->protected_header = protected;

    return signature;
}

APR_DECLARE(apr_jose_recipient_t *) apr_jose_recipient_make(
        apr_jose_recipient_t *recipient, apr_json_value_t *header,
        apr_pool_t *pool)
{

    if (!recipient) {
        recipient = apr_pcalloc(pool, sizeof(apr_jose_recipient_t));
        if (!recipient) {
            return NULL;
        }
    }

    recipient->header = header;

    return recipient;
}

APR_DECLARE(apr_jose_encryption_t *) apr_jose_encryption_make(
        apr_jose_encryption_t *encryption, apr_json_value_t *header,
        apr_json_value_t *protected_header, apr_pool_t *pool)
{

    if (!encryption) {
        encryption = apr_pcalloc(pool, sizeof(apr_jose_encryption_t));
        if (!encryption) {
            return NULL;
        }
    }

    encryption->unprotected = header;
    encryption->protected = protected_header;

    return encryption;
}

APR_DECLARE(apr_jose_t *) apr_jose_jwe_make(apr_jose_t *jose,
        apr_jose_recipient_t *recipient, apr_array_header_t *recipients,
        apr_jose_encryption_t *encryption, apr_jose_t *payload,
        apr_pool_t *pool)
{
    apr_jose_jwe_t *jwe;

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_JWE, pool);
        if (!jose) {
            return NULL;
        }
    }

    jose->cty = payload->cty;

    jwe = jose->jose.jwe = apr_palloc(pool, sizeof(apr_jose_jwe_t));
    if (!jwe) {
        return NULL;
    }

    jwe->recipient = recipient;
    jwe->recipients = recipients;
    jwe->encryption = encryption;
    jwe->payload = payload;

    return jose;
}

APR_DECLARE(apr_jose_t *) apr_jose_jwe_json_make(apr_jose_t *jose,
        apr_jose_recipient_t *recipient, apr_array_header_t *recipients,
        apr_jose_encryption_t *encryption, apr_jose_t *payload,
        apr_pool_t *pool)
{
    apr_jose_jwe_t *jwe;

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_JWE_JSON, pool);
        if (!jose) {
            return NULL;
        }
    }

    if (payload) {
        jose->cty = payload->cty;
    }

    jwe = jose->jose.jwe = apr_palloc(pool, sizeof(apr_jose_jwe_t));
    if (!jwe) {
        return NULL;
    }

    jwe->recipient = recipient;
    jwe->recipients = recipients;
    jwe->encryption = encryption;
    jwe->payload = payload;

    return jose;
}

APR_DECLARE(apr_jose_t *) apr_jose_jwk_make(apr_jose_t *jose,
        apr_json_value_t *key, apr_pool_t *pool)
{
    apr_jose_jwk_t *jwk;

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_JWK, pool);
        if (!jose) {
            return NULL;
        }
    }

    jwk = jose->jose.jwk = apr_palloc(pool, sizeof(apr_jose_jwk_t));
    if (!jwk) {
        return NULL;
    }

    jwk->key = key;

    return jose;
}

APR_DECLARE(apr_jose_t *) apr_jose_jwks_make(apr_jose_t *jose,
        apr_json_value_t *keys, apr_pool_t *pool)
{
    apr_jose_jwks_t *jwks;

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_JWKS, pool);
        if (!jose) {
            return NULL;
        }
    }

    jwks = jose->jose.jwks = apr_palloc(pool, sizeof(apr_jose_jwks_t));
    if (!jwks) {
        return NULL;
    }

    jwks->keys = keys;

    return jose;
}

APR_DECLARE(apr_jose_t *) apr_jose_jws_make(apr_jose_t *jose,
        apr_jose_signature_t *signature, apr_array_header_t *signatures,
        apr_jose_t *payload, apr_pool_t *pool)
{
    apr_jose_jws_t *jws;

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_JWS, pool);
        if (!jose) {
            return NULL;
        }
    }

    if (payload) {
        jose->cty = payload->cty;
    }

    jws = jose->jose.jws = apr_pcalloc(pool, sizeof(apr_jose_jws_t));
    if (!jws) {
        return NULL;
    }

    jws->signature = signature;
    jws->signatures = signatures;
    jws->payload = payload;

    return jose;
}

APR_DECLARE(apr_jose_t *) apr_jose_jws_json_make(apr_jose_t *jose,
        apr_jose_signature_t *signature, apr_array_header_t *signatures,
        apr_jose_t *payload, apr_pool_t *pool)
{
    apr_jose_jws_t *jws;

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_JWS_JSON, pool);
        if (!jose) {
            return NULL;
        }
    }

    if (payload) {
        jose->cty = payload->cty;
    }

    jws = jose->jose.jws = apr_pcalloc(pool, sizeof(apr_jose_jws_t));
    if (!jws) {
        return NULL;
    }

    jws->signature = signature;
    jws->signatures = signatures;
    jws->payload = payload;

    return jose;
}

APR_DECLARE(apr_jose_t *) apr_jose_jwt_make(apr_jose_t *jose, apr_json_value_t *claims,
        apr_pool_t *pool)
{
    apr_jose_jwt_t *jwt;

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_JWT, pool);
        if (!jose) {
            return NULL;
        }
    }

    jose->cty = "JWT";

    jwt = jose->jose.jwt = apr_palloc(pool, sizeof(apr_jose_jwt_t));
    if (!jwt) {
        return NULL;
    }

    jwt->claims = claims;

    return jose;
}

APR_DECLARE(apr_jose_t *) apr_jose_text_make(apr_jose_t *jose, const char *cty,
        const char *in, apr_size_t inlen, apr_pool_t *pool)
{

    if (!jose) {
        jose = apr_jose_make(jose, APR_JOSE_TYPE_TEXT, pool);
        if (!jose) {
            return NULL;
        }
    }

    jose->cty = cty;
    jose->jose.text = apr_palloc(pool, sizeof(apr_jose_text_t));
    if (!jose->jose.text) {
        return NULL;
    }
    jose->jose.text->text = in;
    jose->jose.text->len = inlen;

    return jose;
}
