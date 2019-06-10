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
#include "apr_hash.h"
#include "apu_errno.h"
#include "apr_thread_proc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file apr_crypto.h
 * @brief APR-UTIL Crypto library
 */
/**
 * @defgroup APR_Util_Crypto Crypto routines
 * @ingroup APR
 * @{
 */

#if APU_HAVE_CRYPTO || defined(DOXYGEN)

#ifndef APU_CRYPTO_RECOMMENDED_DRIVER
#if APU_HAVE_COMMONCRYPTO
/** Recommended driver for this platform */
#define APU_CRYPTO_RECOMMENDED_DRIVER "commoncrypto"
#else
#if APU_HAVE_OPENSSL
/** Recommended driver for this platform */
#define APU_CRYPTO_RECOMMENDED_DRIVER "openssl"
#else
#if APU_HAVE_NSS
/** Recommended driver for this platform */
#define APU_CRYPTO_RECOMMENDED_DRIVER "nss"
#else
#if APU_HAVE_MSCNG
/** Recommended driver for this platform */
#define APU_CRYPTO_RECOMMENDED_DRIVER "mscng"
#else
#if APU_HAVE_MSCAPI
/** Recommended driver for this platform */
#define APU_CRYPTO_RECOMMENDED_DRIVER "mscapi"
#else
#endif
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
 *                  OpenSSL    CommonCrypto   NSS       Interop
 *                 Align  Pad  Align  Pad  Align  Pad  Align  Pad
 * 3DES_192/CBC    X      X    X      X    X      X    X      X
 * 3DES_192/ECB    X      X    X      X
 * AES_256/CBC     X      X    X      X    X      X    X      X
 * AES_256/ECB     X      X    X      X    X           X
 * AES_192/CBC     X      X    X      X    X      X
 * AES_192/ECB     X      X    X      X    X
 * AES_128/CBC     X      X    X      X    X      X
 * AES_128/ECB     X      X    X      X    X
 *
 * Conclusion: for padded data, use 3DES_192/CBC or AES_256/CBC. For
 * aligned data, use 3DES_192/CBC, AES_256/CBC or AES_256/ECB.
 */

/**
 * Types of ciphers.
 */
typedef enum
{
    APR_KEY_NONE, APR_KEY_3DES_192, /** 192 bit (3-Key) 3DES */
    APR_KEY_AES_128, /** 128 bit AES */
    APR_KEY_AES_192, /** 192 bit AES */
    APR_KEY_AES_256
/** 256 bit AES */
} apr_crypto_block_key_type_e;

/**
 * Types of modes supported by the ciphers.
 */
typedef enum
{
    APR_MODE_NONE, /** An error condition */
    APR_MODE_ECB, /** Electronic Code Book */
    APR_MODE_CBC
/** Cipher Block Chaining */
} apr_crypto_block_key_mode_e;

/**
 * Types of digests supported by the apr_crypto_key() function.
 */
typedef enum
{
    APR_CRYPTO_DIGEST_NONE, /** An error condition */
    APR_CRYPTO_DIGEST_MD5, /** MD5 */
    APR_CRYPTO_DIGEST_SHA1, /** SHA1 */
    APR_CRYPTO_DIGEST_SHA224, /** SHA224 */
    APR_CRYPTO_DIGEST_SHA256, /** SHA256 */
    APR_CRYPTO_DIGEST_SHA384, /** SHA384 */
    APR_CRYPTO_DIGEST_SHA512, /** SHA512 */
} apr_crypto_block_key_digest_e;

/**
 * Structure returned by the crypto_get_block_key_digests() function.
 */
typedef struct apr_crypto_block_key_digest_t {
    /** The digest used with this crypto operation. */
    apr_crypto_block_key_digest_e type;
    /** The digest size used with this digest operation */
    int digestsize;
    /** The block size used with this digest operation */
    int blocksize;
} apr_crypto_block_key_digest_t;

/**
 * Types of ciphers supported by the apr_
 */
typedef enum
{   
    APR_CRYPTO_CIPHER_AUTO, /** Choose the recommended cipher / autodetect the cipher */
    APR_CRYPTO_CIPHER_AES_256_CTR, /** AES 256 - CTR mode */
    APR_CRYPTO_CIPHER_CHACHA20_CTR, /** ChaCha20 - CTR mode */
} apr_crypto_cipher_e;

/**
 * Structure representing a backend crypto driver.
 *
 * This structure is created with apr_crypto_get_driver().
 */
