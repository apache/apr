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

#include "apr_lib.h"
#include "apu.h"
#include "apr_private.h"
#include "apu_errno.h"

#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

#include "apr_strings.h"
#include "apr_time.h"
#include "apr_buckets.h"
#include "apr_thread_mutex.h"

#include "apr_crypto_internal.h"

#if APU_HAVE_CRYPTO

#ifndef OPENSSL_API_COMPAT
#define OPENSSL_API_COMPAT 0x10101000L /* for ENGINE API */
#endif

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/engine.h>
#include <openssl/crypto.h>
#include <openssl/obj_mac.h> /* for NID_* */
#include <openssl/conf.h>
#include <openssl/comp.h>
#include <openssl/ssl.h>
#include <openssl/opensslv.h>
#if OPENSSL_VERSION_NUMBER >= 0x30000000
#include <openssl/macros.h>
#include <openssl/core_names.h>
#endif

#if defined(LIBRESSL_VERSION_NUMBER)

/* LibreSSL declares OPENSSL_VERSION_NUMBER == 2.0 but does not necessarily
 * include changes from OpenSSL >= 1.1 (new functions, macros, * deprecations,
 * ...), so we have to work around this...
 */
#define APR_USE_OPENSSL_PRE_1_0_API     0
#if LIBRESSL_VERSION_NUMBER < 0x2070000f
#define APR_USE_OPENSSL_PRE_1_1_API     1
#else
#define APR_USE_OPENSSL_PRE_1_1_API     0
#endif
/* TODO: keep up with LibreSSL latest versions */
#define APR_USE_OPENSSL_PRE_1_1_1_API   1
#define APR_USE_OPENSSL_PRE_3_0_API     1

#else  /* defined(LIBRESSL_VERSION_NUMBER) */

#if OPENSSL_VERSION_NUMBER < 0x10000000L
#define APR_USE_OPENSSL_PRE_1_0_API     1
#else
#define APR_USE_OPENSSL_PRE_1_0_API     0
#endif
#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define APR_USE_OPENSSL_PRE_1_1_API     1
#else
#define APR_USE_OPENSSL_PRE_1_1_API     0
#endif
#if OPENSSL_VERSION_NUMBER < 0x10101000L
#define APR_USE_OPENSSL_PRE_1_1_1_API   1
#else
#define APR_USE_OPENSSL_PRE_1_1_1_API   0
#endif
#if OPENSSL_VERSION_NUMBER < 0x30000000L
#define APR_USE_OPENSSL_PRE_3_0_API     1
#else
#define APR_USE_OPENSSL_PRE_3_0_API     0
#endif

#endif /* defined(LIBRESSL_VERSION_NUMBER) */

#if APR_USE_OPENSSL_PRE_3_0_API \
    || (defined(OPENSSL_API_LEVEL) && OPENSSL_API_LEVEL < 30000)
#define APR_USE_OPENSSL_ENGINE_API 1
#else
#define APR_USE_OPENSSL_ENGINE_API 0
#endif

#define LOG_PREFIX "apr_crypto_openssl: "

struct apr_crypto_t {
    apr_pool_t *pool;
    const apr_crypto_driver_t *provider;
    apu_err_t *result;
    apr_crypto_config_t *config;
    apr_hash_t *types;
    apr_hash_t *modes;
    apr_hash_t *digests;
};

struct apr_crypto_config_t {
#if APR_USE_OPENSSL_ENGINE_API
    ENGINE *engine;
#else
    void *engine;
#endif
};

struct apr_crypto_key_t {
    apr_pool_t *pool;
    const apr_crypto_driver_t *provider;
    const apr_crypto_t *f;
    const apr_crypto_key_rec_t *rec;
    const EVP_CIPHER *cipher;
    const EVP_MD *md;
    EVP_PKEY *pkey;
#if !APR_USE_OPENSSL_PRE_3_0_API
    EVP_MAC *mac;
#endif
    unsigned char *key;
    int keyLen;
    int doPad;
    int ivSize;
};

struct apr_crypto_block_t {
    apr_pool_t *pool;
    const apr_crypto_driver_t *provider;
    const apr_crypto_t *f;
    const apr_crypto_key_t *key;
    EVP_CIPHER_CTX *cipherCtx;
    int ivSize;
    int blockSize;
    int doPad;
};

struct apr_crypto_digest_t {
    apr_pool_t *pool;
    const apr_crypto_driver_t *provider;
    const apr_crypto_t *f;
    const apr_crypto_key_t *key;
    apr_crypto_digest_rec_t *rec;
    EVP_MD_CTX *mdCtx;
#if !APR_USE_OPENSSL_PRE_3_0_API
    EVP_MAC_CTX *macCtx;
#endif
    int digestSize;
};

struct cprng_stream_ctx_t {
    EVP_CIPHER_CTX *ctx;
    int malloced;
};

static struct apr_crypto_block_key_digest_t key_digests[] =
{
{ APR_CRYPTO_DIGEST_MD5, 16, 64 },
{ APR_CRYPTO_DIGEST_SHA1, 20, 64 },
{ APR_CRYPTO_DIGEST_SHA224, 28, 64 },
{ APR_CRYPTO_DIGEST_SHA256, 32, 64 },
{ APR_CRYPTO_DIGEST_SHA384, 48, 128 },
{ APR_CRYPTO_DIGEST_SHA512, 64, 128 } };

static struct apr_crypto_block_key_type_t key_types[] =
{
{ APR_KEY_3DES_192, 24, 8, 8 },
{ APR_KEY_AES_128, 16, 16, 16 },
{ APR_KEY_AES_192, 24, 16, 16 },
{ APR_KEY_AES_256, 32, 16, 16 } };

static struct apr_crypto_block_key_mode_t key_modes[] =
{
{ APR_MODE_ECB },
{ APR_MODE_CBC } };

/* sufficient space to wrap a key */
#define BUFFER_SIZE 128

/**
 * Fetch the most recent error from this driver.
 */
static apr_status_t crypto_error(const apu_err_t **result,
        const apr_crypto_t *f)
{
    *result = f->result;
    return APR_SUCCESS;
}

/**
 * Shutdown the crypto library and release resources.
 */
static apr_status_t crypto_shutdown(void)
{
#if HAVE_OPENSSL_INIT_CRYPTO
    /* Openssl v1.1+ handles all termination automatically. Do
     * nothing in this case.
     */

#else
    /* Termination below is for legacy Openssl versions v1.0.x and
     * older.
     */

    ERR_free_strings();
    EVP_cleanup();
    ENGINE_cleanup();
#endif

    return APR_SUCCESS;
}

static apr_status_t crypto_shutdown_helper(void *data)
{
    return crypto_shutdown();
}

/**
 * Initialise the crypto library and perform one time initialisation.
 */
