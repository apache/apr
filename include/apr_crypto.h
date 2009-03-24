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

#ifndef APR_CRYPTO_H
#define APR_CRYPTO_H

#include "apu.h"
#include "apr_pools.h"
#include "apr_tables.h"
#include "apu_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file apr_crypto.h
 * @brief APR-UTIL Crypto library
 */
/**
 * @defgroup APR_Util_Crypto Crypto routines
 * @ingroup APR_Util
 * @{
 */

/** CA certificate type unknown */
#define APR_CRYPTO_CA_TYPE_UNKNOWN           0
/** binary DER encoded CA certificate */
#define APR_CRYPTO_CA_TYPE_DER               1
/** PEM encoded CA certificate */
#define APR_CRYPTO_CA_TYPE_BASE64            2
/** Netscape/Mozilla cert7.db CA certificate database */
#define APR_CRYPTO_CA_TYPE_CERT7_DB          3
/** Netscape/Mozilla secmod file */
#define APR_CRYPTO_CA_TYPE_SECMOD            4
/** Client certificate type unknown */
#define APR_CRYPTO_CERT_TYPE_UNKNOWN         5
/** binary DER encoded client certificate */
#define APR_CRYPTO_CERT_TYPE_DER             6
/** PEM encoded client certificate */
#define APR_CRYPTO_CERT_TYPE_BASE64          7
/** Netscape/Mozilla key3.db client certificate database */
#define APR_CRYPTO_CERT_TYPE_KEY3_DB         8
/** Netscape/Mozilla client certificate nickname */
#define APR_CRYPTO_CERT_TYPE_NICKNAME        9
/** Private key type unknown */
#define APR_CRYPTO_KEY_TYPE_UNKNOWN          10
/** binary DER encoded private key */
#define APR_CRYPTO_KEY_TYPE_DER              11
/** PEM encoded private key */
#define APR_CRYPTO_KEY_TYPE_BASE64           12
/** PKCS#12 encoded client certificate */
#define APR_CRYPTO_CERT_TYPE_PFX             13
/** PKCS#12 encoded private key */
#define APR_CRYPTO_KEY_TYPE_PFX              14
/** Openldap directory full of base64-encoded cert
 * authorities with hashes in corresponding .0 directory
 */
#define APR_CRYPTO_CA_TYPE_CACERTDIR_BASE64  15
/** CMS Key Database with private key and cert chain */
#define APR_CRYPTO_CA_TYPE_CMS               16
/** Symmetrical key */
#define APR_CRYPTO_KEY_TYPE_SYM              17
/** Netscape/Mozilla certificate database directory */
#define APR_CRYPTO_CA_TYPE_DIR               18
/** Crypto engine */
#define APR_CRYPTO_ENGINE                    101

#if APU_HAVE_CRYPTO

#ifndef APU_CRYPTO_RECOMMENDED_DRIVER
#if APU_HAVE_OPENSSL
#define APU_CRYPTO_RECOMMENDED_DRIVER "openssl"
#else
#if APU_HAVE_NSS
#define APU_CRYPTO_RECOMMENDED_DRIVER "nss"
#else
#if APU_HAVE_MSCNG
#define APU_CRYPTO_RECOMMENDED_DRIVER "mscng"
#else
#if APU_HAVE_MSCAPI
#define APU_CRYPTO_RECOMMENDED_DRIVER "mscapi"
#else
#endif
#endif
#endif
#endif
#endif

/**
 * Symmetric Key types understood by the library.
 *
 * NOTE: It is expected that this list will grow over time.
 *
 * Interoperability Matrix:
 *
 * The matrix is based on the testcrypto.c unit test, which attempts to
 * test whether a simple encrypt/decrypt will succeed, as well as testing
 * whether an encrypted string by one library can be decrypted by the
 * others.
 *
 * Some libraries will successfully encrypt and decrypt their own data,
 * but won't decrypt data from another library. It is hoped that over
 * time these anomalies will be found and fixed, but until then it is
 * recommended that ciphers are chosen that interoperate across platform.
 *
 * An X below means the test passes, it does not necessarily mean that
 * encryption performed is correct or secure. Applications should stick
 * to ciphers that pass the interoperablity tests on the right hand side
 * of the table.
 *
 * Aligned data is data whose length is a multiple of the block size for
 * the chosen cipher. Padded data is data that is not aligned by block
 * size and must be padded by the crypto library.
 *
 *                  OpenSSL      NSS      Interop
 *                 Align Pad  Align Pad  Align Pad
 * 3DES_192/CBC    X     X    X     X    X     X
 * 3DES_192/ECB    X     X
 * AES_256/CBC     X     X    X     X    X     X
 * AES_256/ECB     X     X    X          X
 * AES_192/CBC     X     X    X     X
 * AES_192/ECB     X     X    X
 * AES_128/CBC     X     X    X     X
 * AES_128/ECB     X     X    X
 *
 * Conclusion: for padded data, use 3DES_192/CBC or AES_256/CBC. For
 * aligned data, use 3DES_192/CBC, AES_256/CBC or AES_256/ECB.
 */

