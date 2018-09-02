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
 * @file apr_jose.h
 * @brief APR-UTIL JSON Object Signing and Encryption Library
 */
#ifndef APR_JOSE_H
#define APR_JOSE_H

/**
 * @defgroup APR_Util_JOSE JSON Object Signing and Encryption
 * @ingroup APR_Util
 * @{
 *
 * The JOSE (JSON Object Signing and Encryption) library allows the encoding
 * and decoding of JWS (JSON Web Signature), JWE (JSON Web Encryption), JWK
 * (JSON Web Key) and JWT (JSON Web Token) objects, encoded using compact
 * encoding, JSON encoding, or flattened JSON encoding.
 *
 * The following RFCs are supported:
 *
 * - https://tools.ietf.org/html/rfc7515 - JSON Web Signature (JWS)
 * - https://tools.ietf.org/html/rfc7516 - JSON Web Encryption (JWE)
 * - https://tools.ietf.org/html/rfc7517 - JSON Web Key (JWK)
 * - https://tools.ietf.org/html/rfc7519 - JSON Web Token (JWT)
 *
 * Encryption, decryption, signing and verification are implemented as
 * callbacks to the caller's specification, and are not included.
 *
 * When decrypting or verifying, the caller MUST verify that the 'alg'
 * algorithm parameter in the JOSE message matches the algorithm expected
 * by the implementation.
 *
 * It is recommended that the apr_crypto library be used to implement the
 * callbacks, however an alternatively crypto library of the caller's choice
 * may be used instead.
 */
#include "apr.h"
#include "apr_pools.h"
#include "apu_errno.h"
#include "apr_strings.h"
#include "apr_buckets.h"
#include "apr_json.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @package Apache JOSE library
 *
 */

/**
 * HMAC using SHA-256
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_HS256 "HS256"

/**
 * HMAC using SHA-384
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_HS384 "HS384"

/**
 * HMAC using SHA-512
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_HS512 "HS512"

/**
 * RSASSA-PKCS1-v1_5 using SHA-256
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_RS256 "RS256"

/**
 * RSASSA-PKCS1-v1_5 using SHA-384
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_RS384 "RS384"

/**
 * RSASSA-PKCS1-v1_5 using SHA-512
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_RS512 "RS512"

/**
 * ECDSA using P-256 and SHA-256
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_ES256 "ES256"

/**
 * ECDSA using P-384 and SHA-384
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_ES384 "ES384"

/**
 * ECDSA using P-512 and SHA-512
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_ES512 "ES512"

/**
 * RSASSA-PSS using SHA-256 and MGF1 with SHA-256
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_PS256 "PS256"

/**
 * RSASSA-PSS using SHA-384 and MGF1 with SHA-384
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_PS384 "PS384"

/**
 * RSASSA-PSS using SHA-512 and MGF1 with SHA-512
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_PS512 "PS512"

/**
 * No digital signature or MAC performed
 *
 * https://tools.ietf.org/html/rfc7518#section-3.1
 */
#define APR_JOSE_JWA_NONE "none"

/**
 * "kty" (Key Type) Parameter
 *
 * https://tools.ietf.org/html/rfc7517#section-4.1
 */
#define APR_JOSE_JWK_KEY_TYPE "kty"

/**
 * "use" (Public Key Use) Parameter
 *
 * https://tools.ietf.org/html/rfc7517#section-4.2
 */
#define APR_JOSE_JWK_PUBLIC_KEY_USE "use"

/**
 * "key_ops" (Key Operations) Parameter
 *
 * https://tools.ietf.org/html/rfc7517#section-4.3
 */
#define APR_JOSE_JWK_KEY_OPERATIONS "key_ops"

/**
 * "keys" Parameter
 *
 * https://tools.ietf.org/html/rfc7517#section-5.1
 */
#define APR_JOSE_JWK_KEYS "keys"

/**
 * "alg" (Algorithm) Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.1
 * https://tools.ietf.org/html/rfc7516#section-4.1.1
 * https://tools.ietf.org/html/rfc7517#section-4.4
 */
#define APR_JOSE_JWKSE_ALGORITHM "alg"

/**
 * "enc" (Encryption Algorithm) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7516#section-4.1.2
 */
#define APR_JOSE_JWE_ENCRYPTION "enc"