typedef struct apr_crypto_driver_t apr_crypto_driver_t;

/**
 * Structure to support a group of crypto operations.
 *
 * This structure is created with apr_crypto_make().
 */
typedef struct apr_crypto_t apr_crypto_t;

/**
 * Structure representing the configuration of the given backend
 * crypto library.
 */
typedef struct apr_crypto_config_t apr_crypto_config_t;

/**
 * Structure representing a key prepared for encryption, decryption,
 * signing or verifying.
 *
 * This structure is created using the apr_crypto_key() function.
 */
typedef struct apr_crypto_key_t apr_crypto_key_t;

/**
 * Structure representing a block context for encryption, decryption,
 * signing or verifying.
 *
 * This structure is created using the apr_crypto_block_encrypt_init()
 * and apr_crypto_block_decrypt_init() functions.
 */
typedef struct apr_crypto_block_t apr_crypto_block_t;

/**
 * Structure representing a digest context for signing or verifying.
 *
 * This structure is created using the apr_crypto_digest_init() function.
 */
typedef struct apr_crypto_digest_t apr_crypto_digest_t;

/**
 * Structure returned by the crypto_get_block_key_types() function.
 */
typedef struct apr_crypto_block_key_type_t {
    /** The cipher used with this crypto operation. */
    apr_crypto_block_key_type_e type;
    /** The key size used with this crypto operation */
    int keysize;
    /** The block size used with this crypto operation */
    int blocksize;
    /** The initialisation vector size used with this crypto operation */
    int ivsize;
} apr_crypto_block_key_type_t;

/**
 * Structure returned by the crypto_get_block_key_modes() function.
 */
typedef struct apr_crypto_block_key_mode_t {
    /** The mode used with this crypto operation. */
    apr_crypto_block_key_mode_e mode;
} apr_crypto_block_key_mode_t;

/**
 * Structure describing a key to be derived from PBKDF2 to be passed by the
 * apr_crypto_key() function.
 *
 * Derived keys are used for encryption and decryption.
 *
 * Implementations must use apr_crypto_key_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_passphrase_t {
    /** The passphrase used by the key generation algorithm */
    const char *pass;
    /** The length of the passphrase */
    apr_size_t passLen;
    /** The salt used by the key derivation algorithm */
    const unsigned char * salt;
    /** The length of the salt. */
    apr_size_t saltLen;
    /** The number of iterations used by the key derivation function */
    int iterations;
} apr_crypto_passphrase_t;

/**
 * Structure describing a raw key to be passed by the
 * apr_crypto_key() function.
 *
 * Raw keys are used for encryption and decryption, and must match
 * the correct sizes for each cipher.
 *
 * Implementations must use apr_crypto_key_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_secret_t {
    /** The raw secret key used for encrypt / decrypt. Must be
     * the same size as the block size of the cipher being used.
     */
    const unsigned char *secret;
    /** The length of the secret key. */
    apr_size_t secretLen;
} apr_crypto_secret_t;

/**
 * Structure describing a simple digest hash to be generated by the
 * apr_crypto_key() function.
 *
 * Implementations must use apr_crypto_key_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_key_hash_t {
    /** The digest used for the HMAC. */
    apr_crypto_block_key_digest_e digest;
} apr_crypto_key_hash_t;

/**
 * Structure describing a HMAC key and digest to be generated by the
 * apr_crypto_key() function.
 *
 * Implementations must use apr_crypto_key_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_key_hmac_t {
    /** The secret used for the HMAC */
    const unsigned char *secret;
    /** The length of the secret used for the HMAC */
    apr_size_t secretLen;
    /** The digest used for the HMAC. */
    apr_crypto_block_key_digest_e digest;
} apr_crypto_key_hmac_t;

/**
 * Structure describing a CMAC key and digest to be generated by the
 * apr_crypto_key() function.
 *
 * Implementations must use apr_crypto_key_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_key_cmac_t {
    /** The secret used for the CMAC */
    const unsigned char *secret;
    /** The length of the secret used for the CMAC */
    apr_size_t secretLen;
    /** The digest used for the CMAC. */
    apr_crypto_block_key_digest_e digest;
} apr_crypto_key_cmac_t;

/**
 * Structure used to create a hashed digest.
 *
 * Implementations must use apr_crypto_digest_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_digest_hash_t {
    /** The message digest */
    unsigned char *s;
    /** The length of the message digest */
    apr_size_t slen;
    /** The digest algorithm */
    apr_crypto_block_key_digest_e digest;
} apr_crypto_digest_hash_t;

