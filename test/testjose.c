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

#include "apr_jose.h"

#include "abts.h"
#include "testutil.h"


static apr_status_t sign_cb(apr_bucket_brigade *bb, apr_jose_t *jose,
        apr_jose_signature_t *signature, void *ctx, apr_pool_t *pool)
{
    abts_case *tc = ctx;
    apr_json_kv_t *alg = NULL;

    if (signature) {
        apr_json_value_t *ph = signature->protected_header;

        ABTS_INT_EQUAL(tc, APR_JSON_OBJECT, ph->type);

        if (ph->type == APR_JSON_OBJECT) {
            alg = apr_json_object_get(ph, APR_JOSE_JWKSE_ALGORITHM,
                    APR_JSON_VALUE_STRING);
        }
    }

    if (alg) {

        ABTS_INT_EQUAL(tc, APR_JSON_STRING, alg->v->type);

        /* unsecured jws/jwt */
        if (alg->v->type == APR_JSON_STRING
                && !strncmp(alg->v->value.string.p, "none",
                        alg->v->value.string.len)) {

            signature->sig.data = (unsigned const char *) "";
            signature->sig.len = 0;

            return APR_SUCCESS;
        }

        /* hs256 jws - https://tools.ietf.org/html/rfc7515#appendix-A.1 */
        else if (alg->v->type == APR_JSON_STRING
                && !strncmp(alg->v->value.string.p, "HS256",
                        alg->v->value.string.len)) {

            const unsigned char hs256[] = { 116, 24, 223, 180, 151, 153, 224,
                    37, 79, 250, 96, 125, 216, 173, 187, 186, 22, 212, 37, 77,
                    105, 214, 191, 240, 91, 88, 5, 88, 83, 132, 141, 121 };
            signature->sig.data = apr_pmemdup(p, hs256, sizeof(hs256));
            signature->sig.len = sizeof(hs256);

            return APR_SUCCESS;
        }

        /* rs256 jws - https://tools.ietf.org/html/rfc7515#appendix-A.2 */
        else if (alg->v->type == APR_JSON_STRING
                && !strncmp(alg->v->value.string.p, "RS256",
                        alg->v->value.string.len)) {

            const unsigned char rs256[] = { 112, 46, 33, 137, 67, 232, 143, 209,
                    30, 181, 216, 45, 191, 120, 69, 243, 65, 6, 174, 27, 129,
                    255, 247, 115, 17, 22, 173, 209, 113, 125, 131, 101, 109,
                    66, 10, 253, 60, 150, 238, 221, 115, 162, 102, 62, 81, 102,
                    104, 123, 0, 11, 135, 34, 110, 1, 135, 237, 16, 115, 249,
                    69, 229, 130, 173, 252, 239, 22, 216, 90, 121, 142, 232,
                    198, 109, 219, 61, 184, 151, 91, 23, 208, 148, 2, 190, 237,
                    213, 217, 217, 112, 7, 16, 141, 178, 129, 96, 213, 248, 4,
                    12, 167, 68, 87, 98, 184, 31, 190, 127, 249, 217, 46, 10,
                    231, 111, 36, 242, 91, 51, 187, 230, 244, 74, 230, 30, 177,
                    4, 10, 203, 32, 4, 77, 62, 249, 18, 142, 212, 1, 48, 121,
                    91, 212, 189, 59, 65, 238, 202, 208, 102, 171, 101, 25, 129,
                    253, 228, 141, 247, 127, 55, 45, 195, 139, 159, 175, 221,
                    59, 239, 177, 139, 93, 163, 204, 60, 46, 176, 47, 158, 58,
                    65, 214, 18, 202, 173, 21, 145, 18, 115, 160, 95, 35, 185,
                    232, 56, 250, 175, 132, 157, 105, 132, 41, 239, 90, 30, 136,
                    121, 130, 54, 195, 212, 14, 96, 69, 34, 165, 68, 200, 242,
                    122, 122, 45, 184, 6, 99, 209, 108, 247, 202, 234, 86, 222,
                    64, 92, 178, 33, 90, 69, 178, 194, 85, 102, 181, 90, 193,
                    167, 72, 160, 112, 223, 200, 163, 42, 70, 149, 67, 208, 25,
                    238, 251, 71 };
            signature->sig.data = apr_pmemdup(p, rs256, sizeof(rs256));
            signature->sig.len = sizeof(rs256);

            return APR_SUCCESS;
        }

        /* es256 jws - https://tools.ietf.org/html/rfc7515#appendix-A.3 */
        else if (alg->v->type == APR_JSON_STRING
                && !strncmp(alg->v->value.string.p, "ES256",
                        alg->v->value.string.len)) {

            const unsigned char es256[] = { 14, 209, 33, 83, 121, 99, 108, 72,
                    60, 47, 127, 21, 88, 7, 212, 2, 163, 178, 40, 3, 58, 249,
                    124, 126, 23, 129, 154, 195, 22, 158, 166, 101, 197, 10, 7,
                    211, 140, 60, 112, 229, 216, 241, 45, 175, 8, 74, 84, 128,
                    166, 101, 144, 197, 242, 147, 80, 154, 143, 63, 127, 138,
                    131, 163, 84, 213 };
            signature->sig.data = apr_pmemdup(p, es256, sizeof(es256));
            signature->sig.len = sizeof(es256);

            return APR_SUCCESS;
        }

        else {
            apr_errprintf(&jose->result, jose->pool, NULL, 0,
                    "Header 'alg' not recognised: %.*s",
                    (int) alg->v->value.string.len, alg->v->value.string.p);
            return APR_ENOTIMPL;
        }

    }

    else {
        apr_errprintf(&jose->result, jose->pool, NULL, 0,
                "Header 'alg' missing");
        return APR_ENOTIMPL;
    }

    return APR_ENOTIMPL;
}

