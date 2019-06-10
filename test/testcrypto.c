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

#include "testutil.h"
#include "apr.h"
#include "apu.h"
#include "apu_errno.h"
#include "apr_pools.h"
#include "apr_dso.h"
#include "apr_crypto.h"
#include "apr_strings.h"
#include "apr_file_io.h"
#include "apr_thread_proc.h"

#if APU_HAVE_CRYPTO

#define TEST_STRING "12345"
#define ALIGNED_STRING "123456789012345"

static const apr_crypto_driver_t *get_driver(abts_case *tc, apr_pool_t *pool,
        const char *name, const char *params)
{

    const apr_crypto_driver_t *driver = NULL;
    const apu_err_t *result = NULL;
    apr_status_t rv;

    rv = apr_crypto_init(pool);
    ABTS_ASSERT(tc, "failed to init apr_crypto", rv == APR_SUCCESS);

    rv = apr_crypto_get_driver(&driver, name, params, &result, pool);
    if (APR_ENOTIMPL == rv) {
        ABTS_NOT_IMPL(tc,
                apr_psprintf(pool, "Crypto driver '%s' not implemented", (char *)name));
        return NULL;
    }
    if (APR_EDSOOPEN == rv) {
        ABTS_NOT_IMPL(tc,
                apr_psprintf(pool, "Crypto driver '%s' DSO could not be opened", (char *)name));
        return NULL;
    }
    if (APR_SUCCESS != rv && result) {
        char err[1024];
        apr_strerror(rv, err, sizeof(err) - 1);
        fprintf(stderr, "get_driver error %d: %s: '%s' native error %d: %s (%s),",
                rv, err, name, result->rc, result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, apr_psprintf(pool, "failed to apr_crypto_get_driver for '%s' with %d",
                name, rv), rv == APR_SUCCESS);
    ABTS_ASSERT(tc, "apr_crypto_get_driver returned NULL", driver != NULL);
    if (!driver || rv) {
        return NULL;
    }

    return driver;

}

static const apr_crypto_driver_t *get_nss_driver(abts_case *tc,
        apr_pool_t *pool)
{

    /* initialise NSS */
    return get_driver(tc, pool, "nss", "");

}

static const apr_crypto_driver_t *get_openssl_driver(abts_case *tc,
        apr_pool_t *pool)
{

    return get_driver(tc, pool, "openssl", NULL);

}

static const apr_crypto_driver_t *get_commoncrypto_driver(abts_case *tc,
        apr_pool_t *pool)
{

    return get_driver(tc, pool, "commoncrypto", NULL);

}

static apr_crypto_t *make(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver)
{

    apr_crypto_t *f = NULL;

    if (!driver) {
        return NULL;
    }

    /* get the context */
    apr_crypto_make(&f, driver, "engine=openssl", pool);
    ABTS_ASSERT(tc, "apr_crypto_make returned NULL", f != NULL);

    return f;

}

static const apr_crypto_key_t *keyhash(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        apr_crypto_block_key_digest_e digest, const char *description)
{
    apr_crypto_key_t *key = NULL;
    const apu_err_t *result = NULL;
    apr_crypto_key_rec_t *rec = apr_crypto_key_rec_make(APR_CRYPTO_KTYPE_HASH,
            pool);
    apr_status_t rv;

    if (!driver) {
        return NULL;
    }

    if (!f) {
        return NULL;
    }

    rec->k.hash.digest = digest;

    /* init the key */
    rv = apr_crypto_key(&key, rec, f, pool);
    if (APR_ENOCIPHER == rv || APR_ENODIGEST == rv) {
        apr_crypto_error(&result, f);
        ABTS_NOT_IMPL(tc,
                apr_psprintf(pool, "skipped: %s %s key return APR_ENOTIMPL: error %d: %s (%s)\n", description, apr_crypto_driver_name(driver), result->rc, result->reason ? result->reason : "", result->msg ? result->msg : ""));
        return NULL;
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr, "key: %s %s apr error %d / native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EKEYLENGTH", rv != APR_EKEYLENGTH);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_ENOKEY", rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EPADDING",
                rv != APR_EPADDING);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EKEYTYPE",
                rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "failed to apr_crypto_key", rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_key returned NULL context", key != NULL);
    }
    if (rv) {
        return NULL;
    }
    return key;

}

static const apr_crypto_key_t *keyhmac(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        apr_crypto_block_key_digest_e digest, apr_crypto_block_key_type_e type,
        apr_crypto_block_key_mode_e mode, int doPad, apr_size_t secretLen,
        const char *description)
{
    apr_crypto_key_t *key = NULL;
    const apu_err_t *result = NULL;
    apr_crypto_key_rec_t *rec = apr_crypto_key_rec_make(APR_CRYPTO_KTYPE_HMAC,
            pool);
    apr_status_t rv;

    if (!driver) {
        return NULL;
    }

    if (!f) {
        return NULL;
    }

    rec->type = type;
    rec->mode = mode;
    rec->pad = doPad;
    rec->k.hmac.digest = digest;
    rec->k.hmac.secret = apr_pcalloc(pool, secretLen);
    rec->k.hmac.secretLen = secretLen;

    /* init the key */
    rv = apr_crypto_key(&key, rec, f, pool);
    if (APR_ENOCIPHER == rv || APR_ENODIGEST == rv) {
        apr_crypto_error(&result, f);
        ABTS_NOT_IMPL(tc,
                apr_psprintf(pool, "skipped: %s %s key return APR_ENOTIMPL: error %d: %s (%s)\n", description, apr_crypto_driver_name(driver), result->rc, result->reason ? result->reason : "", result->msg ? result->msg : ""));
        return NULL;
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr, "key: %s %s apr error %d / native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EKEYLENGTH", rv != APR_EKEYLENGTH);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_ENOKEY", rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EPADDING",
                rv != APR_EPADDING);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EKEYTYPE",
                rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "failed to apr_crypto_key", rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_key returned NULL context", key != NULL);
    }
    if (rv) {
        return NULL;
    }
    return key;

}

static const apr_crypto_key_t *keysecret(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        apr_crypto_block_key_type_e type, apr_crypto_block_key_mode_e mode,
        int doPad, apr_size_t secretLen, const char *description)
{
    apr_crypto_key_t *key = NULL;
    const apu_err_t *result = NULL;
    apr_crypto_key_rec_t *rec = apr_crypto_key_rec_make(APR_CRYPTO_KTYPE_SECRET,
            pool);
    apr_status_t rv;

    if (!driver) {
        return NULL;
    }

    if (!f) {
        return NULL;
    }

    rec->type = type;
    rec->mode = mode;
    rec->pad = doPad;
    rec->k.secret.secret = apr_pcalloc(pool, secretLen);
    rec->k.secret.secretLen = secretLen;

    /* init the passphrase */
    rv = apr_crypto_key(&key, rec, f, pool);
    if (APR_ENOCIPHER == rv) {
        apr_crypto_error(&result, f);
        ABTS_NOT_IMPL(tc,
                apr_psprintf(pool, "skipped: %s %s key return APR_ENOCIPHER: error %d: %s (%s)\n", description, apr_crypto_driver_name(driver), result->rc, result->reason ? result->reason : "", result->msg ? result->msg : ""));
        return NULL;
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr, "key: %s %s apr error %d / native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EKEYLENGTH", rv != APR_EKEYLENGTH);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_ENOKEY", rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EPADDING",
                rv != APR_EPADDING);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EKEYTYPE",
                rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "failed to apr_crypto_key", rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_key returned NULL context", key != NULL);
    }
    if (rv) {
        return NULL;
    }
    return key;

}

