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

#include "apu.h"

#include "apu_config.h"
#include "apu_errno.h"

#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

#include "apr_strings.h"
#include "apr_time.h"
#include "apr_buckets.h"

#include "apr_crypto_internal.h"

#if APU_HAVE_CRYPTO

#include <openssl/evp.h>
#include <openssl/engine.h>

#define LOG_PREFIX "apr_crypto_openssl: "

struct apr_crypto_config_t {
    ENGINE *engine;
};

struct apr_crypto_key_t {
    const EVP_CIPHER * cipher;
    unsigned char *key;
    int keyLen;
    int doPad;
    int ivSize;
};

struct apr_crypto_block_t {
    const apr_crypto_t *factory;
    apr_pool_t *pool;
    EVP_CIPHER_CTX cipherCtx;
    int initialised;
    int ivSize;
    int blockSize;
    int doPad;
};

/**
 * Shutdown the crypto library and release resources.
 */
static apr_status_t crypto_shutdown(apr_pool_t *pool) {
    ERR_free_strings();
    EVP_cleanup();
    ENGINE_cleanup();
    return APR_SUCCESS;
}

static apr_status_t crypto_shutdown_helper(void *data) {
    apr_pool_t *pool = (apr_pool_t *) data;
    return crypto_shutdown(pool);
}

/**
 * Initialise the crypto library and perform one time initialisation.
 */
static apr_status_t crypto_init(apr_pool_t *pool,
        const apr_array_header_t *params, int *rc) {
    CRYPTO_malloc_init();
    ERR_load_crypto_strings();
    /* SSL_load_error_strings(); */
    OpenSSL_add_all_algorithms();
    ENGINE_load_builtin_engines();
    ENGINE_register_all_complete();

    apr_pool_cleanup_register(pool, pool, crypto_shutdown_helper,
            apr_pool_cleanup_null);

    return APR_SUCCESS;
}

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param driver - driver to use
 * @param ctx The block context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
static apr_status_t crypto_block_cleanup(apr_crypto_block_t *ctx) {

    if (ctx->initialised) {
        EVP_CIPHER_CTX_cleanup(&ctx->cipherCtx);
        ctx->initialised = 0;
    }

    return APR_SUCCESS;

}

static apr_status_t crypto_block_cleanup_helper(void *data) {
    apr_crypto_block_t *block = (apr_crypto_block_t *) data;
    return crypto_block_cleanup(block);
}

/**
 * @brief Clean encryption / decryption factory.
 * @note After cleanup, a factory is free to be reused if necessary.
 * @param driver - driver to use
 * @param f The factory to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
static apr_status_t crypto_cleanup(apr_crypto_t *f) {

    if (f->config->engine) {
        ENGINE_finish(f->config->engine);
        ENGINE_free(f->config->engine);
        f->config->engine = NULL;
    }
    return APR_SUCCESS;

}

static apr_status_t crypto_cleanup_helper(void *data) {
    apr_crypto_t *f = (apr_crypto_t *) data;
    return crypto_cleanup(f);
}

/**
 * @brief Create a context for supporting encryption. Keys, certificates,
 *        algorithms and other parameters will be set per context. More than
 *        one context can be created at one time. A cleanup will be automatically
 *        registered with the given pool to guarantee a graceful shutdown.
 * @param driver - driver to use
 * @param pool - process pool
 * @param params - array of key parameters
 * @param factory - factory pointer will be written here
 * @return APR_ENOENGINE when the engine specified does not exist. APR_EINITENGINE
 * if the engine cannot be initialised.
 */