/**
 * "zip" (Compression Algorithm) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7516#section-4.1.3
 */
#define APR_JOSE_JWE_COMPRESSION "zip"

/**
 * "jku" (JWK Set URL) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.2
 * https://tools.ietf.org/html/rfc7516#section-4.1.4
 */
#define APR_JOSE_JWSE_JWK_SET_URL "jku"

/**
 * "jwk" (JSON Web Key) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.3
 * https://tools.ietf.org/html/rfc7516#section-4.1.5
 */
#define APR_JOSE_JWSE_JWK "jwk"

/**
 * "kid" (Key ID) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.4
 * https://tools.ietf.org/html/rfc7516#section-4.1.6
 */
#define APR_JOSE_JWKSE_KEYID "kid"

/**
 * "x5u" (X.509 URL) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.5
 * https://tools.ietf.org/html/rfc7516#section-4.1.7
 */
#define APR_JOSE_JWKSE_X509_URL "x5u"

/**
 * "x5c" (X.509 Certificate Chain) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.6
 * https://tools.ietf.org/html/rfc7516#section-4.1.8
 */
#define APR_JOSE_JWKSE_X509_CHAIN "x5c"

/**
 * "x5t" (X.509 Certificate SHA-1 Thumbprint) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.7
 * https://tools.ietf.org/html/rfc7516#section-4.1.9
 */
#define APR_JOSE_JWKSE_X509_SHA1_THUMBPRINT "x5t"

/**
 *"x5t#S256" (X.509 Certificate SHA-256 Thumbprint) Header
 * Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.8
 * https://tools.ietf.org/html/rfc7516#section-4.1.10
 */
#define APR_JOSE_JWKSE_X509_SHA256_THUMBPRINT "x5t#S256"

/**
 * "typ" (Type) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.9
 * https://tools.ietf.org/html/rfc7516#section-4.1.11
 */
#define APR_JOSE_JWSE_TYPE "typ"

/**
 * "cty" (Content Type) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.10
 * https://tools.ietf.org/html/rfc7516#section-4.1.12
 */
#define APR_JOSE_JWSE_CONTENT_TYPE "cty"

/**
 * "crit" (Critical) Header Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-4.1.11
 * https://tools.ietf.org/html/rfc7516#section-4.1.13
 */
#define APR_JOSE_JWSE_CRITICAL "crit"

/**
 * "payload" Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-7.2.1
 */
#define APR_JOSE_JWS_PAYLOAD "payload"

/**
 * "signatures" Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-7.2.1
 */
#define APR_JOSE_JWS_SIGNATURES "signatures"

/**
 * "protected" Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-7.2.1
 * https://tools.ietf.org/html/rfc7516#section-7.2.1
 */
#define APR_JOSE_JWSE_PROTECTED "protected"

/**
 * "header" Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-7.2.1
 * https://tools.ietf.org/html/rfc7516#section-7.2.1
 */
#define APR_JOSE_JWSE_HEADER "header"

/**
 * "signature" Parameter
 *
 * https://tools.ietf.org/html/rfc7515#section-7.2.1
 */
#define APR_JOSE_JWS_SIGNATURE "signature"

/**
 * "unprotected" Parameter
 *
 * https://tools.ietf.org/html/rfc7516#section-7.2.1
 */
#define APR_JOSE_JWE_UNPROTECTED "unprotected"

/**
 * "ciphertext" Parameter
 *
 * https://tools.ietf.org/html/rfc7516#section-7.2.1
 */
#define APR_JOSE_JWE_CIPHERTEXT "ciphertext"

/**
 * "recipients" Parameter
 *
 * https://tools.ietf.org/html/rfc7516#section-7.2.1
 */
#define APR_JOSE_JWE_RECIPIENTS "recipients"

/**
 * "encrypted_key" Parameter
 *
 * https://tools.ietf.org/html/rfc7516#section-7.2.1
 */
#define APR_JOSE_JWE_EKEY "encrypted_key"

/**
 * "iv" Parameter
 *
 * https://tools.ietf.org/html/rfc7516#section-7.2.1
 */
#define APR_JOSE_JWE_IV "iv"

/**
 * "tag" Parameter
 *
 * https://tools.ietf.org/html/rfc7516#section-7.2.1
 */