static const apr_crypto_key_t *passphrase(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        apr_crypto_block_key_type_e type, apr_crypto_block_key_mode_e mode,
        int doPad, const char *description)
{

    apr_crypto_key_t *key = NULL;
    const apu_err_t *result = NULL;
    const char *pass = "secret";
    const char *salt = "salt";
    apr_status_t rv;

    if (!driver) {
        return NULL;
    }

    if (!f) {
        return NULL;
    }

    /* init the passphrase */
    rv = apr_crypto_passphrase(&key, NULL, pass, strlen(pass),
            (unsigned char *) salt, strlen(salt), type, mode, doPad, 4096, f,
            pool);
    if (APR_ENOCIPHER == rv) {
        apr_crypto_error(&result, f);
        ABTS_NOT_IMPL(tc, apr_psprintf(pool,
                        "skipped: %s %s passphrase return APR_ENOCIPHER: error %d: %s (%s)\n",
                        description, apr_crypto_driver_name(driver), result->rc,
                        result->reason ? result->reason : "", result->msg ? result->msg : ""));
        return NULL;
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr, "passphrase: %s %s apr error %d / native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_passphrase returned APR_ENOKEY", rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_passphrase returned APR_EPADDING", rv != APR_EPADDING);
        ABTS_ASSERT(tc, "apr_crypto_passphrase returned APR_EKEYTYPE", rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "failed to apr_crypto_passphrase", rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_passphrase returned NULL context", key != NULL);
    }
    if (rv) {
        return NULL;
    }
    return key;

}

static const apr_crypto_key_t *keypassphrase(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        apr_crypto_block_key_type_e type, apr_crypto_block_key_mode_e mode,
        int doPad, const char *description)
{

    apr_crypto_key_t *key = NULL;
    const apu_err_t *result = NULL;
    const char *pass = "secret";
    const char *salt = "salt";
    apr_crypto_key_rec_t *rec = apr_pcalloc(pool, sizeof(apr_crypto_key_rec_t));
    apr_status_t rv;

    if (!driver) {
        return NULL;
    }

    if (!f) {
        return NULL;
    }

    rec->ktype = APR_CRYPTO_KTYPE_PASSPHRASE;
    rec->type = type;
    rec->mode = mode;
    rec->pad = doPad;
    rec->k.passphrase.pass = pass;
    rec->k.passphrase.passLen = strlen(pass);
    rec->k.passphrase.salt = (unsigned char *)salt;
    rec->k.passphrase.saltLen = strlen(salt);
    rec->k.passphrase.iterations = 4096;

    /* init the passphrase */
    rv = apr_crypto_key(&key, rec, f, pool);
    if (APR_ENOCIPHER == rv) {
        apr_crypto_error(&result, f);
        ABTS_NOT_IMPL(tc, apr_psprintf(pool,
                        "skipped: %s %s key passphrase return APR_ENOCIPHER: error %d: %s (%s)\n",
                        description, apr_crypto_driver_name(driver), result->rc,
                        result->reason ? result->reason : "", result->msg ? result->msg : ""));
        return NULL;
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr, "key passphrase: %s %s apr error %d / native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_ENOKEY", rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EPADDING", rv != APR_EPADDING);
        ABTS_ASSERT(tc, "apr_crypto_key returned APR_EKEYTYPE", rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "failed to apr_crypto_key", rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_key returned NULL context", key != NULL);
    }
    if (rv) {
        return NULL;
    }
    return key;

}

static unsigned char *encrypt_block(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        const apr_crypto_key_t *key, const unsigned char *in,
        const apr_size_t inlen, unsigned char **cipherText,
        apr_size_t *cipherTextLen, const unsigned char **iv,
        apr_size_t *blockSize, const char *description)
{

    apr_crypto_block_t *block = NULL;
    const apu_err_t *result = NULL;
    apr_size_t len = 0;
    apr_status_t rv;

    if (!driver || !f || !key || !in) {
        return NULL;
    }

    /* init the encryption */
    rv = apr_crypto_block_encrypt_init(&block, iv, key, blockSize, pool);
    if (APR_ENOTIMPL == rv) {
        ABTS_NOT_IMPL(tc, "apr_crypto_block_encrypt_init returned APR_ENOTIMPL");
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr,
                    "encrypt_init: %s %s (APR %d) native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_block_encrypt_init returned APR_ENOKEY",
                rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_block_encrypt_init returned APR_ENOIV",
                rv != APR_ENOIV);
        ABTS_ASSERT(tc, "apr_crypto_block_encrypt_init returned APR_EKEYTYPE",
                rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "apr_crypto_block_encrypt_init returned APR_EKEYLENGTH",
                rv != APR_EKEYLENGTH);
        ABTS_ASSERT(tc,
                "apr_crypto_block_encrypt_init returned APR_ENOTENOUGHENTROPY",
                rv != APR_ENOTENOUGHENTROPY);
        ABTS_ASSERT(tc, "failed to apr_crypto_block_encrypt_init",
                rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_block_encrypt_init returned NULL context",
                block != NULL);
    }
    if (!block || rv) {
        return NULL;
    }

    /* encrypt the block */
    rv = apr_crypto_block_encrypt(cipherText, cipherTextLen, in, inlen, block);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr, "encrypt: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_block_encrypt returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "failed to apr_crypto_block_encrypt", rv == APR_SUCCESS);
    ABTS_ASSERT(tc, "apr_crypto_block_encrypt failed to allocate buffer", *cipherText != NULL);
    if (rv) {
        return NULL;
    }

    /* finalise the encryption */
    rv = apr_crypto_block_encrypt_finish(*cipherText + *cipherTextLen, &len,
            block);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr,
                "encrypt_finish: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_block_encrypt_finish returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "apr_crypto_block_encrypt_finish returned APR_EPADDING", rv != APR_EPADDING);
    ABTS_ASSERT(tc, "apr_crypto_block_encrypt_finish returned APR_ENOSPACE", rv != APR_ENOSPACE);
    ABTS_ASSERT(tc, "failed to apr_crypto_block_encrypt_finish", rv == APR_SUCCESS);
    *cipherTextLen += len;
    apr_crypto_block_cleanup(block);
    if (rv) {
        return NULL;
    }

    return *cipherText;

}

static unsigned char *decrypt_block(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        const apr_crypto_key_t *key, unsigned char *cipherText,
        apr_size_t cipherTextLen, unsigned char **plainText,
        apr_size_t *plainTextLen, const unsigned char *iv,
        apr_size_t *blockSize, const char *description)
{

    apr_crypto_block_t *block = NULL;
    const apu_err_t *result = NULL;
    apr_size_t len = 0;
    apr_status_t rv;

    if (!driver || !f || !key || !cipherText) {
        return NULL;
    }

    /* init the decryption */
    rv = apr_crypto_block_decrypt_init(&block, blockSize, iv, key, pool);
    if (APR_ENOTIMPL == rv) {
        ABTS_NOT_IMPL(tc, "apr_crypto_block_decrypt_init returned APR_ENOTIMPL");
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr,
                    "decrypt_init: %s %s (APR %d) native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_block_decrypt_init returned APR_ENOKEY", rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_block_decrypt_init returned APR_ENOIV", rv != APR_ENOIV);
        ABTS_ASSERT(tc, "apr_crypto_block_decrypt_init returned APR_EKEYTYPE", rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "apr_crypto_block_decrypt_init returned APR_EKEYLENGTH", rv != APR_EKEYLENGTH);
        ABTS_ASSERT(tc, "failed to apr_crypto_block_decrypt_init", rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_block_decrypt_init returned NULL context", block != NULL);
    }
    if (!block || rv) {
        return NULL;
    }

    /* decrypt the block */
    rv = apr_crypto_block_decrypt(plainText, plainTextLen, cipherText,
            cipherTextLen, block);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr, "decrypt: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_block_decrypt returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "failed to apr_crypto_block_decrypt", rv == APR_SUCCESS);
    ABTS_ASSERT(tc, "apr_crypto_block_decrypt failed to allocate buffer", *plainText != NULL);
    if (rv) {
        return NULL;
    }

    /* finalise the decryption */
    rv = apr_crypto_block_decrypt_finish(*plainText + *plainTextLen, &len,
            block);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr,
                "decrypt_finish: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_block_decrypt_finish returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "apr_crypto_block_decrypt_finish returned APR_EPADDING", rv != APR_EPADDING);
    ABTS_ASSERT(tc, "apr_crypto_block_decrypt_finish returned APR_ENOSPACE", rv != APR_ENOSPACE);
    ABTS_ASSERT(tc, "failed to apr_crypto_block_decrypt_finish", rv == APR_SUCCESS);
    if (rv) {
        return NULL;
    }

    *plainTextLen += len;
    apr_crypto_block_cleanup(block);

    return *plainText;

}

static apr_status_t sign_block(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        const apr_crypto_key_t *key, const unsigned char *in,
        const apr_size_t inlen, unsigned char **signature,
        apr_size_t *signatureLen,
        apr_size_t *blockSize, const char *description)
{

    apr_crypto_digest_t *digest = NULL;
    const apu_err_t *result = NULL;
    apr_crypto_digest_rec_t *rec = apr_crypto_digest_rec_make(
            APR_CRYPTO_DTYPE_SIGN, pool);
    apr_status_t rv;

    if (!driver || !f || !key || !in) {
        return APR_EGENERAL;
    }

    /* init the signature */
    rv = apr_crypto_digest_init(&digest, key, rec, pool);
    if (APR_ENOTIMPL == rv) {
        ABTS_NOT_IMPL(tc, "apr_crypto_digest_init returned APR_ENOTIMPL");
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr,
                    "sign_init: %s %s (APR %d) native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_ENOKEY",
                rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_ENOIV",
                rv != APR_ENOIV);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_EKEYTYPE",
                rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_EKEYLENGTH",
                rv != APR_EKEYLENGTH);
        ABTS_ASSERT(tc,
                "apr_crypto_digest_init returned APR_ENOTENOUGHENTROPY",
                rv != APR_ENOTENOUGHENTROPY);
        ABTS_ASSERT(tc, "failed to apr_crypto_digest_init",
                rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned NULL context",
                digest != NULL);
    }
    if (!digest || rv) {
        return rv;
    }

    /* sign the block */
    rv = apr_crypto_digest_update(digest, in, inlen);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr, "sign: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_digest returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "failed to apr_crypto_digest", rv == APR_SUCCESS);
    if (rv) {
        return rv;
    }

    /* finalise the sign */
    rv = apr_crypto_digest_final(digest);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr,
                "sign_finish: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_digest_final returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "apr_crypto_digest_final returned APR_EPADDING", rv != APR_EPADDING);
    ABTS_ASSERT(tc, "apr_crypto_digest_final returned APR_ENOSPACE", rv != APR_ENOSPACE);
    ABTS_ASSERT(tc, "failed to apr_crypto_digest_final", rv == APR_SUCCESS);
    ABTS_ASSERT(tc, "apr_crypto_digest_final failed to allocate buffer", rec->d.sign.s != NULL);

    apr_crypto_digest_cleanup(digest);

    *signature = rec->d.sign.s;
    *signatureLen = rec->d.sign.slen;

    return rv;

}

static apr_status_t hash_block(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        const apr_crypto_key_t *key, const unsigned char *in,
        const apr_size_t inlen, unsigned char **hash,
        apr_size_t *hashLen,
        apr_size_t *blockSize, const char *description)
{

    apr_crypto_digest_t *digest = NULL;
    const apu_err_t *result = NULL;
    apr_crypto_digest_rec_t *rec = apr_crypto_digest_rec_make(
            APR_CRYPTO_DTYPE_HASH, pool);
    apr_status_t rv;

    if (!driver || !f || !key || !in) {
        return APR_EGENERAL;
    }

    rec->d.hash.digest = APR_CRYPTO_DIGEST_SHA256;

    /* init the signature */
    rv = apr_crypto_digest_init(&digest, key, rec, pool);
    if (APR_ENOTIMPL == rv) {
        ABTS_NOT_IMPL(tc, "apr_crypto_digest_init returned APR_ENOTIMPL");
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr,
                    "sign_init: %s %s (APR %d) native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_ENOKEY",
                rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_ENOIV",
                rv != APR_ENOIV);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_EKEYTYPE",
                rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_EKEYLENGTH",
                rv != APR_EKEYLENGTH);
        ABTS_ASSERT(tc,
                "apr_crypto_digest_init returned APR_ENOTENOUGHENTROPY",
                rv != APR_ENOTENOUGHENTROPY);
        ABTS_ASSERT(tc, "failed to apr_crypto_digest_init",
                rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned NULL context",
                digest != NULL);
    }
    if (!digest || rv) {
        return rv;
    }

    /* sign the block */
    rv = apr_crypto_digest_update(digest, in, inlen);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr, "sign: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_digest returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "failed to apr_crypto_digest", rv == APR_SUCCESS);
    if (rv) {
        return rv;
    }

    /* finalise the sign */
    rv = apr_crypto_digest_final(digest);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr,
                "sign_finish: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_digest_final returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "apr_crypto_digest_final returned APR_EPADDING", rv != APR_EPADDING);
    ABTS_ASSERT(tc, "apr_crypto_digest_final returned APR_ENOSPACE", rv != APR_ENOSPACE);
    ABTS_ASSERT(tc, "failed to apr_crypto_digest_final", rv == APR_SUCCESS);
    ABTS_ASSERT(tc, "apr_crypto_digest_final failed to allocate buffer", rec->d.hash.s != NULL);

    apr_crypto_digest_cleanup(digest);

    *hash = rec->d.hash.s;
    *hashLen = rec->d.hash.slen;

    return rv;

}

