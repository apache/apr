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

/*
 * Cryptographic Pseudo Random Number Generator (CPRNG), based on
 * "Fast-key-erasure random-number generators" from D.J. Bernstein ([1]),
 * and public domain implementation in libpqcrypto's randombytes() ([2]).
 * [1] https://blog.cr.yp.to/20170723-random.html
 * [2] https://libpqcrypto.org/
 */

#include "apu.h"

#include "apr_crypto.h"

#if APU_HAVE_CRYPTO
#if APU_HAVE_CRYPTO_PRNG

#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_thread_mutex.h"
#include "apr_thread_proc.h"

#include <stdlib.h> /* for malloc() */

#define APR_CRYPTO_PRNG_KEY_SIZE 32
#if APR_CRYPTO_PRNG_SEED_SIZE > APR_CRYPTO_PRNG_KEY_SIZE
#error apr_crypto_prng uses 256bit stream ciphers only
#endif

#define APR_CRYPTO_PRNG_BUF_SIZE_MIN (APR_CRYPTO_PRNG_KEY_SIZE * (8 - 1))
#define APR_CRYPTO_PRNG_BUF_SIZE_DEF (APR_CRYPTO_PRNG_KEY_SIZE * (24 - 1))

#if APU_HAVE_OPENSSL

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <openssl/obj_mac.h> /* for NID_* */
#if !defined(NID_chacha20) && !defined(NID_aes_256_ctr)
/* XXX: APU_HAVE_CRYPTO_PRNG && APU_HAVE_OPENSSL shoudn't be defined! */
#error apr_crypto_prng needs OpenSSL implementation for Chacha20 or AES256-CTR
#endif

typedef EVP_CIPHER_CTX cprng_stream_ctx_t;

static apr_status_t cprng_lib_init(apr_pool_t *pool)
{
    return apr_crypto_lib_init("openssl", NULL, NULL, pool);
}

static apr_status_t cprng_stream_ctx_make(cprng_stream_ctx_t **pctx)
{
    EVP_CIPHER_CTX *ctx;
    const EVP_CIPHER *cipher;

    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return APR_ENOMEM;
    }

#if defined(NID_chacha20)
    cipher = EVP_chacha20();
#else
    cipher = EVP_aes_256_ctr();
#endif
    if (EVP_EncryptInit_ex(ctx, cipher, NULL, NULL, NULL) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        return APR_ENOMEM;
    }

    *pctx = ctx;
    return APR_SUCCESS;
}

static APR_INLINE
void cprng_stream_ctx_free(cprng_stream_ctx_t *ctx)
{
    EVP_CIPHER_CTX_free(ctx);
}

static APR_INLINE
apr_status_t cprng_stream_ctx_mix(cprng_stream_ctx_t **pctx,
                                  unsigned char *key, unsigned char *to,
                                  const unsigned char *z, apr_size_t n)
{
    cprng_stream_ctx_t *ctx = *pctx;
    int len;

    EVP_EncryptInit_ex(ctx, NULL, NULL, key, NULL);
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    memset(key, 0, APR_CRYPTO_PRNG_KEY_SIZE);
    EVP_EncryptUpdate(ctx, key, &len, key, APR_CRYPTO_PRNG_KEY_SIZE);
    EVP_EncryptUpdate(ctx, to, &len, z, n);

    return APR_SUCCESS;
}

static apr_status_t cprng_hash_to_seed(pid_t pid, unsigned char seed[])
{
    SHA256_CTX ctx;

    SHA256_Init(&ctx);
    SHA256_Update(&ctx, &pid, sizeof(pid));
    SHA256_Final(seed, &ctx);

    return APR_SUCCESS;
}

#else /* APU_HAVE_OPENSSL */

/* XXX: APU_HAVE_CRYPTO_PRNG shoudn't be defined! */
#error apr_crypto_prng implemented with OpenSSL only for now

#endif /* APU_HAVE_OPENSSL */

struct apr_crypto_prng_t {
    apr_pool_t *pool;
    cprng_stream_ctx_t *ctx;
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock;
#endif
    unsigned char *key, *buf;
    apr_size_t len, pos;
    int flags;
};

static apr_crypto_prng_t *cprng_global = NULL;

#if APR_HAS_THREADS
static apr_threadkey_t *cprng_thread_key = NULL;

static void cprng_thread_destroy(void *cprng)
{
    if (!cprng_global) {
        apr_threadkey_private_delete(cprng_thread_key);
        cprng_thread_key = NULL;
    }
    apr_crypto_prng_destroy(cprng);
}
#endif