#define APR_JOSE_JWE_TAG "tag"

/**
 * "aad" Parameter
 *
 * https://tools.ietf.org/html/rfc7516#section-7.2.1
 */
#define APR_JOSE_JWE_AAD "aad"

/**
 * "iss" (Issuer) Claim
 *
 * https://tools.ietf.org/html/rfc7519#section-4.1.1
 */
#define APR_JOSE_JWT_ISSUER "iss"

/**
 * "sub" (Subject) Claim
 *
 * https://tools.ietf.org/html/rfc7519#section-4.1.2
 */
#define APR_JOSE_JWT_SUBJECT "sub"

/**
 * "aud" (Audience) Claim
 *
 * https://tools.ietf.org/html/rfc7519#section-4.1.3
 */
#define APR_JOSE_JWT_AUDIENCE "aud"

/**
 * "exp" (Expiration Time) Claim
 *
 * https://tools.ietf.org/html/rfc7519#section-4.1.4
 */
#define APR_JOSE_JWT_EXPIRATION_TIME "exp"

/**
 * "nbf" (Not Before) Claim
 *
 * https://tools.ietf.org/html/rfc7519#section-4.1.5
 */
#define APR_JOSE_JWT_NOT_BEFORE "nbf"

/**
 * "iat" (Issued At) Claim
 *
 * https://tools.ietf.org/html/rfc7519#section-4.1.6
 */
#define APR_JOSE_JWT_ISSUED_AT "iat"

/**
 * "jti" (JWT ID) Claim
 *
 * https://tools.ietf.org/html/rfc7519#section-4.1.7
 */
#define APR_JOSE_JWT_ID "jti"

/**
 * "typ" (Type) Header Parameter representing a JWT
 *
 * https://tools.ietf.org/html/rfc7519#section-5.1
 */
#define APR_JOSE_JWSE_TYPE_JWT "JWT"

/**
 * Default options.
 */
#define APR_JOSE_FLAG_NONE    0

/**
 * Return the full JOSE structure, instead of innermost nested structure.
 */
#define APR_JOSE_FLAG_DECODE_ALL    1

/**
 * When verifying or decrypting, break out of processing.
 *
 * If the verification or decryption failed, processing will be aborted
 * with the given error.
 *
 * If the verification or decryption succeeded, processing will be considered
 * successful and will move on to the nested structure.
 */
#define APR_JOSE_FLAG_BREAK    2

/**
 * Forward declaration of the apr_jose_t structure.
 */
typedef struct apr_jose_t apr_jose_t;

/**
 * Enum that represents the type of JOSE object.
 */
typedef enum apr_jose_type_e {
    /** No specific type. */
    APR_JOSE_TYPE_NONE = 0,
    /** JSON Web Key (JWK) */
    APR_JOSE_TYPE_JWK = 1,
    /** JSON Web Key Set (JWKS) */
    APR_JOSE_TYPE_JWKS,
    /** JSON Web Signature (JWS) - compact encoding */
    APR_JOSE_TYPE_JWS,
    /** JSON Web Signature (JWS) - JSON encoding */
    APR_JOSE_TYPE_JWS_JSON,
    /** JSON Web Encryption (JWE) - compact encoding */
    APR_JOSE_TYPE_JWE,
    /** JSON Web Encryption (JWE) - JSON encoding */
    APR_JOSE_TYPE_JWE_JSON,
    /** JSON Web Token (JWT) */
    APR_JOSE_TYPE_JWT,
    /** Generic binary data */
    APR_JOSE_TYPE_DATA,
    /** Generic text data */
    APR_JOSE_TYPE_TEXT,
    /** Generic JSON structure */
    APR_JOSE_TYPE_JSON
} apr_jose_type_e;

/**
 * Unsigned char data of a given length
 */
typedef struct apr_jose_data_t {
    /** Pointer to the data */
    const unsigned char *data;
    /** Length of the data */
    apr_size_t len;
} apr_jose_data_t;

/**
 * Signed char data of a given length
 */
typedef struct apr_jose_text_t {
    /** Pointer to the text */
    const char *text;
    /** Length of the text */
    apr_size_t len;
} apr_jose_text_t;

/**
 * JSON object
 */
typedef struct apr_jose_json_t {
    /** Parsed JSON structure. */
    apr_json_value_t *json;
} apr_jose_json_t;