static apr_status_t verify_block(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t *driver, const apr_crypto_t *f,
        const apr_crypto_key_t *key, const unsigned char *in,
        apr_size_t inlen, const unsigned char *signature,
        apr_size_t signatureLen,
        apr_size_t *blockSize, const char *description)
{

    apr_crypto_digest_t *digest = NULL;
    const apu_err_t *result = NULL;
    apr_crypto_digest_rec_t *rec = apr_crypto_digest_rec_make(
            APR_CRYPTO_DTYPE_VERIFY, pool);
    apr_status_t rv;

    if (!driver || !f || !key || !in || !signature) {
        return APR_EGENERAL;
    }

    rec->d.verify.v = signature;
    rec->d.verify.vlen = signatureLen;

    /* init the decryption */
    rv = apr_crypto_digest_init(&digest, key, rec, pool);
    if (APR_ENOTIMPL == rv) {
        ABTS_NOT_IMPL(tc, "apr_crypto_digest_init returned APR_ENOTIMPL");
    }
    else {
        if (APR_SUCCESS != rv) {
            apr_crypto_error(&result, f);
            fprintf(stderr,
                    "digest_init: %s %s (APR %d) native error %d: %s (%s)\n",
                    description, apr_crypto_driver_name(driver), rv, result->rc,
                    result->reason ? result->reason : "",
                    result->msg ? result->msg : "");
        }
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_ENOKEY", rv != APR_ENOKEY);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_ENOIV", rv != APR_ENOIV);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_EKEYTYPE", rv != APR_EKEYTYPE);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned APR_EKEYLENGTH", rv != APR_EKEYLENGTH);
        ABTS_ASSERT(tc, "failed to apr_crypto_digest_init", rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_digest_init returned NULL context", digest != NULL);
    }
    if (!digest || rv) {
        return APR_EGENERAL;
    }

    /* decrypt the block */
    rv = apr_crypto_digest_update(digest, in, inlen);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr, "decrypt: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_digest returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "failed to apr_crypto_digest", rv == APR_SUCCESS);
    ABTS_ASSERT(tc, "apr_crypto_digest failed to allocate buffer", signature != NULL);
    if (rv) {
        return APR_EGENERAL;
    }

    /* finalise the decryption */
    rv = apr_crypto_digest_final(digest);
    if (APR_SUCCESS != rv) {
        apr_crypto_error(&result, f);
        fprintf(stderr,
                "verify_finish: %s %s (APR %d) native error %d: %s (%s)\n",
                description, apr_crypto_driver_name(driver), rv, result->rc,
                result->reason ? result->reason : "",
                result->msg ? result->msg : "");
    }
    ABTS_ASSERT(tc, "apr_crypto_digest_final returned APR_ECRYPT", rv != APR_ECRYPT);
    ABTS_ASSERT(tc, "apr_crypto_digest_final returned APR_EPADDING", rv != APR_EPADDING);
    ABTS_ASSERT(tc, "apr_crypto_digest_final returned APR_ENOSPACE", rv != APR_ENOSPACE);
    ABTS_ASSERT(tc, "failed to apr_crypto_digest_final", rv == APR_SUCCESS);

    apr_crypto_digest_cleanup(digest);

    return rv;

}

/**
 * Interoperability test.
 *
 * data must point at an array of two driver structures. Data will be encrypted
 * with the first driver, and decrypted with the second.
 *
 * If the two drivers interoperate, the test passes.
 */
static void crypto_block_cross(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t **drivers,
        const apr_crypto_block_key_type_e type,
        const apr_crypto_block_key_mode_e mode, int doPad,
        const unsigned char *in, apr_size_t inlen, apr_size_t secretLen,
        const char *description)
{
    const apr_crypto_driver_t *driver1 = drivers[0];
    const apr_crypto_driver_t *driver2 = drivers[1];
    apr_crypto_t *f1 = NULL;
    apr_crypto_t *f2 = NULL;
    const apr_crypto_key_t *key1 = NULL;
    const apr_crypto_key_t *key2 = NULL;
    const apr_crypto_key_t *key3 = NULL;
    const apr_crypto_key_t *key4 = NULL;
    const apr_crypto_key_t *key5 = NULL;
    const apr_crypto_key_t *key6 = NULL;

    unsigned char *cipherText = NULL;
    apr_size_t cipherTextLen = 0;
    unsigned char *plainText = NULL;
    apr_size_t plainTextLen = 0;
    const unsigned char *iv = NULL;
    apr_size_t blockSize = 0;

    f1 = make(tc, pool, driver1);
    f2 = make(tc, pool, driver2);
    key1 = passphrase(tc, pool, driver1, f1, type, mode, doPad, description);
    key2 = passphrase(tc, pool, driver2, f2, type, mode, doPad, description);

    cipherText = encrypt_block(tc, pool, driver1, f1, key1, in, inlen,
            &cipherText, &cipherTextLen, &iv, &blockSize, description);
    plainText = decrypt_block(tc, pool, driver2, f2, key2, cipherText,
            cipherTextLen, &plainText, &plainTextLen, iv, &blockSize,
            description);

    if (cipherText && plainText) {
        if (memcmp(in, plainText, inlen)) {
            fprintf(stderr, "passphrase cross mismatch: %s %s/%s\n", description,
                    apr_crypto_driver_name(driver1), apr_crypto_driver_name(
                            driver2));
        }
        ABTS_STR_EQUAL(tc, (char *)in, (char *)plainText);
    }

    key3 = keysecret(tc, pool, driver1, f1, type, mode, doPad, secretLen, description);
    key4 = keysecret(tc, pool, driver2, f2, type, mode, doPad, secretLen, description);

    iv = NULL;
    blockSize = 0;
    cipherText = NULL;
    plainText = NULL;
    cipherText = encrypt_block(tc, pool, driver1, f1, key3, in, inlen,
            &cipherText, &cipherTextLen, &iv, &blockSize, description);
    plainText = decrypt_block(tc, pool, driver2, f2, key4, cipherText,
            cipherTextLen, &plainText, &plainTextLen, iv, &blockSize,
            description);

    if (cipherText && plainText) {
        if (memcmp(in, plainText, inlen)) {
            fprintf(stderr, "key secret cross mismatch: %s %s/%s\n", description,
                    apr_crypto_driver_name(driver1), apr_crypto_driver_name(
                            driver2));
        }
        ABTS_STR_EQUAL(tc, (char *)in, (char *)plainText);
    }

    key5 = keypassphrase(tc, pool, driver1, f1, type, mode, doPad, description);
    key6 = keypassphrase(tc, pool, driver2, f2, type, mode, doPad, description);

    iv = NULL;
    blockSize = 0;
    cipherText = NULL;
    plainText = NULL;
    cipherText = encrypt_block(tc, pool, driver1, f1, key5, in, inlen,
            &cipherText, &cipherTextLen, &iv, &blockSize, description);
    plainText = decrypt_block(tc, pool, driver2, f2, key6, cipherText,
            cipherTextLen, &plainText, &plainTextLen, iv, &blockSize,
            description);

    if (cipherText && plainText) {
        if (memcmp(in, plainText, inlen)) {
            fprintf(stderr, "key passphrase cross mismatch: %s %s/%s\n", description,
                    apr_crypto_driver_name(driver1), apr_crypto_driver_name(
                            driver2));
        }
        ABTS_STR_EQUAL(tc, (char *)in, (char *)plainText);
    }

}

/**
 * Interoperability test.
 *
 * data must point at an array of two driver structures. Data will be signed
 * with the first driver, and verified with the second.
 *
 * If the two drivers interoperate, the test passes.
 */
static void crypto_cross_hash(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t **drivers,
        const apr_crypto_block_key_digest_e digest,
        const unsigned char *in, apr_size_t inlen,
        const char *description)
{
    const apr_crypto_driver_t *driver1 = drivers[0];
    const apr_crypto_driver_t *driver2 = drivers[1];
    apr_crypto_t *f1 = NULL;
    apr_crypto_t *f2 = NULL;
    const apr_crypto_key_t *key7 = NULL;
    const apr_crypto_key_t *key8 = NULL;

    apr_size_t blockSize = 0;
    unsigned char *hash1 = NULL;
    unsigned char *hash2 = NULL;
    apr_size_t hash1Len = 0;
    apr_size_t hash2Len = 0;

    apr_status_t rv;

    f1 = make(tc, pool, driver1);
    f2 = make(tc, pool, driver2);

    key7 = keyhash(tc, pool, driver1, f1, digest, description);
    key8 = keyhash(tc, pool, driver2, f2, digest, description);

    blockSize = 0;
    rv = hash_block(tc, pool, driver1, f1, key7, in, inlen,
            &hash1, &hash1Len, &blockSize, description);

    if (APR_SUCCESS != rv && driver1 && driver2) {
        fprintf(stderr, "key hash cross error %d: %s %s/%s\n", rv, description,
                apr_crypto_driver_name(driver1),
                apr_crypto_driver_name(driver2));
    }

    rv = hash_block(tc, pool, driver2, f2, key8, in,
            inlen, &hash2, &hash2Len, &blockSize,
            description);

    if (APR_SUCCESS != rv && driver1 && driver2) {
        fprintf(stderr, "key hash cross error %d: %s %s/%s\n", rv, description,
                apr_crypto_driver_name(driver1),
                apr_crypto_driver_name(driver2));
    }

    if (driver1 && driver2
            && (!hash1 || !hash2 || hash1Len != hash2Len
                    || memcmp(hash1, hash2, hash1Len))) {
        fprintf(stderr, "key hash cross mismatch (hash): %s %s/%s\n", description,
                apr_crypto_driver_name(driver1),
                apr_crypto_driver_name(driver2));
    }

}