static apr_status_t crypto_factory(apr_pool_t *pool,
        const apr_array_header_t *params, apr_crypto_t **factory) {
    apr_crypto_config_t *config = NULL;
    struct apr_crypto_param_t *ents =
            params ? (struct apr_crypto_param_t *) params->elts : NULL;
    int i = 0;
    apr_crypto_t *f = apr_pcalloc(pool, sizeof(apr_crypto_t));
    if (!f) {
        return APR_ENOMEM;
    }
    *factory = f;
    f->pool = pool;
    config = f->config = apr_pcalloc(pool, sizeof(apr_crypto_config_t));
    if (!config) {
        return APR_ENOMEM;
    }
    f->result = apr_pcalloc(pool, sizeof(apu_err_t));
    if (!f->result) {
        return APR_ENOMEM;
    }
    f->keys = apr_array_make(pool, 10, sizeof(apr_crypto_key_t));

    apr_pool_cleanup_register(pool, f, crypto_cleanup_helper,
            apr_pool_cleanup_null);

    for (i = 0; params && i < params->nelts; i++) {
        switch (ents[i].type) {
        case APR_CRYPTO_ENGINE:
            config->engine = ENGINE_by_id(ents[i].path);
            if (!config->engine) {
                return APR_ENOENGINE;
            }
            if (!ENGINE_init(config->engine)) {
                ENGINE_free(config->engine);
                config->engine = NULL;
                return APR_EINITENGINE;
            }
            break;
        }
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
 * @param driver - driver to use
 * @param p The pool to use.
 * @param f The context to use.
 * @param pass The passphrase to use.
 * @param passLen The passphrase length in bytes
 * @param salt The salt to use.
 * @param saltLen The salt length in bytes
 * @param type 3DES_192, AES_128, AES_192, AES_256.
 * @param mode Electronic Code Book / Cipher Block Chaining.
 * @param doPad Pad if necessary.
 * @param key The key returned, see note.
 * @param ivSize The size of the initialisation vector will be returned, based
 *               on whether an IV is relevant for this type of crypto.
 * @return Returns APR_ENOKEY if the pass phrase is missing or empty, or if a backend
 *         error occurred while generating the key. APR_ENOCIPHER if the type or mode
 *         is not supported by the particular backend. APR_EKEYTYPE if the key type is
 *         not known. APR_EPADDING if padding was requested but is not supported.
 *         APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_passphrase(apr_pool_t *p, const apr_crypto_t *f,
        const char *pass, apr_size_t passLen, const unsigned char * salt,
        apr_size_t saltLen, const apr_crypto_block_key_type_e type,
        const apr_crypto_block_key_mode_e mode, const int doPad,
        const int iterations, apr_crypto_key_t **k, apr_size_t *ivSize) {
    apr_crypto_key_t *key = *k;

    if (!key) {
        *k = key = apr_array_push(f->keys);
    }
    if (!key) {
        return APR_ENOMEM;
    }

    /* determine the cipher to be used */
    switch (type) {

    case (KEY_3DES_192):

        /* A 3DES key */
        if (mode == MODE_CBC) {
            key->cipher = EVP_des_ede3_cbc();
        } else {
            key->cipher = EVP_des_ede3_ecb();
        }
        break;

    case (KEY_AES_128):

        if (mode == MODE_CBC) {
            key->cipher = EVP_aes_128_cbc();
        } else {
            key->cipher = EVP_aes_128_ecb();
        }
        break;

    case (KEY_AES_192):

        if (mode == MODE_CBC) {
            key->cipher = EVP_aes_192_cbc();
        } else {
            key->cipher = EVP_aes_192_ecb();
        }
        break;

    case (KEY_AES_256):

        if (mode == MODE_CBC) {
            key->cipher = EVP_aes_256_cbc();
        } else {
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

    /* generate the key */
    if (PKCS5_PBKDF2_HMAC_SHA1(pass, passLen, (unsigned char *) salt, saltLen,
            iterations, key->keyLen, key->key) == 0) {
        return APR_ENOKEY;
    }

    key->doPad = doPad;

    /* note: openssl incorrectly returns non zero IV size values for ECB
     * algorithms, so work around this by ignoring the IV size.
     */
    if (MODE_ECB != mode) {
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
 * @param p The pool to use.
 * @param f The block factory to use.
 * @param type 3DES_192, AES_128, AES_192, AES_256.
 * @param mode Electronic Code Book / Cipher Block Chaining.
 * @param key The key
 * @param keyLen The key length in bytes
 * @param iv Optional initialisation vector.
 * @param doPad Pad if necessary.
 * @param ctx The block context returned, see note.
 * @param blockSize The block size of the cipher.
 * @return Returns APR_ENOIV if an initialisation vector is required but not specified.
 *         Returns APR_EINIT if the backend failed to initialise the context. Returns
 *         APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_encrypt_init(apr_pool_t *p,
        const apr_crypto_t *f, const apr_crypto_key_t *key,
        const unsigned char **iv, apr_crypto_block_t **ctx,
        apr_size_t *blockSize) {
    unsigned char *usedIv;
    apr_crypto_config_t *config = f->config;
    apr_crypto_block_t *block = *ctx;
    if (!block) {
        *ctx = block = apr_pcalloc(p, sizeof(apr_crypto_block_t));
    }
    if (!block) {
        return APR_ENOMEM;
    }
    block->factory = f;
    block->pool = p;

    apr_pool_cleanup_register(p, block, crypto_block_cleanup_helper,
            apr_pool_cleanup_null);

    /* create a new context for encryption */
    EVP_CIPHER_CTX_init(&block->cipherCtx);
    block->initialised = 1;

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
        } else {
            usedIv = (unsigned char *) *iv;
        }
    }

    /* set up our encryption context */
#if CRYPTO_OPENSSL_CONST_BUFFERS
    if (!EVP_EncryptInit_ex(&block->cipherCtx, key->cipher, config->engine,
            key->key, usedIv)) {
#else
        if (!EVP_EncryptInit_ex(&block->cipherCtx, key->cipher, config->engine, (unsigned char *) key->key, (unsigned char *) usedIv)) {
#endif
        return APR_EINIT;
    }

    /* Clear up any read padding */
    if (!EVP_CIPHER_CTX_set_padding(&block->cipherCtx, key->doPad)) {
        return APR_EPADDING;
    }

    if (blockSize) {
        *blockSize = EVP_CIPHER_block_size(key->cipher);
    }

    return APR_SUCCESS;

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
 * @param ctx The block context to use.
 * @param out Address of a buffer to which data will be written,
 *        see note.
 * @param outlen Length of the output will be written here.
 * @param in Address of the buffer to read.
 * @param inlen Length of the buffer to read.
 * @return APR_ECRYPT if an error occurred. Returns APR_ENOTIMPL if
 *         not implemented.
 */
static apr_status_t crypto_block_encrypt(apr_crypto_block_t *ctx,
        unsigned char **out, apr_size_t *outlen, const unsigned char *in,
        apr_size_t inlen) {
    int outl = *outlen;
    unsigned char *buffer;

    /* are we after the maximum size of the out buffer? */
    if (!out) {
        *outlen = inlen + EVP_MAX_BLOCK_LENGTH;
        return APR_SUCCESS;
    }

    /* must we allocate the output buffer from a pool? */
    if (!*out) {
        buffer = apr_palloc(ctx->pool, inlen + EVP_MAX_BLOCK_LENGTH);
        if (!buffer) {
            return APR_ENOMEM;
        }
        *out = buffer;
    }

#if CRYPT_OPENSSL_CONST_BUFFERS
    if (!EVP_EncryptUpdate(&ctx->cipherCtx, (*out), &outl, in, inlen)) {
#else
    if (!EVP_EncryptUpdate(&ctx->cipherCtx, (*out), &outl,
            (unsigned char *) in, inlen)) {
#endif
        return APR_ECRYPT;
    }
    *outlen = outl;

    return APR_SUCCESS;

}

/**
 * @brief Encrypt final data block, write it to out.
 * @note If necessary the final block will be written out after being
 *       padded. Typically the final block will be written to the
 *       same buffer used by apr_crypto_block_encrypt, offset by the
 *       number of bytes returned as actually written by the
 *       apr_crypto_block_encrypt() call. After this call, the context
 *       is cleaned and can be reused by apr_crypto_block_encrypt_init().
 * @param ctx The block context to use.
 * @param out Address of a buffer to which data will be written. This
 *            buffer must already exist, and is usually the same
 *            buffer used by apr_evp_crypt(). See note.
 * @param outlen Length of the output will be written here.
 * @return APR_ECRYPT if an error occurred.
 * @return APR_EPADDING if padding was enabled and the block was incorrectly
 *         formatted.
 * @return APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_encrypt_finish(apr_crypto_block_t *ctx,
        unsigned char *out, apr_size_t *outlen) {
    int len = *outlen;

    if (EVP_EncryptFinal_ex(&ctx->cipherCtx, out, &len) == 0) {
        return APR_EPADDING;
    }
    *outlen = len;

    return APR_SUCCESS;

}

/**
 * @brief Initialise a context for decrypting arbitrary data using the given key.
 * @note If *ctx is NULL, a apr_crypto_block_t will be created from a pool. If
 *       *ctx is not NULL, *ctx must point at a previously created structure.
 * @param p The pool to use.
 * @param f The block factory to use.
 * @param key The key structure.
 * @param iv Optional initialisation vector. If the buffer pointed to is NULL,
 *           an IV will be created at random, in space allocated from the pool.
 *           If the buffer is not NULL, the IV in the buffer will be used.
 * @param ctx The block context returned, see note.
 * @param blockSize The block size of the cipher.
 * @return Returns APR_ENOIV if an initialisation vector is required but not specified.
 *         Returns APR_EINIT if the backend failed to initialise the context. Returns
 *         APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_decrypt_init(apr_pool_t *p,
        const apr_crypto_t *f, const apr_crypto_key_t *key,
        const unsigned char *iv, apr_crypto_block_t **ctx,
        apr_size_t *blockSize) {
    apr_crypto_config_t *config = f->config;
    apr_crypto_block_t *block = *ctx;
    if (!block) {
        *ctx = block = apr_pcalloc(p, sizeof(apr_crypto_block_t));
    }
    if (!block) {
        return APR_ENOMEM;
    }
    block->factory = f;
    block->pool = p;

    apr_pool_cleanup_register(p, block, crypto_block_cleanup_helper,
            apr_pool_cleanup_null);

    /* create a new context for encryption */
    EVP_CIPHER_CTX_init(&block->cipherCtx);
    block->initialised = 1;

    /* generate an IV, if necessary */
    if (key->ivSize) {
        if (iv == NULL) {
            return APR_ENOIV;
        }
    }

    /* set up our encryption context */
#if CRYPTO_OPENSSL_CONST_BUFFERS
    if (!EVP_DecryptInit_ex(&block->cipherCtx, key->cipher, config->engine,
            key->key, iv)) {
#else
        if (!EVP_DecryptInit_ex(&block->cipherCtx, key->cipher, config->engine, (unsigned char *) key->key, (unsigned char *) iv)) {
#endif
        return APR_EINIT;
    }

    /* Clear up any read padding */
    if (!EVP_CIPHER_CTX_set_padding(&block->cipherCtx, key->doPad)) {
        return APR_EPADDING;
    }

    if (blockSize) {
        *blockSize = EVP_CIPHER_block_size(key->cipher);
    }

    return APR_SUCCESS;

}

/**
 * @brief Decrypt data provided by in, write it to out.
 * @note The number of bytes written will be written to outlen. If
 *       out is NULL, outlen will contain the maximum size of the
 *       buffer needed to hold the data, including any data
 *       generated by apr_crypto_block_final below. If *out points
 *       to NULL, a buffer sufficiently large will be created from
 *       the pool provided. If *out points to a not-NULL value, this
 *       value will be used as a buffer instead.
 * @param ctx The block context to use.
 * @param out Address of a buffer to which data will be written,
 *        see note.
 * @param outlen Length of the output will be written here.
 * @param in Address of the buffer to read.
 * @param inlen Length of the buffer to read.
 * @return APR_ECRYPT if an error occurred. Returns APR_ENOTIMPL if
 *         not implemented.
 */
static apr_status_t crypto_block_decrypt(apr_crypto_block_t *ctx,
        unsigned char **out, apr_size_t *outlen, const unsigned char *in,
        apr_size_t inlen) {

    int outl = *outlen;
    unsigned char *buffer;

    /* are we after the maximum size of the out buffer? */
    if (!out) {
        *outlen = inlen + EVP_MAX_BLOCK_LENGTH;
        return APR_SUCCESS;
    }

    /* must we allocate the output buffer from a pool? */
    if (!(*out)) {
        buffer = apr_palloc(ctx->pool, inlen + EVP_MAX_BLOCK_LENGTH);
        if (!buffer) {
            return APR_ENOMEM;
        }
        *out = buffer;
    }

#if CRYPT_OPENSSL_CONST_BUFFERS
    if (!EVP_DecryptUpdate(&ctx->cipherCtx, *out, &outl, in, inlen)) {
#else
    if (!EVP_DecryptUpdate(&ctx->cipherCtx, *out, &outl, (unsigned char *) in,
            inlen)) {
#endif
        return APR_ECRYPT;
    }
    *outlen = outl;

    return APR_SUCCESS;

}

/**
 * @brief Decrypt final data block, write it to out.
 * @note If necessary the final block will be written out after being
 *       padded. Typically the final block will be written to the
 *       same buffer used by apr_evp_crypt, offset by the number of
 *       bytes returned as actually written by the apr_evp_crypt()
 *       call. After this call, the context is cleaned and can be
 *       reused by apr_env_encrypt_init() or apr_env_decrypt_init().
 * @param ctx The block context to use.
 * @param out Address of a buffer to which data will be written. This
 *            buffer must already exist, and is usually the same
 *            buffer used by apr_evp_crypt(). See note.
 * @param outlen Length of the output will be written here.
 * @return APR_ECRYPT if an error occurred.
 * @return APR_EPADDING if padding was enabled and the block was incorrectly
 *         formatted.
 * @return APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_decrypt_finish(apr_crypto_block_t *ctx,
        unsigned char *out, apr_size_t *outlen) {

    int len = *outlen;

    if (EVP_DecryptFinal_ex(&ctx->cipherCtx, out, &len) == 0) {
        return APR_EPADDING;
    }
    *outlen = len;

    return APR_SUCCESS;

}

/**
 * OpenSSL module.
 */
APU_MODULE_DECLARE_DATA const apr_crypto_driver_t apr_crypto_openssl_driver = {
        "openssl", crypto_init, crypto_factory, crypto_passphrase,
        crypto_block_encrypt_init, crypto_block_encrypt,
        crypto_block_encrypt_finish, crypto_block_decrypt_init,
        crypto_block_decrypt, crypto_block_decrypt_finish,
        crypto_block_cleanup, crypto_cleanup, crypto_shutdown };

#endif