/**
 * A JSON web key
 */
typedef struct apr_jose_jwk_t {
    /** Parsed JWK JSON structure */
    apr_json_value_t *key;
} apr_jose_jwk_t;

/**
 * A JSON web key set
 */
typedef struct apr_jose_jwks_t {
    /** Parsed JWK set JSON structure containing a JSON array */
    apr_json_value_t *keys;
} apr_jose_jwks_t;

/**
 * A single signature within a a JSON web signature.
 */
typedef struct apr_jose_signature_t {
    /** JWS Header */
    apr_json_value_t *header;
    /** JWS Protected Header */
    apr_json_value_t *protected_header;
    /** JWS Signature */
    apr_jose_data_t sig;
    /** Result of verification for this signature */
    apr_status_t status;
} apr_jose_signature_t;

/**
 * A JSON web signature
 */
typedef struct apr_jose_jws_t {
    /** JWS Compact / Flattened Signature */
    apr_jose_signature_t *signature;
    /** JWS General Signatures */
    apr_array_header_t *signatures;
    /** JWS Payload */
    apr_jose_t *payload;
} apr_jose_jws_t;

/**
 * An encrypted payload within a a JSON web encryption.
 */
typedef struct apr_jose_encryption_t {
    /** JWE Shared Header */
    apr_json_value_t *unprotected;
    /** JWE Protected Header */
    apr_json_value_t *protected;
    /** JWE Protected Header (basde64url) */
    apr_jose_text_t protected64;
    /** JWE Initialization Vector */
    apr_jose_data_t iv;
    /** JWE AAD */
    apr_jose_data_t aad;
    /** JWE AAD (base64url)*/
    apr_jose_text_t aad64;
    /** JWE Ciphertext */
    apr_jose_data_t cipher;
    /** JWE Authentication Tag */
    apr_jose_data_t tag;
} apr_jose_encryption_t;

/**
 * A single recipient within a a JSON web encryption.
 */
typedef struct apr_jose_recipient_t {
    /** JWE Header */
    apr_json_value_t *header;
    /** JWE Encrypted Key */
    apr_jose_data_t ekey;
    /** Result of decryption for this recipient */
    apr_status_t status;
} apr_jose_recipient_t;

/**
 * A JSON web encryption
 */
typedef struct apr_jose_jwe_t {
    /** JWE Compact / Flattened Recipient */
    apr_jose_recipient_t *recipient;
    /** JWE General Recipients */
    apr_array_header_t *recipients;
    /** JWE Encryption Parameters */
    apr_jose_encryption_t *encryption;
    /** JWE Payload */
    apr_jose_t *payload;
} apr_jose_jwe_t;

/**
 * A JSON web token
 */
typedef struct apr_jose_jwt_t {
    /** Claims associated with the JWT. */
    apr_json_value_t *claims;
} apr_jose_jwt_t;

/**
 * One JOSE structure to rule them all.
 */
struct apr_jose_t {
    /** pool used for allocation */
    apr_pool_t *pool;
    /** content type of this structure */
    const char *typ;
    /** content type of the payload */
    const char *cty;
    /** result of the operation */
    apu_err_t result;
    /** type of the value */
    apr_jose_type_e type;
    /** actual value, depending on the type */
    union {
        apr_jose_jwk_t *jwk;
        apr_jose_jwks_t *jwks;
        apr_jose_jws_t *jws;
        apr_jose_jwe_t *jwe;
        apr_jose_jwt_t *jwt;
        apr_jose_data_t *data;
        apr_jose_text_t *text;
        apr_jose_json_t *json;
    } jose;
};

/**
 * Callbacks for encryption, decryption, signing and verifying.
 */