/**
 * Interoperability test.
 *
 * data must point at an array of two driver structures. Data will be signed
 * with the first driver, and verified with the second.
 *
 * If the two drivers interoperate, the test passes.
 */
static void crypto_cross_sign(abts_case *tc, apr_pool_t *pool,
        const apr_crypto_driver_t **drivers,
        const apr_crypto_block_key_digest_e digest,
        const apr_crypto_block_key_type_e type,
        const apr_crypto_block_key_mode_e mode, int doPad,
        const unsigned char *in, apr_size_t inlen, apr_size_t secretLen,
        const char *description)
{
    const apr_crypto_driver_t *driver1 = drivers[0];
    const apr_crypto_driver_t *driver2 = drivers[1];
    apr_crypto_t *f1 = NULL;
    apr_crypto_t *f2 = NULL;
    const apr_crypto_key_t *key7 = NULL;
    const apr_crypto_key_t *key8 = NULL;

    apr_size_t blockSize = 0;
    unsigned char *signature = NULL;
    apr_size_t signatureLen = 0;

    apr_status_t rv;

    f1 = make(tc, pool, driver1);
    f2 = make(tc, pool, driver2);

    key7 = keyhmac(tc, pool, driver1, f1, digest, type, mode, doPad, secretLen,
            description);
    key8 = keyhmac(tc, pool, driver2, f2, digest, type, mode, doPad, secretLen,
            description);

    blockSize = 0;
    rv = sign_block(tc, pool, driver1, f1, key7, in, inlen,
            &signature, &signatureLen, &blockSize, description);

    if (APR_SUCCESS != rv && driver1 && driver2) {
        fprintf(stderr, "key hmac cross mismatch (sign): %s %s/%s\n", description,
                apr_crypto_driver_name(driver1),
                apr_crypto_driver_name(driver2));
    }

    rv = verify_block(tc, pool, driver2, f2, key8, in,
            inlen, signature, signatureLen, &blockSize,
            description);

    if (APR_SUCCESS != rv && driver1 && driver2) {
        fprintf(stderr, "key hmac cross mismatch (verify): %s %s/%s\n", description,
                apr_crypto_driver_name(driver1),
                apr_crypto_driver_name(driver2));
    }

}

/**
 * Test initialisation.
 */
static void test_crypto_init(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    apr_status_t rv;

    apr_pool_create(&pool, NULL);

    rv = apr_crypto_init(apr_pool_parent_get(pool));
    ABTS_ASSERT(tc, "failed to init apr_crypto", rv == APR_SUCCESS);

    apr_pool_destroy(pool);

}

/**
 * Simple test of OpenSSL key.
 */
static void test_crypto_key_openssl(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *driver;
    apr_crypto_t *f = NULL;

    apr_pool_create(&pool, NULL);
    driver = get_openssl_driver(tc, pool);

    f = make(tc, pool, driver);
    keysecret(tc, pool, driver, f, APR_KEY_AES_256, APR_MODE_CBC, 1, 32,
            "KEY_AES_256/MODE_CBC");
    apr_pool_destroy(pool);

}

/**
 * Simple test of NSS key.
 */
static void test_crypto_key_nss(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *driver;
    apr_crypto_t *f = NULL;

    apr_pool_create(&pool, NULL);
    driver = get_nss_driver(tc, pool);

    f = make(tc, pool, driver);
    keysecret(tc, pool, driver, f, APR_KEY_AES_256, APR_MODE_CBC, 1, 32,
            "KEY_AES_256/MODE_CBC");
    apr_pool_destroy(pool);

}

/**
 * Simple test of CommonCrypto key.
 */
static void test_crypto_key_commoncrypto(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *driver;
    apr_crypto_t *f = NULL;

    apr_pool_create(&pool, NULL);
    driver = get_commoncrypto_driver(tc, pool);

    f = make(tc, pool, driver);
    keysecret(tc, pool, driver, f, APR_KEY_AES_256, APR_MODE_CBC, 1, 32,
            "KEY_AES_256/MODE_CBC");
    apr_pool_destroy(pool);

}

/**
 * Simple test of OpenSSL block crypt.
 */
static void test_crypto_block_openssl(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_openssl_driver(tc, pool);
    drivers[1] = get_openssl_driver(tc, pool);
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 0,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_ECB, 0,
            in, inlen, 24, "KEY_3DES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 0, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 0, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 0, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 0, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 0, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 0, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");
    apr_pool_destroy(pool);

}

/**
 * Simple test of OpenSSL block signatures.
 */
static void test_crypto_digest_openssl(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_openssl_driver(tc, pool);
    drivers[1] = get_openssl_driver(tc, pool);
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_CBC, 0, in, inlen, 32,
            "KEY_AES_256/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_ECB, 0, in, inlen, 32,
            "KEY_AES_256/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_AES_192/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_AES_192/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_CBC, 0, in, inlen, 16,
            "KEY_AES_128/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_ECB, 0, in, inlen, 16,
            "KEY_AES_128/MODE_ECB");

    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_MD5, in, inlen,
            "DIGEST MD5");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA1, in, inlen,
            "DIGEST SHA1");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA224, in, inlen,
            "DIGEST SHA224");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256, in, inlen,
            "DIGEST SHA256");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA384, in, inlen,
            "DIGEST SHA384");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA512, in, inlen,
            "DIGEST SHA512");

    apr_pool_destroy(pool);

}

/**
 * Simple test of NSS block crypt.
 */
static void test_crypto_block_nss(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_nss_driver(tc, pool);
    drivers[1] = get_nss_driver(tc, pool);
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 0,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");
    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
    /* crypto_block_cross(tc, pool, drivers, KEY_3DES_192, MODE_ECB, 0, in, inlen, "KEY_3DES_192/MODE_ECB"); */
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 0, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 0, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 0, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 0, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 0, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 0, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");
    apr_pool_destroy(pool);

}

/**
 * Simple test of NSS block sign/verify.
 */
static void test_crypto_digest_nss(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_nss_driver(tc, pool);
    drivers[1] = get_nss_driver(tc, pool);
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_CBC");
    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
    /* crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256, KEY_3DES_192, MODE_ECB, 0, in, inlen, "KEY_3DES_192/MODE_ECB"); */
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_CBC, 0, in, inlen, 32,
            "KEY_AES_256/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_ECB, 0, in, inlen, 32,
            "KEY_AES_256/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_AES_192/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_AES_192/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_CBC, 0, in, inlen, 16,
            "KEY_AES_128/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_ECB, 0, in, inlen, 16,
            "KEY_AES_128/MODE_ECB");

    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_MD5, in, inlen,
            "DIGEST MD5");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA1, in, inlen,
            "DIGEST SHA1");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA224, in, inlen,
            "DIGEST SHA224");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256, in, inlen,
            "DIGEST SHA256");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA384, in, inlen,
            "DIGEST SHA384");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA512, in, inlen,
            "DIGEST SHA512");

    apr_pool_destroy(pool);

}

/**
 * Simple test of Common Crypto block crypt.
 */
static void test_crypto_block_commoncrypto(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_commoncrypto_driver(tc, pool);
    drivers[1] = get_commoncrypto_driver(tc, pool);
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 0,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_ECB, 0,
            in, inlen, 24, "KEY_3DES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 0, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 0, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 0, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 0, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 0, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 0, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");
    apr_pool_destroy(pool);

}

/**
 * Simple test of Common Crypto block sign.
 */
static void test_crypto_digest_commoncrypto(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_commoncrypto_driver(tc, pool);
    drivers[1] = get_commoncrypto_driver(tc, pool);
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_CBC, 0, in, inlen, 32,
            "KEY_AES_256/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_ECB, 0, in, inlen, 32,
            "KEY_AES_256/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_AES_192/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_AES_192/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_CBC, 0, in, inlen, 16,
            "KEY_AES_128/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_ECB, 0, in, inlen, 16,
            "KEY_AES_128/MODE_ECB");

    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_MD5, in, inlen,
            "DIGEST MD5");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA1, in, inlen,
            "DIGEST SHA1");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA224, in, inlen,
            "DIGEST SHA224");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256, in, inlen,
            "DIGEST SHA256");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA384, in, inlen,
            "DIGEST SHA384");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA512, in, inlen,
            "DIGEST SHA512");

    apr_pool_destroy(pool);

}

/**
 * Encrypt NSS, decrypt OpenSSL.
 */
static void test_crypto_block_nss_openssl(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_nss_driver(tc, pool);
    drivers[1] = get_openssl_driver(tc, pool);

    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 0,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");

    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
    /* crypto_block_cross(tc, pool, drivers, KEY_3DES_192, MODE_ECB, 0, in, inlen, 24, "KEY_3DES_192/MODE_ECB"); */
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 0, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 0, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 0, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 0, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 0, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 0, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");
    apr_pool_destroy(pool);

}

/**
 * Encrypt OpenSSL, decrypt NSS.
 */
static void test_crypto_block_openssl_nss(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_openssl_driver(tc, pool);
    drivers[1] = get_nss_driver(tc, pool);
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 0,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");

    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
    /* crypto_block_cross(tc, pool, drivers, KEY_3DES_192, MODE_ECB, 0, in, inlen, 24, "KEY_3DES_192/MODE_ECB"); */

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 0, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 0, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 0, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 0, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 0, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 0, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");
    apr_pool_destroy(pool);

}

/**
 * Sign NSS, verify OpenSSL.
 */
static void test_crypto_digest_nss_openssl(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_nss_driver(tc, pool);
    drivers[1] = get_openssl_driver(tc, pool);
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_CBC");
    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
/*    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_ECB");*/
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_CBC, 0, in, inlen, 32,
            "KEY_AES_256/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_ECB, 0, in, inlen, 32,
            "KEY_AES_256/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_AES_192/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_AES_192/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_CBC, 0, in, inlen, 16,
            "KEY_AES_128/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_ECB, 0, in, inlen, 16,
            "KEY_AES_128/MODE_ECB");

    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_MD5, in, inlen,
            "DIGEST MD5");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA1, in, inlen,
            "DIGEST SHA1");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA224, in, inlen,
            "DIGEST SHA224");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256, in, inlen,
            "DIGEST SHA256");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA384, in, inlen,
            "DIGEST SHA384");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA512, in, inlen,
            "DIGEST SHA512");

    apr_pool_destroy(pool);

}