static apr_status_t crypto_init(apr_pool_t *pool, const char *params,
        const apu_err_t **result)
{
#if HAVE_OPENSSL_INIT_CRYPTO
    /* Openssl v1.1+ handles all initialisation automatically, apart
     * from hints as to how we want to use the library.
     *
     * We tell openssl we want to include engine support.
     */
    OPENSSL_init_crypto(OPENSSL_INIT_ENGINE_ALL_BUILTIN, NULL);

#else
    /* Configuration below is for legacy versions Openssl v1.0 and
     * older.
     */

#if APR_USE_OPENSSL_PRE_1_1_API
    (void)CRYPTO_malloc_init();
#else
    OPENSSL_malloc_init();
#endif
    ERR_load_crypto_strings();
    /* SSL_load_error_strings(); */
    OpenSSL_add_all_algorithms();
    ENGINE_load_builtin_engines();
    ENGINE_register_all_complete();
#endif

    apr_pool_cleanup_register(pool, pool, crypto_shutdown_helper,
            apr_pool_cleanup_null);

    return APR_SUCCESS;
}

#if OPENSSL_VERSION_NUMBER < 0x0090802fL

/* Code taken from OpenSSL 0.9.8b, see
 * https://github.com/openssl/openssl/commit/cf6bc84148cb15af09b292394aaf2b45f0d5af0d
 */

EVP_CIPHER_CTX *EVP_CIPHER_CTX_new(void)
{
     EVP_CIPHER_CTX *ctx = OPENSSL_malloc(sizeof *ctx);
     if (ctx)
         EVP_CIPHER_CTX_init(ctx);
     return ctx;
}

void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *ctx)
{
    if (ctx) {
        EVP_CIPHER_CTX_cleanup(ctx);
        OPENSSL_free(ctx);
    }
}

#endif

#if APR_USE_OPENSSL_PRE_1_1_API
#define EVP_MD_CTX_new EVP_MD_CTX_create
#define EVP_MD_CTX_free EVP_MD_CTX_destroy
#endif

/**
 * @brief Clean key.
 * @param key The key to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
static apr_status_t crypto_key_cleanup(apr_crypto_key_t *key)
{
    if (key->pkey) {
        EVP_PKEY_free(key->pkey);
        key->pkey = NULL;
    }
#if !APR_USE_OPENSSL_PRE_3_0_API
    if (key->mac) {
        EVP_MAC_free(key->mac);
        key->mac = NULL;
    }
#endif

    return APR_SUCCESS;
}

static apr_status_t crypto_key_cleanup_helper(void *data)
{
    apr_crypto_key_t *key = (apr_crypto_key_t *) data;
    return crypto_key_cleanup(key);
}

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param ctx The block context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
static apr_status_t crypto_block_cleanup(apr_crypto_block_t *ctx)
{

    if (ctx->cipherCtx) {
#if APR_USE_OPENSSL_PRE_1_1_API
        EVP_CIPHER_CTX_cleanup(ctx->cipherCtx);
#else
        EVP_CIPHER_CTX_reset(ctx->cipherCtx);
        EVP_CIPHER_CTX_free(ctx->cipherCtx);
#endif
        ctx->cipherCtx = NULL;
    }

    return APR_SUCCESS;

}

static apr_status_t crypto_block_cleanup_helper(void *data)
{
    apr_crypto_block_t *block = (apr_crypto_block_t *) data;
    return crypto_block_cleanup(block);
}

/**
 * @brief Clean sign / verify context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param ctx The block context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
static apr_status_t crypto_digest_cleanup(apr_crypto_digest_t *ctx)
{
    if (ctx->mdCtx) {
        EVP_MD_CTX_free(ctx->mdCtx);
        ctx->mdCtx = NULL;
    }
#if !APR_USE_OPENSSL_PRE_3_0_API
    if (ctx->macCtx) {
        EVP_MAC_CTX_free(ctx->macCtx);
        ctx->macCtx = NULL;
    }
#endif

    return APR_SUCCESS;

}

static apr_status_t crypto_digest_cleanup_helper(void *data)
{
    apr_crypto_digest_t *digest = (apr_crypto_digest_t *) data;
    return crypto_digest_cleanup(digest);
}

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param f The context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
static apr_status_t crypto_cleanup(apr_crypto_t *f)
{
#if APR_USE_OPENSSL_ENGINE_API
    if (f->config->engine) {
        ENGINE_finish(f->config->engine);
        ENGINE_free(f->config->engine);
        f->config->engine = NULL;
    }
#endif
    return APR_SUCCESS;

}

static apr_status_t crypto_cleanup_helper(void *data)
{
    apr_crypto_t *f = (apr_crypto_t *) data;
    return crypto_cleanup(f);
}

/**
 * @brief Create a context for supporting encryption. Keys, certificates,
 *        algorithms and other parameters will be set per context. More than
 *        one context can be created at one time. A cleanup will be automatically
 *        registered with the given pool to guarantee a graceful shutdown.
 * @param f - context pointer will be written here
 * @param provider - provider to use
 * @param params - array of key parameters
 * @param pool - process pool
 * @return APR_ENOENGINE when the engine specified does not exist. APR_EINITENGINE
 * if the engine cannot be initialised.
 */