typedef struct apr_jose_cb_t {
    /**
     * Callback that encrypts the content of the bucket brigade bb based
     * on the parameters provided by the jwe->protected_header, and writes
     * the resulting encrypted key to recipient->ekey, the initialisation vector
     * to encryption->iv, the additional authentication data to encryption->aad, the
     * cipher text to encryption->cipher, and the tag to encryption->tag.
     *
     * The encrypt function is expected to perform some or all of the
     * following steps:
     *
     *    1.   Determine the Key Management Mode employed by the algorithm used
     *         to determine the Content Encryption Key value.  (This is the
     *         algorithm recorded in the "alg" (algorithm) Header Parameter of
     *         the resulting JWE.)
     *
     *    2.   When Key Wrapping, Key Encryption, or Key Agreement with Key
     *         Wrapping are employed, generate a random CEK value.  See RFC
     *         4086 [RFC4086] for considerations on generating random values.
     *         The CEK MUST have a length equal to that required for the
     *         content encryption algorithm.
     *
     *    3.   When Direct Key Agreement or Key Agreement with Key Wrapping are
     *         employed, use the key agreement algorithm to compute the value
     *         of the agreed upon key.  When Direct Key Agreement is employed,
     *         let the CEK be the agreed upon key.  When Key Agreement with Key
     *         Wrapping is employed, the agreed upon key will be used to wrap
     *         the CEK.
     *
     *    4.   When Key Wrapping, Key Encryption, or Key Agreement with Key
     *         Wrapping are employed, encrypt the CEK to the recipient and let
     *         the result be the JWE Encrypted Key.
     *
     *    5.   When Direct Key Agreement or Direct Encryption are employed, let
     *         the JWE Encrypted Key be the empty octet sequence.
     *
     *    6.   When Direct Encryption is employed, let the CEK be the shared
     *         symmetric key.
     *
     *    8.   If the JWE JSON Serialization is being used, repeat this process
     *         (steps 1-7) for each recipient.
     *
     *    9.   Generate a random JWE Initialization Vector of the correct size
     *         for the content encryption algorithm (if required for the
     *         algorithm); otherwise, let the JWE Initialization Vector be the
     *         empty octet sequence.
     *
     *    11.  If a "zip" parameter was included, compress the plaintext using
     *         the specified compression algorithm and let M be the octet
     *         sequence representing the compressed plaintext; otherwise, let M
     *         be the octet sequence representing the plaintext.
     *
     *    12.  Create the JSON object(s) containing the desired set of Header
     *         Parameters, which together comprise the JOSE Header: one or more
     *         of the JWE Protected Header, the JWE Shared Unprotected Header,
     *         and the JWE Per-Recipient Unprotected Header.
     *
     *    13.  Compute the Encoded Protected Header value BASE64URL(UTF8(JWE
     *         Protected Header)).  If the JWE Protected Header is not present
     *         (which can only happen when using the JWE JSON Serialization and
     *         no "protected" member is present), let this value be the empty
     *         string.
     *
     *    14.  Let the Additional Authenticated Data encryption parameter be
     *         ASCII(Encoded Protected Header).  However, if a JWE AAD value is
     *         present (which can only be the case when using the JWE JSON
     *         Serialization), instead let the Additional Authenticated Data
     *         encryption parameter be ASCII(Encoded Protected Header || '.' ||
     *         BASE64URL(JWE AAD)).
     *
     *    15.  Encrypt M using the CEK, the JWE Initialization Vector, and the
     *         Additional Authenticated Data value using the specified content
     *         encryption algorithm to create the JWE Ciphertext value and the
     *         JWE Authentication Tag (which is the Authentication Tag output
     *         from the encryption operation).
     *
     * @param bb Brigade containing data to be encrypted.
     * @param jose The JOSE structure.
     * @param recipient Structure containing details of the recipient of
     *   this message.
     * @param encryption Structure to be filled out by the callback
     *   containing the encrypted message.
     * @param ctx A context.
     * @param pool The pool to use.
     * @return APR_SUCCESS if encrypted successfully, APR_ENOTIMPL if
     *   encryption is not supported, or any other suitable error. The
     *   jose->result structure may be filled out with further details of
     *   any error.
     */
    apr_status_t (*encrypt)(apr_bucket_brigade *bb, apr_jose_t *jose,
            apr_jose_recipient_t *recipient, apr_jose_encryption_t *encryption,
            void *ctx, apr_pool_t *pool);
    /**
     * Callback that decrypts the ciphertext based
     * on the parameters provided by the recipient and encryption parameters, and writes
     * the resulting decrypted value to the bucket brigade. Base64url versions of the
     * protected header and the aad are provided as part of the JWE decryption
     * mechanism.
     *
     * For security reasons, this callback MUST verify that the algorithm
     * present in the JWE matches the algorithm expected by the decoder.
     *
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
     *
     * @param bb Brigade where decrypted data is to be written.
     * @param jose The JOSE structure.
     * @param recipient Structure containing details of the recipient of
     *   this message, to be used to decrypt the message.
     * @param encryption Structure containing the encrypted message.
     * @param header The JOSE protected header.
     * @param p64 The JOSE protected header in original BASE64URL format,
     *   for use during decryption.
     * @param aad64 The JOSE additional authenticated data in original
     *   BASE64URL format, for use during decryption.
     * @param ctx A context.
     * @param dflags A pointer to a flag. Set to APR_JOSE_FLAG_NONE for
     *   decryption to continue to the next recipient in the JWE, or
     *   APR_JOSE_FLAG_BREAK to stop decrypting further recipients.
     * @param pool The pool to use.
     * @return APR_SUCCESS if decrypted successfully, APR_ENOTIMPL if
     *   decryption is not supported, or any other suitable error. The
     *   jose->result structure may be filled out with further details of
     *   any error.
     */
    apr_status_t (*decrypt)(apr_bucket_brigade *bb, apr_jose_t *jose,
            apr_jose_recipient_t *recipient, apr_jose_encryption_t *encryption,
            apr_json_value_t *header, apr_jose_text_t *ph64,
            apr_jose_text_t *aad64, void *ctx, int *dflags, apr_pool_t *pool);
    /**
     * Callback that signs the content of the bucket brigade bb based
     * on the parameters provided by the signature protected header, and writes
     * the resulting binary signature to signature->sig.
     *
     * The sign function is expected to perform some or all of the
     * following steps:
     *
     *    5.  Compute the JWS Signature in the manner defined for the
     *        particular algorithm being used over the JWS Signing Input
     *        ASCII(BASE64URL(UTF8(JWS Protected Header)) || '.' ||
     *        BASE64URL(JWS Payload)).  The "alg" (algorithm) Header Parameter
     *        MUST be present in the JOSE Header, with the algorithm value
     *        accurately representing the algorithm used to construct the JWS
     *        Signature.
     *
     * @param bb Brigade containing data to be signed.
     * @param jose The JOSE structure.
     * @param signature Structure to be filled out by the callback
     *   containing the signature of the message.
     * @param ctx A context.
     * @param pool The pool to use.
     * @return APR_SUCCESS if signed successfully, APR_ENOTIMPL if
     *   signing is not supported, or any other suitable error. The
     *   jose->result structure may be filled out with further details of
     *   any error.
     */
    apr_status_t (*sign)(apr_bucket_brigade *bb, apr_jose_t *jose,
            apr_jose_signature_t *signature, void *ctx, apr_pool_t *pool);
    /**
     * Callback that verifies the content of the bucket brigade bb based
     * on the parameters provided by the signature protected header and
     * signature->sig.
     *
     * For security reasons, this callback MUST verify that the algorithm
     * present in the JWS matches the algorithm expected by the decoder.
     *
     * The verify function is expected to perform some or all of the
     * following steps:
     *
     *   8.  Validate the JWS Signature against the JWS Signing Input
     *       ASCII(BASE64URL(UTF8(JWS Protected Header)) || '.' ||
     *       BASE64URL(JWS Payload)) in the manner defined for the algorithm
     *       being used, which MUST be accurately represented by the value of
     *       the "alg" (algorithm) Header Parameter, which MUST be present.
     *       See Section 10.6 for security considerations on algorithm
     *       validation.  Record whether the validation succeeded or not.
     *
     *   9.  If the JWS JSON Serialization is being used, repeat this process
     *       (steps 4-8) for each digital signature or MAC value contained in
     *       the representation.
     *
     *   10. If none of the validations in step 9 succeeded, then the JWS MUST
     *       be considered invalid.  Otherwise, in the JWS JSON Serialization
     *       case, return a result to the application indicating which of the
     *       validations succeeded and failed.  In the JWS Compact
     *       Serialization case, the result can simply indicate whether or not
     *       the JWS was successfully validated.
     *
     * @param bb Brigade containing data to be verified.
     * @param jose The JOSE structure.
     * @param signature Structure containing the signature to be verified.
     * @param ctx A context.
     * @param dflags A pointer to a flag. Set to APR_JOSE_FLAG_NONE for
     *   verification to continue to the next recipient in the JWE, or
     *   APR_JOSE_FLAG_BREAK to stop verifying further recipients.
     * @param pool The pool to use.
     * @return APR_SUCCESS if verified successfully, APR_ENOTIMPL if
     *   verification is not supported, or any other suitable error. The
     *   jose->result structure may be filled out with further details of
     *   any error.
     */
    apr_status_t (*verify)(apr_bucket_brigade *bb, apr_jose_t *jose,
            apr_jose_signature_t *signature, void *ctx, int *vflags,
            apr_pool_t *pool);
    /** Context to be passed to the callback. */
    void *ctx;
} apr_jose_cb_t;