/**
 * Sign OpenSSL, verify NSS.
 */
static void test_crypto_digest_openssl_nss(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_openssl_driver(tc, pool);
    drivers[1] = get_nss_driver(tc, pool);
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_CBC");
    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
/*    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_ECB");*/
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_CBC, 0, in, inlen, 32,
            "KEY_AES_256/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_ECB, 0, in, inlen, 32,
            "KEY_AES_256/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_AES_192/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_AES_192/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_CBC, 0, in, inlen, 16,
            "KEY_AES_128/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_ECB, 0, in, inlen, 16,
            "KEY_AES_128/MODE_ECB");

    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_MD5, in, inlen,
            "DIGEST MD5");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA1, in, inlen,
            "DIGEST SHA1");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA224, in, inlen,
            "DIGEST SHA224");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256, in, inlen,
            "DIGEST SHA256");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA384, in, inlen,
            "DIGEST SHA384");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA512, in, inlen,
            "DIGEST SHA512");

    apr_pool_destroy(pool);

}

/**
 * Encrypt OpenSSL, decrypt CommonCrypto.
 */
static void test_crypto_block_openssl_commoncrypto(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] =
    { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_openssl_driver(tc, pool);
    drivers[1] = get_commoncrypto_driver(tc, pool);

    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 0, in,
            inlen, 24, "KEY_3DES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_ECB, 0, in,
            inlen, 24, "KEY_3DES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 0, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 0, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 0, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 0, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 0, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 0, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");
    apr_pool_destroy(pool);

}

/**
 * Sign OpenSSL, verify CommonCrypto.
 */
static void test_crypto_digest_openssl_commoncrypto(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_openssl_driver(tc, pool);
    drivers[1] = get_commoncrypto_driver(tc, pool);
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_CBC");
    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
/*    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_ECB");*/
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_CBC, 0, in, inlen, 32,
            "KEY_AES_256/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_ECB, 0, in, inlen, 32,
            "KEY_AES_256/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_AES_192/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_AES_192/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_CBC, 0, in, inlen, 16,
            "KEY_AES_128/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_ECB, 0, in, inlen, 16,
            "KEY_AES_128/MODE_ECB");

    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_MD5, in, inlen,
            "DIGEST MD5");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA1, in, inlen,
            "DIGEST SHA1");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA224, in, inlen,
            "DIGEST SHA224");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256, in, inlen,
            "DIGEST SHA256");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA384, in, inlen,
            "DIGEST SHA384");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA512, in, inlen,
            "DIGEST SHA512");

    apr_pool_destroy(pool);

}

/**
 * Encrypt OpenSSL, decrypt CommonCrypto.
 */
static void test_crypto_block_commoncrypto_openssl(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] =
    { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_commoncrypto_driver(tc, pool);
    drivers[1] = get_openssl_driver(tc, pool);

    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 0, in,
            inlen, 24, "KEY_3DES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_ECB, 0, in,
            inlen, 24, "KEY_3DES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 0, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 0, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 0, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 0, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 0, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 0, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");
    apr_pool_destroy(pool);

}

/**
 * Sign OpenSSL, verify CommonCrypto.
 */
static void test_crypto_digest_commoncrypto_openssl(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) ALIGNED_STRING;
    apr_size_t inlen = sizeof(ALIGNED_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_commoncrypto_driver(tc, pool);
    drivers[1] = get_openssl_driver(tc, pool);
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_CBC");
    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
/*    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_3DES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_3DES_192/MODE_ECB");*/
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_CBC, 0, in, inlen, 32,
            "KEY_AES_256/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_256, APR_MODE_ECB, 0, in, inlen, 32,
            "KEY_AES_256/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_CBC, 0, in, inlen, 24,
            "KEY_AES_192/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_192, APR_MODE_ECB, 0, in, inlen, 24,
            "KEY_AES_192/MODE_ECB");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_CBC, 0, in, inlen, 16,
            "KEY_AES_128/MODE_CBC");
    crypto_cross_sign(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256,
            APR_KEY_AES_128, APR_MODE_ECB, 0, in, inlen, 16,
            "KEY_AES_128/MODE_ECB");

    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_MD5, in, inlen,
            "DIGEST MD5");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA1, in, inlen,
            "DIGEST SHA1");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA224, in, inlen,
            "DIGEST SHA224");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA256, in, inlen,
            "DIGEST SHA256");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA384, in, inlen,
            "DIGEST SHA384");
    crypto_cross_hash(tc, pool, drivers, APR_CRYPTO_DIGEST_SHA512, in, inlen,
            "DIGEST SHA512");

    apr_pool_destroy(pool);

}

/**
 * Simple test of OpenSSL block crypt.
 */
static void test_crypto_block_openssl_pad(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) TEST_STRING;
    apr_size_t inlen = sizeof(TEST_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_openssl_driver(tc, pool);
    drivers[1] = get_openssl_driver(tc, pool);

    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 1,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_ECB, 1,
            in, inlen, 24, "KEY_3DES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 1, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 1, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 1, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 1, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 1, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 1, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");

    apr_pool_destroy(pool);

}

/**
 * Simple test of NSS block crypt.
 */
static void test_crypto_block_nss_pad(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] =
    { NULL, NULL };

    const unsigned char *in = (const unsigned char *) TEST_STRING;
    apr_size_t inlen = sizeof(TEST_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_nss_driver(tc, pool);
    drivers[1] = get_nss_driver(tc, pool);

    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 1,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");
    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
    /* crypto_block_cross(tc, pool, drivers, KEY_3DES_192, MODE_ECB, 1, in, inlen, 24, "KEY_3DES_192/MODE_ECB"); */

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 1, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");

    /* KEY_AES_256 / MODE_ECB doesn't support padding on NSS */
    /*crypto_block_cross(tc, pool, drivers, KEY_AES_256, MODE_ECB, 1, in, inlen, 32, "KEY_AES_256/MODE_ECB");*/

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 1, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");

    /* KEY_AES_256 / MODE_ECB doesn't support padding on NSS */
    /*crypto_block_cross(tc, pool, drivers, KEY_AES_192, MODE_ECB, 1, in, inlen, 24, "KEY_AES_192/MODE_ECB");*/

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 1, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");

    /* KEY_AES_256 / MODE_ECB doesn't support padding on NSS */
    /*crypto_block_cross(tc, pool, drivers, KEY_AES_128, MODE_ECB, 1, in, inlen, 16, "KEY_AES_128/MODE_ECB");*/

    apr_pool_destroy(pool);

}

/**
 * Simple test of Common Crypto block crypt.
 */
static void test_crypto_block_commoncrypto_pad(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) TEST_STRING;
    apr_size_t inlen = sizeof(TEST_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_commoncrypto_driver(tc, pool);
    drivers[1] = get_commoncrypto_driver(tc, pool);

    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 1,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_ECB, 1,
            in, inlen, 24, "KEY_3DES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 1, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 1, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 1, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 1, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 1, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 1, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");

    apr_pool_destroy(pool);

}

/**
 * Encrypt NSS, decrypt OpenSSL.
 */
static void test_crypto_block_nss_openssl_pad(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) TEST_STRING;
    apr_size_t inlen = sizeof(TEST_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_nss_driver(tc, pool);
    drivers[1] = get_openssl_driver(tc, pool);

    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 1,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");

    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
    /* crypto_block_cross(tc, pool, drivers, KEY_3DES_192, MODE_ECB, 1, in, inlen, 24, "KEY_3DES_192/MODE_ECB"); */

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 1, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");

    /* KEY_AES_256 / MODE_ECB doesn't support padding on NSS */
    /*crypto_block_cross(tc, pool, drivers, KEY_AES_256, MODE_ECB, 1, in, inlen, 32, "KEY_AES_256/MODE_ECB");*/

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 1, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");

    /* KEY_AES_192 / MODE_ECB doesn't support padding on NSS */
    /*crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 1, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");*/

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 1, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");

    /* KEY_AES_192 / MODE_ECB doesn't support padding on NSS */
    /*crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 1, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");*/

    apr_pool_destroy(pool);

}

/**
 * Encrypt OpenSSL, decrypt NSS.
 */
static void test_crypto_block_openssl_nss_pad(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] = { NULL, NULL };

    const unsigned char *in = (const unsigned char *) TEST_STRING;
    apr_size_t inlen = sizeof(TEST_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_openssl_driver(tc, pool);
    drivers[1] = get_nss_driver(tc, pool);
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 1,
            in, inlen, 24, "KEY_3DES_192/MODE_CBC");

    /* KEY_3DES_192 / MODE_ECB doesn't work on NSS */
    /* crypto_block_cross(tc, pool, drivers, KEY_3DES_192, MODE_ECB, 1, in, inlen, 24, "KEY_3DES_192/MODE_ECB"); */

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 1, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");

    /* KEY_AES_256 / MODE_ECB doesn't support padding on NSS */
    /*crypto_block_cross(tc, pool, drivers, KEY_AES_256, MODE_ECB, 1, in, inlen, 32, "KEY_AES_256/MODE_ECB");*/

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 1, in, inlen,
            24, "KEY_AES_192/MODE_CBC");

    /* KEY_AES_192 / MODE_ECB doesn't support padding on NSS */
    /*crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 1, in, inlen,
            24, "KEY_AES_192/MODE_ECB");*/

    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 1, in, inlen,
            16, "KEY_AES_128/MODE_CBC");

    /* KEY_AES_128 / MODE_ECB doesn't support padding on NSS */
    /*crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 1, in, inlen,
            16, "KEY_AES_128/MODE_ECB");*/

    apr_pool_destroy(pool);

}

/**
 * Encrypt CommonCrypto, decrypt OpenSSL.
 */