/**
 * Structure used to create a signature.
 *
 * Implementations must use apr_crypto_digest_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_digest_sign_t {
    /** The message digest */
    unsigned char *s;
    /** The length of the message digest */
    apr_size_t slen;
    /** The digest algorithm */
    apr_crypto_block_key_digest_e digest;
} apr_crypto_digest_sign_t;

/**
 * Structure used to create a signature for verification.
 *
 * Implementations must use apr_crypto_digest_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_digest_verify_t {
    /** The message digest generated */
    unsigned char *s;
    /** The length of the message digest */
    apr_size_t slen;
    /** The message digest to be verified against */
    const unsigned char *v;
    /** The length of the message digest */
    apr_size_t vlen;
    /** The digest algorithm */
    apr_crypto_block_key_digest_e digest;
} apr_crypto_digest_verify_t;

/**
 * Types of keys supported by the apr_crypto_key() function and the
 * apr_crypto_key_rec_t structure.
 */
typedef enum {
    /**
     * Key is derived from a passphrase.
     *
     * Used with the encrypt / decrypt functions.
     */
    APR_CRYPTO_KTYPE_PASSPHRASE     = 1,
    /**
     * Key is derived from a raw key.
     *
     * Used with the encrypt / decrypt functions.
     */
    APR_CRYPTO_KTYPE_SECRET         = 2,
    /**
     * Simple digest, no key.
     *
     * Used with the digest functions.
     */
    APR_CRYPTO_KTYPE_HASH           = 3,
    /**
     * HMAC Key is derived from a raw key.
     *
     * Used with the digest functions.
     */
    APR_CRYPTO_KTYPE_HMAC           = 4,
    /**
     * CMAC Key is derived from a raw key.
     *
     * Used with the digest functions.
     */
    APR_CRYPTO_KTYPE_CMAC           = 5,
} apr_crypto_key_type;

/**
 * Types of digests supported by the apr_crypto_digest() functions and the
 * apr_crypto_digest_rec_t structure.
 */
typedef enum {
    /**
     * Simple digest operation.
     *
     * Use with apr_crypto_key_rec_t APR_CRYPTO_KTYPE_HASH.
     */
    APR_CRYPTO_DTYPE_HASH   = 1,
    /**
     * Sign operation.
     *
     * Use with apr_crypto_key_rec_t APR_CRYPTO_KTYPE_HMAC or
     * APR_CRYPTO_KTYPE_CMAC.
     */
    APR_CRYPTO_DTYPE_SIGN     = 2,
    /**
     * Verify operation.
     *
     * Use with apr_crypto_key_rec_t APR_CRYPTO_KTYPE_HMAC or
     * APR_CRYPTO_KTYPE_CMAC.
     */
    APR_CRYPTO_DTYPE_VERIFY   = 3,
} apr_crypto_digest_type_e;

/**
 * Structure describing a key to be generated by the
 * apr_crypto_key() function.
 *
 * Implementations must use apr_crypto_key_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_key_rec_t {
    /** The type of the key. */
    apr_crypto_key_type ktype;
    /** The cipher used with this crypto operation. */
    apr_crypto_block_key_type_e type;
    /** The mode used with this crypto operation. */
    apr_crypto_block_key_mode_e mode;
    /** Non zero if padding should be used with this crypto operation. */
    int pad;
    /** Details of each key, based on the key type. */
    union {
        /**
         * This key is generated using a PBE algorithm from a given
         * passphrase, and can be used to encrypt / decrypt.
         *
         * Key type: APR_CRYPTO_KTYPE_PASSPHRASE
         */
        apr_crypto_passphrase_t passphrase;
        /**
         * This is a raw key matching the block size of the given
         * cipher, and can be used to encrypt / decrypt.
         *
         * Key type: APR_CRYPTO_KTYPE_SECRET
         */
        apr_crypto_secret_t secret;
        /**
         * This represents a simple digest with no key.
         *
         * Key type: APR_CRYPTO_KTYPE_HASH
         */
        apr_crypto_key_hash_t hash;
        /**
         * This is a key of arbitrary length used with an HMAC.
         *
         * Key type: APR_CRYPTO_KTYPE_HMAC
         */
        apr_crypto_key_hmac_t hmac;
        /**
         * This is a key of arbitrary length used with a CMAC.
         *
         * Key type: APR_CRYPTO_KTYPE_CMAC
         */
        apr_crypto_key_cmac_t cmac;
    } k;
} apr_crypto_key_rec_t;

