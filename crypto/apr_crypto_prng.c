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
 *
 * The CPRNG key is changed as soon as it's used to initialize the stream
 * cipher, so it never resides in memory at the same time as the keystream
 * it produced (a.k.a. the random bytes, which for efficiency are pooled).
 *
 * Likewise, the keystream is always cleared from the internal state before
 * being returned to the user, thus there is no way to recover the produced
 * random bytes afterward (e.g. from a memory/core dump after a crash).
 *
 * IOW, this CPRNG ensures forward secrecy, one may want to run it in a process
 * and/or environment protected from live memory eavesdropping, thus keep the
 * pooled/future random bytes secret by design, and use it as a replacement
 * for some blocking/inefficient system RNG. The random bytes could then be
 * serviced through a named pipe/socket, RPC, or any specific API. This is
 * outside the scope of this/below code, though.
 *
 * [1] https://blog.cr.yp.to/20170723-random.html
 * [2] https://libpqcrypto.org/
 */

#include "apu.h"

#include "apr_crypto.h"
#include "apr_crypto_internal.h"

#if APU_HAVE_CRYPTO
#if APU_HAVE_CRYPTO_PRNG

#include "apr_ring.h"
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_thread_mutex.h"
#include "apr_thread_proc.h"

#include <stdlib.h> /* for malloc() */

/* Be consistent with the .h (the seed is xor-ed with key on reseed). */
#if CPRNG_KEY_SIZE != APR_CRYPTO_PRNG_SEED_SIZE
#error apr_crypto_prng handles stream ciphers with 256bit keys only
#endif

#define CPRNG_BUF_SIZE_MIN (CPRNG_KEY_SIZE * (8 - 1))
#define CPRNG_BUF_SIZE_DEF (CPRNG_KEY_SIZE * (24 - 1))

APR_TYPEDEF_STRUCT(apr_crypto_t,
    apr_pool_t *pool;
    apr_crypto_driver_t *provider;
)

struct apr_crypto_prng_t {
    APR_RING_ENTRY(apr_crypto_prng_t) link;
    apr_pool_t *pool;
    apr_crypto_t *crypto;
    cprng_stream_ctx_t *ctx;
#if APR_HAS_THREADS
    apr_thread_mutex_t *mutex;
#endif
    unsigned char *key, *buf;
    apr_size_t len, pos;
    int flags;
    apr_crypto_cipher_e cipher;
};

static apr_crypto_prng_t *cprng_global = NULL;
static APR_RING_HEAD(apr_cprng_ring, apr_crypto_prng_t) *cprng_ring;

#if APR_HAS_THREADS
static apr_thread_mutex_t *cprng_ring_mutex;

static apr_threadkey_t *cprng_thread_key = NULL;

#define cprng_lock(g) \
    if ((g)->mutex) \
        apr_thread_mutex_lock((g)->mutex)

#define cprng_unlock(g) \
    if ((g)->mutex) \
        apr_thread_mutex_unlock((g)->mutex)

#define cprng_ring_lock() \
    if (cprng_ring_mutex) \
        apr_thread_mutex_lock(cprng_ring_mutex)

#define cprng_ring_unlock() \
    if (cprng_ring_mutex) \
        apr_thread_mutex_unlock(cprng_ring_mutex)

static void cprng_thread_destroy(void *cprng)
{
    if (!cprng_global) {
        apr_threadkey_private_delete(cprng_thread_key);
        cprng_thread_key = NULL;
    }
    apr_crypto_prng_destroy(cprng);
}

#else  /* !APR_HAS_THREADS */
#define cprng_lock(g)
#define cprng_unlock(g)
#define cprng_ring_lock()
#define cprng_ring_unlock()
#endif /* !APR_HAS_THREADS */