static void test_crypto_block_commoncrypto_openssl_pad(abts_case *tc,
        void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] =
    { NULL, NULL };

    const unsigned char *in = (const unsigned char *) TEST_STRING;
    apr_size_t inlen = sizeof(TEST_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_commoncrypto_driver(tc, pool);
    drivers[1] = get_openssl_driver(tc, pool);

    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 1, in,
            inlen, 24, "KEY_3DES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_ECB, 1, in,
            inlen, 24, "KEY_3DES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 1, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 1, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 1, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 1, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 1, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 1, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");

    apr_pool_destroy(pool);

}

/**
 * Encrypt OpenSSL, decrypt CommonCrypto.
 */
static void test_crypto_block_openssl_commoncrypto_pad(abts_case *tc,
        void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *drivers[] =
    { NULL, NULL };

    const unsigned char *in = (const unsigned char *) TEST_STRING;
    apr_size_t inlen = sizeof(TEST_STRING);

    apr_pool_create(&pool, NULL);
    drivers[0] = get_openssl_driver(tc, pool);
    drivers[1] = get_commoncrypto_driver(tc, pool);

    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_CBC, 1, in,
            inlen, 24, "KEY_3DES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_3DES_192, APR_MODE_ECB, 1, in,
            inlen, 24, "KEY_3DES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_CBC, 1, in,
            inlen, 32, "KEY_AES_256/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_256, APR_MODE_ECB, 1, in,
            inlen, 32, "KEY_AES_256/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_CBC, 1, in,
            inlen, 24, "KEY_AES_192/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_192, APR_MODE_ECB, 1, in,
            inlen, 24, "KEY_AES_192/MODE_ECB");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_CBC, 1, in,
            inlen, 16, "KEY_AES_128/MODE_CBC");
    crypto_block_cross(tc, pool, drivers, APR_KEY_AES_128, APR_MODE_ECB, 1, in,
            inlen, 16, "KEY_AES_128/MODE_ECB");

    apr_pool_destroy(pool);

}

/**
 * Get Types, OpenSSL.
 */
static void test_crypto_get_block_key_types_openssl(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *driver;
    apr_crypto_t *f;
    apr_hash_t *types;
    int *key_3des_192;
    int *key_aes_128;
    int *key_aes_192;
    int *key_aes_256;

    apr_pool_create(&pool, NULL);
    driver = get_openssl_driver(tc, pool);
    if (driver) {

        f = make(tc, pool, driver);
        apr_crypto_get_block_key_types(&types, f);

        key_3des_192 = apr_hash_get(types, "3des192", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_3des_192);
        ABTS_INT_EQUAL(tc, *key_3des_192, APR_KEY_3DES_192);

        key_aes_128 = apr_hash_get(types, "aes128", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_aes_128);
        ABTS_INT_EQUAL(tc, *key_aes_128, APR_KEY_AES_128);

        key_aes_192 = apr_hash_get(types, "aes192", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_aes_192);
        ABTS_INT_EQUAL(tc, *key_aes_192, APR_KEY_AES_192);

        key_aes_256 = apr_hash_get(types, "aes256", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_aes_256);
        ABTS_INT_EQUAL(tc, *key_aes_256, APR_KEY_AES_256);

    }

    apr_pool_destroy(pool);

}

/**
 * Get Types, NSS.
 */
static void test_crypto_get_block_key_types_nss(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *driver;
    apr_crypto_t *f;
    apr_hash_t *types;
    int *key_3des_192;
    int *key_aes_128;
    int *key_aes_192;
    int *key_aes_256;

    apr_pool_create(&pool, NULL);
    driver = get_nss_driver(tc, pool);
    if (driver) {

        f = make(tc, pool, driver);
        apr_crypto_get_block_key_types(&types, f);

        key_3des_192 = apr_hash_get(types, "3des192", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_3des_192);
        ABTS_INT_EQUAL(tc, *key_3des_192, APR_KEY_3DES_192);

        key_aes_128 = apr_hash_get(types, "aes128", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_aes_128);
        ABTS_INT_EQUAL(tc, *key_aes_128, APR_KEY_AES_128);

        key_aes_192 = apr_hash_get(types, "aes192", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_aes_192);
        ABTS_INT_EQUAL(tc, *key_aes_192, APR_KEY_AES_192);

        key_aes_256 = apr_hash_get(types, "aes256", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_aes_256);
        ABTS_INT_EQUAL(tc, *key_aes_256, APR_KEY_AES_256);

    }

    apr_pool_destroy(pool);

}

/**
 * Get Types, Common Crypto.
 */
static void test_crypto_get_block_key_types_commoncrypto(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *driver;
    apr_crypto_t *f;
    apr_hash_t *types;
    int *key_3des_192;
    int *key_aes_128;
    int *key_aes_192;
    int *key_aes_256;

    apr_pool_create(&pool, NULL);
    driver = get_commoncrypto_driver(tc, pool);
    if (driver) {

        f = make(tc, pool, driver);
        apr_crypto_get_block_key_types(&types, f);

        key_3des_192 = apr_hash_get(types, "3des192", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_3des_192);
        ABTS_INT_EQUAL(tc, *key_3des_192, APR_KEY_3DES_192);

        key_aes_128 = apr_hash_get(types, "aes128", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_aes_128);
        ABTS_INT_EQUAL(tc, *key_aes_128, APR_KEY_AES_128);

        key_aes_192 = apr_hash_get(types, "aes192", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_aes_192);
        ABTS_INT_EQUAL(tc, *key_aes_192, APR_KEY_AES_192);

        key_aes_256 = apr_hash_get(types, "aes256", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, key_aes_256);
        ABTS_INT_EQUAL(tc, *key_aes_256, APR_KEY_AES_256);

    }

    apr_pool_destroy(pool);

}

/**
 * Get Modes, OpenSSL.
 */
static void test_crypto_get_block_key_modes_openssl(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *driver;
    apr_crypto_t *f;
    apr_hash_t *modes;
    int *mode_ecb;
    int *mode_cbc;

    apr_pool_create(&pool, NULL);
    driver = get_openssl_driver(tc, pool);
    if (driver) {

        f = make(tc, pool, driver);
        apr_crypto_get_block_key_modes(&modes, f);

        mode_ecb = apr_hash_get(modes, "ecb", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, mode_ecb);
        ABTS_INT_EQUAL(tc, *mode_ecb, APR_MODE_ECB);

        mode_cbc = apr_hash_get(modes, "cbc", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, mode_cbc);
        ABTS_INT_EQUAL(tc, *mode_cbc, APR_MODE_CBC);

    }

    apr_pool_destroy(pool);

}

/**
 * Get Modes, NSS.
 */
static void test_crypto_get_block_key_modes_nss(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *driver;
    apr_crypto_t *f;
    apr_hash_t *modes;
    int *mode_ecb;
    int *mode_cbc;

    apr_pool_create(&pool, NULL);
    driver = get_nss_driver(tc, pool);
    if (driver) {

        f = make(tc, pool, driver);
        apr_crypto_get_block_key_modes(&modes, f);

        mode_ecb = apr_hash_get(modes, "ecb", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, mode_ecb);
        ABTS_INT_EQUAL(tc, *mode_ecb, APR_MODE_ECB);

        mode_cbc = apr_hash_get(modes, "cbc", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, mode_cbc);
        ABTS_INT_EQUAL(tc, *mode_cbc, APR_MODE_CBC);

    }

    apr_pool_destroy(pool);

}

/**
 * Get Modes, Common Crypto.
 */
static void test_crypto_get_block_key_modes_commoncrypto(abts_case *tc, void *data)
{
    apr_pool_t *pool = NULL;
    const apr_crypto_driver_t *driver;
    apr_crypto_t *f;
    apr_hash_t *modes;
    int *mode_ecb;
    int *mode_cbc;

    apr_pool_create(&pool, NULL);
    driver = get_commoncrypto_driver(tc, pool);
    if (driver) {

        f = make(tc, pool, driver);
        apr_crypto_get_block_key_modes(&modes, f);

        mode_ecb = apr_hash_get(modes, "ecb", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, mode_ecb);
        ABTS_INT_EQUAL(tc, *mode_ecb, APR_MODE_ECB);

        mode_cbc = apr_hash_get(modes, "cbc", APR_HASH_KEY_STRING);
        ABTS_PTR_NOTNULL(tc, mode_cbc);
        ABTS_INT_EQUAL(tc, *mode_cbc, APR_MODE_CBC);

    }

    apr_pool_destroy(pool);

}

static void test_crypto_memzero(abts_case *tc, void *data)
{
    /* Aligned message */
    struct {
        char buf[7 * sizeof(int)];
        int untouched;
    } msg;
    /* A bit of type punning such that 'msg' might look unused
     * after the call to apr_crypto_memzero().
     */
    int *ptr = (int *)&msg;
    int i;

    /* Fill buf with non-zeros (odds) */
    for (i = 1; i < 2 * sizeof(msg.buf); i += 2) {
        msg.buf[i / 2] = (char)i;
        ABTS_ASSERT(tc, "test_crypto_memzero() barrier", msg.buf[i / 2] != 0);
    }

    /* Zero out the whole, and check it */
    apr_crypto_memzero(&msg, sizeof msg);
    for (i = 0; i < sizeof(msg) / sizeof(*ptr); ++i) {
        ABTS_ASSERT(tc, "test_crypto_memzero() optimized out", ptr[i] == 0);
    }
}