/**
 * Structure describing a digest to be hashed, signed or verified.
 *
 * This structure is passed to the apr_crypto_digest_init() and
 * apr_crypto_digest() functions.
 *
 * Implementations must use apr_crypto_digest_rec_make() to allocate
 * this structure.
 */
typedef struct apr_crypto_digest_rec_t {
    /** The type of the digest record. */
    apr_crypto_digest_type_e dtype;
    /** Details of each digest, based on the digest type. */
    union {
        apr_crypto_digest_hash_t hash;
        apr_crypto_digest_sign_t sign;
        apr_crypto_digest_verify_t verify;
    } d;
} apr_crypto_digest_rec_t;

/**
 * @brief Perform once-only initialisation. Call once only.
 *
 * @param pool - pool to register any shutdown cleanups, etc
 * @return APR_ENOTIMPL in case of no crypto support.
 */
APR_DECLARE(apr_status_t) apr_crypto_init(apr_pool_t *pool);

/* TODO: doxygen */
APR_DECLARE(apr_status_t) apr_crypto_lib_version(const char *name,
                                                 const char **version);
APR_DECLARE(apr_status_t) apr_crypto_lib_init(const char *name,
                                              const char *params,
                                              const apu_err_t **result,
                                              apr_pool_t *pool);
APR_DECLARE(apr_status_t) apr_crypto_lib_term(const char *name);
APR_DECLARE(int) apr_crypto_lib_is_active(const char *name);

/**
 * @brief Zero out the buffer provided when the pool is cleaned up.
 *
 * @param pool - pool to register the cleanup
 * @param buffer - buffer to zero out
 * @param size - size of the buffer to zero out
 */
APR_DECLARE(apr_status_t) apr_crypto_clear(apr_pool_t *pool, void *buffer,
        apr_size_t size);

/**
 * @brief Always zero out the buffer provided, without being optimized out by
 * the compiler.
 *
 * @param buffer - buffer to zero out
 * @param size - size of the buffer to zero out
 */
APR_DECLARE(apr_status_t) apr_crypto_memzero(void *buffer, apr_size_t size);

/**
 * @brief Timing attacks safe buffers comparison, where the executing time does
 * not depend on the bytes compared but solely on the number of bytes.
 *
 * @param buf1 - first buffer to compare
 * @param buf2 - second buffer to compare
 * @param size - size of the buffers to compare
 * @return 1 if the buffers are equals, 0 otherwise.
 */
APR_DECLARE(int) apr_crypto_equals(const void *buf1, const void *buf2,
                                   apr_size_t size);

/**
 * @brief Get the driver struct for a name
 *
 * @param driver - pointer to driver struct.
 * @param name - driver name
 * @param params - array of initialisation parameters
 * @param result - result and error message on failure
 * @param pool - (process) pool to register cleanup
 * @return APR_SUCCESS for success
 * @return APR_ENOTIMPL for no driver (when DSO not enabled)
 * @return APR_EDSOOPEN if DSO driver file can't be opened
 * @return APR_ESYMNOTFOUND if the driver file doesn't contain a driver
 * @remarks NSS: the params can have "dir", "key3", "cert7" and "secmod"
 *  keys, each followed by an equal sign and a value. Such key/value pairs can
 *  be delimited by space or tab. If the value contains a space, surround the
 *  whole key value pair in quotes: "dir=My Directory".
 * @remarks OpenSSL: currently no params are supported.
 */
APR_DECLARE(apr_status_t) apr_crypto_get_driver(
        const apr_crypto_driver_t **driver,
        const char *name, const char *params, const apu_err_t **result,
        apr_pool_t *pool);

/**
 * @brief Return the name of the driver.
 *
 * @param driver - The driver in use.
 * @return The name of the driver.
 */
APR_DECLARE(const char *) apr_crypto_driver_name(
        const apr_crypto_driver_t *driver);

/**
 * @brief Get the result of the last operation on a context. If the result
 *        is NULL, the operation was successful.
 * @param result - the result structure
 * @param f - context pointer
 * @return APR_SUCCESS for success
 */
APR_DECLARE(apr_status_t) apr_crypto_error(const apu_err_t **result,
        const apr_crypto_t *f);

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
        const apr_crypto_driver_t *driver, const char *params,
        apr_pool_t *pool);