APR_DECLARE(apr_status_t) apr_crypto_prng_init(apr_pool_t *pool, apr_crypto_t *crypto,
        apr_crypto_cipher_e cipher, apr_size_t bufsize, const unsigned char seed[], int flags)
{
    apr_status_t rv;

    if (cprng_global) {
        return APR_EREINIT;
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

    cprng_ring = apr_palloc(pool, sizeof(*cprng_ring));
    if (!cprng_ring) {
        return APR_ENOMEM;
    }
    APR_RING_INIT(cprng_ring, apr_crypto_prng_t, link);

#if APR_HAS_THREADS
    rv = apr_thread_mutex_create(&cprng_ring_mutex, APR_THREAD_MUTEX_DEFAULT,
                                 pool);
    if (rv != APR_SUCCESS) {
        apr_threadkey_private_delete(cprng_thread_key);
        cprng_thread_key = NULL;
        return rv;
    }

    /* Global CPRNG is locked (and obviously not per-thread) */
    flags = (flags | APR_CRYPTO_PRNG_LOCKED) & ~APR_CRYPTO_PRNG_PER_THREAD;
#endif

    return apr_crypto_prng_create(&cprng_global, crypto, cipher, bufsize, flags,
            seed, pool);
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

    if (!cprng_thread_key || !cprng_global) {
        return APR_EINIT;
    }

    rv = apr_threadkey_private_get(&private, cprng_thread_key);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    cprng = private;
    if (!cprng) {
        rv = apr_crypto_prng_create(&cprng, cprng_global->crypto, cprng_global->cipher, 0,
                APR_CRYPTO_PRNG_PER_THREAD, NULL, NULL);
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
#if APR_HAS_THREADS
        cprng_ring_mutex = NULL;
#endif
        cprng_ring = NULL;
    }
    else if (cprng_global && !(cprng->flags & APR_CRYPTO_PRNG_PER_THREAD)) {
        cprng_ring_lock();
        APR_RING_REMOVE(cprng, link);
        cprng_ring_unlock();
    }

    if (cprng->ctx) {
        cprng->crypto->provider->cprng_stream_ctx_free(cprng->ctx);
    }

    if (cprng->key) {
        apr_crypto_memzero(cprng->key, CPRNG_KEY_SIZE + cprng->len);
    }

    if (!cprng->pool) {
        free(cprng->key);
        free(cprng);
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_create(apr_crypto_prng_t **pcprng,
        apr_crypto_t *crypto, apr_crypto_cipher_e cipher, apr_size_t bufsize, int flags,
        const unsigned char seed[], apr_pool_t *pool)
{
    apr_status_t rv;
    apr_crypto_prng_t *cprng;

    *pcprng = NULL;

    if (!cprng_global && pcprng != &cprng_global) {
        return APR_EINIT;
    }

    if (bufsize > APR_INT32_MAX - CPRNG_KEY_SIZE
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
    cprng->cipher = cipher;
    cprng->flags = flags;
    cprng->pool = pool;

    if (bufsize == 0) {
        bufsize = CPRNG_BUF_SIZE_DEF;
    }
    else if (bufsize < CPRNG_BUF_SIZE_MIN) {
        bufsize = CPRNG_BUF_SIZE_MIN;
    }
    else if (bufsize % CPRNG_KEY_SIZE) {
        bufsize += CPRNG_KEY_SIZE;
        bufsize -= bufsize % CPRNG_KEY_SIZE;
    }
    if (pool) {
        cprng->key = apr_palloc(pool, CPRNG_KEY_SIZE + bufsize);
    }
    else {
        cprng->key = malloc(CPRNG_KEY_SIZE + bufsize);
    }
    if (!cprng->key) {
        cprng_cleanup(cprng);
        return APR_ENOMEM;
    }
    cprng->buf = cprng->key + CPRNG_KEY_SIZE;
    cprng->len = bufsize;

    if (crypto) {
        cprng->crypto = crypto;
    }
    else if (cprng_global && cprng_global->crypto) {
        cprng->crypto = cprng_global->crypto;
    }
    else {
        const apr_crypto_driver_t *driver = NULL;
        if (!pool) {
            return APR_EINVAL;
        }

        rv = apr_crypto_init(pool);
        if (rv != APR_SUCCESS) {
            cprng_cleanup(cprng);
            return rv;
        }

        rv = apr_crypto_get_driver(&driver, "openssl",
                NULL, NULL, pool);
        if (rv != APR_SUCCESS) {
            cprng_cleanup(cprng);
            return rv;
        }

        rv = apr_crypto_make(&cprng->crypto, driver, NULL, pool);
        if (rv != APR_SUCCESS) {
            cprng_cleanup(cprng);
            return rv;
        }
    }

    rv = cprng->crypto->provider->cprng_stream_ctx_make(&cprng->ctx,
            cprng->crypto, cprng->cipher, pool);
    if (rv != APR_SUCCESS) {
        cprng_cleanup(cprng);
        return rv;
    }

    if (seed) {
        memset(cprng->key, 0, CPRNG_KEY_SIZE);
    }
    rv = apr_crypto_prng_reseed(cprng, seed);
    if (rv != APR_SUCCESS) {
        cprng_cleanup(cprng);
        return rv;
    }

#if APR_HAS_THREADS
    if (flags & APR_CRYPTO_PRNG_LOCKED) {
        rv = apr_thread_mutex_create(&cprng->mutex, APR_THREAD_MUTEX_DEFAULT,
                                     pool);
        if (rv != APR_SUCCESS) {
            cprng_cleanup(cprng);
            return rv;
        }
    }
#endif

    if (cprng_global && !(flags & APR_CRYPTO_PRNG_PER_THREAD)) {
        cprng_ring_lock();
        APR_RING_INSERT_TAIL(cprng_ring, cprng, apr_crypto_prng_t, link);
        cprng_ring_unlock();
    }
    else {
        APR_RING_ELEM_INIT(cprng, link);
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

static apr_status_t cprng_stream_bytes(apr_crypto_prng_t *cprng,
                                       void *to, apr_size_t len)
{
    apr_status_t rv;

    rv = cprng->crypto->provider->cprng_stream_ctx_bytes(&cprng->ctx,
            cprng->key, to, len, cprng->buf);
    if (rv != APR_SUCCESS && len) {
        apr_crypto_memzero(to, len);
    }
    return rv;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_reseed(apr_crypto_prng_t *cprng,
                                                 const unsigned char seed[])
{
    apr_status_t rv = APR_SUCCESS;

    if (!cprng) {
        /* Fall through with global CPRNG. */
        cprng = cprng_global;
        if (!cprng) {
            return APR_EINIT;
        }
    }

    cprng_lock(cprng);

    cprng->pos = cprng->len;
    apr_crypto_memzero(cprng->buf, cprng->len);
    if (seed) {
        apr_size_t n = 0;
        do {
            cprng->key[n] ^= seed[n];
        } while (++n < CPRNG_KEY_SIZE);
    }
    else if (cprng_global && cprng_global != cprng) {
        /* Use the global CPRNG: no need for more than the initial entropy. */
        rv = apr_crypto_random_bytes(cprng->key, CPRNG_KEY_SIZE);
    }
    else {
        /* Use the system entropy, i.e. one of "/dev/[u]random", getrandom(),
         * arc4random()... This may block but still we really want to wait for
         * the system to gather enough entropy for these 32 initial bytes, much
         * more than we want non-random bytes, and that's once and for all! 
         */
        rv = apr_generate_random_bytes(cprng->key, CPRNG_KEY_SIZE);
    }
    if (rv == APR_SUCCESS) {
        /* Init/set the stream with the new key, without buffering for now
         * so that the buffer and/or the next random bytes won't be generated
         * directly from this key but from the first stream bytes it generates,
         * i.e. the next key is always extracted from the stream cipher state
         * and cleared upon use.
         */
        rv = cprng_stream_bytes(cprng, NULL, 0);
    }

    cprng_unlock(cprng);

    return rv;
}

static apr_status_t cprng_bytes(apr_crypto_prng_t *cprng,
                                void *buf, apr_size_t len)
{
    unsigned char *ptr = buf;
    apr_status_t rv;
    apr_size_t n;

    while (len) {
        n = cprng->len - cprng->pos;
        if (n == 0) {
            n = cprng->len;
            if (len >= n) {
                do {
                    rv = cprng_stream_bytes(cprng, ptr, n);
                    if (rv != APR_SUCCESS) {
                        return rv;
                    }
                    ptr += n;
                    len -= n;
                } while (len >= n);
                if (!len) {
                    break;
                }
            }
            rv = cprng_stream_bytes(cprng, cprng->buf, n);
            if (rv != APR_SUCCESS) {
                return rv;
            }
            cprng->pos = 0;
        }
        if (n > len) {
            n = len;
        }

        /* Random bytes are consumed then zero-ed to ensure
         * both forward secrecy and cleared next mixed data.
         */
        memcpy(ptr, cprng->buf + cprng->pos, n);
        apr_crypto_memzero(cprng->buf + cprng->pos, n);
        cprng->pos += n;

        ptr += n;
        len -= n;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_bytes(apr_crypto_prng_t *cprng,
                                                void *buf, apr_size_t len)
{
    apr_status_t rv;

    if (!cprng) {
        /* Fall through with global CPRNG. */
        cprng = cprng_global;
        if (!cprng) {
            return APR_EINIT;
        }
    }

    cprng_lock(cprng);

    rv = cprng_bytes(cprng, buf, len);

    cprng_unlock(cprng);

    return rv;
}

APR_DECLARE(apr_status_t) apr_crypto_prng_rekey(apr_crypto_prng_t *cprng)
{
    apr_status_t rv;

    if (!cprng) {
        /* Fall through with global CPRNG. */
        cprng = cprng_global;
        if (!cprng) {
            return APR_EINIT;
        }
    }

    cprng_lock(cprng);

    /* Clear state and renew the key. */
    cprng->pos = cprng->len;
    apr_crypto_memzero(cprng->buf, cprng->len);
    rv = cprng_stream_bytes(cprng, NULL, 0);

    cprng_unlock(cprng);

    if (cprng == cprng_global) {
        /* Forward to all maintained CPRNGs. */
        cprng_ring_lock();
        for (cprng = APR_RING_FIRST(cprng_ring);
             cprng != APR_RING_SENTINEL(cprng_ring, apr_crypto_prng_t, link);
             cprng = APR_RING_NEXT(cprng, link)) {
            apr_status_t rt = apr_crypto_prng_rekey(cprng);
            if (rt != APR_SUCCESS && rv == APR_SUCCESS) {
                rv = rt;
            }
        }
        cprng_ring_unlock();
    }

    return rv;
}

#if APR_HAS_FORK
APR_DECLARE(apr_status_t) apr_crypto_prng_after_fork(apr_crypto_prng_t *cprng,
                                                     int flags)
{
    apr_status_t rv = APR_SUCCESS;
    int is_child = flags & APR_CRYPTO_FORK_INCHILD;

    if (!cprng) {
        /* Fall through with global CPRNG. */
        cprng = cprng_global;
        if (!cprng) {
            return APR_EINIT;
        }
    }

    cprng_lock(cprng);

    /* Make sure the parent and child processes never share the same state,
     * and that further fork()s from either process will not either.
     * This is done by rekeying (and clearing the buffers) in both processes,
     * and by rekeying a second time in the parent process to ensure both that
     * keys are different and that after apr_crypto_prng_after_fork() is called
     * the keys are unknown to each other processes.
     * The new key to be used by the parent process is generated in the same
     * pass as the rekey, and since cprng_stream_bytes() is designed to burn
     * and never reuse keys we are sure that this key is unique to the parent,
     * and that nothing is left over from the initial state in both processes.
     */
    cprng->pos = cprng->len;
    apr_crypto_memzero(cprng->buf, cprng->len);
    if (!is_child) {
        rv = cprng_stream_bytes(cprng, cprng->key, CPRNG_KEY_SIZE);
    }
    if (rv == APR_SUCCESS) {
        rv = cprng_stream_bytes(cprng, NULL, 0);
    }

    cprng_unlock(cprng);

    if (cprng == cprng_global) {
        /* Forward to all maintained CPRNGs. */
        cprng_ring_lock();
        for (cprng = APR_RING_FIRST(cprng_ring);
             cprng != APR_RING_SENTINEL(cprng_ring, apr_crypto_prng_t, link);
             cprng = APR_RING_NEXT(cprng, link)) {
            apr_status_t rt = apr_crypto_prng_after_fork(cprng, flags);
            if (rt != APR_SUCCESS && rv == APR_SUCCESS) {
                rv = rt;
            }
        }
        cprng_ring_unlock();
    }

    return rv;
}
#endif /* APR_HAS_FORK */

#endif /* APU_HAVE_CRYPTO_PRNG */
#endif /* APU_HAVE_CRYPTO */