/**
 * @brief Get the result of the last operation on the jose. If the result
 *        is NULL, the operation was successful.
 * @param jose - context pointer
 * @return The apu_err_t is returned.
 */
APR_DECLARE(apu_err_t *) apr_jose_error(apr_jose_t *jose)
        __attribute__((nonnull(1)));

/**
 * Make a generic JOSE structure.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param type the type of structure to create.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_make(apr_jose_t *jose, apr_jose_type_e type,
        apr_pool_t *pool)
        __attribute__((nonnull(3)));

/**
 * Make a JSON Web Key for encoding or decoding.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param key the json representing the key. May be NULL.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_jwk_make(apr_jose_t *jose,
        apr_json_value_t *key, apr_pool_t *pool)
        __attribute__((nonnull(3)));

/**
 * Make a JSON Web Key Set.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param keys the array of keys in JSON format. May be NULL.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_jwks_make(apr_jose_t *jose,
        apr_json_value_t *keys, apr_pool_t *pool)
        __attribute__((nonnull(3)));

/**
 * Make a signature structure for JWS.
 *
 * @param signature the result.
 * @param header the unprotected header.
 * @param protected the protected header.
 * @param pool the pool to use.
 * @return The apr_jose_signature_t is returned.
 */
APR_DECLARE(apr_jose_signature_t *) apr_jose_signature_make(
        apr_jose_signature_t *signature, apr_json_value_t *header,
        apr_json_value_t *protected, apr_pool_t *pool)
        __attribute__((nonnull(4)));