APR_DECLARE(apr_status_t) apr_crypto_prng_init(apr_pool_t *pool,
                                               apr_size_t bufsize,
                                               const unsigned char seed[],
                                               int flags)
{
    apr_status_t rv;

    if (cprng_global) {
        return APR_EREINIT;
    }

    rv = cprng_lib_init(pool);
    if (rv != APR_SUCCESS && rv != APR_EREINIT) {
        return rv;
    }

    if (flags & APR_CRYPTO_PRNG_PER_THREAD) {
#if !APR_HAS_THREADS
        return APR_ENOTIMPL;
#else
        rv = apr_threadkey_private_create(&cprng_thread_key,
                                          cprng_thread_destroy, pool);
        if (rv != APR_SUCCESS) {
            return rv;
        }
#endif
    }

#if APR_HAS_THREADS
    /* Global CPRNG is locked */
    flags = (flags | APR_CRYPTO_PRNG_LOCKED) & ~APR_CRYPTO_PRNG_PER_THREAD;
#endif
    return apr_crypto_prng_create(&cprng_global, bufsize, flags, seed, pool);
}

APR_DECLARE(apr_status_t) apr_crypto_prng_term(void)
{
    if (!cprng_global) {
        return APR_EINIT;
    }

    apr_crypto_prng_destroy(cprng_global);
    cprng_global = NULL;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_after_fork(apr_proc_t *proc)
{
    unsigned char seedb[APR_CRYPTO_PRNG_SEED_SIZE], *seed = NULL;

    if (!cprng_global) {
        return APR_EINIT;
    }

    if (proc) {
        apr_status_t rv;
        rv = cprng_hash_to_seed(proc->pid, seedb);
        if (rv != APR_SUCCESS) {
            return rv;
        }
        seed = seedb;
    }

    return apr_crypto_prng_reseed(cprng_global, seed);
}

APR_DECLARE(apr_status_t) apr_crypto_random_bytes(void *buf, apr_size_t len)
{
    if (!cprng_global) {
        return APR_EINIT;
    }

    return apr_crypto_prng_bytes(cprng_global, buf, len);
}

#if APR_HAS_THREADS
APR_DECLARE(apr_status_t) apr_crypto_random_thread_bytes(void *buf,
                                                         apr_size_t len)
{
    apr_status_t rv;
    apr_crypto_prng_t *cprng;
    void *private = NULL;

    if (!cprng_thread_key) {
        return APR_EINIT;
    }

    rv = apr_threadkey_private_get(&private, cprng_thread_key);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    cprng = private;
    if (!cprng) {
        rv = apr_crypto_prng_create(&cprng, 0, 0, NULL, NULL);
        if (rv != APR_SUCCESS) {
            return rv;
        }

        rv = apr_threadkey_private_set(cprng, cprng_thread_key);
        if (rv != APR_SUCCESS) {
            apr_crypto_prng_destroy(cprng);
            return rv;
        }
    }

    return apr_crypto_prng_bytes(cprng, buf, len);
}
#endif

static apr_status_t cprng_cleanup(void *arg)
{
    apr_crypto_prng_t *cprng = arg;

    if (cprng == cprng_global) {
        cprng_global = NULL;
    }

    if (cprng->ctx) {
        cprng_stream_ctx_free(cprng->ctx);
    }

    if (cprng->key) {
        apr_crypto_memzero(cprng->key, APR_CRYPTO_PRNG_KEY_SIZE + cprng->len);
    }

    if (!cprng->pool) {
        free(cprng->key);
        free(cprng);
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_create(apr_crypto_prng_t **pcprng,
                                                 apr_size_t bufsize, int flags,
                                                 const unsigned char seed[],
                                                 apr_pool_t *pool)
{
    apr_status_t rv;
    apr_crypto_prng_t *cprng;

    *pcprng = NULL;

    if (bufsize > APR_INT32_MAX - APR_CRYPTO_PRNG_KEY_SIZE
            || (flags & APR_CRYPTO_PRNG_LOCKED && !pool)
            || (flags & ~APR_CRYPTO_PRNG_MASK)) {
        return APR_EINVAL;
    }

#if !APR_HAS_THREADS
    if (flags & (APR_CRYPTO_PRNG_LOCKED | APR_CRYPTO_PRNG_PER_THREAD)) {
        return APR_ENOTIMPL;
    }
#endif

    if (pool) {
        cprng = apr_pcalloc(pool, sizeof(*cprng));
    }
    else {
        cprng = calloc(1, sizeof(*cprng));
    }
    if (!cprng) {
        return APR_ENOMEM;
    }
    cprng->flags = flags;
    cprng->pool = pool;

    if (bufsize == 0) {
        bufsize = APR_CRYPTO_PRNG_BUF_SIZE_DEF;
    }
    else if (bufsize < APR_CRYPTO_PRNG_BUF_SIZE_MIN) {
        bufsize = APR_CRYPTO_PRNG_BUF_SIZE_MIN;
    }
    else if (bufsize % APR_CRYPTO_PRNG_KEY_SIZE) {
        bufsize += APR_CRYPTO_PRNG_KEY_SIZE;
        bufsize -= bufsize % APR_CRYPTO_PRNG_KEY_SIZE;
    }
    if (pool) {
        cprng->key = apr_palloc(pool, APR_CRYPTO_PRNG_KEY_SIZE + bufsize);
    }
    else {
        cprng->key = malloc(APR_CRYPTO_PRNG_KEY_SIZE + bufsize);
    }
    if (!cprng->key) {
        cprng_cleanup(cprng);
        return APR_ENOMEM;
    }
    cprng->buf = cprng->key + APR_CRYPTO_PRNG_KEY_SIZE;
    cprng->len = bufsize;

    if (seed) {
        memset(cprng->key, 0, APR_CRYPTO_PRNG_KEY_SIZE);
    }
    rv = apr_crypto_prng_reseed(cprng, seed);
    if (rv != APR_SUCCESS) {
        cprng_cleanup(cprng);
        return rv;
    }

#if APR_HAS_THREADS
    if (flags & APR_CRYPTO_PRNG_LOCKED) {
        rv = apr_thread_mutex_create(&cprng->lock, APR_THREAD_MUTEX_DEFAULT,
                                     pool);
        if (rv != APR_SUCCESS) {
            cprng_cleanup(cprng);
            return rv;
        }
    }
#endif

    rv = cprng_stream_ctx_make(&cprng->ctx);
    if (rv != APR_SUCCESS) {
        cprng_cleanup(cprng);
        return rv;
    }

    if (pool) {
        apr_pool_cleanup_register(pool, cprng, cprng_cleanup,
                                  apr_pool_cleanup_null);
    }

    *pcprng = cprng;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_destroy(apr_crypto_prng_t *cprng)
{
    if (!cprng->pool) {
        return cprng_cleanup(cprng);
    }

    return apr_pool_cleanup_run(cprng->pool, cprng, cprng_cleanup);
}

APR_DECLARE(apr_status_t) apr_crypto_prng_reseed(apr_crypto_prng_t *cprng,
                                                 const unsigned char seed[])
{
    apr_status_t rv = APR_SUCCESS;

#if APR_HAS_THREADS
    if (cprng->lock) {
        rv = apr_thread_mutex_lock(cprng->lock);
        if (rv != APR_SUCCESS) {
            return rv;
        }
    }
#endif

    if (seed) {
        apr_size_t n = 0;
        do {
            cprng->key[n] ^= seed[n];
        } while (++n < APR_CRYPTO_PRNG_KEY_SIZE);
    }
    else {
        rv = apr_generate_random_bytes(cprng->key, APR_CRYPTO_PRNG_KEY_SIZE);
    }
    apr_crypto_memzero(cprng->buf, cprng->len);
    cprng->pos = cprng->len;

#if APR_HAS_THREADS
    if (cprng->lock) {
        apr_status_t rt = apr_thread_mutex_unlock(cprng->lock);
        if (rv == APR_SUCCESS && rt != APR_SUCCESS) {
            rv = rt;
        }
    }
#endif

    return rv;
}

static APR_INLINE
apr_status_t cprng_stream_mix(apr_crypto_prng_t *cprng, unsigned char *to)
{
    return cprng_stream_ctx_mix(&cprng->ctx, cprng->key, to,
                                cprng->buf, cprng->len);
}

APR_DECLARE(apr_status_t) apr_crypto_prng_bytes(apr_crypto_prng_t *cprng,
                                                void *buf, apr_size_t len)
{
    unsigned char *ptr = buf;
    apr_status_t rv;
    apr_size_t n;

#if APR_HAS_THREADS
    if (cprng->lock) {
        rv = apr_thread_mutex_lock(cprng->lock);
        if (rv != APR_SUCCESS) {
            return rv;
        }
    }
#endif

    while (len) {
        if (cprng->pos == cprng->len) {
            if (len >= cprng->len) {
                do {
                    rv = cprng_stream_mix(cprng, ptr);
                    if (rv != APR_SUCCESS) {
                        return rv;
                    }
                    ptr += cprng->len;
                    len -= cprng->len;
                } while (len >= cprng->len);
                if (!len) {
                    break;
                }
            }
            rv = cprng_stream_mix(cprng, cprng->buf);
            if (rv != APR_SUCCESS) {
                return rv;
            }
            cprng->pos = 0;
        }

        /* Random bytes are consumed then zero-ed to ensure
         * both forward secrecy and cleared next mixed data.
         */
        n = cprng->len - cprng->pos;
        if (n > len) {
            n = len;
        }
        memcpy(ptr, cprng->buf + cprng->pos, n);
        apr_crypto_memzero(cprng->buf + cprng->pos, n);
        cprng->pos += n;

        ptr += n;
        len -= n;
    }

#if APR_HAS_THREADS
    if (cprng->lock) {
        apr_status_t rt = apr_thread_mutex_unlock(cprng->lock);
        if (rv == APR_SUCCESS && rt != APR_SUCCESS) {
            rv = rt;
        }
    }
#endif

    return APR_SUCCESS;
}

#endif /* APU_HAVE_CRYPTO_PRNG */
#endif /* APU_HAVE_CRYPTO */
