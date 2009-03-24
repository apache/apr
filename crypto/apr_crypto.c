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

#include <ctype.h>
#include <stdio.h>

#include "apu_config.h"
#include "apu.h"

#include "apr_pools.h"
#include "apr_dso.h"
#include "apr_strings.h"
#include "apr_hash.h"
#include "apr_thread_mutex.h"
#include "apr_lib.h"

#if APU_HAVE_CRYPTO

#include "apu_internal.h"
#include "apr_crypto_internal.h"
#include "apr_crypto.h"
#include "apu_version.h"

static apr_hash_t *drivers = NULL;

#define ERROR_SIZE 1024

#define CLEANUP_CAST (apr_status_t (*)(void*))

#if !APU_DSO_BUILD
#define DRIVER_LOAD(name,driver,pool,params) \
    {   \
        extern const apr_crypto_driver_t driver; \
        apr_hash_set(drivers,name,APR_HASH_KEY_STRING,&driver); \
        if (driver.init) {     \
            driver.init(pool, params); \
        }  \
    }
#endif

static apr_status_t apr_crypto_term(void *ptr) {
    /* set drivers to NULL so init can work again */
    drivers = NULL;

    /* Everything else we need is handled by cleanups registered
     * when we created mutexes and loaded DSOs
     */
    return APR_SUCCESS;
}

APU_DECLARE(apr_status_t) apr_crypto_init(apr_pool_t *pool,
        const apr_array_header_t *params) {
    apr_status_t ret = APR_SUCCESS;
    apr_pool_t *parent;

    if (drivers != NULL) {
        return APR_SUCCESS;
    }

    /* Top level pool scope, need process-scope lifetime */
    for (parent = pool; parent; parent = apr_pool_parent_get(pool))
        pool = parent;
#if APU_DSO_BUILD
    /* deprecate in 2.0 - permit implicit initialization */
    apu_dso_init(pool);
#endif
    drivers = apr_hash_make(pool);

#if !APU_DSO_BUILD
    /* Load statically-linked drivers: */
#if APU_HAVE_OPENSSL
    DRIVER_LOAD("openssl", apr_crypto_openssl_driver, pool, params);
#endif
#if APU_HAVE_NSS
    DRIVER_LOAD("nss", apr_crypto_nss_driver, pool, params);
#endif
#if APU_HAVE_MSCAPI
    DRIVER_LOAD("mscapi", apr_crypto_mscapi_driver, pool, params);
#endif
#if APU_HAVE_MSCNG
    DRIVER_LOAD("mscng", apr_crypto_mscng_driver, pool, params);
#endif
#endif /* APU_DSO_BUILD */

    apr_pool_cleanup_register(pool, NULL, apr_crypto_term,
            apr_pool_cleanup_null);

    return ret;
}

APU_DECLARE(apr_status_t) apr_crypto_get_driver(apr_pool_t *pool, const char *name,
        const apr_crypto_driver_t **driver, const apr_array_header_t *params,
        const apu_err_t **result) {
#if APU_DSO_BUILD
    char modname[32];
    char symname[34];
    apr_dso_handle_t *dso;
    apr_dso_handle_sym_t symbol;
#endif
    apr_status_t rv;
    int rc = 0;

#if APU_DSO_BUILD
    rv = apu_dso_mutex_lock();
    if (rv) {
        return rv;
    }
#endif
    *driver = apr_hash_get(drivers, name, APR_HASH_KEY_STRING);
    if (*driver) {
#if APU_DSO_BUILD
        apu_dso_mutex_unlock();
#endif
        return APR_SUCCESS;
    }

#if APU_DSO_BUILD
    /* The driver DSO must have exactly the same lifetime as the
     * drivers hash table; ignore the passed-in pool */
    pool = apr_hash_pool_get(drivers);

#if defined(NETWARE)
    apr_snprintf(modname, sizeof(modname), "crypto%s.nlm", name);
#elif defined(WIN32)
    apr_snprintf(modname, sizeof(modname),
            "apr_crypto_%s-" APU_STRINGIFY(APU_MAJOR_VERSION) ".dll", name);
#else
    apr_snprintf(modname, sizeof(modname), "apr_crypto_%s-" APU_STRINGIFY(APU_MAJOR_VERSION) ".so", name);
#endif
    apr_snprintf(symname, sizeof(symname), "apr_crypto_%s_driver", name);
    rv = apu_dso_load(&dso, &symbol, modname, symname, pool);
    if (rv != APR_SUCCESS) { /* APR_EDSOOPEN or APR_ESYMNOTFOUND? */
        if (rv == APR_EINIT) { /* previously loaded?!? */
            name = apr_pstrdup(pool, name);
            apr_hash_set(drivers, name, APR_HASH_KEY_STRING, *driver);
            rv = APR_SUCCESS;
        }
        goto unlock;
    }
    *driver = symbol;
    if ((*driver)->init) {
        rv = (*driver)->init(pool, params, &rc);
    }
    name = apr_pstrdup(pool, name);
    apr_hash_set(drivers, name, APR_HASH_KEY_STRING, *driver);

    unlock: apu_dso_mutex_unlock();

    if (APR_SUCCESS != rv && result) {
        char *buffer = apr_pcalloc(pool, ERROR_SIZE);
        apu_err_t *err = apr_pcalloc(pool, sizeof(apu_err_t));
        if (err && buffer) {
            apr_dso_error(dso, buffer, ERROR_SIZE - 1);
            err->msg = buffer;
            err->reason = modname;
            err->rc = rc;
            *result = err;
        }
    }

#else /* not builtin and !APR_HAS_DSO => not implemented */
    rv = APR_ENOTIMPL;
#endif

    return rv;
}