/**
 * Make a recipient structure for JWE.
 *
 * @param recipient the result.
 * @param unprotected the unprotected header.
 * @param pool the pool to use.
 * @return The apr_jose_recipient_t is returned.
 */
APR_DECLARE(apr_jose_recipient_t *) apr_jose_recipient_make(apr_jose_recipient_t *recipient,
        apr_json_value_t *unprotected, apr_pool_t *pool)
        __attribute__((nonnull(3)));

/**
 * Make an encryption structure for JWE.
 *
 * @param encryption the result.
 * @param unprotected the unprotected shared header.
 * @param protected the protected header.
 * @param pool the pool to use.
 * @return The apr_jose_encryption_t is returned.
 */
APR_DECLARE(apr_jose_encryption_t *) apr_jose_encryption_make(apr_jose_encryption_t *encryption,
        apr_json_value_t *unprotected, apr_json_value_t *protected,
        apr_pool_t *pool)
        __attribute__((nonnull(4)));

/**
 * Make a compact encoded JWE.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param recipient the recipient for compact / flattened JWE.
 * @param recipients the recipients array for general JWE.
 * @param encryption the encryption structure.
 * @param payload the JOSE payload to encrypt.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_jwe_make(apr_jose_t *jose,
        apr_jose_recipient_t *recipient, apr_array_header_t *recipients,
        apr_jose_encryption_t *encryption, apr_jose_t *payload,
        apr_pool_t *pool)
        __attribute__((nonnull(6)));

/**
 * Make a JSON encoded JWE.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param recipient the recipient for compact / flattened JWE.
 * @param recipients the recipients array for general JWE.
 * @param encryption the encryption structure.
 * @param payload the JOSE payload to encrypt.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_jwe_json_make(apr_jose_t *jose,
        apr_jose_recipient_t *recipient,
        apr_array_header_t *recipients, apr_jose_encryption_t *encryption,
        apr_jose_t *payload, apr_pool_t *pool)
        __attribute__((nonnull(6)));

/**
 * Make a compact encoded JWS.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param signature the header / protected header / signature used with compact or flattened syntax. May be NULL.
 * @param signatures array of header / protected header / signature used with general JSON syntax.
 * @param payload the payload to be wrapped by this JWS.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_jws_make(apr_jose_t *jose,
        apr_jose_signature_t *signature, apr_array_header_t *signatures,
        apr_jose_t *payload, apr_pool_t *pool)
        __attribute__((nonnull(5)));

/**
 * Make a JSON encoded JWS.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param signature the header / protected header / signature used with compact or flattened syntax. May be NULL.
 * @param signatures array of header / protected header / signature used with general JSON syntax.
 * @param payload the payload to be wrapped by this JWS.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_jws_json_make(apr_jose_t *jose,
        apr_jose_signature_t *signature, apr_array_header_t *signatures,
        apr_jose_t *payload, apr_pool_t *pool)
        __attribute__((nonnull(5)));

/**
 * Make a JWT claims payload.
 *
 * To create a useful JWT, this payload needs to be wrapped in a JWS
 * or JWE (or both), as required by the caller.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param claims the claims to sign.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_jwt_make(apr_jose_t *jose,
        apr_json_value_t *claims, apr_pool_t *pool)
        __attribute__((nonnull(3)));

/**
 * Make a data buffer for encoding from the given data and length.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param typ the content type of this data.
 * @param in the plaintext to sign.
 * @param inlen length of the plaintext.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_data_make(apr_jose_t *jose, const char *typ,
        const unsigned char *in, apr_size_t inlen, apr_pool_t *pool)
        __attribute__((nonnull(5)));

/**
 * Make a UTF-8 text buffer for encoding from the given string
 * and length.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param cty the content type.
 * @param in the UTF-8 encoded text string.
 * @param inlen length of the UTF-8 encoded text string.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_text_make(apr_jose_t *jose, const char *cty,
        const char *in, apr_size_t inlen, apr_pool_t *pool)
        __attribute__((nonnull(5)));

/**
 * Make a json structure for encoding.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param cty the content type.
 * @param json the json object to add.
 * @param pool pool used to allocate the result from.
 * @return The apr_jose_t is returned.
 */