static apr_status_t crypto_make(apr_crypto_t **ff,
        const apr_crypto_driver_t *provider, const char *params,
        apr_pool_t *pool)
{
    apr_crypto_t *f;
    apr_crypto_config_t *config;
    const char *engine = NULL;
    apr_status_t status = APR_SUCCESS;
    struct {
        const char *field;
        const char *value;
        int set;
    } fields[] = {
        { "engine", NULL, 0 },
        { NULL, NULL, 0 }
    };
    const char *ptr;
    size_t klen;
    char **elts = NULL;
    char *elt;
    int i = 0, j;

    *ff = NULL;

    if (params) {
        if (APR_SUCCESS != (status = apr_tokenize_to_argv(params, &elts, pool))) {
            return status;
        }
        while ((elt = elts[i])) {
            ptr = strchr(elt, '=');
            if (ptr) {
                for (klen = ptr - elt; klen && apr_isspace(elt[klen - 1]); --klen)
                    ;
                ptr++;
            }
            else {
                for (klen = strlen(elt); klen && apr_isspace(elt[klen - 1]); --klen)
                    ;
            }
            elt[klen] = 0;

            for (j = 0; fields[j].field != NULL; ++j) {
                if (!strcasecmp(fields[j].field, elt)) {
                    fields[j].set = 1;
                    if (ptr) {
                        fields[j].value = ptr;
                    }
                    break;
                }
            }

            i++;
        }
        engine = fields[0].value;
    }

    f = apr_pcalloc(pool, sizeof(apr_crypto_t));
    if (!f) {
        return APR_ENOMEM;
    }
    f->config = config = apr_pcalloc(pool, sizeof(apr_crypto_config_t));
    if (!config) {
        return APR_ENOMEM;
    }
    f->pool = pool;
    f->provider = provider;

    /* The default/builtin "openssl" engine is the same as NULL though with
     * openssl-3+ it's called something else, keep NULL for that name.
     */
    if (engine && strcasecmp(engine, "openssl") != 0) {
#if APR_USE_OPENSSL_ENGINE_API
        config->engine = ENGINE_by_id(engine);
        if (!config->engine) {
            return APR_ENOENGINE;
        }
        if (!ENGINE_init(config->engine)) {
            status = APR_EINITENGINE;
            goto cleanup;
        }
#else
        return APR_ENOTIMPL;
#endif
    }

    f->result = apr_pcalloc(pool, sizeof(apu_err_t));
    if (!f->result) {
        status = APR_ENOMEM;
        goto cleanup;
    }

    f->digests = apr_hash_make(pool);
    if (!f->digests) {
        status = APR_ENOMEM;
        goto cleanup;
    }
    apr_hash_set(f->digests, "md5", APR_HASH_KEY_STRING, &(key_digests[i = 0]));
    apr_hash_set(f->digests, "sha1", APR_HASH_KEY_STRING, &(key_digests[++i]));
    apr_hash_set(f->digests, "sha224", APR_HASH_KEY_STRING, &(key_digests[++i]));
    apr_hash_set(f->digests, "sha256", APR_HASH_KEY_STRING, &(key_digests[++i]));
    apr_hash_set(f->digests, "sha384", APR_HASH_KEY_STRING, &(key_digests[++i]));
    apr_hash_set(f->digests, "sha512", APR_HASH_KEY_STRING, &(key_digests[++i]));

    f->types = apr_hash_make(pool);
    if (!f->types) {
        status = APR_ENOMEM;
        goto cleanup;
    }
    apr_hash_set(f->types, "3des192", APR_HASH_KEY_STRING, &(key_types[i = 0]));
    apr_hash_set(f->types, "aes128", APR_HASH_KEY_STRING, &(key_types[++i]));
    apr_hash_set(f->types, "aes192", APR_HASH_KEY_STRING, &(key_types[++i]));
    apr_hash_set(f->types, "aes256", APR_HASH_KEY_STRING, &(key_types[++i]));

    f->modes = apr_hash_make(pool);
    if (!f->modes) {
        status = APR_ENOMEM;
        goto cleanup;
    }
    apr_hash_set(f->modes, "ecb", APR_HASH_KEY_STRING, &(key_modes[i = 0]));
    apr_hash_set(f->modes, "cbc", APR_HASH_KEY_STRING, &(key_modes[++i]));

    f->digests = apr_hash_make(pool);
    if (!f->digests) {
        status = APR_ENOMEM;
        goto cleanup;
    }

    *ff = f;
    apr_pool_cleanup_register(pool, f, crypto_cleanup_helper,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;

cleanup:
    crypto_cleanup(f);
    return status;
}

/**
 * @brief Get a hash table of key digests, keyed by the name of the digest against
 * a pointer to apr_crypto_block_key_digest_t.
 *
 * @param digests - hashtable of key digests keyed to constants.
 * @param f - encryption context
 * @return APR_SUCCESS for success
 */
static apr_status_t crypto_get_block_key_digests(apr_hash_t **digests,
        const apr_crypto_t *f)
{
    *digests = f->digests;
    return APR_SUCCESS;
}

/**
 * @brief Get a hash table of key types, keyed by the name of the type against
 * a pointer to apr_crypto_block_key_type_t.
 *
 * @param types - hashtable of key types keyed to constants.
 * @param f - encryption context
 * @return APR_SUCCESS for success
 */
static apr_status_t crypto_get_block_key_types(apr_hash_t **types,
        const apr_crypto_t *f)
{
    *types = f->types;
    return APR_SUCCESS;
}

/**
 * @brief Get a hash table of key modes, keyed by the name of the mode against
 * a pointer to apr_crypto_block_key_mode_t.
 *
 * @param modes - hashtable of key modes keyed to constants.
 * @param f - encryption context
 * @return APR_SUCCESS for success
 */
static apr_status_t crypto_get_block_key_modes(apr_hash_t **modes,
        const apr_crypto_t *f)
{
    *modes = f->modes;
    return APR_SUCCESS;
}

/*
 * Work out which mechanism to use.
 */
static apr_status_t crypto_cipher_mechanism(apr_crypto_key_t *key,
        const apr_crypto_block_key_type_e type,
        const apr_crypto_block_key_mode_e mode, const int doPad, apr_pool_t *p)
{
    /* determine the cipher to be used */
    switch (type) {

    case (APR_KEY_3DES_192):

        /* A 3DES key */
        if (mode == APR_MODE_CBC) {
            key->cipher = EVP_des_ede3_cbc();
        }
        else {
            key->cipher = EVP_des_ede3_ecb();
        }
        break;

    case (APR_KEY_AES_128):

        if (mode == APR_MODE_CBC) {
            key->cipher = EVP_aes_128_cbc();
        }
        else {
            key->cipher = EVP_aes_128_ecb();
        }
        break;

    case (APR_KEY_AES_192):

        if (mode == APR_MODE_CBC) {
            key->cipher = EVP_aes_192_cbc();
        }
        else {
            key->cipher = EVP_aes_192_ecb();
        }
        break;

    case (APR_KEY_AES_256):

        if (mode == APR_MODE_CBC) {
            key->cipher = EVP_aes_256_cbc();
        }
        else {
            key->cipher = EVP_aes_256_ecb();
        }
        break;

    default:

        /* unknown key type, give up */
        return APR_EKEYTYPE;

    }

    /* find the length of the key we need */
    key->keyLen = EVP_CIPHER_key_length(key->cipher);

    /* make space for the key */
    key->key = apr_pcalloc(p, key->keyLen);
    if (!key->key) {
        return APR_ENOMEM;
    }

    return APR_SUCCESS;
}

/**
 * @brief Create a key from the provided secret or passphrase. The key is cleaned
 *        up when the context is cleaned, and may be reused with multiple encryption
 *        or decryption operations.
 * @note If *key is NULL, a apr_crypto_key_t will be created from a pool. If
 *       *key is not NULL, *key must point at a previously created structure.
 * @param key The key returned, see note.
 * @param rec The key record, from which the key will be derived.
 * @param f The context to use.
 * @param p The pool to use.
 * @return Returns APR_ENOKEY if the pass phrase is missing or empty, or if a backend
 *         error occurred while generating the key. APR_ENOCIPHER if the type or mode
 *         is not supported by the particular backend. APR_EKEYTYPE if the key type is
 *         not known. APR_EPADDING if padding was requested but is not supported.
 *         APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_key(apr_crypto_key_t **k,
        const apr_crypto_key_rec_t *rec, const apr_crypto_t *f, apr_pool_t *p)
{
    apr_crypto_key_t *key = *k;
    apr_status_t rv;

    if (!key) {
        *k = key = apr_pcalloc(p, sizeof *key);
        if (!key) {
            return APR_ENOMEM;
        }
        apr_pool_cleanup_register(p, key, crypto_key_cleanup_helper,
                                  apr_pool_cleanup_null);
    }
    else {
        crypto_key_cleanup(key);
    }
    key->pool = p;
    key->f = f;
    key->provider = f->provider;
    key->rec = rec;

    switch (rec->ktype) {

    case APR_CRYPTO_KTYPE_PASSPHRASE: {

        /* decide on what cipher mechanism we will be using */
        rv = crypto_cipher_mechanism(key, rec->type, rec->mode, rec->pad, p);
        if (APR_SUCCESS != rv) {
            return rv;
        }

        /* generate the key */
        if (PKCS5_PBKDF2_HMAC_SHA1(rec->k.passphrase.pass,
                rec->k.passphrase.passLen,
                (unsigned char *) rec->k.passphrase.salt,
                rec->k.passphrase.saltLen, rec->k.passphrase.iterations,
                key->keyLen, key->key) == 0) {
            return APR_ENOKEY;
        }

        break;
    }

    case APR_CRYPTO_KTYPE_SECRET: {

        /* decide on what cipher mechanism we will be using */
        rv = crypto_cipher_mechanism(key, rec->type, rec->mode, rec->pad, p);
        if (APR_SUCCESS != rv) {
            return rv;
        }

        /* sanity check - key correct size? */
        if (rec->k.secret.secretLen != key->keyLen) {
            return APR_EKEYLENGTH;
        }

        /* copy the key */
        memcpy(key->key, rec->k.secret.secret, rec->k.secret.secretLen);

        break;
    }
    case APR_CRYPTO_KTYPE_HASH: {

        switch (rec->k.hash.digest) {
        case APR_CRYPTO_DIGEST_MD5:
            key->md = EVP_md5();
            break;
        case APR_CRYPTO_DIGEST_SHA1:
            key->md = EVP_sha1();
            break;
        case APR_CRYPTO_DIGEST_SHA224:
            key->md = EVP_sha224();
            break;
        case APR_CRYPTO_DIGEST_SHA256:
            key->md = EVP_sha256();
            break;
        case APR_CRYPTO_DIGEST_SHA384:
            key->md = EVP_sha384();
            break;
        case APR_CRYPTO_DIGEST_SHA512:
            key->md = EVP_sha512();
            break;
        default:
            return APR_ENODIGEST;
        }

        break;
    }
    case APR_CRYPTO_KTYPE_HMAC:
    case APR_CRYPTO_KTYPE_CMAC: {

        switch (rec->k.hmac.digest) {
        case APR_CRYPTO_DIGEST_MD5:
            key->md = EVP_md5();
            break;
        case APR_CRYPTO_DIGEST_SHA1:
            key->md = EVP_sha1();
            break;
        case APR_CRYPTO_DIGEST_SHA224:
            key->md = EVP_sha224();
            break;
        case APR_CRYPTO_DIGEST_SHA256:
            key->md = EVP_sha256();
            break;
        case APR_CRYPTO_DIGEST_SHA384:
            key->md = EVP_sha384();
            break;
        case APR_CRYPTO_DIGEST_SHA512:
            key->md = EVP_sha512();
            break;
        default:
            return APR_ENODIGEST;
        }

        /* create hmac key */
#if APR_USE_OPENSSL_PRE_3_0_API
        if (rec->ktype == APR_CRYPTO_KTYPE_HMAC) {
            apr_crypto_config_t *config = f->config;
            key->pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC,
                                             config->engine,
                                             rec->k.hmac.secret,
                                             rec->k.hmac.secretLen);
        }
        else {
#if !APR_USE_OPENSSL_PRE_1_1_1_API
            apr_crypto_config_t *config = f->config;
            /* decide on what cipher mechanism we will be using */
            rv = crypto_cipher_mechanism(key, rec->type, rec->mode, rec->pad, p);
            if (APR_SUCCESS != rv) {
                return rv;
            }
            key->pkey = EVP_PKEY_new_CMAC_key(config->engine,
                                              rec->k.cmac.secret,
                                              rec->k.cmac.secretLen,
                                              key->cipher);
#else
            return APR_ENOTIMPL;
#endif
        }
        if (!key->pkey) {
            return APR_ENOKEY;
        }