static void test_crypto_equals(abts_case *tc, void *data)
{
    /* Buffers of each type of scalar */
    union {
        char c;
        short s;
        int i;
        long l;
        float f;
        double d;
        void *p;
    } buf0[7], buf1[7], buf[7];
    char *ptr = (char *)buf;
    int i;

#define TEST_SCALAR_MATCH(i, x, r) \
    ABTS_ASSERT(tc, "test_crypto_equals(" APR_STRINGIFY(x) ")" \
                                   " != " APR_STRINGIFY(r), \
                apr_crypto_equals(&buf##r[i].x, &buf[i].x, \
                                  sizeof(buf[i].x)) == r)

    /* Fill buf with non-zeros (odds) */
    for (i = 1; i < 2 * sizeof(buf); i += 2) {
        ptr[i / 2] = (char)i;
    }
    /* Set buf1 = buf */
    memcpy(buf1, buf, sizeof buf);
    /* Set buf0 = {0} */
    memset(buf0, 0, sizeof buf0);

    /* Check that buf1 == buf for each scalar */
    TEST_SCALAR_MATCH(0, c, 1);
    TEST_SCALAR_MATCH(1, s, 1);
    TEST_SCALAR_MATCH(2, i, 1);
    TEST_SCALAR_MATCH(3, l, 1);
    TEST_SCALAR_MATCH(4, f, 1);
    TEST_SCALAR_MATCH(5, d, 1);
    TEST_SCALAR_MATCH(6, p, 1);

    /* Check that buf0 != buf for each scalar */
    TEST_SCALAR_MATCH(0, c, 0);
    TEST_SCALAR_MATCH(1, s, 0);
    TEST_SCALAR_MATCH(2, i, 0);
    TEST_SCALAR_MATCH(3, l, 0);
    TEST_SCALAR_MATCH(4, f, 0);
    TEST_SCALAR_MATCH(5, d, 0);
    TEST_SCALAR_MATCH(6, p, 0);
}

#if APU_HAVE_CRYPTO_PRNG
/*
 * KAT for CHACHA20:
 * # iv=$(printf "%.32d" 0)
 * # key=$(printf "%.64d" 0)
 * # key=$(openssl enc -chacha20 -e \
 *                     -in /dev/zero -K $key -iv $iv \
 *         | xxd -l32 -c64 -p)
 * # openssl enc -chacha20 -e \
 *               -in /dev/zero -K $key -iv $iv \
 *   | xxd -l128 -c8 -i
 */
static const unsigned char test_PRNG_kat0_chacha20[128] = {
  0xb0, 0xfd, 0x14, 0xff, 0x96, 0xa0, 0xbd, 0xa1,
  0x54, 0xc3, 0x29, 0x08, 0x2c, 0x9c, 0x65, 0x33,
  0xbb, 0x4c, 0x94, 0x73, 0xbf, 0x5d, 0xde, 0x13,
  0x8f, 0x82, 0xc9, 0xac, 0x55, 0x53, 0xd9, 0x58,
  0xaf, 0xbd, 0xad, 0x28, 0x45, 0xb9, 0x3c, 0xdb,
  0xb2, 0xfe, 0x64, 0x63, 0xd2, 0xfe, 0x16, 0x2a,
  0xda, 0xe0, 0xf6, 0xe6, 0x76, 0xf0, 0x49, 0x42,
  0x18, 0xf5, 0xce, 0x05, 0x96, 0xe7, 0x9f, 0x5c,
  0x55, 0x1a, 0xaa, 0x9b, 0xa4, 0x6f, 0xaa, 0xd5,
  0x28, 0xf6, 0x76, 0x3d, 0xde, 0x93, 0xc0, 0x3f,
  0xa3, 0xb1, 0x21, 0xb2, 0xff, 0xc0, 0x53, 0x3a,
  0x69, 0x5e, 0xd5, 0x6e, 0x8f, 0xda, 0x05, 0x89,
  0xa2, 0xed, 0xeb, 0xfa, 0xd4, 0xae, 0xd3, 0x35,
  0x7c, 0x7a, 0xad, 0xad, 0x93, 0x28, 0x02, 0x7b,
  0xb8, 0x79, 0xb5, 0x57, 0x47, 0x97, 0xa1, 0xb7,
  0x3d, 0xce, 0x7c, 0xd0, 0x9f, 0x24, 0x51, 0x01
};

/*
 * KAT for AES256-CTR:
 * # iv=$(printf "%.32d" 0)
 * # key=$(printf "%.64d" 0)
 * # key=$(openssl enc -aes-256-ctr -e \
 *                     -in /dev/zero -K $key -iv $iv \
 *         | xxd -l32 -c64 -p)
 * # openssl enc -aes-256-ctr -e \
 *               -in /dev/zero -K $key -iv $iv \
 *   | xxd -l128 -c8 -i
 */
static const unsigned char test_PRNG_kat0_aes256[128] = {
  0x79, 0x04, 0x2a, 0x33, 0xfa, 0x41, 0x1a, 0x37,
  0x97, 0x3a, 0xec, 0xa0, 0xfc, 0xde, 0x6b, 0x2b,
  0x16, 0xa4, 0x5f, 0xa1, 0x2a, 0xe3, 0xf5, 0x4c,
  0x84, 0x28, 0x83, 0xeb, 0x60, 0xce, 0x44, 0xe9,
  0x9c, 0x4c, 0xa2, 0x6e, 0x70, 0xcc, 0x26, 0x68,
  0xf8, 0x99, 0x5a, 0xa1, 0x9f, 0xde, 0x99, 0xb9,
  0x80, 0x0b, 0xb6, 0x83, 0x14, 0x9d, 0x72, 0x93,
  0xf4, 0xd1, 0x49, 0xf3, 0xf0, 0x9e, 0x49, 0x80,
  0x76, 0x84, 0x01, 0x1e, 0x79, 0x9e, 0x70, 0x70,
  0x61, 0x7c, 0x13, 0xce, 0x2d, 0x64, 0xca, 0x08,
  0xb7, 0xc1, 0xd5, 0x61, 0xf1, 0x95, 0x5d, 0x1b,
  0x92, 0x8c, 0xd2, 0x70, 0xef, 0x26, 0xfe, 0x24,
  0x01, 0xd8, 0x65, 0x63, 0x68, 0x71, 0x09, 0x4e,
  0x7b, 0x01, 0x36, 0x19, 0x85, 0x13, 0x16, 0xfd,
  0xc5, 0x0c, 0xe6, 0x71, 0x42, 0xbf, 0x81, 0xb0,
  0xd1, 0x59, 0x28, 0xa1, 0x04, 0xe9, 0x8d, 0xad
};

static void test_crypto_prng(abts_case *tc, apr_crypto_cipher_e cipher, const unsigned char *test_PRNG_kat0)
{
    unsigned char randbytes[128], seed[APR_CRYPTO_PRNG_SEED_SIZE];
    apr_crypto_prng_t *cprng = NULL;
    apr_pool_t *pool = NULL;
    apr_status_t rv;
    int i;
    int flags = 0;

    rv = apr_pool_create(&pool, NULL);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, pool);

#if APR_HAS_THREADS
    flags = APR_CRYPTO_PRNG_PER_THREAD;
#endif
    rv = apr_crypto_prng_init(pool, NULL, cipher, 0, NULL, flags);

    if (APR_ENOCIPHER == rv) {
        apr_pool_destroy(pool);
        return;
    }

    ABTS_ASSERT(tc, "apr_crypto_prng_init returned APR_EREINIT", rv != APR_EREINIT);
    ABTS_ASSERT(tc, "apr_crypto_prng_init returned APR_ENOTIMPL", rv != APR_ENOTIMPL);
    ABTS_ASSERT(tc, "apr_crypto_prng_init returned APR_ENOCIPHER", rv != APR_ENOCIPHER);
    ABTS_ASSERT(tc, "apr_crypto_prng_init failed", rv == APR_SUCCESS);

    for (i = 0; i < 10; ++i) {
        /* Initial seed full of zeros (deterministic) */
        memset(seed, 0, sizeof(seed));

        rv = apr_crypto_prng_create(&cprng, NULL, cipher, 0, 0, seed, pool);
        ABTS_ASSERT(tc, "apr_crypto_prng_create returned APR_EINIT", rv != APR_EINIT);
        ABTS_ASSERT(tc, "apr_crypto_prng_create returned APR_EINVAL", rv != APR_EINVAL);
        ABTS_ASSERT(tc, "apr_crypto_prng_create returned APR_ENOTIMPL", rv != APR_ENOTIMPL);
        ABTS_ASSERT(tc, "apr_crypto_prng_create returned APR_ENOCIPHER", rv != APR_ENOCIPHER);
        ABTS_ASSERT(tc, "apr_crypto_prng_create returned APR_EDSOOPEN", rv != APR_EDSOOPEN);
        ABTS_ASSERT(tc, "apr_crypto_prng_create failed", rv == APR_SUCCESS);
        if (!cprng) {
            break;
        }

        /* Second time and more, change one bit of the seed */
        if (i != 0) {
            unsigned char pos = 0;
            rv = apr_generate_random_bytes(&pos, sizeof pos);
            ABTS_ASSERT(tc, "apr_generate_random_bytes failed",
                        rv == APR_SUCCESS);

            seed[pos % APR_CRYPTO_PRNG_SEED_SIZE] = 1;
            rv = apr_crypto_prng_reseed(cprng, seed);
            ABTS_ASSERT(tc, "apr_crypto_prng_reseed failed",
                        rv == APR_SUCCESS);
        }

        rv = apr_crypto_prng_bytes(cprng, randbytes, 128 - 32);
        ABTS_ASSERT(tc, "apr_crypto_prng_bytes failed", rv == APR_SUCCESS);

        /* Should match the first time only */
        if (i != 0) {
            ABTS_ASSERT(tc, "test vector should not match",
                        /* first 32 bytes (256 bits) are used for the next key */
                        memcmp(randbytes, test_PRNG_kat0 + 32, 128 - 32) != 0);
        }
        else {
            ABTS_ASSERT(tc, "test vector should match",
                        /* first 32 bytes (256 bits) are used for the next key */
                        memcmp(randbytes, test_PRNG_kat0 + 32, 128 - 32) == 0);
        }

        rv = apr_crypto_prng_destroy(cprng);
        ABTS_ASSERT(tc, "apr_crypto_prng_destroy failed", rv == APR_SUCCESS);
    }

    apr_pool_destroy(pool);
}

static void test_crypto_prng_aes256(abts_case *tc, void *data)
{
    return test_crypto_prng(tc, APR_CRYPTO_CIPHER_AES_256_CTR, test_PRNG_kat0_aes256);
}

static void test_crypto_prng_chacha20(abts_case *tc, void *data)
{
    return test_crypto_prng(tc, APR_CRYPTO_CIPHER_CHACHA20_CTR, test_PRNG_kat0_chacha20);
}

