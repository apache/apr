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

#include "apu.h"
#include "apr_private.h"
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
#include "apr_version.h"

static apr_hash_t *drivers = NULL;

#define ERROR_SIZE 1024

APR_TYPEDEF_STRUCT(apr_crypto_t,
    apr_pool_t *pool;
    apr_crypto_driver_t *provider;
)

APR_TYPEDEF_STRUCT(apr_crypto_key_t,
    apr_pool_t *pool;
    apr_crypto_driver_t *provider;
    const apr_crypto_t *f;
)

APR_TYPEDEF_STRUCT(apr_crypto_block_t,
    apr_pool_t *pool;
    apr_crypto_driver_t *provider;
    const apr_crypto_t *f;
)

APR_TYPEDEF_STRUCT(apr_crypto_digest_t,
    apr_pool_t *pool;
    apr_crypto_driver_t *provider;
    const apr_crypto_t *f;
)

typedef struct apr_crypto_clear_t {
    void *buffer;
    apr_size_t size;
} apr_crypto_clear_t;

#if !APR_HAVE_MODULAR_DSO
#define DRIVER_LOAD(name,driver_name,pool,params,rv,result) \
    {   \
        extern const apr_crypto_driver_t driver_name; \
        apr_hash_set(drivers,name,APR_HASH_KEY_STRING,&driver_name); \
        if (driver_name.init) {     \
            rv = driver_name.init(pool, params, result); \
        }  \
        *driver = &driver_name; \
    }
#endif