#else
        if (rec->ktype == APR_CRYPTO_KTYPE_HMAC) {
            key->mac = EVP_MAC_fetch(NULL, "HMAC", NULL);
        }
        else {
            key->mac = EVP_MAC_fetch(NULL, "CMAC", NULL);
        }
        if (!key->mac) {
            return APR_ENOMEM;
        }
#endif

        break;
    }

    default: {

        return APR_ENOKEY;

    }
    }

    key->doPad = rec->pad;

    /* note: openssl incorrectly returns non zero IV size values for ECB
     * algorithms, so work around this by ignoring the IV size.
     */
    if (APR_MODE_ECB != rec->mode && key->cipher) {
        key->ivSize = EVP_CIPHER_iv_length(key->cipher);
    }

    return APR_SUCCESS;
}

/**
 * @brief Create a key from the given passphrase. By default, the PBKDF2
 *        algorithm is used to generate the key from the passphrase. It is expected
 *        that the same pass phrase will generate the same key, regardless of the
 *        backend crypto platform used. The key is cleaned up when the context
 *        is cleaned, and may be reused with multiple encryption or decryption
 *        operations.
 * @note If *key is NULL, a apr_crypto_key_t will be created from a pool. If
 *       *key is not NULL, *key must point at a previously created structure.
 * @param key The key returned, see note.
 * @param ivSize The size of the initialisation vector will be returned, based
 *               on whether an IV is relevant for this type of crypto.
 * @param pass The passphrase to use.
 * @param passLen The passphrase length in bytes
 * @param salt The salt to use.
 * @param saltLen The salt length in bytes
 * @param type 3DES_192, AES_128, AES_192, AES_256.
 * @param mode Electronic Code Book / Cipher Block Chaining.
 * @param doPad Pad if necessary.
 * @param iterations Iteration count
 * @param f The context to use.
 * @param p The pool to use.
 * @return Returns APR_ENOKEY if the pass phrase is missing or empty, or if a backend
 *         error occurred while generating the key. APR_ENOCIPHER if the type or mode
 *         is not supported by the particular backend. APR_EKEYTYPE if the key type is
 *         not known. APR_EPADDING if padding was requested but is not supported.
 *         APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_passphrase(apr_crypto_key_t **k, apr_size_t *ivSize,
        const char *pass, apr_size_t passLen, const unsigned char * salt,
        apr_size_t saltLen, const apr_crypto_block_key_type_e type,
        const apr_crypto_block_key_mode_e mode, const int doPad,
        const int iterations, const apr_crypto_t *f, apr_pool_t *p)
{
    apr_crypto_key_t *key = *k;
    apr_crypto_key_rec_t *rec;
    apr_status_t rv;

    if (!key) {
        *k = key = apr_pcalloc(p, sizeof *key);
        if (!key) {
            return APR_ENOMEM;
        }
    }

    key->f = f;
    key->provider = f->provider;
    key->rec = rec = apr_pcalloc(p, sizeof(apr_crypto_key_rec_t));
    if (!key->rec) {
        return APR_ENOMEM;
    }
    rec->ktype = APR_CRYPTO_KTYPE_PASSPHRASE;

    /* decide on what cipher mechanism we will be using */
    rv = crypto_cipher_mechanism(key, type, mode, doPad, p);
    if (APR_SUCCESS != rv) {
        return rv;
    }

    /* generate the key */
    if (PKCS5_PBKDF2_HMAC_SHA1(pass, passLen, (unsigned char *) salt, saltLen,
            iterations, key->keyLen, key->key) == 0) {
        return APR_ENOKEY;
    }

    key->doPad = doPad;

    /* note: openssl incorrectly returns non zero IV size values for ECB
     * algorithms, so work around this by ignoring the IV size.
     */
    if (APR_MODE_ECB != mode) {
        key->ivSize = EVP_CIPHER_iv_length(key->cipher);
    }
    if (ivSize) {
        *ivSize = key->ivSize;
    }

    return APR_SUCCESS;
}