/**
 * @brief Get a hash table of key digests, keyed by the name of the digest against
 * a pointer to apr_crypto_block_key_digest_t, which in turn begins with an
 * integer.
 *
 * @param digests - hashtable of key digests keyed to constants.
 * @param f - encryption context
 * @return APR_SUCCESS for success
 */
APR_DECLARE(apr_status_t) apr_crypto_get_block_key_digests(apr_hash_t **digests,
        const apr_crypto_t *f);

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
        const apr_crypto_t *f);

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
        const apr_crypto_t *f);

/**
 * @brief Create a key record to be passed to apr_crypto_key().
 * @param ktype The apr_crypto_key_type to use.
 * @param p The pool to use.
 * @return Returns a blank structure of the correct size.
 */
APR_DECLARE(apr_crypto_key_rec_t *) apr_crypto_key_rec_make(
        apr_crypto_key_type ktype, apr_pool_t *p);

/**
 * @brief Create a digest record to be passed to apr_crypto_digest_init().
 * @param dtype The type of digest record to create.
 * @param p The pool to use.
 * @return Returns a blank structure of the correct size.
 */
APR_DECLARE(apr_crypto_digest_rec_t *) apr_crypto_digest_rec_make(
        apr_crypto_digest_type_e dtype, apr_pool_t *p);

/**
 * @brief Create a key from the provided secret or passphrase. The key is cleaned
 *        up when the context is cleaned, and may be reused with multiple
 *        encryption, decryption, signing or verifying operations. The choice of
 *        key type much match the intended operation.
 * @note If *key is NULL, a apr_crypto_key_t will be created from a pool. If
 *       *key is not NULL, *key must point at a previously created structure.
 * @param key The key returned, see note.
 * @param rec The key record, from which the key will be derived.
 * @param f The context to use.
 * @param p The pool to use.
 * @return APR_ENOKEY if the pass phrase is missing or empty, or if a backend
 *         error occurred while generating the key.
 * @return APR_ENOCIPHER if the type or mode
 *         is not supported by the particular backend.
 * @return APR_EKEYTYPE if the key type is
 *         not known.
 * @return APR_EPADDING if padding was requested but is not supported.
 * @return APR_ENOTIMPL if not implemented.
 */
APR_DECLARE(apr_status_t) apr_crypto_key(apr_crypto_key_t **key,
        const apr_crypto_key_rec_t *rec, const apr_crypto_t *f, apr_pool_t *p);

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
 * @return APR_ENOKEY if the pass phrase is missing or empty, or if a backend
 *         error occurred while generating the key.
 * @return APR_ENOCIPHER if the type or mode
 *         is not supported by the particular backend.
 * @return APR_EKEYTYPE if the key type is
 *         not known.
 * @return APR_EPADDING if padding was requested but is not supported.
 * @return APR_ENOTIMPL if not implemented.
 * @deprecated Replaced by apr_crypto_key().
 */
APR_DECLARE(apr_status_t) apr_crypto_passphrase(apr_crypto_key_t **key,
        apr_size_t *ivSize, const char *pass, apr_size_t passLen,
        const unsigned char * salt, apr_size_t saltLen,
        const apr_crypto_block_key_type_e type,
        const apr_crypto_block_key_mode_e mode, const int doPad,
        const int iterations, const apr_crypto_t *f, apr_pool_t *p);

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
 * @return APR_ENOIV if an initialisation vector is required but not specified.
 * @return APR_EINIT if the backend failed to initialise the context.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_encrypt_init(
        apr_crypto_block_t **ctx, const unsigned char **iv,
        const apr_crypto_key_t *key, apr_size_t *blockSize, apr_pool_t *p);

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
 * @return APR_ECRYPT if an error occurred.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_encrypt(unsigned char **out,
        apr_size_t *outlen, const unsigned char *in, apr_size_t inlen,
        apr_crypto_block_t *ctx);

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
 *            buffer used by apr_crypto_block_encrypt(). See note.
 * @param outlen Length of the output will be written here.
 * @param ctx The block context to use.
 * @return APR_ECRYPT if an error occurred.
 * @return APR_EPADDING if padding was enabled and the block was incorrectly
 *         formatted.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_encrypt_finish(unsigned char *out,
        apr_size_t *outlen, apr_crypto_block_t *ctx);