#if APR_HAS_FORK
static void test_crypto_fork_random(abts_case *tc, void *data)
{
    unsigned char randbytes[1024];
    apr_pool_t *pool = NULL;
    apr_file_t *pread = NULL;
    apr_file_t *pwrite = NULL;
    apr_size_t nbytes;
    apr_proc_t proc;
    apr_status_t rv;
    int flags = 0;

    rv = apr_pool_create(&pool, NULL);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, pool);

#if APR_HAS_THREADS
    flags = APR_CRYPTO_PRNG_PER_THREAD;
#endif
    rv = apr_crypto_prng_init(pool, NULL, APR_CRYPTO_CIPHER_AUTO, 0, NULL, flags);
    ABTS_ASSERT(tc, "apr_crypto_prng_init returned APR_EREINIT", rv != APR_EREINIT);
    ABTS_ASSERT(tc, "apr_crypto_prng_init returned APR_ENOTIMPL", rv != APR_ENOTIMPL);
    ABTS_ASSERT(tc, "apr_crypto_prng_init failed", rv == APR_SUCCESS);

    rv = apr_file_pipe_create(&pread, &pwrite, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, pread);
    ABTS_PTR_NOTNULL(tc, pwrite);

    rv = apr_proc_fork(&proc, pool);
    if (rv == APR_INCHILD) {
        apr_file_close(pread);
        rv = apr_crypto_random_bytes(randbytes, 1024);
        if (rv == APR_SUCCESS) {
            apr_file_write_full(pwrite, randbytes, 1024, &nbytes);
        }
        apr_file_close(pwrite);

        exit(rv != APR_SUCCESS);
    }
    else if (rv == APR_INPARENT) {
        int exitcode;
        apr_exit_why_e why;
        unsigned char childbytes[1024];

        apr_file_close(pwrite);
        rv = apr_file_read_full(pread, childbytes, 1024, &nbytes);
        ABTS_INT_EQUAL(tc, (int)nbytes, 1024);
        apr_file_close(pread);

        apr_proc_wait(&proc, &exitcode, &why, APR_WAIT);
        if (why != APR_PROC_EXIT) {
            ABTS_ASSERT(tc, "apr_proc_wait returned APR_PROC_SIGNAL", why != APR_PROC_SIGNAL);
            ABTS_ASSERT(tc, "apr_proc_wait returned APR_PROC_SIGNAL_CORE", why != (APR_PROC_SIGNAL | APR_PROC_SIGNAL_CORE));
            ABTS_FAIL(tc, "child terminated abnormally");
        }
        else if (exitcode != 0) {
            ABTS_FAIL(tc, "apr_crypto_random_bytes failed in child");
        }

        rv = apr_crypto_random_bytes(randbytes, 1024);
        ABTS_ASSERT(tc, "apr_crypto_random_bytes failed in parent",
                    rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "parent and child generated same random bytes",
                    memcmp(randbytes, childbytes, 1024) != 0);
    }
    else {
        ABTS_FAIL(tc, "apr_proc_fork failed");
    }

    apr_pool_destroy(pool);
}
#endif

#if APR_HAS_THREADS
#define NUM_THREADS 8

static void *APR_THREAD_FUNC thread_func(apr_thread_t *thd, void *data)
{
    unsigned char *randbytes = data;
    apr_status_t rv;

    rv = apr_crypto_random_thread_bytes(randbytes, 800);
    apr_thread_exit(thd, rv);

    return NULL;
}

static void test_crypto_thread_random(abts_case *tc, void *data)
{
    static unsigned char zerobytes[800];
    unsigned char *randbytes[NUM_THREADS];
    apr_thread_t *threads[NUM_THREADS];
    apr_pool_t *pool = NULL;
    apr_status_t rv, ret;
    int i, j;
    int flags = 0;

    rv = apr_pool_create(&pool, NULL);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
    ABTS_PTR_NOTNULL(tc, pool);

#if APR_HAS_THREADS
    flags = APR_CRYPTO_PRNG_PER_THREAD;
#endif
    rv = apr_crypto_prng_init(pool, NULL, APR_CRYPTO_CIPHER_AUTO, 0, NULL, flags);
    ABTS_ASSERT(tc, "apr_crypto_prng_init returned APR_EREINIT", rv != APR_EREINIT);
    ABTS_ASSERT(tc, "apr_crypto_prng_init returned APR_ENOTIMPL", rv != APR_ENOTIMPL);
    ABTS_ASSERT(tc, "apr_crypto_prng_init failed", rv == APR_SUCCESS);

    for (i = 0; i < NUM_THREADS; ++i) {
        randbytes[i] = apr_pcalloc(pool, 800);
        rv = apr_thread_create(&threads[i], NULL, thread_func,
                               randbytes[i], pool);
        ABTS_ASSERT(tc, "apr_thread_create failed", rv == APR_SUCCESS);
    }
    for (i = 0; i < NUM_THREADS; ++i) {
        rv = apr_thread_join(&ret, threads[i]);
        ABTS_ASSERT(tc, "apr_thread_join failed", rv == APR_SUCCESS);
        ABTS_ASSERT(tc, "apr_crypto_random_thread_bytes failed",
                    ret == APR_SUCCESS);
    }
    for (i = 0; i < NUM_THREADS; ++i) {
        ABTS_ASSERT(tc, "some thread generated zero bytes",
                    memcmp(randbytes[i], zerobytes, 800) != 0);
        for (j = 0; j < i; ++j) {
            ABTS_ASSERT(tc, "two threads generated same random bytes",
                        memcmp(randbytes[i], randbytes[j], 800) != 0);
        }
    }

    apr_pool_destroy(pool);
}
#endif
#endif

abts_suite *testcrypto(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    /* test simple init and shutdown */
    abts_run_test(suite, test_crypto_init, NULL);

    /* test key parsing - openssl */
    abts_run_test(suite, test_crypto_key_openssl, NULL);

    /* test key parsing - nss */
    abts_run_test(suite, test_crypto_key_nss, NULL);

    /* test key parsing - commoncrypto */
    abts_run_test(suite, test_crypto_key_commoncrypto, NULL);

    /* test a simple encrypt / decrypt operation - openssl */
    abts_run_test(suite, test_crypto_block_openssl, NULL);

    /* test a simple sign / verify operation - openssl */
    abts_run_test(suite, test_crypto_digest_openssl, NULL);

    /* test a padded encrypt / decrypt operation - openssl */
    abts_run_test(suite, test_crypto_block_openssl_pad, NULL);

    /* test a simple encrypt / decrypt operation - nss */
    abts_run_test(suite, test_crypto_block_nss, NULL);

    /* test a simple sign / verify operation - nss */
    abts_run_test(suite, test_crypto_digest_nss, NULL);

    /* test a padded encrypt / decrypt operation - nss */
    abts_run_test(suite, test_crypto_block_nss_pad, NULL);

    /* test a simple encrypt / decrypt operation - commoncrypto */
    abts_run_test(suite, test_crypto_block_commoncrypto, NULL);

    /* test a simple sign / verify operation - commoncrypto */
    abts_run_test(suite, test_crypto_digest_commoncrypto, NULL);

    /* test a padded encrypt / decrypt operation - commoncrypto */
    abts_run_test(suite, test_crypto_block_commoncrypto_pad, NULL);


    /* test encrypt nss / decrypt openssl */
    abts_run_test(suite, test_crypto_block_nss_openssl, NULL);

    /* test padded encrypt nss / decrypt openssl */
    abts_run_test(suite, test_crypto_block_nss_openssl_pad, NULL);

    /* test sign nss / verify openssl */
    abts_run_test(suite, test_crypto_digest_nss_openssl, NULL);


    /* test encrypt openssl / decrypt nss */
    abts_run_test(suite, test_crypto_block_openssl_nss, NULL);

    /* test padded encrypt openssl / decrypt nss */
    abts_run_test(suite, test_crypto_block_openssl_nss_pad, NULL);

    /* test sign openssl / verify nss */
    abts_run_test(suite, test_crypto_digest_openssl_nss, NULL);


    /* test encrypt openssl / decrypt commoncrypto */
    abts_run_test(suite, test_crypto_block_openssl_commoncrypto, NULL);

    /* test padded encrypt openssl / decrypt commoncrypto */
    abts_run_test(suite, test_crypto_block_openssl_commoncrypto_pad, NULL);

    /* test sign openssl / verify commoncrypto */
    abts_run_test(suite, test_crypto_digest_openssl_commoncrypto, NULL);


    /* test encrypt commoncrypto / decrypt openssl */
    abts_run_test(suite, test_crypto_block_commoncrypto_openssl, NULL);

    /* test padded encrypt commoncrypto / decrypt openssl */
    abts_run_test(suite, test_crypto_block_commoncrypto_openssl_pad, NULL);

    /* test sign commoncrypto / verify openssl */
    abts_run_test(suite, test_crypto_digest_commoncrypto_openssl, NULL);


    /* test block key types openssl */
    abts_run_test(suite, test_crypto_get_block_key_types_openssl, NULL);

    /* test block key types nss */
    abts_run_test(suite, test_crypto_get_block_key_types_nss, NULL);

    /* test block key types commoncrypto */
    abts_run_test(suite, test_crypto_get_block_key_types_commoncrypto, NULL);

    /* test block key modes openssl */
    abts_run_test(suite, test_crypto_get_block_key_modes_openssl, NULL);

    /* test block key modes nss */
    abts_run_test(suite, test_crypto_get_block_key_modes_nss, NULL);

    /* test block key modes commoncrypto */
    abts_run_test(suite, test_crypto_get_block_key_modes_commoncrypto, NULL);

    abts_run_test(suite, test_crypto_memzero, NULL);
    abts_run_test(suite, test_crypto_equals, NULL);

#if APU_HAVE_CRYPTO_PRNG
    abts_run_test(suite, test_crypto_prng_aes256, NULL);
    abts_run_test(suite, test_crypto_prng_chacha20, NULL);
#if APR_HAS_FORK
    abts_run_test(suite, test_crypto_fork_random, NULL);
#endif
#if APR_HAS_THREADS
    abts_run_test(suite, test_crypto_thread_random, NULL);
#endif
#endif

    return suite;
}

#else

/**
 * Dummy test suite when crypto is turned off.
 */
abts_suite *testcrypto(abts_suite *suite)
{
    return ADD_SUITE(suite);
}

#endif /* APU_HAVE_CRYPTO */