/**
 * @brief Initialise a context for encrypting arbitrary data using the given key.
 * @note If *ctx is NULL, a apr_crypto_block_t will be created from a pool. If
 *       *ctx is not NULL, *ctx must point at a previously created structure.
 * @param ctx The block context returned, see note.
 * @param iv Optional initialisation vector. If the buffer pointed to is NULL,
 *           an IV will be created at random, in space allocated from the pool.
 *           If the buffer pointed to is not NULL, the IV in the buffer will be
 *           used.
 * @param key The key structure.
 * @param blockSize The block size of the cipher.
 * @param p The pool to use.
 * @return Returns APR_ENOIV if an initialisation vector is required but not specified.
 *         Returns APR_EINIT if the backend failed to initialise the context. Returns
 *         APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_encrypt_init(apr_crypto_block_t **ctx,
        const unsigned char **iv, const apr_crypto_key_t *key,
        apr_size_t *blockSize, apr_pool_t *p)
{
    unsigned char *usedIv;
    apr_crypto_config_t *config = key->f->config;
    apr_crypto_block_t *block = *ctx;

    if (!block) {
        *ctx = block = apr_pcalloc(p, sizeof(apr_crypto_block_t));
        if (!block) {
            return APR_ENOMEM;
        }
        apr_pool_cleanup_register(p, block, crypto_block_cleanup_helper,
                                  apr_pool_cleanup_null);
    }
    else {
        crypto_block_cleanup(block);
    }
    block->f = key->f;
    block->pool = p;
    block->provider = key->provider;
    block->key = key;

    switch (key->rec->ktype) {

    case APR_CRYPTO_KTYPE_PASSPHRASE:
    case APR_CRYPTO_KTYPE_SECRET: {

        /* create a new context for encryption */
        if (!block->cipherCtx) {
            block->cipherCtx = EVP_CIPHER_CTX_new();
            if (!block->cipherCtx) {
                return APR_ENOMEM;
            }
        }

        /* generate an IV, if necessary */
        usedIv = NULL;
        if (key->ivSize) {
            if (iv == NULL) {
                return APR_ENOIV;
            }
            if (*iv == NULL) {
                usedIv = apr_pcalloc(p, key->ivSize);
                if (!usedIv) {
                    return APR_ENOMEM;
                }
                if (!((RAND_status() == 1)
                        && (RAND_bytes(usedIv, key->ivSize) == 1))) {
                    return APR_ENOIV;
                }
                *iv = usedIv;
            }
            else {
                usedIv = (unsigned char *) *iv;
            }
        }

        /* set up our encryption context */
#if CRYPTO_OPENSSL_CONST_BUFFERS
        if (!EVP_EncryptInit_ex(block->cipherCtx, key->cipher, config->engine,
                key->key, usedIv)) {
#else
        if (!EVP_EncryptInit_ex(block->cipherCtx, key->cipher, config->engine,
                                (unsigned char *) key->key, (unsigned char *) usedIv)) {
#endif
            return APR_EINIT;
        }

        /* Clear up any read padding */
        if (!EVP_CIPHER_CTX_set_padding(block->cipherCtx, key->doPad)) {
            return APR_EPADDING;
        }

        if (blockSize) {
            *blockSize = EVP_CIPHER_block_size(key->cipher);
        }

        return APR_SUCCESS;

    }
    default: {

        return APR_EINVAL;

    }
    }

}

/**
 * @brief Encrypt data provided by in, write it to out.
 * @note The number of bytes written will be written to outlen. If
 *       out is NULL, outlen will contain the maximum size of the
 *       buffer needed to hold the data, including any data
 *       generated by apr_crypto_block_encrypt_finish below. If *out points
 *       to NULL, a buffer sufficiently large will be created from
 *       the pool provided. If *out points to a not-NULL value, this
 *       value will be used as a buffer instead.
 * @param out Address of a buffer to which data will be written,
 *        see note.
 * @param outlen Length of the output will be written here.
 * @param in Address of the buffer to read.
 * @param inlen Length of the buffer to read.
 * @param ctx The block context to use.
 * @return APR_ECRYPT if an error occurred. Returns APR_ENOTIMPL if
 *         not implemented.
 */