/**
 * @brief Initialise a context for decrypting arbitrary data using the given key.
 * @note If *ctx is NULL, a apr_crypto_block_t will be created from a pool. If
 *       *ctx is not NULL, *ctx must point at a previously created structure.
 * @param ctx The block context returned, see note.
 * @param blockSize The block size of the cipher.
 * @param iv Optional initialisation vector.
 * @param key The key structure to use.
 * @param p The pool to use.
 * @return APR_ENOIV if an initialisation vector is required but not specified.
 * @return APR_EINIT if the backend failed to initialise the context.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_decrypt_init(
        apr_crypto_block_t **ctx, apr_size_t *blockSize,
        const unsigned char *iv, const apr_crypto_key_t *key, apr_pool_t *p);

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
 * @return APR_ECRYPT if an error occurred.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_decrypt(unsigned char **out,
        apr_size_t *outlen, const unsigned char *in, apr_size_t inlen,
        apr_crypto_block_t *ctx);

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
 *            buffer used by apr_crypto_block_decrypt(). See note.
 * @param outlen Length of the output will be written here.
 * @param ctx The block context to use.
 * @return APR_ECRYPT if an error occurred.
 * @return APR_EPADDING if padding was enabled and the block was incorrectly
 *         formatted.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_decrypt_finish(unsigned char *out,
        apr_size_t *outlen, apr_crypto_block_t *ctx);

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param ctx The block context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APR_DECLARE(apr_status_t) apr_crypto_block_cleanup(apr_crypto_block_t *ctx);

/**
 * @brief Initialise a context for hashing, signing or verifying arbitrary
 *        data.
 *
 *        This function supports:
 *        - Simple hashing (MD5, SHA1, SHA224, SHA256, SHA384, SHA512).
 *        - HMAC (with a secret key)
 *        - CMAC (with a secret key)
 *
 *        Details of the key and the type of digest to be performed are
 *        passed in the constant apr_crypto_key_t structure, which can be
 *        reused by many calls to apr_crypto_digest_init().
 *
 *        Details of this particular operation are read from and written to
 *        the apr_crypto_digest_rec_t structure, which is expected to
 *        contain the message digest to be verified, as well as message
 *        digest generated during the hashing or signing process. This
 *        structure will be modified by each digest operation, and cannot be
 *        shared.
 * @note If *d is NULL, a apr_crypto_digest_t will be created from a pool. If
 *       *d is not NULL, *d must point at a previously created structure.
 * @param d The digest context returned, see note.
 * @param key The key structure to use.
 * @param rec The digest record indicating whether we want to sign or verify.
 *        This record contains digest we want to verify against, as well as
 *        the signature we have generated.
 * @param p The pool to use.
 * @return APR_SUCCESS if successful.
 * @return APR_ENOIV if an initialisation vector is required but not specified.
 * @return APR_EINIT if the backend failed to initialise the context.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_digest_init(apr_crypto_digest_t **d,
        const apr_crypto_key_t *key, apr_crypto_digest_rec_t *rec, apr_pool_t *p);

/**
 * @brief Update the digest with data provided by in.
 * @param digest The block context to use.
 * @param in Address of the buffer to digest.
 * @param inlen Length of the buffer to digest.
 * @return APR_SUCCESS if successful.
 * @return APR_ECRYPT if an error occurred.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_digest_update(apr_crypto_digest_t *digest,
        const unsigned char *in, apr_size_t inlen);

/**
 * @brief Finalise the digest and write the result.
 *
 *        The result is written to the apr_crypto_digest_rec_t structure
 *        passed into apr_crypto_digest_init().
 *
 *        If verification is requested, this function will return the
 *        result of the verification.
 * @note After this call, the context is cleaned and can be reused by
 *   apr_crypto_digest_init().
 * @param digest The digest context to use.
 * @return APR_SUCCESS if hash, signing or verification was successful.
 * @return APR_ENOVERIFY if the verification failed.
 * @return APR_ECRYPT if an error occurred.
 * @return APR_EPADDING if padding was enabled and the block was incorrectly
 *         formatted.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_digest_final(apr_crypto_digest_t *digest);

/**
 * @brief One shot digest on a single memory buffer.
 * @param key The key structure to use.
 * @param rec The digest record indicating whether we want to sign or verify.
 *        This record contains digest we want to verify against, as well as
 *        the signature we have generated. This record will contain the digest
 *        calculated.
 * @param in Address of the buffer to digest.
 * @param inlen Length of the buffer to digest.
 * @param p The pool to use.
 * @return APR_ENOIV if an initialisation vector is required but not specified.
 * @return APR_EINIT if the backend failed to initialise the context.
 * @return APR_ENOTIMPL if not implemented.
 * @return APR_EINVAL if the key type does not support the given operation.
 */