APR_DECLARE(apr_jose_t *) apr_jose_json_make(apr_jose_t *jose, const char *cty,
        apr_json_value_t *json, apr_pool_t *pool)
        __attribute__((nonnull(4)));

/**
 * Sign or encrypt the apr_jose_t, and write it to the brigade.
 * @param brigade brigade the result will be appended to.
 * @param flush The flush function to use if the brigade is full
 * @param ctx The structure to pass to the flush function
 * @param jose the JOSE to encode.
 * @param cb callbacks for sign and encrypt.
 * @param pool pool to be used.
 * @return APR_SUCCESS is returned if encoding was successful, otherwise
 * an APR status code, along with an apu_err_t with an explanation
 * allocated from jose->pool.
 */
APR_DECLARE(apr_status_t) apr_jose_encode(apr_bucket_brigade *brigade,
        apr_brigade_flush flush, void *ctx, apr_jose_t *jose,
        apr_jose_cb_t *cb, apr_pool_t *pool)
		__attribute__((nonnull(1, 4, 6)));

/**
 * Decode, decrypt and verify the utf8-encoded JOSE string into apr_jose_t.
 *
 * The JOSE structure may be nested to the given limit.
 * @param jose If jose points at NULL, a JOSE structure will be
 *   created. If the jose pointer is not NULL, the structure will
 *   be reused.
 * @param typ content type of this object.
 * @param brigade the JOSE structure to decode.
 * @param cb callbacks for verify and decrypt.
 * @param level depth limit of JOSE and JSON nesting.
 * @param flags APR_JOSE_FLAG_NONE to return payload only. APR_JOSE_FLAG_DECODE_ALL
 *   to return the full JWS/JWE structure.
 * @param pool pool used to allocate the result from.
 */
APR_DECLARE(apr_status_t) apr_jose_decode(apr_jose_t **jose, const char *typ,
        apr_bucket_brigade *brigade, apr_jose_cb_t *cb, int level, int flags,
        apr_pool_t *pool)
        __attribute__((nonnull(1, 3, 7)));


#ifdef __cplusplus
}
#endif
/** @} */
#endif /* APR_JOSE_H */