static apr_status_t crypto_block_encrypt(unsigned char **out,
        apr_size_t *outlen, const unsigned char *in, apr_size_t inlen,
        apr_crypto_block_t *block)
{
    switch (block->key->rec->ktype) {

    case APR_CRYPTO_KTYPE_PASSPHRASE:
    case APR_CRYPTO_KTYPE_SECRET: {

        int outl = *outlen;
        unsigned char *buffer;

        /* are we after the maximum size of the out buffer? */
        if (!out) {
            *outlen = inlen + EVP_MAX_BLOCK_LENGTH;
            return APR_SUCCESS;
        }

        /* must we allocate the output buffer from a pool? */
        if (!*out) {
            buffer = apr_pcalloc(block->pool, inlen + EVP_MAX_BLOCK_LENGTH);
            if (!buffer) {
                return APR_ENOMEM;
            }
            *out = buffer;
        }

#if CRYPT_OPENSSL_CONST_BUFFERS
        if (!EVP_EncryptUpdate(block->cipherCtx, (*out), &outl, in, inlen)) {
#else
        if (!EVP_EncryptUpdate(block->cipherCtx, (*out), &outl,
                (unsigned char *) in, inlen)) {
#endif
            return APR_ECRYPT;
        }
        *outlen = outl;

        return APR_SUCCESS;

    }
    default: {

        return APR_EINVAL;

    }
    }

}

/**
 * @brief Encrypt final data block, write it to out.
 * @note If necessary the final block will be written out after being
 *       padded. Typically the final block will be written to the
 *       same buffer used by apr_crypto_block_encrypt, offset by the
 *       number of bytes returned as actually written by the
 *       apr_crypto_block_encrypt() call. After this call, the context
 *       is cleaned and can be reused by apr_crypto_block_encrypt_init().
 * @param out Address of a buffer to which data will be written. This
 *            buffer must already exist, and is usually the same
 *            buffer used by apr_evp_crypt(). See note.
 * @param outlen Length of the output will be written here.
 * @param ctx The block context to use.
 * @return APR_ECRYPT if an error occurred.
 * @return APR_EPADDING if padding was enabled and the block was incorrectly
 *         formatted.
 * @return APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_encrypt_finish(unsigned char *out,
        apr_size_t *outlen, apr_crypto_block_t *block)
{
    switch (block->key->rec->ktype) {

    case APR_CRYPTO_KTYPE_PASSPHRASE:
    case APR_CRYPTO_KTYPE_SECRET: {

        apr_status_t rc = APR_SUCCESS;
        int len = *outlen;

        if (EVP_EncryptFinal_ex(block->cipherCtx, out, &len) == 0) {
            rc = APR_EPADDING;
        }
        else {
            *outlen = len;
        }

        return rc;

    }
    default: {

        return APR_EINVAL;

    }
    }

}

/**
 * @brief Initialise a context for decrypting arbitrary data using the given key.
 * @note If *ctx is NULL, a apr_crypto_block_t will be created from a pool. If
 *       *ctx is not NULL, *ctx must point at a previously created structure.
 * @param ctx The block context returned, see note.
 * @param blockSize The block size of the cipher.
 * @param iv Optional initialisation vector. If the buffer pointed to is NULL,
 *           an IV will be created at random, in space allocated from the pool.
 *           If the buffer is not NULL, the IV in the buffer will be used.
 * @param key The key structure.
 * @param p The pool to use.
 * @return Returns APR_ENOIV if an initialisation vector is required but not specified.
 *         Returns APR_EINIT if the backend failed to initialise the context. Returns
 *         APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_decrypt_init(apr_crypto_block_t **ctx,
        apr_size_t *blockSize, const unsigned char *iv,
        const apr_crypto_key_t *key, apr_pool_t *p)
{
    apr_crypto_config_t *config = key->f->config;
    apr_crypto_block_t *block = *ctx;

    if (!block) {
        *ctx = block = apr_pcalloc(p, sizeof(apr_crypto_block_t));
        if (!block) {
            return APR_ENOMEM;
        }
        apr_pool_cleanup_register(p, block, crypto_block_cleanup_helper,
                                  apr_pool_cleanup_null);
    }
    else {
        crypto_block_cleanup(block);
    }
    block->f = key->f;
    block->pool = p;
    block->provider = key->provider;
    block->key = key;

    switch (key->rec->ktype) {

    case APR_CRYPTO_KTYPE_PASSPHRASE:
    case APR_CRYPTO_KTYPE_SECRET: {

        /* create a new context for encryption */
        if (!block->cipherCtx) {
            block->cipherCtx = EVP_CIPHER_CTX_new();
            if (!block->cipherCtx) {
                return APR_ENOMEM;
            }
        }

        /* generate an IV, if necessary */
        if (key->ivSize) {
            if (iv == NULL) {
                return APR_ENOIV;
            }
        }

        /* set up our encryption context */
#if CRYPTO_OPENSSL_CONST_BUFFERS
        if (!EVP_DecryptInit_ex(block->cipherCtx, key->cipher, config->engine,
                key->key, iv)) {
#else
            if (!EVP_DecryptInit_ex(block->cipherCtx, key->cipher, config->engine,
                                    (unsigned char *) key->key, (unsigned char *) iv)) {
#endif
            return APR_EINIT;
        }

        /* Clear up any read padding */
        if (!EVP_CIPHER_CTX_set_padding(block->cipherCtx, key->doPad)) {
            return APR_EPADDING;
        }

        if (blockSize) {
            *blockSize = EVP_CIPHER_block_size(key->cipher);
        }

        return APR_SUCCESS;

    }
    default: {

        return APR_EINVAL;

    }
    }

}

/**
 * @brief Decrypt data provided by in, write it to out.
 * @note The number of bytes written will be written to outlen. If
 *       out is NULL, outlen will contain the maximum size of the
 *       buffer needed to hold the data, including any data
 *       generated by apr_crypto_block_decrypt_finish below. If *out points
 *       to NULL, a buffer sufficiently large will be created from
 *       the pool provided. If *out points to a not-NULL value, this
 *       value will be used as a buffer instead.
 * @param out Address of a buffer to which data will be written,
 *        see note.
 * @param outlen Length of the output will be written here.
 * @param in Address of the buffer to read.
 * @param inlen Length of the buffer to read.
 * @param ctx The block context to use.
 * @return APR_ECRYPT if an error occurred. Returns APR_ENOTIMPL if
 *         not implemented.
 */