static apr_status_t apr_crypto_term(void *ptr)
{
    /* set drivers to NULL so init can work again */
    drivers = NULL;

    /* Everything else we need is handled by cleanups registered
     * when we created mutexes and loaded DSOs
     */
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_init(apr_pool_t *pool)
{
    apr_pool_t *rootp;

    if (drivers != NULL) {
        return APR_SUCCESS;
    }

    /* Top level pool scope, for drivers' process-scope lifetime */
    rootp = pool;
    for (;;) {
        apr_pool_t *p = apr_pool_parent_get(rootp);
        if (!p || p == rootp) {
            break;
        }
        rootp = p;
    }
#if APR_HAVE_MODULAR_DSO
    /* deprecate in 2.0 - permit implicit initialization */
    apu_dso_init(rootp);
#endif
    drivers = apr_hash_make(rootp);
    apr_pool_cleanup_register(rootp, NULL, apr_crypto_term,
                              apr_pool_cleanup_null);

    return APR_SUCCESS;
}

static apr_status_t crypto_clear(void *ptr)
{
    apr_crypto_clear_t *clear = (apr_crypto_clear_t *)ptr;

    apr_crypto_memzero(clear->buffer, clear->size);
    clear->buffer = NULL;
    clear->size = 0;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_clear(apr_pool_t *pool,
        void *buffer, apr_size_t size)
{
    apr_crypto_clear_t *clear = apr_palloc(pool, sizeof(apr_crypto_clear_t));

    clear->buffer = buffer;
    clear->size = size;

    apr_pool_cleanup_register(pool, clear, crypto_clear,
            apr_pool_cleanup_null);

    return APR_SUCCESS;
}

#if defined(HAVE_WEAK_SYMBOLS)
void apr__memzero_explicit(void *buffer, apr_size_t size);

__attribute__ ((weak))
void apr__memzero_explicit(void *buffer, apr_size_t size)
{
    memset(buffer, 0, size);
}
#endif

APR_DECLARE(apr_status_t) apr_crypto_memzero(void *buffer, apr_size_t size)
{
#if defined(WIN32)
    SecureZeroMemory(buffer, size);
#elif defined(HAVE_MEMSET_S)
    if (size) {
        return memset_s(buffer, (rsize_t)size, 0, (rsize_t)size);
    }
#elif defined(HAVE_EXPLICIT_BZERO)
    explicit_bzero(buffer, size);
#elif defined(HAVE_WEAK_SYMBOLS)
    apr__memzero_explicit(buffer, size);
#else
    apr_size_t i;
    volatile unsigned char *volatile ptr = buffer;
    for (i = 0; i < size; ++i) {
        ptr[i] = 0;
    }
#endif
    return APR_SUCCESS;
}

APR_DECLARE(int) apr_crypto_equals(const void *buf1, const void *buf2,
                                   apr_size_t size)
{
    const unsigned char *p1 = buf1;
    const unsigned char *p2 = buf2;
    unsigned char diff = 0;
    apr_size_t i;

    for (i = 0; i < size; ++i) {
        diff |= p1[i] ^ p2[i];
    }

    return 1 & ((diff - 1) >> 8);
}

APR_DECLARE(apr_crypto_key_rec_t *) apr_crypto_key_rec_make(
        apr_crypto_key_type ktype, apr_pool_t *p)
{
    apr_crypto_key_rec_t *key = apr_pcalloc(p, sizeof(apr_crypto_key_rec_t));
    key->ktype = ktype;
    return key;
}

APR_DECLARE(apr_crypto_digest_rec_t *) apr_crypto_digest_rec_make(
         apr_crypto_digest_type_e dtype, apr_pool_t *p)
{
    apr_crypto_digest_rec_t *rec = apr_pcalloc(p, sizeof(apr_crypto_digest_rec_t));
    if (rec) {
        rec->dtype = dtype;
    }
    return rec;
}

APR_DECLARE(apr_status_t) apr_crypto_get_driver(
        const apr_crypto_driver_t **driver, const char *name,
        const char *params, const apu_err_t **result, apr_pool_t *pool)
{
#if APR_HAVE_MODULAR_DSO
    char modname[32];
    char symname[34];
    apr_dso_handle_t *dso;
    apr_dso_handle_sym_t symbol;
    apr_pool_t *rootp;
#endif
    apr_status_t rv;

    if (result) {
        *result = NULL; /* until further notice */
    }

#if APR_HAVE_MODULAR_DSO
    rv = apu_dso_mutex_lock();
    if (rv) {
        return rv;
    }
#endif
    *driver = apr_hash_get(drivers, name, APR_HASH_KEY_STRING);
    if (*driver) {
#if APR_HAVE_MODULAR_DSO
        apu_dso_mutex_unlock();
#endif
        return APR_SUCCESS;
    }

#if APR_HAVE_MODULAR_DSO
    /* The driver DSO must have exactly the same lifetime as the
     * drivers hash table; ignore the passed-in pool */
    rootp = apr_hash_pool_get(drivers);

#if defined(NETWARE)
    apr_snprintf(modname, sizeof(modname), "crypto%s.nlm", name);
#elif defined(WIN32) || defined(__CYGWIN__)
    apr_snprintf(modname, sizeof(modname),
            "apr_crypto_%s-" APR_STRINGIFY(APR_MAJOR_VERSION) ".dll", name);
#else
    apr_snprintf(modname, sizeof(modname),
            "apr_crypto_%s-" APR_STRINGIFY(APR_MAJOR_VERSION) ".so", name);
#endif
    apr_snprintf(symname, sizeof(symname), "apr_crypto_%s_driver", name);
    rv = apu_dso_load(&dso, &symbol, modname, symname, rootp);
    if (rv == APR_SUCCESS || rv == APR_EINIT) { /* previously loaded?!? */
        apr_crypto_driver_t *d = symbol;
        rv = APR_SUCCESS;
        if (d->init) {
            rv = d->init(rootp, params, result);
            if (rv == APR_EREINIT) {
                rv = APR_SUCCESS;
            }
        }
        if (rv == APR_SUCCESS) {
            apr_hash_set(drivers, d->name, APR_HASH_KEY_STRING, d);
            *driver = d;
        }
    }
    apu_dso_mutex_unlock();

    if (APR_SUCCESS != rv && result && !*result) {
        char *buffer = apr_pcalloc(pool, ERROR_SIZE);
        apu_err_t *err = apr_pcalloc(pool, sizeof(apu_err_t));
        if (err && buffer) {
            apr_dso_error(dso, buffer, ERROR_SIZE - 1);
            err->msg = buffer;
            err->reason = apr_pstrdup(pool, modname);
            *result = err;
        }
    }

#else /* not builtin and !APR_HAS_DSO => not implemented */
    rv = APR_ENOTIMPL;

    /* Load statically-linked drivers: */
#if APU_HAVE_OPENSSL
    if (!strcmp(name, "openssl")) {
        DRIVER_LOAD("openssl", apr_crypto_openssl_driver, pool, params, rv, result);
    }
    else
#endif
#if APU_HAVE_NSS
    if (!strcmp(name, "nss")) {
        DRIVER_LOAD("nss", apr_crypto_nss_driver, pool, params, rv, result);
    }
    else
#endif
#if APU_HAVE_COMMONCRYPTO
    if (!strcmp(name, "commoncrypto")) {
        DRIVER_LOAD("commoncrypto", apr_crypto_commoncrypto_driver, pool, params, rv, result);
    }
    else
#endif
#if APU_HAVE_MSCAPI
    if (!strcmp(name, "mscapi")) {
        DRIVER_LOAD("mscapi", apr_crypto_mscapi_driver, pool, params, rv, result);
    }
    else
#endif
#if APU_HAVE_MSCNG
    if (!strcmp(name, "mscng")) {
        DRIVER_LOAD("mscng", apr_crypto_mscng_driver, pool, params, rv, result);
    }
    else
#endif
    ;

#endif /* !APR_HAVE_MODULAR_DSO */

    return rv;
}

struct crypto_lib {
    const char *name;
    apr_pool_t *pool;
    apr_status_t (*term)(void);
    struct crypto_lib *next;
};
static apr_hash_t *active_libs = NULL;
static struct crypto_lib *spare_libs = NULL;

static apr_status_t crypto_libs_cleanup(void *nil)
{
    active_libs = NULL;
    spare_libs = NULL;
    return APR_SUCCESS;
}

static void spare_lib_push(struct crypto_lib *lib)
{
    lib->name = NULL;
    lib->pool = NULL;
    lib->term = NULL;
    lib->next = spare_libs;
    spare_libs = lib;
}

static struct crypto_lib *spare_lib_pop(void)
{
    struct crypto_lib *lib;
    lib = spare_libs;
    if (lib) {
        spare_libs = lib->next;
        lib->next = NULL;
    }
    return lib;
}

static apr_status_t crypto_lib_free(struct crypto_lib *lib)
{
    apr_status_t rv;

    apr_hash_set(active_libs, lib->name, APR_HASH_KEY_STRING, NULL);
    rv = lib->term();
    spare_lib_push(lib);

    return rv;
}

static apr_status_t crypto_lib_cleanup(void *arg)
{
    crypto_lib_free(arg);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_lib_version(const char *name,
                                                 const char **version)
{
    apr_status_t rv = APR_ENOTIMPL;

    return rv;
}

APR_DECLARE(apr_status_t) apr_crypto_lib_init(const char *name,
                                              const char *params,
                                              const apu_err_t **result,
                                              apr_pool_t *pool)
{
    apr_status_t rv;
    apr_pool_t *rootp;
    struct crypto_lib *lib;

    if (!name) {
        return APR_EINVAL;
    }

    if (apr_crypto_lib_is_active(name)) {
        return APR_EREINIT;
    }

    rootp = pool;
    for (;;) {
        apr_pool_t *p = apr_pool_parent_get(rootp);
        if (!p || p == rootp) {
            break;
        }
        rootp = p;
    }

    if (!active_libs) {
        active_libs = apr_hash_make(rootp);
        if (!active_libs) {
            return APR_ENOMEM;
        }
        apr_pool_cleanup_register(rootp, NULL, crypto_libs_cleanup,
                                  apr_pool_cleanup_null);
    }

    lib = spare_lib_pop();
    if (!lib) {
        lib = apr_pcalloc(rootp, sizeof(*lib));
        if (!lib) {
            return APR_ENOMEM;
        }
    }

    rv = APR_ENOTIMPL;
    ;
    if (rv == APR_SUCCESS) {
        lib->pool = pool;
        apr_hash_set(active_libs, lib->name, APR_HASH_KEY_STRING, lib);
        if (apr_pool_parent_get(pool)) {
            apr_pool_cleanup_register(pool, lib, crypto_lib_cleanup,
                                      apr_pool_cleanup_null);
        }
    }
    else {
        spare_lib_push(lib);
    }
    return rv;
}

static apr_status_t crypto_lib_term(const char *name)
{
    apr_status_t rv;
    struct crypto_lib *lib;

    lib = apr_hash_get(active_libs, name, APR_HASH_KEY_STRING);
    if (!lib) {
        return APR_EINIT;
    }
    if (!apr_pool_parent_get(lib->pool)) {
        return APR_EBUSY;
    }

    rv = APR_ENOTIMPL;
    ;
    if (rv == APR_SUCCESS) {
        apr_pool_cleanup_kill(lib->pool, lib, crypto_lib_cleanup);
        rv = crypto_lib_free(lib);
    }
    return rv;
}

APR_DECLARE(apr_status_t) apr_crypto_lib_term(const char *name)
{
    if (!active_libs) {
        return APR_EINIT;
    }

    if (!name) {
        apr_status_t rv = APR_SUCCESS;
        apr_hash_index_t *hi = apr_hash_first(NULL, active_libs);
        for (; hi; hi = apr_hash_next(hi)) {
            apr_status_t rt = crypto_lib_term(apr_hash_this_key(hi));
            if (rt != APR_SUCCESS && (rv == APR_SUCCESS || rv == APR_EBUSY)) {
                rv = rt;
            }
        }
        return rv;
    }

    return crypto_lib_term(name);
}

APR_DECLARE(int) apr_crypto_lib_is_active(const char *name)
{
    return active_libs && apr_hash_get(active_libs, name, APR_HASH_KEY_STRING);
}

/**
 * @brief Return the name of the driver.
 *
 * @param driver - The driver in use.
 * @return The name of the driver.
 */
APR_DECLARE(const char *)apr_crypto_driver_name(
        const apr_crypto_driver_t *driver)
{
    return driver->name;
}

/**
 * @brief Get the result of the last operation on a context. If the result
 *        is NULL, the operation was successful.
 * @param result - the result structure
 * @param f - context pointer
 * @return APR_SUCCESS for success
 */
APR_DECLARE(apr_status_t) apr_crypto_error(const apu_err_t **result,
        const apr_crypto_t *f)
{
    return f->provider->error(result, f);
}

/**
 * @brief Create a context for supporting encryption. Keys, certificates,
 *        algorithms and other parameters will be set per context. More than
 *        one context can be created at one time. A cleanup will be automatically
 *        registered with the given pool to guarantee a graceful shutdown.
 * @param f - context pointer will be written here
 * @param driver - driver to use
 * @param params - array of key parameters
 * @param pool - process pool
 * @return APR_ENOENGINE when the engine specified does not exist. APR_EINITENGINE
 * if the engine cannot be initialised.
 * @remarks NSS: currently no params are supported.
 * @remarks OpenSSL: the params can have "engine" as a key, followed by an equal
 *  sign and a value.
 */
APR_DECLARE(apr_status_t) apr_crypto_make(apr_crypto_t **f,
        const apr_crypto_driver_t *driver, const char *params, apr_pool_t *pool)
{
    return driver->make(f, driver, params, pool);
}

/**
 * @brief Get a hash table of digests, keyed by the name of the digest against
 * a pointer to apr_crypto_digest_t, which in turn begins with an
 * integer.
 *
 * @param digests - hashtable of digests keyed to constants.
 * @param f - encryption context
 * @return APR_SUCCESS for success
 */
APR_DECLARE(apr_status_t) apr_crypto_get_block_key_digests(apr_hash_t **digests,
        const apr_crypto_t *f)
{
    return f->provider->get_block_key_digests(digests, f);
}

/**
 * @brief Get a hash table of key types, keyed by the name of the type against
 * a pointer to apr_crypto_block_key_type_t, which in turn begins with an
 * integer.
 *
 * @param types - hashtable of key types keyed to constants.
 * @param f - encryption context
 * @return APR_SUCCESS for success
 */
APR_DECLARE(apr_status_t) apr_crypto_get_block_key_types(apr_hash_t **types,
        const apr_crypto_t *f)
{
    return f->provider->get_block_key_types(types, f);
}

/**
 * @brief Get a hash table of key modes, keyed by the name of the mode against
 * a pointer to apr_crypto_block_key_mode_t, which in turn begins with an
 * integer.
 *
 * @param modes - hashtable of key modes keyed to constants.
 * @param f - encryption context
 * @return APR_SUCCESS for success
 */
APR_DECLARE(apr_status_t) apr_crypto_get_block_key_modes(apr_hash_t **modes,
        const apr_crypto_t *f)
{
    return f->provider->get_block_key_modes(modes, f);
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
APR_DECLARE(apr_status_t) apr_crypto_key(apr_crypto_key_t **key,
        const apr_crypto_key_rec_t *rec, const apr_crypto_t *f, apr_pool_t *p)
{
    return f->provider->key(key, rec, f, p);
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
 * @param iterations Number of iterations to use in algorithm
 * @param f The context to use.
 * @param p The pool to use.
 * @return Returns APR_ENOKEY if the pass phrase is missing or empty, or if a backend
 *         error occurred while generating the key. APR_ENOCIPHER if the type or mode
 *         is not supported by the particular backend. APR_EKEYTYPE if the key type is
 *         not known. APR_EPADDING if padding was requested but is not supported.
 *         APR_ENOTIMPL if not implemented.
 */
APR_DECLARE(apr_status_t) apr_crypto_passphrase(apr_crypto_key_t **key,
        apr_size_t *ivSize, const char *pass, apr_size_t passLen,
        const unsigned char * salt, apr_size_t saltLen,
        const apr_crypto_block_key_type_e type,
        const apr_crypto_block_key_mode_e mode, const int doPad,
        const int iterations, const apr_crypto_t *f, apr_pool_t *p)
{
    return f->provider->passphrase(key, ivSize, pass, passLen, salt, saltLen,
            type, mode, doPad, iterations, f, p);
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
 * @param key The key structure to use.
 * @param blockSize The block size of the cipher.
 * @param p The pool to use.
 * @return Returns APR_ENOIV if an initialisation vector is required but not specified.
 *         Returns APR_EINIT if the backend failed to initialise the context. Returns
 *         APR_ENOTIMPL if not implemented.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_encrypt_init(
        apr_crypto_block_t **ctx, const unsigned char **iv,
        const apr_crypto_key_t *key, apr_size_t *blockSize, apr_pool_t *p)
{
    return key->provider->block_encrypt_init(ctx, iv, key, blockSize, p);
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
APR_DECLARE(apr_status_t) apr_crypto_block_encrypt(unsigned char **out,
        apr_size_t *outlen, const unsigned char *in, apr_size_t inlen,
        apr_crypto_block_t *ctx)
{
    return ctx->provider->block_encrypt(out, outlen, in, inlen, ctx);
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
APR_DECLARE(apr_status_t) apr_crypto_block_encrypt_finish(unsigned char *out,
        apr_size_t *outlen, apr_crypto_block_t *ctx)
{
    return ctx->provider->block_encrypt_finish(out, outlen, ctx);
}

/**
 * @brief Initialise a context for decrypting arbitrary data using the given key.
 * @note If *ctx is NULL, a apr_crypto_block_t will be created from a pool. If
 *       *ctx is not NULL, *ctx must point at a previously created structure.
 * @param ctx The block context returned, see note.
 * @param blockSize The block size of the cipher.
 * @param iv Optional initialisation vector.
 * @param key The key structure to use.
 * @param p The pool to use.
 * @return Returns APR_ENOIV if an initialisation vector is required but not specified.
 *         Returns APR_EINIT if the backend failed to initialise the context. Returns
 *         APR_ENOTIMPL if not implemented.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_decrypt_init(
        apr_crypto_block_t **ctx, apr_size_t *blockSize,
        const unsigned char *iv, const apr_crypto_key_t *key, apr_pool_t *p)
{
    return key->provider->block_decrypt_init(ctx, blockSize, iv, key, p);
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
APR_DECLARE(apr_status_t) apr_crypto_block_decrypt(unsigned char **out,
        apr_size_t *outlen, const unsigned char *in, apr_size_t inlen,
        apr_crypto_block_t *ctx)
{
    return ctx->provider->block_decrypt(out, outlen, in, inlen, ctx);
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
APR_DECLARE(apr_status_t) apr_crypto_block_decrypt_finish(unsigned char *out,
        apr_size_t *outlen, apr_crypto_block_t *ctx)
{
    return ctx->provider->block_decrypt_finish(out, outlen, ctx);
}

APR_DECLARE(apr_status_t) apr_crypto_digest_init(apr_crypto_digest_t **d,
        const apr_crypto_key_t *key, apr_crypto_digest_rec_t *rec, apr_pool_t *p)
{
    return key->provider->digest_init(d, key, rec, p);
}

APR_DECLARE(apr_status_t) apr_crypto_digest_update(apr_crypto_digest_t *digest,
        const unsigned char *in, apr_size_t inlen)
{
    return digest->provider->digest_update(digest, in, inlen);
}

APR_DECLARE(apr_status_t) apr_crypto_digest_final(apr_crypto_digest_t *digest)
{
    return digest->provider->digest_final(digest);
}

APR_DECLARE(apr_status_t) apr_crypto_digest(const apr_crypto_key_t *key,
        apr_crypto_digest_rec_t *rec, const unsigned char *in, apr_size_t inlen,
        apr_pool_t *p)
{
    return key->provider->digest(key, rec, in, inlen, p);
}

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param ctx The block context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_cleanup(apr_crypto_block_t *ctx)
{
    return ctx->provider->block_cleanup(ctx);
}

/**
 * @brief Clean sign / verify context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param ctx The digest context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APR_DECLARE(apr_status_t) apr_crypto_digest_cleanup(apr_crypto_digest_t *ctx)
{
    return ctx->provider->digest_cleanup(ctx);
}

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param f The context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APR_DECLARE(apr_status_t) apr_crypto_cleanup(apr_crypto_t *f)
{
    return f->provider->cleanup(f);
}

/**
 * @brief Shutdown the crypto library.
 * @note After shutdown, it is expected that the init function can be called again.
 * @param driver - driver to use
 * @return Returns APR_ENOTIMPL if not supported.
 */
APR_DECLARE(apr_status_t) apr_crypto_shutdown(const apr_crypto_driver_t *driver)
{
    return driver->shutdown();
}

#endif /* APU_HAVE_CRYPTO */