static apr_status_t verify_cb(apr_bucket_brigade *bb,
        apr_jose_t *jose, apr_jose_signature_t *signature, void *ctx,
        int *vflags, apr_pool_t *pool)
{
    abts_case *tc = ctx;
    apr_json_kv_t *alg = NULL;

    *vflags = APR_JOSE_FLAG_NONE;

    if (signature) {
        apr_json_value_t *ph = signature->protected_header;

        ABTS_INT_EQUAL(tc, APR_JSON_OBJECT, ph->type);

        alg = apr_json_object_get(ph, APR_JOSE_JWKSE_ALGORITHM,
                APR_JSON_VALUE_STRING);
    }

    if (alg) {

        ABTS_INT_EQUAL(tc, APR_JSON_STRING, alg->v->type);

        /* unsecured jws/jwt */
        if (alg->v->type == APR_JSON_STRING
                && !strncmp(alg->v->value.string.p, "none",
                        alg->v->value.string.len)) {

            if (!memcmp(signature->sig.data, (unsigned const char *) "",
                    signature->sig.len)) {
                return APR_SUCCESS;
            }
        }

        /* hs256 jws - https://tools.ietf.org/html/rfc7515#appendix-A.1 */
        else if (alg->v->type == APR_JSON_STRING
                && !strncmp(alg->v->value.string.p, "HS256",
                        alg->v->value.string.len)) {

            const unsigned char hs256[] = { 116, 24, 223, 180, 151, 153, 224,
                    37, 79, 250, 96, 125, 216, 173, 187, 186, 22, 212, 37, 77,
                    105, 214, 191, 240, 91, 88, 5, 88, 83, 132, 141, 121 };

            if (!memcmp(signature->sig.data, hs256, sizeof(hs256))) {
                return APR_SUCCESS;
            }

        }

        /* rs256 jws - https://tools.ietf.org/html/rfc7515#appendix-A.2 */
        else if (alg->v->type == APR_JSON_STRING
                && !strncmp(alg->v->value.string.p, "RS256",
                        alg->v->value.string.len)) {

            const unsigned char rs256[] = { 112, 46, 33, 137, 67, 232, 143, 209,
                    30, 181, 216, 45, 191, 120, 69, 243, 65, 6, 174, 27, 129,
                    255, 247, 115, 17, 22, 173, 209, 113, 125, 131, 101, 109,
                    66, 10, 253, 60, 150, 238, 221, 115, 162, 102, 62, 81, 102,
                    104, 123, 0, 11, 135, 34, 110, 1, 135, 237, 16, 115, 249,
                    69, 229, 130, 173, 252, 239, 22, 216, 90, 121, 142, 232,
                    198, 109, 219, 61, 184, 151, 91, 23, 208, 148, 2, 190, 237,
                    213, 217, 217, 112, 7, 16, 141, 178, 129, 96, 213, 248, 4,
                    12, 167, 68, 87, 98, 184, 31, 190, 127, 249, 217, 46, 10,
                    231, 111, 36, 242, 91, 51, 187, 230, 244, 74, 230, 30, 177,
                    4, 10, 203, 32, 4, 77, 62, 249, 18, 142, 212, 1, 48, 121,
                    91, 212, 189, 59, 65, 238, 202, 208, 102, 171, 101, 25, 129,
                    253, 228, 141, 247, 127, 55, 45, 195, 139, 159, 175, 221,
                    59, 239, 177, 139, 93, 163, 204, 60, 46, 176, 47, 158, 58,
                    65, 214, 18, 202, 173, 21, 145, 18, 115, 160, 95, 35, 185,
                    232, 56, 250, 175, 132, 157, 105, 132, 41, 239, 90, 30, 136,
                    121, 130, 54, 195, 212, 14, 96, 69, 34, 165, 68, 200, 242,
                    122, 122, 45, 184, 6, 99, 209, 108, 247, 202, 234, 86, 222,
                    64, 92, 178, 33, 90, 69, 178, 194, 85, 102, 181, 90, 193,
                    167, 72, 160, 112, 223, 200, 163, 42, 70, 149, 67, 208, 25,
                    238, 251, 71 };

            if (!memcmp(signature->sig.data, rs256, sizeof(rs256))) {
                return APR_SUCCESS;
            }

        }

        /* es256 jws - https://tools.ietf.org/html/rfc7515#appendix-A.3 */
        else if (alg->v->type == APR_JSON_STRING
                && !strncmp(alg->v->value.string.p, "ES256",
                        alg->v->value.string.len)) {

            const unsigned char es256[] = { 14, 209, 33, 83, 121, 99, 108, 72,
                    60, 47, 127, 21, 88, 7, 212, 2, 163, 178, 40, 3, 58, 249,
                    124, 126, 23, 129, 154, 195, 22, 158, 166, 101, 197, 10, 7,
                    211, 140, 60, 112, 229, 216, 241, 45, 175, 8, 74, 84, 128,
                    166, 101, 144, 197, 242, 147, 80, 154, 143, 63, 127, 138,
                    131, 163, 84, 213 };

            if (!memcmp(signature->sig.data, es256, sizeof(es256))) {
                return APR_SUCCESS;
            }

            return APR_SUCCESS;
        }

        else {
            apr_errprintf(&jose->result, jose->pool, NULL, 0,
                    "Header 'alg' not recognised: %.*s",
                    (int) alg->v->value.string.len, alg->v->value.string.p);
            return APR_ENOTIMPL;
        }

    }

    else {
        apr_errprintf(&jose->result, jose->pool, NULL, 0,
                "Header 'alg' missing");
        return APR_ENOTIMPL;
    }

    return APR_ENOTIMPL;
}