static apr_status_t crypto_block_decrypt(unsigned char **out,
        apr_size_t *outlen, const unsigned char *in, apr_size_t inlen,
        apr_crypto_block_t *block)
{
    switch (block->key->rec->ktype) {

    case APR_CRYPTO_KTYPE_PASSPHRASE:
    case APR_CRYPTO_KTYPE_SECRET: {

        int outl = *outlen;
        unsigned char *buffer;

        /* are we after the maximum size of the out buffer? */
        if (!out) {
            *outlen = inlen + EVP_MAX_BLOCK_LENGTH;
            return APR_SUCCESS;
        }

        /* must we allocate the output buffer from a pool? */
        if (!(*out)) {
            buffer = apr_pcalloc(block->pool, inlen + EVP_MAX_BLOCK_LENGTH);
            if (!buffer) {
                return APR_ENOMEM;
            }
            *out = buffer;
        }

#if CRYPT_OPENSSL_CONST_BUFFERS
        if (!EVP_DecryptUpdate(block->cipherCtx, *out, &outl, in, inlen)) {
#else
        if (!EVP_DecryptUpdate(block->cipherCtx, *out, &outl, (unsigned char *) in,
                inlen)) {
#endif
            return APR_ECRYPT;
        }
        *outlen = outl;

        return APR_SUCCESS;

    }
    default: {

        return APR_EINVAL;

    }
    }

}

/**
 * @brief Decrypt final data block, write it to out.
 * @note If necessary the final block will be written out after being
 *       padded. Typically the final block will be written to the
 *       same buffer used by apr_crypto_block_decrypt, offset by the
 *       number of bytes returned as actually written by the
 *       apr_crypto_block_decrypt() call. After this call, the context
 *       is cleaned and can be reused by apr_crypto_block_decrypt_init().
 * @param out Address of a buffer to which data will be written. This
 *            buffer must already exist, and is usually the same
 *            buffer used by apr_evp_crypt(). See note.
 * @param outlen Length of the output will be written here.
 * @param ctx The block context to use.
 * @return APR_ECRYPT if an error occurred.
 * @return APR_EPADDING if padding was enabled and the block was incorrectly
 *         formatted.
 * @return APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_decrypt_finish(unsigned char *out,
        apr_size_t *outlen, apr_crypto_block_t *block)
{
    switch (block->key->rec->ktype) {

    case APR_CRYPTO_KTYPE_PASSPHRASE:
    case APR_CRYPTO_KTYPE_SECRET: {

        apr_status_t rc = APR_SUCCESS;
        int len = *outlen;

        if (EVP_DecryptFinal_ex(block->cipherCtx, out, &len) == 0) {
            rc = APR_EPADDING;
        }
        else {
            *outlen = len;
        }

        return rc;

    }
    default: {

        return APR_EINVAL;

    }
    }

}

static apr_status_t crypto_digest_init(apr_crypto_digest_t **d,
        const apr_crypto_key_t *key, apr_crypto_digest_rec_t *rec, apr_pool_t *p)
{
    apr_crypto_config_t *config = key->f->config;
    apr_crypto_digest_t *digest = *d;

    if (!digest) {
        *d = digest = apr_pcalloc(p, sizeof(apr_crypto_digest_t));
        if (!digest) {
            return APR_ENOMEM;
        }
        apr_pool_cleanup_register(p, digest, crypto_digest_cleanup_helper,
                                  apr_pool_cleanup_null);
    }
    else {
        crypto_digest_cleanup(digest);
    }
    digest->f = key->f;
    digest->pool = p;
    digest->provider = key->provider;
    digest->key = key;
    digest->rec = rec;

    switch (key->rec->ktype) {

    case APR_CRYPTO_KTYPE_HASH: {
        if (!digest->mdCtx) {
            digest->mdCtx = EVP_MD_CTX_new();
            if (!digest->mdCtx) {
                return APR_ENOMEM;
            }
        }
        if (!EVP_DigestInit_ex(digest->mdCtx, key->md, config->engine)) {
            return APR_EINIT;
        }

        break;
    }
    case APR_CRYPTO_KTYPE_HMAC:
    case APR_CRYPTO_KTYPE_CMAC: {
#if APR_USE_OPENSSL_PRE_3_0_API
        if (!digest->mdCtx) {
            digest->mdCtx = EVP_MD_CTX_new();
            if (!digest->mdCtx) {
                return APR_ENOMEM;
            }
        }
        if (!EVP_DigestSignInit(digest->mdCtx, NULL, key->md,
                                config->engine, key->pkey)) {
            return APR_EINIT;
        }
#else
        OSSL_PARAM params[2];
        if (!digest->macCtx) {
            digest->macCtx = EVP_MAC_CTX_new(key->mac);
            if (!digest->macCtx) {
                return APR_ENOMEM;
            }
        }
        if (key->rec->ktype == APR_CRYPTO_KTYPE_HMAC) {
            params[0] =
                OSSL_PARAM_construct_utf8_string("digest",
                                                 (char *)EVP_MD_name(key->md),
                                                 0);
        }
        else {
            params[0] =
                OSSL_PARAM_construct_utf8_string("cipher",
                                                 (char *)EVP_CIPHER_name(key->cipher),
                                                 0);
        }
        params[1] = OSSL_PARAM_construct_end();
        if (!EVP_MAC_init(digest->macCtx,
                          key->rec->k.hmac.secret,
                          key->rec->k.hmac.secretLen,
                          params)) {
            return APR_EINIT;
        }
#endif
        break;
    }
    default: {
        return APR_EINVAL;
    }
    }

    return APR_SUCCESS;

}

static apr_status_t crypto_digest_update(apr_crypto_digest_t *digest,
        const unsigned char *in, apr_size_t inlen)
{
    switch (digest->key->rec->ktype) {

    case APR_CRYPTO_KTYPE_HASH: {
        if (!EVP_DigestUpdate(digest->mdCtx, in, inlen)) {
            return APR_ECRYPT;
        }

        return APR_SUCCESS;

    }
    case APR_CRYPTO_KTYPE_HMAC:
    case APR_CRYPTO_KTYPE_CMAC: {
#if APR_USE_OPENSSL_PRE_3_0_API
        if (!EVP_DigestSignUpdate(digest->mdCtx, in, inlen)) {
            return APR_ECRYPT;
        }
#else
        if (!EVP_MAC_update(digest->macCtx, in, inlen)) {
            return APR_ECRYPT;
        }
#endif

        return APR_SUCCESS;

    }
    default: {
        return APR_EINVAL;
    }
    }

}

static apr_status_t crypto_digest_final(apr_crypto_digest_t *digest)
{
    apr_status_t status = APR_SUCCESS;

    switch (digest->key->rec->ktype) {

    case APR_CRYPTO_KTYPE_HASH: {
        switch (digest->rec->dtype) {
        case APR_CRYPTO_DTYPE_HASH: {
            unsigned int len = EVP_MD_CTX_size(digest->mdCtx);

            /* must we allocate the output buffer from a pool? */
            if (!digest->rec->d.hash.s || digest->rec->d.hash.slen != len) {
                digest->rec->d.hash.slen = len;
                digest->rec->d.hash.s = apr_pcalloc(digest->pool, len);
                if (!digest->rec->d.hash.s) {
                    return APR_ENOMEM;
                }
            }

            /* then, determine the signature */
            if (EVP_DigestFinal_ex(digest->mdCtx, digest->rec->d.hash.s, &len)
                    == 0) {
                OPENSSL_cleanse(digest->rec->d.hash.s,
                                digest->rec->d.hash.slen);
                status = APR_ECRYPT;
            }

            break;
        }
        default:
            status = APR_ENODIGEST;
        }

        break;
    }
    case APR_CRYPTO_KTYPE_HMAC:
    case APR_CRYPTO_KTYPE_CMAC: {
        size_t len;

        /* first, determine the signature length */
#if APR_USE_OPENSSL_PRE_3_0_API
        if (!EVP_DigestSignFinal(digest->mdCtx, NULL, &len)) {
            status = APR_ECRYPT;
        }
#else
        if (!EVP_MAC_final(digest->macCtx, NULL, &len, 0)) {
            status = APR_ECRYPT;
        }
#endif
        if (status == APR_SUCCESS) {
            switch (digest->rec->dtype) {
            case APR_CRYPTO_DTYPE_SIGN: {
                /* must we allocate the output buffer from a pool? */
                if (!digest->rec->d.sign.s || digest->rec->d.sign.slen != len) {
                    digest->rec->d.sign.slen = len;
                    digest->rec->d.sign.s = apr_pcalloc(digest->pool, len);
                    if (!digest->rec->d.sign.s) {
                        return APR_ENOMEM;
                    }
                }

                /* then, determine the signature */
#if APR_USE_OPENSSL_PRE_3_0_API
                if (!EVP_DigestSignFinal(digest->mdCtx,
                                         digest->rec->d.sign.s, &len)) {
                    status = APR_ECRYPT;
                }
#else
                if (!EVP_MAC_final(digest->macCtx,
                                   digest->rec->d.sign.s, &len, len)) {
                    status = APR_ECRYPT;
                }
#endif
                if (status != APR_SUCCESS) {
                    OPENSSL_cleanse(digest->rec->d.sign.s,
                                    digest->rec->d.sign.slen);
                }

                break;
            }
            case APR_CRYPTO_DTYPE_VERIFY: {
                /* must we allocate the output buffer from a pool? */
                if (!digest->rec->d.verify.s
                        || digest->rec->d.verify.slen != len) {
                    digest->rec->d.verify.slen = len;
                    digest->rec->d.verify.s = apr_pcalloc(digest->pool, len);
                    if (!digest->rec->d.verify.s) {
                        return APR_ENOMEM;
                    }
                }

                /* then, determine the signature */
#if APR_USE_OPENSSL_PRE_3_0_API
                if (!EVP_DigestSignFinal(digest->mdCtx,
                                         digest->rec->d.verify.s, &len)) {
                    status = APR_ECRYPT;
                }
#else
                if (!EVP_MAC_final(digest->macCtx,
                                   digest->rec->d.verify.s, &len, len)) {
                    status = APR_ECRYPT;
                }
#endif
                if (status == APR_SUCCESS
                    && (len != digest->rec->d.verify.vlen
                        || CRYPTO_memcmp(digest->rec->d.verify.v,
                                         digest->rec->d.verify.s, len))) {
                    status = APR_ENOVERIFY;
                }
                if (status != APR_SUCCESS) {
                    OPENSSL_cleanse(digest->rec->d.verify.s,
                                    digest->rec->d.verify.slen);
                }

                break;
            }
            default:
                status = APR_ENODIGEST;
            }
        }

        break;
    }
    default:
        status = APR_EINVAL;
    }

    return status;
}

