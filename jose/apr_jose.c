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

APR_DECLARE(apr_status_t) apr_jose_make(apr_jose_t **jose, apr_jose_type_e type,
        apr_pool_t *pool)
{
    apr_jose_t *j;

    if (*jose) {
        j = *jose;
    } else {
        *jose = j = apr_pcalloc(pool, sizeof(apr_jose_t));
        if (!j) {
            return APR_ENOMEM;
        }
    }

    j->pool = pool;
    j->type = type;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_data_make(apr_jose_t **jose, const char *typ,
        const unsigned char *in, apr_size_t inlen, apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_DATA, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    j->typ = typ;
    j->jose.data = apr_palloc(pool, sizeof(apr_jose_data_t));
    if (!j->jose.data) {
        return APR_ENOMEM;
    }
    j->jose.data->data = in;
    j->jose.data->len = inlen;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_json_make(apr_jose_t **jose, const char *cty,
        apr_json_value_t *json, apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_JSON, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    j->cty = cty;
    j->jose.json = apr_palloc(pool, sizeof(apr_jose_json_t));
    if (!j->jose.json) {
        return APR_ENOMEM;
    }
    j->jose.json->json = json;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_signature_make(
        apr_jose_signature_t **signature, apr_json_value_t *header,
        apr_json_value_t *protected, apr_pool_t *pool)
{
    apr_jose_signature_t *s;

    *signature = s = apr_pcalloc(pool, sizeof(apr_jose_signature_t));
    if (!s) {
        return APR_ENOMEM;
    }

    s->header = header;
    s->protected_header = protected;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_recipient_make(
        apr_jose_recipient_t **recipient, apr_json_value_t *header,
        apr_pool_t *pool)
{
    apr_jose_recipient_t *r;

    *recipient = r = apr_pcalloc(pool, sizeof(apr_jose_recipient_t));
    if (!r) {
        return APR_ENOMEM;
    }

    r->header = header;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_encryption_make(
        apr_jose_encryption_t **encryption, apr_json_value_t *header,
        apr_json_value_t *protected_header, apr_pool_t *pool)
{
    apr_jose_encryption_t *e;

    *encryption = e = apr_pcalloc(pool, sizeof(apr_jose_encryption_t));
    if (!e) {
        return APR_ENOMEM;
    }

    e->unprotected = header;
    e->protected = protected_header;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_jwe_make(apr_jose_t **jose,
        apr_jose_recipient_t *recipient, apr_array_header_t *recipients,
        apr_jose_encryption_t *encryption, apr_jose_t *payload,
        apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_jose_jwe_t *jwe;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_JWE, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    j->cty = payload->cty;

    jwe = j->jose.jwe = apr_palloc(pool, sizeof(apr_jose_jwe_t));
    if (!jwe) {
        return APR_ENOMEM;
    }

    jwe->recipient = recipient;
    jwe->recipients = recipients;
    jwe->encryption = encryption;
    jwe->payload = payload;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_jwe_json_make(apr_jose_t **jose,
        apr_jose_recipient_t *recipient, apr_array_header_t *recipients,
        apr_jose_encryption_t *encryption, apr_jose_t *payload,
        apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_jose_jwe_t *jwe;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_JWE_JSON, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    if (payload) {
        j->cty = payload->cty;
    }

    jwe = j->jose.jwe = apr_palloc(pool, sizeof(apr_jose_jwe_t));
    if (!jwe) {
        return APR_ENOMEM;
    }

    jwe->recipient = recipient;
    jwe->recipients = recipients;
    jwe->encryption = encryption;
    jwe->payload = payload;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_jwk_make(apr_jose_t **jose,
        apr_json_value_t *key, apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_jose_jwk_t *jwk;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_JWK, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    jwk = j->jose.jwk = apr_palloc(pool, sizeof(apr_jose_jwk_t));
    if (!jwk) {
        return APR_ENOMEM;
    }

    jwk->key = key;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_jwks_make(apr_jose_t **jose,
        apr_json_value_t *keys, apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_jose_jwks_t *jwks;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_JWKS, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    jwks = j->jose.jwks = apr_palloc(pool, sizeof(apr_jose_jwks_t));
    if (!jwks) {
        return APR_ENOMEM;
    }

    jwks->keys = keys;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_jws_make(apr_jose_t **jose,
        apr_jose_signature_t *signature, apr_array_header_t *signatures,
        apr_jose_t *payload, apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_jose_jws_t *jws;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_JWS, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    if (payload) {
        j->cty = payload->cty;
    }

    jws = j->jose.jws = apr_pcalloc(pool, sizeof(apr_jose_jws_t));
    if (!jws) {
        return APR_ENOMEM;
    }

    jws->signature = signature;
    jws->signatures = signatures;
    jws->payload = payload;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_jws_json_make(apr_jose_t **jose,
        apr_jose_signature_t *signature, apr_array_header_t *signatures,
        apr_jose_t *payload, apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_jose_jws_t *jws;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_JWS_JSON, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    if (payload) {
        j->cty = payload->cty;
    }

    jws = j->jose.jws = apr_pcalloc(pool, sizeof(apr_jose_jws_t));
    if (!jws) {
        return APR_ENOMEM;
    }

    jws->signature = signature;
    jws->signatures = signatures;
    jws->payload = payload;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_jwt_make(apr_jose_t **jose, apr_json_value_t *claims,
        apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_jose_jwt_t *jwt;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_JWT, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    j->cty = "JWT";

    jwt = j->jose.jwt = apr_palloc(pool, sizeof(apr_jose_jwt_t));
    if (!jwt) {
        return APR_ENOMEM;
    }

    jwt->claims = claims;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_jose_text_make(apr_jose_t **jose, const char *cty,
        const char *in, apr_size_t inlen, apr_pool_t *pool)
{
    apr_jose_t *j;
    apr_status_t status;

    status = apr_jose_make(jose, APR_JOSE_TYPE_TEXT, pool);
    if (APR_SUCCESS != status) {
        return status;
    }
    j = *jose;

    j->cty = cty;
    j->jose.text = apr_palloc(pool, sizeof(apr_jose_text_t));
    if (!j->jose.text) {
        return APR_ENOMEM;
    }
    j->jose.text->text = in;
    j->jose.text->len = inlen;

    return APR_SUCCESS;
}