typedef enum {
    KEY_NONE, KEY_3DES_192, /** 192 bit (3-Key) 3DES */
    KEY_AES_128, /** 128 bit AES */
    KEY_AES_192, /** 192 bit AES */
    KEY_AES_256
/** 256 bit AES */
} apr_crypto_block_key_type_e;

typedef enum {
    MODE_NONE, /** An error condition */
    MODE_ECB, /** Electronic Code Book */
    MODE_CBC
/** Cipher Block Chaining */
} apr_crypto_block_key_mode_e;

/**
 * Certificate and private key structure.
 *
 * The various crypto backends expect certificates and keys in a wide
 * array of formats. This structure is analogous to apr_ldap_opt_tls_cert_t
 * from the LDAP interface. Ultimately that interface should be meshed with
 * this one.
 * @param type Type of certificate APR_CRYPTO_*_TYPE_*
 * @param path Path, file or nickname of the certificate
 * @param password Optional password, can be NULL
 */
typedef struct apr_crypto_param_t {
    int type;
    const char *path;
    const char *password;
} apr_crypto_param_t;

/* These are opaque structs.  Instantiation is up to each backend */
typedef struct apr_crypto_driver_t apr_crypto_driver_t;
typedef struct apr_crypto_config_t apr_crypto_config_t;
typedef struct apr_crypto_key_t apr_crypto_key_t;
typedef struct apr_crypto_block_t apr_crypto_block_t;

/**
 * Public factory API, common to all backends.
 */
typedef struct apr_crypto_t {
    apr_pool_t *pool;
    apu_err_t *result;
    apr_array_header_t *keys;
    apr_crypto_config_t *config;
} apr_crypto_t;

/**
 * @brief Perform once-only initialisation. Call once only.
 *
 * @param pool - pool to register any shutdown cleanups, etc
 * @return APR_NOTIMPL in case of no crypto support.
 */
APU_DECLARE(apr_status_t) apr_crypto_init(apr_pool_t *pool,
        const apr_array_header_t *params);

/**
 * @brief Get the driver struct for a name
 *
 * @param pool - (process) pool to register cleanup
 * @param name - driver name
 * @param driver - pointer to driver struct.
 * @return APR_SUCCESS for success
 * @return APR_ENOTIMPL for no driver (when DSO not enabled)
 * @return APR_EDSOOPEN if DSO driver file can't be opened
 * @return APR_ESYMNOTFOUND if the driver file doesn't contain a driver
 */
APU_DECLARE(apr_status_t) apr_crypto_get_driver(apr_pool_t *pool, const char *name,
        const apr_crypto_driver_t **driver, const apr_array_header_t *params,
        const apu_err_t **result);

/**
 * @brief Return the name of the driver.
 *
 * @param driver - The driver in use.
 * @return The name of the driver.
 */
APU_DECLARE(const char *)apr_crypto_driver_name (const apr_crypto_driver_t *driver);

/**
 * @brief Get the result of the last operation on a factory. If the result
 *        is NULL, the operation was successful.
 * @param driver - driver to use
 * @param factory - factory pointer will be written here
 * @param result - the result structure
 * @return APR_SUCCESS for success
 */
APU_DECLARE(apr_status_t) apr_crypto_error(const apr_crypto_t *f,
        const apu_err_t **result);

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
APU_DECLARE(apr_status_t) apr_crypto_factory(const apr_crypto_driver_t *driver,
        apr_pool_t *pool, const apr_array_header_t *params, apr_crypto_t **f);

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
        const int iterations, apr_crypto_key_t **key, apr_size_t *ivSize);

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
        apr_size_t *blockSize);

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
        apr_size_t inlen);

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
        unsigned char *out, apr_size_t *outlen);

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
        apr_size_t *blockSize);

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
        apr_size_t inlen);

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
        unsigned char *out, apr_size_t *outlen);

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param driver - driver to use
 * @param ctx The block context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APU_DECLARE(apr_status_t) apr_crypto_block_cleanup(
        const apr_crypto_driver_t *driver, apr_crypto_block_t *ctx);

/**
 * @brief Clean encryption / decryption factory.
 * @note After cleanup, a factory is free to be reused if necessary.
 * @param driver - driver to use
 * @param f The factory to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APU_DECLARE(apr_status_t) apr_crypto_cleanup(const apr_crypto_driver_t *driver,
        apr_crypto_t *f);

/**
 * @brief Shutdown the crypto library.
 * @note After shutdown, it is expected that the init function can be called again.
 * @param driver - driver to use
 * @param p The pool to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APU_DECLARE(apr_status_t) apr_crypto_shutdown(const apr_crypto_driver_t *driver,
        apr_pool_t *p);

#endif /* APU_HAVE_CRYPTO */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