static apr_status_t crypto_digest(
        const apr_crypto_key_t *key, apr_crypto_digest_rec_t *rec, const unsigned char *in,
        apr_size_t inlen, apr_pool_t *p)
{
    apr_crypto_digest_t *digest = NULL;
    apr_status_t status = APR_SUCCESS;

    status = crypto_digest_init(&digest, key, rec, p);
    if (APR_SUCCESS == status) {
        status = crypto_digest_update(digest, in, inlen);
        if (APR_SUCCESS == status) {
            status = crypto_digest_final(digest);
        }
    }

    return status;
}

static void cprng_stream_ctx_free(cprng_stream_ctx_t *sctx)
{
    if (sctx->ctx) {
        EVP_CIPHER_CTX_free(sctx->ctx);
    }
    if (sctx->malloced) {
        free(sctx);
    }
}

static apr_status_t cprng_stream_ctx_make(cprng_stream_ctx_t **psctx,
        apr_crypto_t *f, apr_crypto_cipher_e cipher, apr_pool_t *pool)
{
    cprng_stream_ctx_t *sctx;
    EVP_CIPHER_CTX *ctx;
    const EVP_CIPHER *ecipher;

    *psctx = NULL;

    if (pool) {
        sctx = apr_palloc(pool, sizeof(cprng_stream_ctx_t));
    }
    else {
        sctx = malloc(sizeof(cprng_stream_ctx_t));
    }
    if (!sctx) {
        return APR_ENOMEM;
    }

    sctx->malloced = !pool;
    sctx->ctx = ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        cprng_stream_ctx_free(sctx);
        return APR_ENOMEM;
    }

    /* We only handle Chacha20 and AES256-CTR stream ciphers, for now.
     * AES256-CTR should be in any openssl version of this century but is used
     * only if Chacha20 is missing (openssl < 1.1). This is because Chacha20 is
     * fast (enough) in software and timing attacks safe, though AES256-CTR can
     * be faster and constant-time but only when the CPU (aesni) or some crypto
     * hardware are in place.
     */
    switch (cipher) {
    case APR_CRYPTO_CIPHER_AUTO: {
#if defined(NID_chacha20)
        ecipher = EVP_chacha20();
#elif defined(NID_aes_256_ctr)
        ecipher = EVP_aes_256_ctr();
#else
        cprng_stream_ctx_free(sctx);
        return APR_ENOCIPHER;
#endif
    }
    case APR_CRYPTO_CIPHER_AES_256_CTR: {
#if defined(NID_aes_256_ctr)
        ecipher = EVP_aes_256_ctr();
        break;
#else
        cprng_stream_ctx_free(sctx);
        return APR_ENOCIPHER;
#endif
    }
    case APR_CRYPTO_CIPHER_CHACHA20: {
#if defined(NID_chacha20)
        ecipher = EVP_chacha20();
        break;
#else
        cprng_stream_ctx_free(sctx);
        return APR_ENOCIPHER;
#endif
    }
    default: {
        cprng_stream_ctx_free(sctx);
        return APR_ENOCIPHER;
    }
    }

    if (EVP_EncryptInit_ex(ctx, ecipher, f->config->engine, NULL, NULL) <= 0) {
        cprng_stream_ctx_free(sctx);
        return APR_ENOMEM;
    }

    *psctx = sctx;
    return APR_SUCCESS;
}

static APR_INLINE
void cprng_stream_setkey(cprng_stream_ctx_t *sctx,
                         const unsigned char *key,
                         const unsigned char *iv)
{
    switch(EVP_CIPHER_CTX_nid(sctx->ctx)) {
#if defined(NID_chacha20)
    case NID_chacha20:
        /* With CHACHA20, iv=NULL is the same as zeros but it's faster
         * to (re-)init; use that for efficiency.
         */
        EVP_EncryptInit_ex(sctx->ctx, NULL, NULL, key, NULL);
        break;
#endif
#if defined(NID_aes_256_ctr)
    case NID_aes_256_ctr:
        /* With AES256-CTR, iv=NULL seems to peek up and random one (for
         * the initial CTR), while we can live with zeros (fixed CTR);
         * efficiency still.
         */
        EVP_EncryptInit_ex(sctx->ctx, NULL, NULL, key, iv);
        break;
#endif
    default:
        assert(0);
        break;
    }
}

static apr_status_t cprng_stream_ctx_bytes(cprng_stream_ctx_t **pctx,
        unsigned char *key, unsigned char *to, apr_size_t n, const unsigned char *z)
{
    cprng_stream_ctx_t *sctx = *pctx;
    int len;

    /* We never encrypt twice with the same key, so no IV is needed (can
     * be zeros). When EVP_EncryptInit() is called multiple times it clears
     * its previous resources appropriately, and since we don't want the key
     * and its keystream to reside in memory at the same time, we have to
     * EVP_EncryptInit() twice: firstly to set the key and then finally to
     * overwrite the key (with zeros) after the keystream is produced.
     * As for EVP_EncryptFinish(), we don't need it either because padding
     * is disabled (irrelevant for a stream cipher).
     */
    cprng_stream_setkey(sctx, key, z);
    EVP_CIPHER_CTX_set_padding(sctx->ctx, 0);
    EVP_EncryptUpdate(sctx->ctx, key, &len, z, CPRNG_KEY_SIZE);
    if (n) {
        EVP_EncryptUpdate(sctx->ctx, to, &len, z, n);
    }
    cprng_stream_setkey(sctx, z, z);

    return APR_SUCCESS;
}

/**
 * OpenSSL module.
 */
APR_MODULE_DECLARE_DATA const apr_crypto_driver_t apr_crypto_openssl_driver = {
    "openssl", crypto_init, crypto_make, crypto_get_block_key_digests, crypto_get_block_key_types,
    crypto_get_block_key_modes, crypto_passphrase,
    crypto_block_encrypt_init, crypto_block_encrypt,
    crypto_block_encrypt_finish, crypto_block_decrypt_init,
    crypto_block_decrypt, crypto_block_decrypt_finish,
    crypto_digest_init, crypto_digest_update, crypto_digest_final, crypto_digest,
    crypto_block_cleanup, crypto_digest_cleanup, crypto_cleanup, crypto_shutdown, crypto_error,
    crypto_key, cprng_stream_ctx_make, cprng_stream_ctx_free, cprng_stream_ctx_bytes
};

#endif