/**
 * @brief Return the name of the driver.
 *
 * @param pool - pool to register any shutdown cleanups, etc
 * @return APR_SUCCESS for success.
 */
APU_DECLARE(const char *)apr_crypto_driver_name (const apr_crypto_driver_t *driver)
{
    return driver->name;
}

/**
 * @brief Get the result of a previous operation on this context.
 * @param pool - process pool
 * @param params - array of key parameters
 * @param factory - factory pointer will be written here
 */
APU_DECLARE(apr_status_t) apr_crypto_error(const apr_crypto_t *f,
        const apu_err_t **result) {
    *result = f->result;
    return APR_SUCCESS;
}

/**
 * @brief Create a general encryption context
 * @param driver - driver to use
 * @param pool - process pool
 * @param params - array of key parameters
 * @param factory - factory pointer will be written here
 */
APU_DECLARE(apr_status_t) apr_crypto_factory(const apr_crypto_driver_t *driver,
        apr_pool_t *pool, const apr_array_header_t *params, apr_crypto_t **f) {
    return driver->factory(pool, params, f);
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
APU_DECLARE(apr_status_t) apr_crypto_passphrase(const apr_crypto_driver_t *driver,
        apr_pool_t *p, const apr_crypto_t *f, const char *pass,
        apr_size_t passLen, const unsigned char * salt, apr_size_t saltLen,
        const apr_crypto_block_key_type_e type,
        const apr_crypto_block_key_mode_e mode, const int doPad,
        const int iterations, apr_crypto_key_t **key, apr_size_t *ivSize) {
    return driver->passphrase(p, f, pass, passLen, salt, saltLen, type, mode,
            doPad, iterations, key, ivSize);
}

/**
 * @brief Initialise a context for encrypting arbitrary data using the given key.
 * @note If *ctx is NULL, a apr_crypto_block_t will be created from a pool. If
 *       *ctx is not NULL, *ctx must point at a previously created structure.
 * @param driver - driver to use
 * @param p The pool to use.
 * @param f The block factory to use.
 * @param key The key structure to use.
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
APU_DECLARE(apr_status_t) apr_crypto_block_encrypt_init(
        const apr_crypto_driver_t *driver, apr_pool_t *p,
        const apr_crypto_t *f, const apr_crypto_key_t *key,
        const unsigned char **iv, apr_crypto_block_t **ctx,
        apr_size_t *blockSize) {
    return driver->block_encrypt_init(p, f, key, iv, ctx, blockSize);
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
 * @param driver - driver to use
 * @param ctx The block context to use.
 * @param out Address of a buffer to which data will be written,
 *        see note.
 * @param outlen Length of the output will be written here.
 * @param in Address of the buffer to read.
 * @param inlen Length of the buffer to read.
 * @return APR_ECRYPT if an error occurred. Returns APR_ENOTIMPL if
 *         not implemented.
 */
APU_DECLARE(apr_status_t) apr_crypto_block_encrypt(
        const apr_crypto_driver_t *driver, apr_crypto_block_t *ctx,
        unsigned char **out, apr_size_t *outlen, const unsigned char *in,
        apr_size_t inlen) {
    return driver->block_encrypt(ctx, out, outlen, in, inlen);
}

/**
 * @brief Encrypt final data block, write it to out.
 * @note If necessary the final block will be written out after being
 *       padded. Typically the final block will be written to the
 *       same buffer used by apr_crypto_block_encrypt, offset by the
 *       number of bytes returned as actually written by the
 *       apr_crypto_block_encrypt() call. After this call, the context
 *       is cleaned and can be reused by apr_crypto_block_encrypt_init().
 * @param driver - driver to use
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
APU_DECLARE(apr_status_t) apr_crypto_block_encrypt_finish(
        const apr_crypto_driver_t *driver, apr_crypto_block_t *ctx,
        unsigned char *out, apr_size_t *outlen) {
    return driver->block_encrypt_finish(ctx, out, outlen);
}

/**
 * @brief Initialise a context for decrypting arbitrary data using the given key.
 * @note If *ctx is NULL, a apr_crypto_block_t will be created from a pool. If
 *       *ctx is not NULL, *ctx must point at a previously created structure.
 * @param driver - driver to use
 * @param p The pool to use.
 * @param f The block factory to use.
 * @param key The key structure to use.
 * @param iv Optional initialisation vector.
 * @param ctx The block context returned, see note.
 * @param blockSize The block size of the cipher.
 * @return Returns APR_ENOIV if an initialisation vector is required but not specified.
 *         Returns APR_EINIT if the backend failed to initialise the context. Returns
 *         APR_ENOTIMPL if not implemented.
 */
APU_DECLARE(apr_status_t) apr_crypto_block_decrypt_init(
        const apr_crypto_driver_t *driver, apr_pool_t *p,
        const apr_crypto_t *f, const apr_crypto_key_t *key,
        const unsigned char *iv, apr_crypto_block_t **ctx,
        apr_size_t *blockSize) {
    return driver->block_decrypt_init(p, f, key, iv, ctx, blockSize);
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
 * @param driver - driver to use
 * @param ctx The block context to use.
 * @param out Address of a buffer to which data will be written,
 *        see note.
 * @param outlen Length of the output will be written here.
 * @param in Address of the buffer to read.
 * @param inlen Length of the buffer to read.
 * @return APR_ECRYPT if an error occurred. Returns APR_ENOTIMPL if
 *         not implemented.
 */
APU_DECLARE(apr_status_t) apr_crypto_block_decrypt(
        const apr_crypto_driver_t *driver, apr_crypto_block_t *ctx,
        unsigned char **out, apr_size_t *outlen, const unsigned char *in,
        apr_size_t inlen) {
    return driver->block_decrypt(ctx, out, outlen, in, inlen);
}

/**
 * @brief Decrypt final data block, write it to out.
 * @note If necessary the final block will be written out after being
 *       padded. Typically the final block will be written to the
 *       same buffer used by apr_crypto_block_decrypt, offset by the
 *       number of bytes returned as actually written by the
 *       apr_crypto_block_decrypt() call. After this call, the context
 *       is cleaned and can be reused by apr_crypto_block_decrypt_init().
 * @param driver - driver to use
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
APU_DECLARE(apr_status_t) apr_crypto_block_decrypt_finish(
        const apr_crypto_driver_t *driver, apr_crypto_block_t *ctx,
        unsigned char *out, apr_size_t *outlen) {
    return driver->block_decrypt_finish(ctx, out, outlen);
}

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param driver - driver to use
 * @param ctx The block context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APU_DECLARE(apr_status_t) apr_crypto_block_cleanup(
        const apr_crypto_driver_t *driver, apr_crypto_block_t *ctx) {
    return driver->block_cleanup(ctx);
}

/**
 * @brief Clean encryption / decryption factory.
 * @note After cleanup, a factory is free to be reused if necessary.
 * @param driver - driver to use
 * @param f The factory to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APU_DECLARE(apr_status_t) apr_crypto_cleanup(const apr_crypto_driver_t *driver,
        apr_crypto_t *f) {
    return driver->cleanup(f);
}

/**
 * @brief Shutdown the crypto library.
 * @note After shutdown, it is expected that the init function can be called again.
 * @param driver - driver to use
 * @param p The pool to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APU_DECLARE(apr_status_t) apr_crypto_shutdown(const apr_crypto_driver_t *driver,
        apr_pool_t *p) {
    return driver->shutdown(p);
}

#endif /* APU_HAVE_CRYPTO */