APR_DECLARE(apr_status_t) apr_crypto_digest(const apr_crypto_key_t *key,
        apr_crypto_digest_rec_t *rec, const unsigned char *in, apr_size_t inlen,
        apr_pool_t *p);

/**
 * @brief Clean digest context.
 * @note After cleanup, a digest context is free to be reused if necessary.
 * @param ctx The digest context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APR_DECLARE(apr_status_t) apr_crypto_digest_cleanup(apr_crypto_digest_t *ctx);

/**
 * @brief Clean encryption / decryption context.
 * @note After cleanup, a context is free to be reused if necessary.
 * @param f The context to use.
 * @return Returns APR_ENOTIMPL if not supported.
 */
APR_DECLARE(apr_status_t) apr_crypto_cleanup(apr_crypto_t *f);

/**
 * @brief Shutdown the crypto library.
 * @note After shutdown, it is expected that the init function can be called again.
 * @param driver - driver to use
 * @return Returns APR_ENOTIMPL if not supported.
 */
APR_DECLARE(apr_status_t) apr_crypto_shutdown(
        const apr_crypto_driver_t *driver);

#if APU_HAVE_CRYPTO_PRNG

/**
 * Cryptographic Pseudo Random Number Generator (CPRNG).
 *
 * Allows to generate cryptographically secure random bytes indefinitely
 * given an initial seed of \ref APR_CRYPTO_PRNG_SEED_SIZE bytes (32), which
 * is either provided by the caller or automatically gathered from the system.
 * The CPRNG can also be re-seeded at any time, or after a process is fork()ed.
 *
 * The internal key is renewed every \ref APR_CRYPTO_PRNG_SEED_SIZE random
 * bytes produced and those data once returned to the caller are cleared from
 * the internal state, which ensures forward secrecy.
 *
 * This CPRNG is fast, based on a stream cipher, and will never block besides
 * the initial seed or any reseed if it depends on the system entropy.
 *
 * Finally, it can be used either globally (locked in multithread environment),
 * per-thread (a lock free instance is automatically created for each thread on
 * first use), or created as standalone instance (manageable independently).
 */

#define APR_CRYPTO_PRNG_SEED_SIZE 32

#define APR_CRYPTO_PRNG_LOCKED      (0x1)
#define APR_CRYPTO_PRNG_PER_THREAD  (0x2)
#define APR_CRYPTO_PRNG_MASK        (0x3)

/** Opaque CPRNG state */
typedef struct apr_crypto_prng_t apr_crypto_prng_t;

/**
 * @brief Perform global initialisation. Call once only.
 *
 * @param pool Used to allocate memory and register cleanups
 * @param crypto The crypto context to use. If NULL, one will be created from
 *               the recommended crypto implementation.
 * @param cipher The cipher to use.
 * @param bufsize The size of the buffer used to cache upcoming random bytes.
 * @param seed A custom seed of \ref APR_CRYPTO_PRNG_SEED_SIZE bytes,
 *             or NULL for the seed to be gathered from system entropy.
 * @param flags \ref APR_CRYPTO_PRNG_PER_THREAD to allow for per-thread CPRNG,
 *              or zero.
 * @return APR_EREINIT if called more than once,
 *         any system error (APR_ENOMEM, ...).
 */
APR_DECLARE(apr_status_t) apr_crypto_prng_init(apr_pool_t *pool, apr_crypto_t *crypto,
        apr_crypto_cipher_e cipher, apr_size_t bufsize, const unsigned char seed[], int flags);

/**
 * @brief Terminate global initialisation if needed, before automatic cleanups.
 *
 * @return APR_EINIT if \ref apr_crypto_prng_init() was not called.
 */
APR_DECLARE(apr_status_t) apr_crypto_prng_term(void);

/**
 * @brief Generate cryptographically secure random bytes from the global CPRNG.
 *
 * @param buf The destination buffer
 * @param len The destination length
 * @return APR_EINIT if \ref apr_crypto_prng_init() was not called.
 *         any system error (APR_ENOMEM, ...).
 */
APR_DECLARE(apr_status_t) apr_crypto_random_bytes(void *buf, apr_size_t len);