static apr_status_t encrypt_cb(apr_bucket_brigade *brigade, apr_jose_t *jose,
        apr_jose_recipient_t *recipient, apr_jose_encryption_t *encryption,
        void *ctx, apr_pool_t *p)
{
    abts_case *tc = ctx;
    apr_json_value_t *protected_header;
    apr_json_value_t *header;
    apr_json_kv_t *alg = NULL;

    if (encryption) {

        if (encryption->protected) {

            protected_header = encryption->protected;
            if (protected_header) {

                char buf[1024];
                apr_size_t len = sizeof(buf);

                apr_bucket_brigade *bb = apr_brigade_create(p,
                        brigade->bucket_alloc);

                apr_json_encode(bb, NULL, NULL, protected_header,
                        APR_JSON_FLAGS_WHITESPACE, p);

                apr_brigade_flatten(bb, buf, &len);

                /* RSAES-OAEP and AES GCM jwe - https://tools.ietf.org/html/rfc7516#appendix-A.1 */
                if (!strncmp("{\"alg\":\"RSA-OAEP\",\"enc\":\"A256GCM\"}", buf, len)) {

                    const unsigned char iv[] = { 227, 197, 117, 252, 2, 219, 233,
                            68, 180, 225, 77, 219 };

                    const unsigned char aad[] = { 101, 121, 74, 104, 98, 71, 99,
                            105, 79, 105, 74, 83, 85, 48, 69, 116, 84, 48, 70, 70,
                            85, 67, 73, 115, 73, 109, 86, 117, 89, 121, 73, 54, 73,
                            107, 69, 121, 78, 84, 90, 72, 81, 48, 48, 105, 102, 81 };

                    const unsigned char cipher[] = { 229, 236, 166, 241, 53, 191,
                            115, 196, 174, 43, 73, 109, 39, 122, 233, 96, 140, 206,
                            120, 52, 51, 237, 48, 11, 190, 219, 186, 80, 111, 104,
                            50, 142, 47, 167, 59, 61, 181, 127, 196, 21, 40, 82,
                            242, 32, 123, 143, 168, 226, 73, 216, 176, 144, 138,
                            247, 106, 60, 16, 205, 160, 109, 64, 63, 192 };

                    const unsigned char tag[] = { 92, 80, 104, 49, 133, 25, 161,
                            215, 173, 101, 219, 211, 136, 91, 210, 145 };

                    encryption->iv.data = apr_pmemdup(p, iv, sizeof(iv));
                    encryption->iv.len = sizeof(iv);

                    encryption->aad.data = apr_pmemdup(p, aad, sizeof(aad));
                    encryption->aad.len = sizeof(aad);

                    encryption->cipher.data = apr_pmemdup(p, cipher, sizeof(cipher));
                    encryption->cipher.len = sizeof(cipher);

                    encryption->tag.data = apr_pmemdup(p, tag, sizeof(tag));
                    encryption->tag.len = sizeof(tag);

                }

                /* RSAES-PKCS1-v1_5 - https://tools.ietf.org/html/rfc7516#appendix-A.2 */
                else if (!strncmp("{\"alg\":\"RSA1_5\",\"enc\":\"A128CBC-HS256\"}", buf, len)) {

                    const unsigned char iv[] = { 3, 22, 60, 12, 43, 67, 104, 105,
                            108, 108, 105, 99, 111, 116, 104, 101 };

                    const unsigned char cipher[] = { 40, 57, 83, 181, 119, 33, 133,
                            148, 198, 185, 243, 24, 152, 230, 6, 75, 129, 223, 127,
                            19, 210, 82, 183, 230, 168, 33, 215, 104, 143, 112, 56,
                            102 };

                    const unsigned char tag[] = { 246, 17, 244, 190, 4, 95, 98, 3,
                            231, 0, 115, 157, 242, 203, 100, 191 };

                    encryption->iv.data = apr_pmemdup(p, iv, sizeof(iv));
                    encryption->iv.len = sizeof(iv);

                    encryption->cipher.data = apr_pmemdup(p, cipher, sizeof(cipher));
                    encryption->cipher.len = sizeof(cipher);

                    encryption->tag.data = apr_pmemdup(p, tag, sizeof(tag));
                    encryption->tag.len = sizeof(tag);

                }

                /* A128KW A128CBC-HS256 - https://tools.ietf.org/html/rfc7516#appendix-A.3 */
                else if (!strncmp("{\"alg\":\"A128KW\",\"enc\":\"A128CBC-HS256\"}", buf, len)) {

                    const unsigned char iv[] = { 3, 22, 60, 12, 43, 67, 104, 105,
                            108, 108, 105, 99, 111, 116, 104, 101 };

                    const unsigned char cipher[] = { 40, 57, 83, 181, 119, 33, 133,
                            148, 198, 185, 243, 24, 152, 230, 6, 75, 129, 223, 127,
                            19, 210, 82, 183, 230, 168, 33, 215, 104, 143, 112, 56,
                            102 };

                    const unsigned char tag[] = { 83, 73, 191, 98, 104, 205, 211,
                            128, 201, 189, 199, 133, 32, 38, 194, 85 };

                    encryption->iv.data = apr_pmemdup(p, iv, sizeof(iv));
                    encryption->iv.len = sizeof(iv);

                    encryption->cipher.data = apr_pmemdup(p, cipher, sizeof(cipher));
                    encryption->cipher.len = sizeof(cipher);

                    encryption->tag.data = apr_pmemdup(p, tag, sizeof(tag));
                    encryption->tag.len = sizeof(tag);

                }

                /* General JWE JSON - https://tools.ietf.org/html/rfc7516#appendix-A.4 */
                else if (!strncmp("{\"enc\":\"A128CBC-HS256\"}", buf, len)) {

                    const unsigned char iv[] = { 3, 22, 60, 12, 43, 67, 104, 105,
                            108, 108, 105, 99, 111, 116, 104, 101 };

                    const unsigned char cipher[] = { 40, 57, 83, 181, 119, 33, 133,
                            148, 198, 185, 243, 24, 152, 230, 6, 75, 129, 223, 127,
                            19, 210, 82, 183, 230, 168, 33, 215, 104, 143, 112, 56,
                            102 };

                    const unsigned char tag[] = { 51, 63, 149, 60, 252, 148,
                            225, 25, 92, 185, 139, 245, 35, 2, 47, 207 };

                    encryption->iv.data = apr_pmemdup(p, iv, sizeof(iv));
                    encryption->iv.len = sizeof(iv);

                    encryption->cipher.data = apr_pmemdup(p, cipher, sizeof(cipher));
                    encryption->cipher.len = sizeof(cipher);

                    encryption->tag.data = apr_pmemdup(p, tag, sizeof(tag));
                    encryption->tag.len = sizeof(tag);

                }

                else {
                    apr_errprintf(&jose->result, jose->pool, NULL, 0,
                            "Protected header not recognised: %.*s", (int)len, buf);
                    return APR_ENOTIMPL;
                }


            }
        }


        if (recipient) {
            header = recipient->header;
            if (header) {

                ABTS_INT_EQUAL(tc, APR_JSON_OBJECT, header->type);

                alg = apr_json_object_get(header, APR_JOSE_JWKSE_ALGORITHM,
                        APR_JSON_VALUE_STRING);
            }
        }

        if (!alg) {
            protected_header = encryption->protected;
            if (protected_header) {

                ABTS_INT_EQUAL(tc, APR_JSON_OBJECT, protected_header->type);

                alg = apr_json_object_get(protected_header,
                        APR_JOSE_JWKSE_ALGORITHM, APR_JSON_VALUE_STRING);
            }
        }

        if (alg && recipient) {

            ABTS_INT_EQUAL(tc, APR_JSON_STRING, alg->v->type);

            /* RSAES-OAEP and AES GCM jwe - https://tools.ietf.org/html/rfc7516#appendix-A.1 */
            if (alg->v->type == APR_JSON_STRING
                    && !strncmp(alg->v->value.string.p, "RSA-OAEP",
                            alg->v->value.string.len)) {

                const unsigned char ekey[] = { 56, 163, 154, 192, 58, 53, 222,
                        4, 105, 218, 136, 218, 29, 94, 203, 22, 150, 92, 129,
                        94, 211, 232, 53, 89, 41, 60, 138, 56, 196, 216, 82, 98,
                        168, 76, 37, 73, 70, 7, 36, 8, 191, 100, 136, 196, 244,
                        220, 145, 158, 138, 155, 4, 117, 141, 230, 199, 247,
                        173, 45, 182, 214, 74, 177, 107, 211, 153, 11, 205, 196,
                        171, 226, 162, 128, 171, 182, 13, 237, 239, 99, 193, 4,
                        91, 219, 121, 223, 107, 167, 61, 119, 228, 173, 156,
                        137, 134, 200, 80, 219, 74, 253, 56, 185, 91, 177, 34,
                        158, 89, 154, 205, 96, 55, 18, 138, 43, 96, 218, 215,
                        128, 124, 75, 138, 243, 85, 25, 109, 117, 140, 26, 155,
                        249, 67, 167, 149, 231, 100, 6, 41, 65, 214, 251, 232,
                        87, 72, 40, 182, 149, 154, 168, 31, 193, 126, 215, 89,
                        28, 111, 219, 125, 182, 139, 235, 195, 197, 23, 234, 55,
                        58, 63, 180, 68, 202, 206, 149, 75, 205, 248, 176, 67,
                        39, 178, 60, 98, 193, 32, 238, 122, 96, 158, 222, 57,
                        183, 111, 210, 55, 188, 215, 206, 180, 166, 150, 166,
                        106, 250, 55, 229, 72, 40, 69, 214, 216, 104, 23, 40,
                        135, 212, 28, 127, 41, 80, 175, 174, 168, 115, 171, 197,
                        89, 116, 92, 103, 246, 83, 216, 182, 176, 84, 37, 147,
                        35, 45, 219, 172, 99, 226, 233, 73, 37, 124, 42, 72, 49,
                        242, 35, 127, 184, 134, 117, 114, 135, 206 };

                recipient->ekey.data = apr_pmemdup(p, ekey, sizeof(ekey));
                recipient->ekey.len = sizeof(ekey);

                return APR_SUCCESS;
            }

            /* RSAES-PKCS1-v1_5 - https://tools.ietf.org/html/rfc7516#appendix-A.2 */
            if (alg->v->type == APR_JSON_STRING
                    && !strncmp(alg->v->value.string.p, "RSA1_5",
                            alg->v->value.string.len)) {

                const unsigned char ekey[] = { 80, 104, 72, 58, 11, 130, 236,
                        139, 132, 189, 255, 205, 61, 86, 151, 176, 99, 40, 44,
                        233, 176, 189, 205, 70, 202, 169, 72, 40, 226, 181, 156,
                        223, 120, 156, 115, 232, 150, 209, 145, 133, 104, 112,
                        237, 156, 116, 250, 65, 102, 212, 210, 103, 240, 177,
                        61, 93, 40, 71, 231, 223, 226, 240, 157, 15, 31, 150,
                        89, 200, 215, 198, 203, 108, 70, 117, 66, 212, 238, 193,
                        205, 23, 161, 169, 218, 243, 203, 128, 214, 127, 253,
                        215, 139, 43, 17, 135, 103, 179, 220, 28, 2, 212, 206,
                        131, 158, 128, 66, 62, 240, 78, 186, 141, 125, 132, 227,
                        60, 137, 43, 31, 152, 199, 54, 72, 34, 212, 115, 11,
                        152, 101, 70, 42, 219, 233, 142, 66, 151, 250, 126, 146,
                        141, 216, 190, 73, 50, 177, 146, 5, 52, 247, 28, 197,
                        21, 59, 170, 247, 181, 89, 131, 241, 169, 182, 246, 99,
                        15, 36, 102, 166, 182, 172, 197, 136, 230, 120, 60, 58,
                        219, 243, 149, 94, 222, 150, 154, 194, 110, 227, 225,
                        112, 39, 89, 233, 112, 207, 211, 241, 124, 174, 69, 221,
                        179, 107, 196, 225, 127, 167, 112, 226, 12, 242, 16, 24,
                        28, 120, 182, 244, 213, 244, 153, 194, 162, 69, 160,
                        244, 248, 63, 165, 141, 4, 207, 249, 193, 79, 131, 0,
                        169, 233, 127, 167, 101, 151, 125, 56, 112, 111, 248,
                        29, 232, 90, 29, 147, 110, 169, 146, 114, 165, 204, 71,
                        136, 41, 252 };

                recipient->ekey.data = apr_pmemdup(p, ekey, sizeof(ekey));
                recipient->ekey.len = sizeof(ekey);

                return APR_SUCCESS;
            }

            /* A128KW A128CBC-HS256 - https://tools.ietf.org/html/rfc7516#appendix-A.3 */
            if (alg->v->type == APR_JSON_STRING
                    && !strncmp(alg->v->value.string.p, "A128KW",
                            alg->v->value.string.len)) {

                const unsigned char ekey[] = { 232, 160, 123, 211, 183, 76, 245,
                        132, 200, 128, 123, 75, 190, 216, 22, 67, 201, 138, 193,
                        186, 9, 91, 122, 31, 246, 90, 28, 139, 57, 3, 76, 124,
                        193, 11, 98, 37, 173, 61, 104, 57 };

                recipient->ekey.data = apr_pmemdup(p, ekey, sizeof(ekey));
                recipient->ekey.len = sizeof(ekey);

                return APR_SUCCESS;
            }

        }
    }

    return APR_ENOTIMPL;
}

