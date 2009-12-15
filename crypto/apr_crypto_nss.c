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
#include <stdlib.h>

#include "apr_strings.h"
#include "apr_time.h"
#include "apr_buckets.h"

#include "apr_crypto_internal.h"

#if APU_HAVE_CRYPTO

#include <prerror.h>

#ifdef HAVE_NSS_NSS_H
#include <nss/nss.h>
#endif
#ifdef HAVE_NSS_H
#include <nss.h>
#endif

#ifdef HAVE_NSS_PK11PUB_H
#include <nss/pk11pub.h>
#endif
#ifdef HAVE_PK11PUB_H
#include <pk11pub.h>
#endif

struct apr_crypto_t {
    apr_pool_t *pool;
    apu_err_t *result;
    apr_array_header_t *keys;
    apr_crypto_config_t *config;
};

struct apr_crypto_config_t {
};

struct apr_crypto_key_t {
    CK_MECHANISM_TYPE cipherMech;
    SECOidTag cipherOid;
    PK11SymKey *symKey;
    int ivSize;
};

struct apr_crypto_block_t {
    const apr_crypto_t *f;
    apr_pool_t *pool;
    PK11Context *ctx;
    apr_crypto_key_t *key;
    int blockSize;
};

/**
 * Fetch the most recent error from this driver.
 */
static apr_status_t crypto_error(const apr_crypto_t *f, const apu_err_t **result) {
	*result = f->result;
	return APR_SUCCESS;
}

/**
 * Shutdown the crypto library and release resources.
 *
 * It is safe to shut down twice.
 */
static apr_status_t crypto_shutdown(apr_pool_t *pool)
{
    if (NSS_IsInitialized()) {
        SECStatus s = NSS_Shutdown();
        if (s != SECSuccess) {
            return APR_EINIT;
        }
    }
    return APR_SUCCESS;
}

static apr_status_t crypto_shutdown_helper(void *data)
{
    apr_pool_t *pool = (apr_pool_t *) data;
    return crypto_shutdown(pool);
}

/**
 * Initialise the crypto library and perform one time initialisation.
 */
static apr_status_t crypto_init(apr_pool_t *pool, const apr_array_header_t *params, int *rc)
{
    SECStatus s;
    const char *dir = NULL;
    const char *keyPrefix = NULL;
    const char *certPrefix = NULL;
    const char *secmod = NULL;
    PRUint32 flags = 0;
    struct apr_crypto_param_t *ents = params ? (struct apr_crypto_param_t *)params->elts : NULL;
    int i = 0;

    /* sanity check - we can only initialise NSS once */
    if (NSS_IsInitialized()) {
        return APR_EREINIT;
    }

    apr_pool_cleanup_register(pool, pool,
                              crypto_shutdown_helper,
                              apr_pool_cleanup_null);

    for (i = 0; params && i < params->nelts; i++) {
        switch (ents[i].type) {
        case APR_CRYPTO_CA_TYPE_DIR:
            dir = ents[i].path;
            break;
        case APR_CRYPTO_CERT_TYPE_KEY3_DB:
            keyPrefix = ents[i].path;
            break;
        case APR_CRYPTO_CA_TYPE_CERT7_DB:
            certPrefix = ents[i].path;
            break;
        case APR_CRYPTO_CA_TYPE_SECMOD:
            secmod = ents[i].path;
            break;
        default:
            return APR_EINIT;
        }
    }

    if (keyPrefix || certPrefix || secmod) {
        s = NSS_Initialize(dir, certPrefix, keyPrefix, secmod, flags);
    }
    else if (dir) {
        s = NSS_InitReadWrite(dir);
    }
    else {
        s = NSS_NoDB_Init(NULL);
    }
    if (s != SECSuccess) {
        if (rc) {
            *rc = PR_GetError();
        }
        return APR_ECRYPT;
    }

    return APR_SUCCESS;

}

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param driver - driver to use
 * @param ctx The block context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