#if APR_HAS_THREADS
/**
 * @brief Generate cryptographically secure random bytes from the CPRNG of
 *        the current thread.
 *
 * @param buf The destination buffer
 * @param len The destination length
 * @return APR_EINIT if \ref apr_crypto_prng_init() was not called or
 *                   called without \ref APR_CRYPTO_PRNG_PER_THREAD,
 *         any system error (APR_ENOMEM, ...).
 */
APR_DECLARE(apr_status_t) apr_crypto_random_thread_bytes(void *buf,
        apr_size_t len);
#endif

/**
 * @brief Create a standalone CPRNG.
 *
 * @param pcprng The CPRNG created.
 * @param crypto The crypto context to use. If NULL, one will be created from
 *               the recommended crypto implementation.
 * @param cipher The cipher to use.
 * @param bufsize The size of the buffer used to cache upcoming random bytes.
 * @param flags \ref APR_CRYPTO_PRNG_LOCKED to control concurrent accesses,
 *              or zero.
 * @param seed A custom seed of \ref APR_CRYPTO_PRNG_SEED_SIZE bytes,
 *             or NULL for the seed to be gathered from system entropy.
 * @param pool Used to allocate memory and register cleanups, or NULL
 *             if the memory should be managed outside (besides per-thread
 *             which has an automatic memory management with no pool, when
 *             NULL is given the caller is responsible for calling
 *             \ref apr_crypto_prng_destroy() or some memory would leak.
 * @return APR_EINVAL if \ref bufsize is too large or flags are unknown,
 *         APR_ENOTIMPL if \ref APR_CRYPTO_PRNG_LOCKED with !APR_HAS_THREADS,
 *         APR_ENOCIPHER if neither Chacha20 nor AES-256-CTR are available,
 *         any system error (APR_ENOMEM, ...).
 */
APR_DECLARE(apr_status_t) apr_crypto_prng_create(apr_crypto_prng_t **pcprng,
        apr_crypto_t *crypto, apr_crypto_cipher_e cipher, apr_size_t bufsize,
        int flags, const unsigned char seed[], apr_pool_t *pool);

/**
 * @brief Destroy a standalone CPRNG.
 *
 * @param cprng The CPRNG to destroy.
 * @return APR_SUCCESS.
 */
APR_DECLARE(apr_status_t) apr_crypto_prng_destroy(apr_crypto_prng_t *cprng);

/**
 * @brief Rekey a CPRNG.
 *
 * @param cprng The CPRNG, or NULL for all the created CPRNGs (but per-thread).
 * @return Any system error (APR_ENOMEM, ...).
 */
APR_DECLARE(apr_status_t) apr_crypto_prng_rekey(apr_crypto_prng_t *cprng);

/**
 * @brief Reseed a CPRNG.
 *
 * @param cprng The CPRNG to reseed, or NULL for the global CPRNG.
 * @param seed A custom seed of \ref APR_CRYPTO_PRNG_SEED_SIZE bytes,
 *             or NULL for the seed to be gathered from system entropy.
 * @return Any system error (APR_ENOMEM, ...).
 */
APR_DECLARE(apr_status_t) apr_crypto_prng_reseed(apr_crypto_prng_t *cprng,
                                                 const unsigned char seed[]);

#if APR_HAS_FORK
#define APR_CRYPTO_FORK_INPARENT 0
#define APR_CRYPTO_FORK_INCHILD  1

/**
 * @brief Rekey a CPRNG in the parent and/or child process after a fork(),
 *        so that they don't share the same state.
 *
 * @param cprng The CPRNG, or NULL for all the created CPRNGs (but per-thread).
 * @param in_child Whether in the child process (non zero), or in the parent
 *                 process otherwise (zero).
 *
 * @return Any system error (APR_ENOMEM, ...).
 */
APR_DECLARE(apr_status_t) apr_crypto_prng_after_fork(apr_crypto_prng_t *cprng,
                                                     int flags);
#endif

/**
 * @brief Generate cryptographically secure random bytes from a CPRNG.
 *
 * @param cprng The CPRNG, or NULL for the global CPRNG.
 * @param buf The destination buffer
 * @param len The destination length
 * @return Any system error (APR_ENOMEM, ...).
 */
APR_DECLARE(apr_status_t) apr_crypto_prng_bytes(apr_crypto_prng_t *cprng,
                                                void *buf, apr_size_t len);

#endif /* APU_HAVE_CRYPTO_PRNG */

#endif /* APU_HAVE_CRYPTO */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