static apr_status_t decrypt_cb(apr_bucket_brigade *brigade, apr_jose_t *jose,
        apr_jose_recipient_t *recipient, apr_jose_encryption_t *encryption,
        apr_json_value_t *header, apr_jose_text_t *ph64, apr_jose_text_t *aad64,
        void *ctx, int *dflags, apr_pool_t *pool)
{

    *dflags = APR_JOSE_FLAG_NONE;

    if (encryption && recipient && header) {

        char buf[1024];
        apr_size_t len = sizeof(buf);

        apr_bucket_brigade *bb = apr_brigade_create(p,
                brigade->bucket_alloc);

        apr_json_encode(bb, NULL, NULL, header,
                APR_JSON_FLAGS_WHITESPACE, p);

        apr_brigade_flatten(bb, buf, &len);

        /* RSAES-OAEP and AES GCM jwe - https://tools.ietf.org/html/rfc7516#appendix-A.1 */
        if (!strncmp("{\"alg\":\"RSA-OAEP\",\"enc\":\"A256GCM\"}", buf, len)) {

            const char plaintext[] = { 84, 104, 101, 32, 116, 114, 117,
                    101, 32, 115, 105, 103, 110, 32, 111, 102, 32, 105, 110,
                    116, 101, 108, 108, 105, 103, 101, 110, 99, 101, 32, 105,
                    115, 32, 110, 111, 116, 32, 107, 110, 111, 119, 108, 101,
                    100, 103, 101, 32, 98, 117, 116, 32, 105, 109, 97, 103, 105,
                    110, 97, 116, 105, 111, 110, 46 };

            apr_brigade_write(brigade, NULL, NULL, plaintext, sizeof(plaintext));

            return APR_SUCCESS;
        }

        /* RSAES-PKCS1-v1_5 - https://tools.ietf.org/html/rfc7516#appendix-A.2 */
        else if (!strncmp("{\"jku\":\"https://server.example.com/keys.jwks\",\"enc\":\"A128CBC-HS256\",\"alg\":\"RSA1_5\",\"kid\":\"2011-04-29\"}", buf, len)) {

            const char plaintext[] = { 76, 105, 118, 101, 32, 108, 111, 110,
                    103, 32, 97, 110, 100, 32, 112, 114, 111, 115, 112, 101,
                    114, 46 };

            apr_brigade_write(brigade, NULL, NULL, plaintext, sizeof(plaintext));

            return APR_SUCCESS;
        }

        /* Flattened JWE JSON - https://tools.ietf.org/html/rfc7516#appendix-A.5 */
        else if (!strncmp("{\"jku\":\"https://server.example.com/keys.jwks\",\"enc\":\"A128CBC-HS256\",\"alg\":\"A128KW\",\"kid\":\"7\"}", buf, len)) {

            const char plaintext[] = { 76, 105, 118, 101, 32, 108, 111, 110,
                    103, 32, 97, 110, 100, 32, 112, 114, 111, 115, 112, 101,
                    114, 46 };

            apr_brigade_write(brigade, NULL, NULL, plaintext, sizeof(plaintext));

            return APR_SUCCESS;
        }


        else {
            apr_errprintf(&jose->result, pool, NULL, 0,
                    "Header not recognised: %.*s", (int)len, buf);
            return APR_ENOTIMPL;
        }

    }

    return APR_ENOTIMPL;
}

/**
 * Test from https://tools.ietf.org/html/rfc7515#appendix-A.5
 */