static apr_status_t crypto_block_cleanup(apr_crypto_block_t *block)
{

    if (block->ctx) {
        PK11_DestroyContext(block->ctx, PR_TRUE);
        block->ctx = NULL;
    }

    return APR_SUCCESS;

}

static apr_status_t crypto_block_cleanup_helper(void *data)
{
    apr_crypto_block_t *block = (apr_crypto_block_t *) data;
    return crypto_block_cleanup(block);
}

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param driver - driver to use
 * @param f The context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
static apr_status_t crypto_cleanup(apr_crypto_t *f)
{
    apr_crypto_key_t *key;
    if (f->keys) {
        while ((key = apr_array_pop(f->keys))) {
            if (key->symKey) {
                PK11_FreeSymKey(key->symKey);
                key->symKey = NULL;
            }
        }
    }
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
 * @param driver - driver to use
 * @param pool - process pool
 * @param params - array of key parameters
 * @param context - context pointer will be written here
 * @return APR_ENOENGINE when the engine specified does not exist. APR_EINITENGINE
 * if the engine cannot be initialised.
 */
static apr_status_t crypto_make(apr_pool_t *pool,
                                const apr_array_header_t *params,
                                apr_crypto_t **ff)
{
    apr_crypto_config_t *config = NULL;
    /* struct apr_crypto_param_t *ents = params ? (struct apr_crypto_param_t *)params->elts : NULL; */
    /* int i = 0; */
    apr_crypto_t *f;

    f = apr_pcalloc(pool, sizeof(apr_crypto_t));
    if (!f) {
        return APR_ENOMEM;
    }
    *ff = f;
    f->pool = pool;
    config = f->config = apr_pcalloc(pool, sizeof(apr_crypto_config_t));
    if (!config) {
        return APR_ENOMEM;
    }
    f->result = apr_pcalloc(pool, sizeof(apu_err_t));
    if (!f->result) {
        return APR_ENOMEM;
    }
    f->keys = apr_array_make(pool,
                             10, sizeof(apr_crypto_key_t));

    apr_pool_cleanup_register(pool, f,
                              crypto_cleanup_helper,
                              apr_pool_cleanup_null);

    /*
    for (i = 0; params && i < params->nelts; i++) {
        switch (ents[i].type) {
        default:
            f->result->rc = -1;
            f->result->reason = "The NSS module currently supports "
                "no per context initialisation parameters at this time, but "
                "may do in future.";
            return APR_EINIT;
        }
    }
    */

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
static apr_status_t crypto_passphrase(apr_pool_t *p,
                                      const apr_crypto_t *f,
                                      const char *pass,
                                      apr_size_t passLen,
                                      const unsigned char * salt,
                                      apr_size_t saltLen,
                                      const apr_crypto_block_key_type_e type,
                                      const apr_crypto_block_key_mode_e mode,
                                      const int doPad,
                                      const int iterations,
                                      apr_crypto_key_t **k,
                                      apr_size_t *ivSize)
{
    apr_status_t rv = APR_SUCCESS;
    PK11SlotInfo * slot;
    SECItem passItem;
    SECItem saltItem;
    SECAlgorithmID *algid;
    void *wincx = NULL; /* what is wincx? */
    apr_crypto_key_t *key = *k;

    if (!key) {
        *k = key = apr_array_push(f->keys);
    }
    if (!key) {
        return APR_ENOMEM;
    }

    /* decide on what cipher mechanism we will be using */
    switch (type) {

    case (KEY_3DES_192) :
        if (MODE_CBC == mode) {
            key->cipherOid = SEC_OID_DES_EDE3_CBC;
        }
        else if (MODE_ECB == mode) {
            return APR_ENOCIPHER;
            /* No OID for CKM_DES3_ECB; */
        }
        break;
    case (KEY_AES_128) :
        if (MODE_CBC == mode) {
            key->cipherOid = SEC_OID_AES_128_CBC;
        }
        else {
            key->cipherOid = SEC_OID_AES_128_ECB;
        }
        break;
    case (KEY_AES_192) :
        if (MODE_CBC == mode) {
            key->cipherOid = SEC_OID_AES_192_CBC;
        }
        else {
            key->cipherOid = SEC_OID_AES_192_ECB;
        }
        break;
    case (KEY_AES_256) :
        if (MODE_CBC == mode) {
            key->cipherOid = SEC_OID_AES_256_CBC;
        }
        else {
            key->cipherOid = SEC_OID_AES_256_ECB;
        }
        break;
    default:
        /* unknown key type, give up */
        return APR_EKEYTYPE;
    }

    /* AES_128_CBC --> CKM_AES_CBC --> CKM_AES_CBC_PAD */
    key->cipherMech = PK11_AlgtagToMechanism(key->cipherOid);
    if (key->cipherMech == CKM_INVALID_MECHANISM) {
        return APR_ENOCIPHER;
    }
    if (doPad) {
        CK_MECHANISM_TYPE paddedMech;
        paddedMech = PK11_GetPadMechanism(key->cipherMech);
        if (CKM_INVALID_MECHANISM == paddedMech || key->cipherMech == paddedMech) {
            return APR_EPADDING;
        }
        key->cipherMech = paddedMech;
    }

    /* Turn the raw passphrase and salt into SECItems */
    passItem.data = (unsigned char*)pass;
    passItem.len = passLen;
    saltItem.data = (unsigned char*)salt;
    saltItem.len = saltLen;

    /* generate the key */
    /* pbeAlg and cipherAlg are the same. NSS decides the keylength. */
    algid = PK11_CreatePBEV2AlgorithmID(key->cipherOid, key->cipherOid, SEC_OID_HMAC_SHA1, 0, iterations, &saltItem);
    if (algid) {
        slot = PK11_GetBestSlot(key->cipherMech, wincx);
        if (slot) {
            key->symKey = PK11_PBEKeyGen(slot, algid, &passItem, PR_FALSE, wincx);
            PK11_FreeSlot(slot);
        }
        SECOID_DestroyAlgorithmID(algid, PR_TRUE);
    }

    /* sanity check? */
    if (!key->symKey) {
        PRErrorCode perr = PORT_GetError();
        if (perr) {
            f->result->rc = perr;
            f->result->msg = PR_ErrorToName(perr);
            rv = APR_ENOKEY;
        }
    }

    key->ivSize = PK11_GetIVLength(key->cipherMech);
    if (ivSize) {
        *ivSize = key->ivSize;
    }

    return rv;
}

/**
 * @brief Initialise a context for encrypting arbitrary data using the given key.
 * @note If *ctx is NULL, a apr_crypto_block_t will be created from a pool. If
 *       *ctx is not NULL, *ctx must point at a previously created structure.
 * @param p The pool to use.
 * @param f The block context to use.
 * @param key The key structure.
 * @param iv Optional initialisation vector. If the buffer pointed to is NULL,
 *           an IV will be created at random, in space allocated from the pool.
 *           If the buffer pointed to is not NULL, the IV in the buffer will be
 *           used.
 * @param ctx The block context returned, see note.
 * @param blockSize The block size of the cipher.
 * @return Returns APR_ENOIV if an initialisation vector is required but not specified.
 *         Returns APR_EINIT if the backend failed to initialise the context. Returns
 *         APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_encrypt_init(apr_pool_t *p,
                                              const apr_crypto_t *f,
                                              const apr_crypto_key_t *key,
                                              const unsigned char **iv,
                                              apr_crypto_block_t **ctx,
                                              apr_size_t *blockSize)
{
    PRErrorCode perr;
    SECItem * secParam;
    SECItem ivItem;
    unsigned char * usedIv;
    apr_crypto_block_t *block = *ctx;
    if (!block) {
        *ctx = block = apr_pcalloc(p, sizeof(apr_crypto_block_t));
    }
    if (!block) {
        return APR_ENOMEM;
    }
    block->f = f;
    block->pool = p;

    apr_pool_cleanup_register(p, block,
                              crypto_block_cleanup_helper,
                              apr_pool_cleanup_null);

    if (key->ivSize) {
        if (iv == NULL) {
            return APR_ENOIV;
        }
        if (*iv == NULL) {
            usedIv = apr_pcalloc(p, key->ivSize);
            if (!usedIv) {
                return APR_ENOMEM;
            }
            SECStatus s = PK11_GenerateRandom(usedIv, key->ivSize);
            if (s != SECSuccess) {
                return APR_ENOIV;
            }
            *iv = usedIv;
        }
        else {
             usedIv = (unsigned char *)*iv;
        }
        ivItem.data = usedIv;
        ivItem.len = key->ivSize;
        secParam = PK11_ParamFromIV(key->cipherMech, &ivItem);
    }
    else {
        secParam = PK11_GenerateNewParam(key->cipherMech, key->symKey);
    }
    block->blockSize = PK11_GetBlockSize(key->cipherMech, secParam);
    block->ctx = PK11_CreateContextBySymKey(key->cipherMech, CKA_ENCRYPT, key->symKey, secParam);

    /* did an error occur? */
    perr = PORT_GetError();
    if (perr || !block->ctx) {
        f->result->rc = perr;
        f->result->msg = PR_ErrorToName(perr);
        return APR_EINIT;
    }

    if (blockSize) {
    *blockSize = PK11_GetBlockSize(key->cipherMech, secParam);
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
static apr_status_t crypto_block_encrypt(apr_crypto_block_t *block,
                                         unsigned char **out,
                                         apr_size_t *outlen,
                                         const unsigned char *in,
                                         apr_size_t inlen)
{

    unsigned char *buffer;
    int outl = (int) *outlen;
    if (!out) {
        *outlen = inlen + block->blockSize;
        return APR_SUCCESS;
    }
    if (!*out) {
        buffer = apr_palloc(block->pool, inlen + block->blockSize);
        if (!buffer) {
            return APR_ENOMEM;
        }
        *out = buffer;
    }

    SECStatus s = PK11_CipherOp(block->ctx, *out, &outl, inlen, (unsigned char*)in, inlen);
    if (s != SECSuccess) {
        PRErrorCode perr = PORT_GetError();
        if (perr) {
            block->f->result->rc = perr;
            block->f->result->msg = PR_ErrorToName(perr);
        }
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
static apr_status_t crypto_block_encrypt_finish(apr_crypto_block_t *block,
                                                unsigned char *out,
                                                apr_size_t *outlen)
{

    apr_status_t rv = APR_SUCCESS;
    unsigned int outl = *outlen;

    SECStatus s = PK11_DigestFinal(block->ctx, out, &outl, block->blockSize);
    *outlen = outl;

    if (s != SECSuccess) {
        PRErrorCode perr = PORT_GetError();
        if (perr) {
            block->f->result->rc = perr;
            block->f->result->msg = PR_ErrorToName(perr);
        }
        rv = APR_ECRYPT;
    }
    crypto_block_cleanup(block);

    return rv;

}

/**
 * @brief Initialise a context for decrypting arbitrary data using the given key.
 * @note If *ctx is NULL, a apr_crypto_block_t will be created from a pool. If
 *       *ctx is not NULL, *ctx must point at a previously created structure.
 * @param p The pool to use.
 * @param f The block context to use.
 * @param key The key structure.
 * @param iv Optional initialisation vector. If the buffer pointed to is NULL,
 *           an IV will be created at random, in space allocated from the pool.
 *           If the buffer pointed to is not NULL, the IV in the buffer will be
 *           used.
 * @param ctx The block context returned, see note.
 * @param blockSize The block size of the cipher.
 * @return Returns APR_ENOIV if an initialisation vector is required but not specified.
 *         Returns APR_EINIT if the backend failed to initialise the context. Returns
 *         APR_ENOTIMPL if not implemented.
 */
static apr_status_t crypto_block_decrypt_init(apr_pool_t *p,
                                              const apr_crypto_t *f,
                                              const apr_crypto_key_t *key,
                                              const unsigned char *iv,
                                              apr_crypto_block_t **ctx,
                                              apr_size_t *blockSize)
{
    PRErrorCode perr;
    SECItem * secParam;
    apr_crypto_block_t *block = *ctx;
    if (!block) {
        *ctx = block = apr_pcalloc(p, sizeof(apr_crypto_block_t));
    }
    if (!block) {
        return APR_ENOMEM;
    }
    block->f = f;
    block->pool = p;

    apr_pool_cleanup_register(p, block,
                              crypto_block_cleanup_helper,
                              apr_pool_cleanup_null);

    if (key->ivSize) {
        SECItem ivItem;
        if (iv == NULL) {
            return APR_ENOIV; /* Cannot initialise without an IV */
        }
        ivItem.data = (unsigned char*)iv;
        ivItem.len = key->ivSize;
        secParam = PK11_ParamFromIV(key->cipherMech, &ivItem);
    }
    else {
        secParam = PK11_GenerateNewParam(key->cipherMech, key->symKey);
    }
    block->blockSize = PK11_GetBlockSize(key->cipherMech, secParam);
    block->ctx = PK11_CreateContextBySymKey(key->cipherMech, CKA_DECRYPT, key->symKey, secParam);

    /* did an error occur? */
    perr = PORT_GetError();
    if (perr || !block->ctx) {
        f->result->rc = perr;
        f->result->msg = PR_ErrorToName(perr);
        return APR_EINIT;
    }

    if (blockSize) {
        *blockSize = PK11_GetBlockSize(key->cipherMech, secParam);
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
static apr_status_t crypto_block_decrypt(apr_crypto_block_t *block,
                                         unsigned char **out,
                                         apr_size_t *outlen,
                                         const unsigned char *in,
                                         apr_size_t inlen)
{

    unsigned char *buffer;
    int outl = (int) *outlen;
    if (!out) {
        *outlen = inlen + block->blockSize;
        return APR_SUCCESS;
    }
    if (!*out) {
        buffer = apr_palloc(block->pool, inlen + block->blockSize);
        if (!buffer) {
            return APR_ENOMEM;
        }
        *out = buffer;
    }

    SECStatus s = PK11_CipherOp(block->ctx, *out, &outl, inlen, (unsigned char*)in, inlen);
    if (s != SECSuccess) {
        PRErrorCode perr = PORT_GetError();
        if (perr) {
            block->f->result->rc = perr;
            block->f->result->msg = PR_ErrorToName(perr);
        }
        return APR_ECRYPT;
    }
    *outlen = outl;

    return APR_SUCCESS;

}

/**
 * @brief Encrypt final data block, write it to out.
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
static apr_status_t crypto_block_decrypt_finish(apr_crypto_block_t *block,
                                                unsigned char *out,
                                                apr_size_t *outlen)
{

    apr_status_t rv = APR_SUCCESS;
    unsigned int outl = *outlen;

    SECStatus s = PK11_DigestFinal(block->ctx, out, &outl, block->blockSize);
    *outlen = outl;

    if (s != SECSuccess) {
        PRErrorCode perr = PORT_GetError();
        if (perr) {
            block->f->result->rc = perr;
            block->f->result->msg = PR_ErrorToName(perr);
        }
        rv = APR_ECRYPT;
    }
    crypto_block_cleanup(block);

    return rv;

}

/**
 * OpenSSL module.
 */
APU_MODULE_DECLARE_DATA const apr_crypto_driver_t apr_crypto_nss_driver = {
    "nss",
    crypto_init,
    crypto_error,
    crypto_make,
    crypto_passphrase,
    crypto_block_encrypt_init,
    crypto_block_encrypt,
    crypto_block_encrypt_finish,
    crypto_block_decrypt_init,
    crypto_block_decrypt,
    crypto_block_decrypt_finish,
    crypto_block_cleanup,
    crypto_cleanup,
    crypto_shutdown
};

#endif