static void test_jose_encode_jws_compact_unsecured(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_jose_t *jdata;
    apr_jose_signature_t signature;
    char buf[1024];
    apr_size_t len = sizeof(buf);
    apr_off_t offset;
    apr_status_t status;

    const char *ph = "{\"alg\":\"none\"}";
    const unsigned char pl[] = {123, 34, 105, 115, 115, 34, 58, 34, 106, 111, 101, 34, 44, 13, 10,
                       32, 34, 101, 120, 112, 34, 58, 49, 51, 48, 48, 56, 49, 57, 51, 56,
                       48, 44, 13, 10, 32, 34, 104, 116, 116, 112, 58, 47, 47, 101, 120, 97,
                       109, 112, 108, 101, 46, 99, 111, 109, 47, 105, 115, 95, 114, 111,
                       111, 116, 34, 58, 116, 114, 117, 101, 125};
    const char *expect = "eyJhbGciOiJub25lIn0"
            "."
            "eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGFt"
            "cGxlLmNvbS9pc19yb290Ijp0cnVlfQ"
            ".";

    apr_jose_cb_t cb;

    cb.sign = sign_cb;
    cb.ctx = tc;

    signature.header = NULL;
    apr_json_decode(&signature.protected_header, ph, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    jdata = apr_jose_data_make(NULL, "JWT", pl, sizeof(pl), p);
    jose = apr_jose_jws_make(NULL, &signature, NULL, jdata, p);

    status = apr_jose_encode(bb, NULL, NULL, jose, &cb, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    apr_brigade_flatten(bb, buf, &len);
    ABTS_STR_NEQUAL(tc, expect, buf, len);
}

/**
 * Test from https://tools.ietf.org/html/rfc7515#appendix-A.1
 */
static void test_jose_encode_jws_compact_hs256(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_jose_t *jdata;
    apr_jose_signature_t signature;
    char buf[1024];
    apr_size_t len = sizeof(buf);
    apr_off_t offset;
    apr_status_t status;

    const unsigned char ph[] = { 123, 34, 116, 121, 112, 34, 58, 34, 74, 87, 84,
            34, 44, 13, 10, 32, 34, 97, 108, 103, 34, 58, 34, 72, 83, 50, 53,
            54, 34, 125 };

    const unsigned char pl[] = {123, 34, 105, 115, 115, 34, 58, 34, 106, 111, 101, 34, 44, 13, 10,
                       32, 34, 101, 120, 112, 34, 58, 49, 51, 48, 48, 56, 49, 57, 51, 56,
                       48, 44, 13, 10, 32, 34, 104, 116, 116, 112, 58, 47, 47, 101, 120, 97,
                       109, 112, 108, 101, 46, 99, 111, 109, 47, 105, 115, 95, 114, 111,
                       111, 116, 34, 58, 116, 114, 117, 101, 125};
    const char *expect = "eyJ0eXAiOiJKV1QiLA0KICJhbGciOiJIUzI1NiJ9"
            "."
            "eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGFt"
            "cGxlLmNvbS9pc19yb290Ijp0cnVlfQ"
            "."
            "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";

    apr_jose_cb_t cb;

    cb.sign = sign_cb;
    cb.ctx = tc;

    signature.header = NULL;
    apr_json_decode(&signature.protected_header, (const char *) ph, sizeof(ph), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    jdata = apr_jose_data_make(NULL, "JWT", pl, sizeof(pl), p);
    jose = apr_jose_jws_make(NULL, &signature, NULL, jdata, p);

    status = apr_jose_encode(bb, NULL, NULL, jose, &cb, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    apr_brigade_flatten(bb, buf, &len);
    ABTS_STR_NEQUAL(tc, expect, buf, len);
}

/**
 * Test from https://tools.ietf.org/html/rfc7515#appendix-A.6
 */
static void test_jose_encode_jws_json_general(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_jose_t *jdata;
    apr_jose_signature_t **signature;
    apr_jose_signature_t signature1;
    apr_jose_signature_t signature2;
    apr_array_header_t *signatures = apr_array_make(p, 2,
            sizeof(apr_jose_signature_t *));
    char buf[1024];
    apr_size_t len = sizeof(buf);
    apr_off_t offset;
    apr_status_t status;

    const unsigned char pl[] = { 123, 34, 105, 115, 115, 34, 58, 34, 106, 111,
            101, 34, 44, 13, 10, 32, 34, 101, 120, 112, 34, 58, 49, 51, 48, 48,
            56, 49, 57, 51, 56, 48, 44, 13, 10, 32, 34, 104, 116, 116, 112, 58,
            47, 47, 101, 120, 97, 109, 112, 108, 101, 46, 99, 111, 109, 47, 105,
            115, 95, 114, 111, 111, 116, 34, 58, 116, 114, 117, 101, 125 };

    const char *s1h = "{\"kid\":\"2010-12-29\"}";
    const char *s1ph = "{\"alg\":\"RS256\"}";
    const char *s2h = "{\"kid\":\"e9bc097a-ce51-4036-9562-d2ade882db0d\"}";
    const char *s2ph = "{\"alg\":\"ES256\"}";

    const char *expect = "{"
            "\"payload\":"
            "\"eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGF"
            "tcGxlLmNvbS9pc19yb290Ijp0cnVlfQ\","
            "\"signatures\":["
            "{\"protected\":\"eyJhbGciOiJSUzI1NiJ9\","
            "\"header\":"
            "{\"kid\":\"2010-12-29\"},"
            "\"signature\":"
            "\"cC4hiUPoj9Eetdgtv3hF80EGrhuB__dzERat0XF9g2VtQgr9PJbu3XOiZj5RZ"
            "mh7AAuHIm4Bh-0Qc_lF5YKt_O8W2Fp5jujGbds9uJdbF9CUAr7t1dnZcAcQjb"
            "KBYNX4BAynRFdiuB--f_nZLgrnbyTyWzO75vRK5h6xBArLIARNPvkSjtQBMHl"
            "b1L07Qe7K0GarZRmB_eSN9383LcOLn6_dO--xi12jzDwusC-eOkHWEsqtFZES"
            "c6BfI7noOPqvhJ1phCnvWh6IeYI2w9QOYEUipUTI8np6LbgGY9Fs98rqVt5AX"
            "LIhWkWywlVmtVrBp0igcN_IoypGlUPQGe77Rw\"},"
            "{\"protected\":\"eyJhbGciOiJFUzI1NiJ9\","
            "\"header\":"
            "{\"kid\":\"e9bc097a-ce51-4036-9562-d2ade882db0d\"},"
            "\"signature\":"
            "\"DtEhU3ljbEg8L38VWAfUAqOyKAM6-Xx-F4GawxaepmXFCgfTjDxw5djxLa8IS"
            "lSApmWQxfKTUJqPP3-Kg6NU1Q\"}]"
            "}";

    apr_jose_cb_t cb;

    cb.sign = sign_cb;
    cb.ctx = tc;

    apr_json_decode(&signature1.header, (const char *) s1h, strlen(s1h), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    apr_json_decode(&signature1.protected_header, (const char *) s1ph, strlen(s1ph), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    apr_json_decode(&signature2.header, (const char *) s2h, strlen(s2h), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    apr_json_decode(&signature2.protected_header, (const char *) s2ph, strlen(s2ph), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    signature = apr_array_push(signatures);
    *signature = &signature1;
    signature = apr_array_push(signatures);
    *signature = &signature2;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    jdata = apr_jose_data_make(NULL, "JWT", pl, sizeof(pl), p);
    jose = apr_jose_jws_json_make(NULL, NULL, signatures, jdata, p);

    status = apr_jose_encode(bb, NULL, NULL, jose, &cb, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    apr_brigade_flatten(bb, buf, &len);
    ABTS_STR_NEQUAL(tc, expect, buf, len);

}

/**
 * Test from https://tools.ietf.org/html/rfc7515#appendix-A.7
 */
static void test_jose_encode_jws_json_flattened(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_jose_t *jdata;
    apr_jose_signature_t signature2;
    char buf[1024];
    apr_size_t len = sizeof(buf);
    apr_off_t offset;
    apr_status_t status;

    const unsigned char pl[] = { 123, 34, 105, 115, 115, 34, 58, 34, 106, 111,
            101, 34, 44, 13, 10, 32, 34, 101, 120, 112, 34, 58, 49, 51, 48, 48,
            56, 49, 57, 51, 56, 48, 44, 13, 10, 32, 34, 104, 116, 116, 112, 58,
            47, 47, 101, 120, 97, 109, 112, 108, 101, 46, 99, 111, 109, 47, 105,
            115, 95, 114, 111, 111, 116, 34, 58, 116, 114, 117, 101, 125 };

    const char *s2h = "{\"kid\":\"e9bc097a-ce51-4036-9562-d2ade882db0d\"}";
    const char *s2ph = "{\"alg\":\"ES256\"}";

    const char *expect = "{"
            "\"payload\":"
            "\"eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGF"
            "tcGxlLmNvbS9pc19yb290Ijp0cnVlfQ\","
            "\"protected\":\"eyJhbGciOiJFUzI1NiJ9\","
            "\"header\":"
            "{\"kid\":\"e9bc097a-ce51-4036-9562-d2ade882db0d\"},"
            "\"signature\":"
            "\"DtEhU3ljbEg8L38VWAfUAqOyKAM6-Xx-F4GawxaepmXFCgfTjDxw5djxLa8IS"
            "lSApmWQxfKTUJqPP3-Kg6NU1Q\""
            "}";

    apr_jose_cb_t cb;

    cb.sign = sign_cb;
    cb.ctx = tc;

    apr_json_decode(&signature2.header, (const char *) s2h, strlen(s2h), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    apr_json_decode(&signature2.protected_header, (const char *) s2ph, strlen(s2ph), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    jdata = apr_jose_data_make(NULL, "JWT", pl, sizeof(pl), p);
    jose = apr_jose_jws_json_make(NULL, &signature2, NULL, jdata, p);

    status = apr_jose_encode(bb, NULL, NULL, jose, &cb, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    apr_brigade_flatten(bb, buf, &len);
    ABTS_STR_NEQUAL(tc, expect, buf, len);

}

/**
 * Test from https://tools.ietf.org/html/rfc7516#appendix-A.1
 */
static void test_jose_encode_jwe_compact_rsaes_oaep_aes_gcm(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_jose_t *jdata;
    apr_jose_encryption_t *encryption;
    apr_jose_recipient_t *recipient;
    apr_json_value_t *header = NULL;
    apr_json_value_t *protected_header = NULL;
    char buf[1024];
    apr_size_t len = sizeof(buf);
    apr_off_t offset;
    apr_status_t status;

    const char *ph = "{\"alg\":\"RSA-OAEP\",\"enc\":\"A256GCM\"}";

    const unsigned char pl[] = { 84, 104, 101, 32, 116, 114, 117, 101, 32, 115,
            105, 103, 110, 32, 111, 102, 32, 105, 110, 116, 101, 108, 108, 105,
            103, 101, 110, 99, 101, 32, 105, 115, 32, 110, 111, 116, 32, 107,
            110, 111, 119, 108, 101, 100, 103, 101, 32, 98, 117, 116, 32, 105,
            109, 97, 103, 105, 110, 97, 116, 105, 111, 110, 46 };

    const char *expect = "eyJhbGciOiJSU0EtT0FFUCIsImVuYyI6IkEyNTZHQ00ifQ."
            "OKOawDo13gRp2ojaHV7LFpZcgV7T6DVZKTyKOMTYUmKoTCVJRgckCL9kiMT03JGe"
            "ipsEdY3mx_etLbbWSrFr05kLzcSr4qKAq7YN7e9jwQRb23nfa6c9d-StnImGyFDb"
            "Sv04uVuxIp5Zms1gNxKKK2Da14B8S4rzVRltdYwam_lDp5XnZAYpQdb76FdIKLaV"
            "mqgfwX7XWRxv2322i-vDxRfqNzo_tETKzpVLzfiwQyeyPGLBIO56YJ7eObdv0je8"
            "1860ppamavo35UgoRdbYaBcoh9QcfylQr66oc6vFWXRcZ_ZT2LawVCWTIy3brGPi"
            "6UklfCpIMfIjf7iGdXKHzg."
            "48V1_ALb6US04U3b."
            "5eym8TW_c8SuK0ltJ3rpYIzOeDQz7TALvtu6UG9oMo4vpzs9tX_EFShS8iB7j6ji"
            "SdiwkIr3ajwQzaBtQD_A."
            "XFBoMYUZodetZdvTiFvSkQ";

    apr_jose_cb_t cb;

    cb.encrypt = encrypt_cb;
    cb.ctx = tc;

    apr_json_decode(&protected_header, ph, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    jdata = apr_jose_data_make(NULL, "JWT", pl, sizeof(pl), p);
    recipient = apr_jose_recipient_make(NULL, header, p);
    encryption = apr_jose_encryption_make(NULL, NULL, protected_header, p);
    jose = apr_jose_jwe_make(NULL, recipient, NULL, encryption, jdata, p);

    status = apr_jose_encode(bb, NULL, NULL, jose, &cb, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    apr_brigade_flatten(bb, buf, &len);
    ABTS_STR_NEQUAL(tc, expect, buf, len);
}

/**
 * Test from https://tools.ietf.org/html/rfc7516#appendix-A.4
 */
static void test_jose_encode_jwe_json_general(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_jose_t *jdata;
    apr_json_value_t *header = NULL;
    apr_json_value_t *protected_header = NULL;
    apr_jose_recipient_t **recipient;
    apr_jose_recipient_t recipient1;
    apr_jose_recipient_t recipient2;
    apr_array_header_t *recipients = apr_array_make(p, 2,
            sizeof(apr_jose_recipient_t *));
    apr_jose_encryption_t *encryption;
    char buf[1024];
    apr_size_t len = sizeof(buf);
    apr_off_t offset;
    apr_status_t status;

    const char *r1h = "{\"alg\":\"RSA1_5\",\"kid\":\"2011-04-29\"}";
    const char *r2h = "{\"alg\":\"A128KW\",\"kid\":\"7\"}";

    const char *ph = "{\"enc\":\"A128CBC-HS256\"}";
    const char *h = "{\"jku\":\"https://server.example.com/keys.jwks\"}";

    const unsigned char pl[] = { 76, 105, 118, 101, 32, 108, 111, 110, 103, 32,
            97, 110, 100, 32, 112, 114, 111, 115, 112, 101, 114, 46 };

    const char *expect = "{"
            "\"protected\":"
            "\"eyJlbmMiOiJBMTI4Q0JDLUhTMjU2In0\","
            "\"unprotected\":"
            "{\"jku\":\"https://server.example.com/keys.jwks\"},"
            "\"recipients\":["
            "{\"header\":"
            "{\"alg\":\"RSA1_5\",\"kid\":\"2011-04-29\"},"
            "\"encrypted_key\":"
            "\"UGhIOguC7IuEvf_NPVaXsGMoLOmwvc1GyqlIKOK1nN94nHPoltGRhWhw7Zx0-"
            "kFm1NJn8LE9XShH59_i8J0PH5ZZyNfGy2xGdULU7sHNF6Gp2vPLgNZ__deLKx"
            "GHZ7PcHALUzoOegEI-8E66jX2E4zyJKx-YxzZIItRzC5hlRirb6Y5Cl_p-ko3"
            "YvkkysZIFNPccxRU7qve1WYPxqbb2Yw8kZqa2rMWI5ng8OtvzlV7elprCbuPh"
            "cCdZ6XDP0_F8rkXds2vE4X-ncOIM8hAYHHi29NX0mcKiRaD0-D-ljQTP-cFPg"
            "wCp6X-nZZd9OHBv-B3oWh2TbqmScqXMR4gp_A\"},"
            "{\"header\":"
            "{\"alg\":\"A128KW\",\"kid\":\"7\"},"
            "\"encrypted_key\":"
            "\"6KB707dM9YTIgHtLvtgWQ8mKwboJW3of9locizkDTHzBC2IlrT1oOQ\"}],"
            "\"iv\":"
            "\"AxY8DCtDaGlsbGljb3RoZQ\","
            "\"ciphertext\":"
            "\"KDlTtXchhZTGufMYmOYGS4HffxPSUrfmqCHXaI9wOGY\","
            "\"tag\":"
            "\"Mz-VPPyU4RlcuYv1IwIvzw\""
            "}\";";

    apr_jose_cb_t cb;

    cb.encrypt = encrypt_cb;
    cb.ctx = tc;

    apr_json_decode(&recipient1.header, (const char *) r1h, strlen(r1h), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    apr_json_decode(&recipient2.header, (const char *) r2h, strlen(r2h), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    recipient = apr_array_push(recipients);
    *recipient = &recipient1;
    recipient = apr_array_push(recipients);
    *recipient = &recipient2;

    apr_json_decode(&header, h, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    apr_json_decode(&protected_header, ph, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    jdata = apr_jose_data_make(NULL, "plain", pl, sizeof(pl), p);
    encryption = apr_jose_encryption_make(NULL, header, protected_header, p);
    jose = apr_jose_jwe_json_make(NULL, NULL, recipients, encryption, jdata, p);

    status = apr_jose_encode(bb, NULL, NULL, jose, &cb, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    apr_brigade_flatten(bb, buf, &len);
    ABTS_STR_NEQUAL(tc, expect, buf, len);
}

/**
 * Test from https://tools.ietf.org/html/rfc7516#appendix-A.5
 */
static void test_jose_encode_jwe_json_flattened(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_jose_t *jdata;
    apr_json_value_t *header = NULL;
    apr_json_value_t *protected_header = NULL;
    apr_jose_recipient_t recipient;
    apr_jose_encryption_t *encryption;
    char buf[1024];
    apr_size_t len = sizeof(buf);
    apr_off_t offset;
    apr_status_t status;

    const char *rh = "{\"alg\":\"A128KW\",\"kid\":\"7\"}";

    const char *ph = "{\"enc\":\"A128CBC-HS256\"}";
    const char *h = "{\"jku\":\"https://server.example.com/keys.jwks\"}";

    const unsigned char pl[] = { 76, 105, 118, 101, 32, 108, 111, 110, 103, 32,
            97, 110, 100, 32, 112, 114, 111, 115, 112, 101, 114, 46 };

    const char *expect = "{"
            "\"protected\":"
            "\"eyJlbmMiOiJBMTI4Q0JDLUhTMjU2In0\","
            "\"unprotected\":"
            "{\"jku\":\"https://server.example.com/keys.jwks\"},"
            "\"header\":"
            "{\"alg\":\"A128KW\",\"kid\":\"7\"},"
            "\"encrypted_key\":"
            "\"6KB707dM9YTIgHtLvtgWQ8mKwboJW3of9locizkDTHzBC2IlrT1oOQ\","
            "\"iv\":"
            "\"AxY8DCtDaGlsbGljb3RoZQ\","
            "\"ciphertext\":"
            "\"KDlTtXchhZTGufMYmOYGS4HffxPSUrfmqCHXaI9wOGY\","
            "\"tag\":"
            "\"Mz-VPPyU4RlcuYv1IwIvzw\""
            "}";

    apr_jose_cb_t cb;

    cb.encrypt = encrypt_cb;
    cb.ctx = tc;

    apr_json_decode(&recipient.header, (const char *) rh, strlen(rh), &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    apr_json_decode(&header, h, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);
    apr_json_decode(&protected_header, ph, APR_JSON_VALUE_STRING, &offset,
            APR_JSON_FLAGS_WHITESPACE, 10, p);

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    jdata = apr_jose_data_make(NULL, "plain", pl, sizeof(pl), p);
    encryption = apr_jose_encryption_make(NULL, header, protected_header, p);
    jose = apr_jose_jwe_json_make(NULL, &recipient, NULL, encryption, jdata, p);

    status = apr_jose_encode(bb, NULL, NULL, jose, &cb, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);

    apr_brigade_flatten(bb, buf, &len);
    ABTS_STR_NEQUAL(tc, expect, buf, len);
}

/**
 * Test from https://tools.ietf.org/html/rfc7515#appendix-A.5
 */
static void test_jose_decode_jws_compact_unsecured(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_json_kv_t *kv;
    apr_status_t status;

    const char *source = "eyJhbGciOiJub25lIn0"
            "."
            "eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGFt"
            "cGxlLmNvbS9pc19yb290Ijp0cnVlfQ"
            ".";

    apr_jose_cb_t cb;

    cb.verify = verify_cb;
    cb.decrypt = decrypt_cb;
    cb.ctx = tc;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    apr_brigade_write(bb, NULL, NULL, source, strlen(source));

    status = apr_jose_decode(&jose, "JWT", bb, &cb, 10, APR_JOSE_FLAG_NONE, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
    ABTS_PTR_NOTNULL(tc, jose);
    ABTS_INT_EQUAL(tc, APR_JOSE_TYPE_JWT, jose->type);

    kv = apr_json_object_get(jose->jose.jwt->claims, "iss",
            APR_JSON_VALUE_STRING);
    ABTS_PTR_NOTNULL(tc, kv);
    ABTS_INT_EQUAL(tc, APR_JSON_STRING, kv->v->type);

}

/**
 * Test from https://tools.ietf.org/html/rfc7515#appendix-A.1
 */
static void test_jose_decode_jws_compact_hs256(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_json_kv_t *kv;
    apr_status_t status;

    const char *source = "eyJ0eXAiOiJKV1QiLA0KICJhbGciOiJIUzI1NiJ9"
            "."
            "eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGFt"
            "cGxlLmNvbS9pc19yb290Ijp0cnVlfQ"
            "."
            "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";

    apr_jose_cb_t cb;

    cb.verify = verify_cb;
    cb.decrypt = decrypt_cb;
    cb.ctx = tc;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    apr_brigade_write(bb, NULL, NULL, source, strlen(source));

    status = apr_jose_decode(&jose, "JWT", bb, &cb, 10, APR_JOSE_FLAG_NONE, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
    ABTS_PTR_NOTNULL(tc, jose);
    ABTS_INT_EQUAL(tc, APR_JOSE_TYPE_JWT, jose->type);

    kv = apr_json_object_get(jose->jose.jwt->claims, "iss",
            APR_JSON_VALUE_STRING);
    ABTS_PTR_NOTNULL(tc, kv);
    ABTS_INT_EQUAL(tc, APR_JSON_STRING, kv->v->type);

}

/**
 * Test from https://tools.ietf.org/html/rfc7515#appendix-A.6
 */
static void test_jose_decode_jws_json_general(abts_case *tc, void *data)
{
    const char *source = "{"
            "\"payload\":"
            "\"eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGF"
            "tcGxlLmNvbS9pc19yb290Ijp0cnVlfQ\","
            "\"signatures\":["
            "{\"protected\":\"eyJhbGciOiJSUzI1NiJ9\","
            "\"header\":"
            "{\"kid\":\"2010-12-29\"},"
            "\"signature\":"
            "\"cC4hiUPoj9Eetdgtv3hF80EGrhuB__dzERat0XF9g2VtQgr9PJbu3XOiZj5RZ"
            "mh7AAuHIm4Bh-0Qc_lF5YKt_O8W2Fp5jujGbds9uJdbF9CUAr7t1dnZcAcQjb"
            "KBYNX4BAynRFdiuB--f_nZLgrnbyTyWzO75vRK5h6xBArLIARNPvkSjtQBMHl"
            "b1L07Qe7K0GarZRmB_eSN9383LcOLn6_dO--xi12jzDwusC-eOkHWEsqtFZES"
            "c6BfI7noOPqvhJ1phCnvWh6IeYI2w9QOYEUipUTI8np6LbgGY9Fs98rqVt5AX"
            "LIhWkWywlVmtVrBp0igcN_IoypGlUPQGe77Rw\"},"
            "{\"protected\":\"eyJhbGciOiJFUzI1NiJ9\","
            "\"header\":"
            "{\"kid\":\"e9bc097a-ce51-4036-9562-d2ade882db0d\"},"
            "\"signature\":"
            "\"DtEhU3ljbEg8L38VWAfUAqOyKAM6-Xx-F4GawxaepmXFCgfTjDxw5djxLa8IS"
            "lSApmWQxfKTUJqPP3-Kg6NU1Q\"}]"
            "}";

    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_status_t status;

    apr_jose_cb_t cb;

    cb.verify = verify_cb;
    cb.decrypt = decrypt_cb;
    cb.ctx = tc;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    apr_brigade_write(bb, NULL, NULL, source, strlen(source));

    status = apr_jose_decode(&jose, "JOSE+JSON", bb, &cb, 10, APR_JOSE_FLAG_NONE, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
    ABTS_PTR_NOTNULL(tc, jose);

    /*
     * There is nothing in this structure to identify the MIME type of the payload,
     * so raw data is returned.
     */
    ABTS_INT_EQUAL(tc, APR_JOSE_TYPE_DATA, jose->type);

}

/**
 * Test from https://tools.ietf.org/html/rfc7515#appendix-A.7
 */
static void test_jose_decode_jws_json_flattened(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_status_t status;

    const char *source = "{"
            "\"payload\":"
            "\"eyJpc3MiOiJqb2UiLA0KICJleHAiOjEzMDA4MTkzODAsDQogImh0dHA6Ly9leGF"
            "tcGxlLmNvbS9pc19yb290Ijp0cnVlfQ\","
            "\"protected\":\"eyJhbGciOiJFUzI1NiJ9\","
            "\"header\":"
            "{\"kid\":\"e9bc097a-ce51-4036-9562-d2ade882db0d\"},"
            "\"signature\":"
            "\"DtEhU3ljbEg8L38VWAfUAqOyKAM6-Xx-F4GawxaepmXFCgfTjDxw5djxLa8IS"
            "lSApmWQxfKTUJqPP3-Kg6NU1Q\""
            "}";

    apr_jose_cb_t cb;

    cb.verify = verify_cb;
    cb.decrypt = decrypt_cb;
    cb.ctx = tc;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    apr_brigade_write(bb, NULL, NULL, source, strlen(source));

    status = apr_jose_decode(&jose, "JOSE+JSON", bb, &cb, 10, APR_JOSE_FLAG_NONE, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
    ABTS_PTR_NOTNULL(tc, jose);
    ABTS_INT_EQUAL(tc, APR_JOSE_TYPE_DATA, jose->type);

}

/**
 * Test from https://tools.ietf.org/html/rfc7516#appendix-A.1
 */
static void test_jose_decode_jwe_compact_rsaes_oaep_aes_gcm(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_status_t status;

    const char *source = "eyJhbGciOiJSU0EtT0FFUCIsImVuYyI6IkEyNTZHQ00ifQ."
            "OKOawDo13gRp2ojaHV7LFpZcgV7T6DVZKTyKOMTYUmKoTCVJRgckCL9kiMT03JGe"
            "ipsEdY3mx_etLbbWSrFr05kLzcSr4qKAq7YN7e9jwQRb23nfa6c9d-StnImGyFDb"
            "Sv04uVuxIp5Zms1gNxKKK2Da14B8S4rzVRltdYwam_lDp5XnZAYpQdb76FdIKLaV"
            "mqgfwX7XWRxv2322i-vDxRfqNzo_tETKzpVLzfiwQyeyPGLBIO56YJ7eObdv0je8"
            "1860ppamavo35UgoRdbYaBcoh9QcfylQr66oc6vFWXRcZ_ZT2LawVCWTIy3brGPi"
            "6UklfCpIMfIjf7iGdXKHzg."
            "48V1_ALb6US04U3b."
            "5eym8TW_c8SuK0ltJ3rpYIzOeDQz7TALvtu6UG9oMo4vpzs9tX_EFShS8iB7j6ji"
            "SdiwkIr3ajwQzaBtQD_A."
            "XFBoMYUZodetZdvTiFvSkQ";

    apr_jose_cb_t cb;

    cb.verify = verify_cb;
    cb.decrypt = decrypt_cb;
    cb.ctx = tc;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    apr_brigade_write(bb, NULL, NULL, source, strlen(source));

    status = apr_jose_decode(&jose, "JWE", bb, &cb, 10, APR_JOSE_FLAG_NONE, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
    ABTS_PTR_NOTNULL(tc, jose);
    ABTS_INT_EQUAL(tc, APR_JOSE_TYPE_DATA, jose->type);

}

/**
 * Test from https://tools.ietf.org/html/rfc7516#appendix-A.4
 */
static void test_jose_decode_jwe_json_general(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_status_t status;

    const char *source = "{"
            "\"protected\":"
            "\"eyJlbmMiOiJBMTI4Q0JDLUhTMjU2In0\","
            "\"unprotected\":"
            "{\"jku\":\"https://server.example.com/keys.jwks\"},"
            "\"recipients\":["
            "{\"header\":"
            "{\"alg\":\"RSA1_5\",\"kid\":\"2011-04-29\"},"
            "\"encrypted_key\":"
            "\"UGhIOguC7IuEvf_NPVaXsGMoLOmwvc1GyqlIKOK1nN94nHPoltGRhWhw7Zx0-"
            "kFm1NJn8LE9XShH59_i8J0PH5ZZyNfGy2xGdULU7sHNF6Gp2vPLgNZ__deLKx"
            "GHZ7PcHALUzoOegEI-8E66jX2E4zyJKx-YxzZIItRzC5hlRirb6Y5Cl_p-ko3"
            "YvkkysZIFNPccxRU7qve1WYPxqbb2Yw8kZqa2rMWI5ng8OtvzlV7elprCbuPh"
            "cCdZ6XDP0_F8rkXds2vE4X-ncOIM8hAYHHi29NX0mcKiRaD0-D-ljQTP-cFPg"
            "wCp6X-nZZd9OHBv-B3oWh2TbqmScqXMR4gp_A\"},"
            "{\"header\":"
            "{\"alg\":\"A128KW\",\"kid\":\"7\"},"
            "\"encrypted_key\":"
            "\"6KB707dM9YTIgHtLvtgWQ8mKwboJW3of9locizkDTHzBC2IlrT1oOQ\"}],"
            "\"iv\":"
            "\"AxY8DCtDaGlsbGljb3RoZQ\","
            "\"ciphertext\":"
            "\"KDlTtXchhZTGufMYmOYGS4HffxPSUrfmqCHXaI9wOGY\","
            "\"tag\":"
            "\"Mz-VPPyU4RlcuYv1IwIvzw\""
            "}";

    apr_jose_cb_t cb;

    cb.verify = verify_cb;
    cb.decrypt = decrypt_cb;
    cb.ctx = tc;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    apr_brigade_write(bb, NULL, NULL, source, strlen(source));

    status = apr_jose_decode(&jose, "JOSE+JSON", bb, &cb, 10,
            APR_JOSE_FLAG_NONE, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
    ABTS_PTR_NOTNULL(tc, jose);
    ABTS_INT_EQUAL(tc, APR_JOSE_TYPE_DATA, jose->type);

}

/**
 * Test from https://tools.ietf.org/html/rfc7516#appendix-A.5
 */
static void test_jose_decode_jwe_json_flattened(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_jose_t *jose;
    apr_status_t status;

    const char *source = "{"
            "\"protected\":"
            "\"eyJlbmMiOiJBMTI4Q0JDLUhTMjU2In0\","
            "\"unprotected\":"
            "{\"jku\":\"https://server.example.com/keys.jwks\"},"
            "\"header\":"
            "{\"alg\":\"A128KW\",\"kid\":\"7\"},"
            "\"encrypted_key\":"
            "\"6KB707dM9YTIgHtLvtgWQ8mKwboJW3of9locizkDTHzBC2IlrT1oOQ\","
            "\"iv\":"
            "\"AxY8DCtDaGlsbGljb3RoZQ\","
            "\"ciphertext\":"
            "\"KDlTtXchhZTGufMYmOYGS4HffxPSUrfmqCHXaI9wOGY\","
            "\"tag\":"
            "\"Mz-VPPyU4RlcuYv1IwIvzw\""
            "}";

    apr_jose_cb_t cb;

    cb.verify = verify_cb;
    cb.decrypt = decrypt_cb;
    cb.ctx = tc;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    apr_brigade_write(bb, NULL, NULL, source, strlen(source));

    status = apr_jose_decode(&jose, "JOSE+JSON", bb, &cb, 10,
            APR_JOSE_FLAG_NONE, p);

    ABTS_INT_EQUAL(tc, APR_SUCCESS, status);
    ABTS_PTR_NOTNULL(tc, jose);
    ABTS_INT_EQUAL(tc, APR_JOSE_TYPE_DATA, jose->type);

}

abts_suite *testjose(abts_suite *suite)
{
        suite = ADD_SUITE(suite);

        abts_run_test(suite, test_jose_decode_jws_compact_unsecured, NULL);
        abts_run_test(suite, test_jose_decode_jws_compact_hs256, NULL);
        abts_run_test(suite, test_jose_decode_jws_json_general, NULL);
        abts_run_test(suite, test_jose_decode_jws_json_flattened, NULL);
        abts_run_test(suite, test_jose_decode_jwe_compact_rsaes_oaep_aes_gcm, NULL);
        abts_run_test(suite, test_jose_decode_jwe_json_general, NULL);
        abts_run_test(suite, test_jose_decode_jwe_json_flattened, NULL);

        abts_run_test(suite, test_jose_encode_jws_compact_unsecured, NULL);
        abts_run_test(suite, test_jose_encode_jws_compact_hs256, NULL);
        abts_run_test(suite, test_jose_encode_jws_json_general, NULL);
        abts_run_test(suite, test_jose_encode_jws_json_flattened, NULL);
        abts_run_test(suite, test_jose_encode_jwe_compact_rsaes_oaep_aes_gcm, NULL);
        abts_run_test(suite, test_jose_encode_jwe_json_general, NULL);
        abts_run_test(suite, test_jose_encode_jwe_json_flattened, NULL);

        return suite;
}
