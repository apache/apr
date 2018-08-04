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
#include "apr_crypto.h"
#include "apr_crypto_internal.h"
#include "apr_strings.h"

#if APU_HAVE_CRYPTO

#if APU_HAVE_OPENSSL
#include "apr_thread_mutex.h"

#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/conf.h>
#include <openssl/comp.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

#if APR_HAS_THREADS && APR_USE_OPENSSL_PRE_1_1_API
static apr_status_t ossl_thread_setup(apr_pool_t *pool);
#else
static APR_INLINE apr_status_t ossl_thread_setup(apr_pool_t *pool)
{
    return APR_SUCCESS;
}
#endif

const char *apr__crypto_openssl_version(void)
{
    return OPENSSL_VERSION_TEXT;
}

apr_status_t apr__crypto_openssl_init(const char *params,
                                      const apu_err_t **result,
                                      apr_pool_t *pool)
{
    /* Both undefined (or no-op) with LibreSSL */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    CRYPTO_malloc_init();
#elif !defined(LIBRESSL_VERSION_NUMBER)
    OPENSSL_malloc_init();
#endif
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
    ENGINE_load_builtin_engines();
    ENGINE_register_all_complete();

    SSL_load_error_strings();
    SSL_library_init();

    return ossl_thread_setup(pool);
}

apr_status_t apr__crypto_openssl_term(void)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(LIBRESSL_VERSION_NUMBER)

#ifdef OPENSSL_FIPS
    FIPS_mode_set(0);
#endif
    CONF_modules_unload(1);
    OBJ_cleanup();
    EVP_cleanup();
#if !defined(LIBRESSL_VERSION_NUMBER)
    RAND_cleanup();
#endif
    ENGINE_cleanup();
#ifndef OPENSSL_NO_COMP
    COMP_zlib_cleanup();
#endif
#if OPENSSL_VERSION_NUMBER >= 0x1000000fL
    ERR_remove_thread_state(NULL);
#else
    ERR_remove_state(0);
#endif
    ERR_free_strings();
    CRYPTO_cleanup_all_ex_data();

#else   /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
    OPENSSL_cleanup();
#endif  /* OPENSSL_VERSION_NUMBER >= 0x10100000L */

    return APR_SUCCESS;
}

/*
 * To ensure thread-safetyness in OpenSSL - work in progress
 * Taken from httpd's mod_ssl code.
 */

#if APR_HAS_THREADS && APR_USE_OPENSSL_PRE_1_1_API

static apr_thread_mutex_t **ossl_locks;
static int                  ossl_num_locks;

static void ossl_thread_locking(int mode, int type, const char *file, int line)
{
    if (type < ossl_num_locks) {
        if (mode & CRYPTO_LOCK) {
            (void)apr_thread_mutex_lock(ossl_locks[type]);
        }
        else {
            (void)apr_thread_mutex_unlock(ossl_locks[type]);
        }
    }
}

/* Dynamic lock structure */
struct CRYPTO_dynlock_value {
    apr_pool_t *pool;
    apr_thread_mutex_t *mutex;
    const char* file;
    int line;
};

/* Global reference to the pool passed into ossl_thread_setup() */
static apr_pool_t *ossl_dynlock_pool = NULL;

/*
 * Dynamic lock creation callback
 */
static
struct CRYPTO_dynlock_value *ossl_dynlock_create(const char *file, int line)
{
    struct CRYPTO_dynlock_value *value;
    apr_status_t rv;
    apr_pool_t *p;

    /*
     * We need a pool to allocate our mutex.  Since we can't clear
     * allocated memory from a pool, create a subpool that we can blow
     * away in the destruction callback.
     */
    rv = apr_pool_create(&p, ossl_dynlock_pool);
    if (rv != APR_SUCCESS) {
        return NULL;
    }

    value = apr_palloc(p, sizeof(*value));

    rv = apr_thread_mutex_create(&value->mutex, APR_THREAD_MUTEX_DEFAULT, p);
    if (rv != APR_SUCCESS) {
        apr_pool_destroy(p);
        return NULL;
    }

    /* Keep our own copy of the place from which we were created,
       using our own pool. */
    value->file = apr_pstrdup(p, file);
    value->line = line;
    value->pool = p;
    return value;
}

/*
 * Dynamic locking and unlocking function
 */

static void ossl_dynlock_locking(int mode, struct CRYPTO_dynlock_value *l,
                                 const char *file, int line)
{
    if (mode & CRYPTO_LOCK) {
        (void)apr_thread_mutex_lock(l->mutex);
    }
    else {
        (void)apr_thread_mutex_unlock(l->mutex);
    }
}

/*
 * Dynamic lock destruction callback
 */
static void ossl_dynlock_destroy(struct CRYPTO_dynlock_value *l,
                                 const char *file, int line)
{
    /* Trust that whomever owned the CRYPTO_dynlock_value we were
     * passed has no future use for it...
     */
    apr_pool_destroy(l->pool);
}

/* Windows and BeOS can use the default THREADID callback shipped with OpenSSL
 * 1.0.x, as can any platform with a thread-safe errno.
 */
#define OSSL_DEFAULT_THREADID_IS_SAFE (OPENSSL_VERSION_NUMBER >= 0x10000000L \
                                       && (defined(_REENTRANT) \
                                           || __BEOS__ \
                                           || _WIN32 \
                                           ))
#if OSSL_DEFAULT_THREADID_IS_SAFE

/* We don't need to set up a threadid callback on this platform. */
static APR_INLINE apr_status_t ossl_thread_id_setup(apr_pool_t *pool)
{
    return APR_SUCCESS;
}

static APR_INLINE apr_status_t ossl_thread_id_cleanup(void)
{
    return APR_SUCCESS;
}

#else

/**
 * Used by both versions of ossl_thread_id(). Returns an unsigned long that
 * should be unique to the currently executing thread.
 */
static unsigned long ossl_thread_id_internal(void)
{
    /* OpenSSL needs this to return an unsigned long.  On OS/390, the pthread
     * id is a structure twice that big.  Use the TCB pointer instead as a
     * unique unsigned long.
     */
#ifdef __MVS__
    struct PSA {
        char unmapped[540]; /* PSATOLD is at offset 540 in the PSA */
        unsigned long PSATOLD;
    } *psaptr = 0; /* PSA is at address 0 */

    return psaptr->PSATOLD;
#else
    return (unsigned long) apr_os_thread_current();
#endif
}

#ifndef OPENSSL_NO_DEPRECATED

static unsigned long ossl_thread_id(void)
{
    return ossl_thread_id_internal();
}

#else

static void ossl_thread_id(CRYPTO_THREADID *id)
{
    /* XXX Ideally we would be using the _set_pointer() callback on platforms
     * that have a pointer-based thread "identity". But this entire API is
     * fraught with problems (see PR60947) and has been removed completely in
     * OpenSSL 1.1.0, so I'm not too invested in fixing it right now. */
    CRYPTO_THREADID_set_numeric(id, ossl_thread_id_internal());
}

#endif /* OPENSSL_NO_DEPRECATED */

static apr_status_t ossl_thread_id_cleanup(void)
{
#ifndef OPENSSL_NO_DEPRECATED
    CRYPTO_set_id_callback(NULL);
#else
    /* XXX This does nothing. The new-style THREADID callback is write-once. */
    CRYPTO_THREADID_set_callback(NULL);
#endif

    return APR_SUCCESS;
}

static apr_status_t ossl_thread_id_setup(apr_pool_t *pool)
{
#ifndef OPENSSL_NO_DEPRECATED
    /* This API is deprecated, but we prefer it to its replacement since it
     * allows us to unset the callback when this module is being unloaded. */
    CRYPTO_set_id_callback(ossl_thread_id);
#else
    /* This is a last resort. We can only set this once, which means that we'd
     * better not get reloaded into a different address. See PR60947.
     */
    CRYPTO_THREADID_set_callback(ossl_thread_id);

    if (CRYPTO_THREADID_get_callback() != ossl_thread_id) {
        return APR_EGENERAL;
    }
#endif

    return APR_SUCCESS;
}

#endif /* OSSL_DEFAULT_THREADID_IS_SAFE */

static apr_status_t ossl_thread_cleanup(void *data)
{
    CRYPTO_set_dynlock_create_callback(NULL);
    CRYPTO_set_dynlock_lock_callback(NULL);
    CRYPTO_set_dynlock_destroy_callback(NULL);
    ossl_dynlock_pool = NULL;

    CRYPTO_set_locking_callback(NULL);

    ossl_thread_id_cleanup();

    return APR_SUCCESS;
}

static apr_status_t ossl_thread_setup(apr_pool_t *pool)
{
    apr_status_t rv;
    int i, num_locks;

    rv = ossl_thread_id_setup(pool);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    num_locks = CRYPTO_num_locks();
    ossl_locks = apr_palloc(pool, num_locks * sizeof(*ossl_locks));
    if (!ossl_locks) {
        return APR_ENOMEM;
    }
    for (i = 0; i < num_locks; i++) {
        rv = apr_thread_mutex_create(&ossl_locks[i],
                                     APR_THREAD_MUTEX_DEFAULT, pool);
        if (rv != APR_SUCCESS) {
            return rv;
        }
    }
    ossl_num_locks = num_locks;

    CRYPTO_set_locking_callback(ossl_thread_locking);

    /* Set up dynamic locking scaffolding for OpenSSL to use at its
     * convenience.
     */
    ossl_dynlock_pool = pool;
    CRYPTO_set_dynlock_create_callback(ossl_dynlock_create);
    CRYPTO_set_dynlock_lock_callback(ossl_dynlock_locking);
    CRYPTO_set_dynlock_destroy_callback(ossl_dynlock_destroy);

    apr_pool_cleanup_register(pool, NULL, ossl_thread_cleanup,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

#endif /* #if APR_HAS_THREADS && APR_USE_OPENSSL_PRE_1_1_API */


#endif /* APU_HAVE_OPENSSL */


#if APU_HAVE_NSS

#include <prerror.h>

#ifdef HAVE_NSS_NSS_H
#include <nss/nss.h>
#endif
#ifdef HAVE_NSS_H
#include <nss.h>
#endif

const char *apr__crypto_nss_version(void)
{
    return NSS_VERSION;
}

apr_status_t apr__crypto_nss_init(const char *params,
                                 const apu_err_t **result,
                                 apr_pool_t *pool)
{
    SECStatus s;
    const char *dir = NULL;
    const char *keyPrefix = NULL;
    const char *certPrefix = NULL;
    const char *secmod = NULL;
    int noinit = 0;
    PRUint32 flags = 0;

    struct {
        const char *field;
        const char *value;
        int set;
    } fields[] = {
        { "dir", NULL, 0 },
        { "key3", NULL, 0 },
        { "cert7", NULL, 0 },
        { "secmod", NULL, 0 },
        { "noinit", NULL, 0 },
        { NULL, NULL, 0 }
    };
    const char *ptr;
    size_t klen;
    char **elts = NULL;
    char *elt;
    int i = 0, j;
    apr_status_t status;

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
                if (klen && !strcasecmp(fields[j].field, elt)) {
                    fields[j].set = 1;
                    if (ptr) {
                        fields[j].value = ptr;
                    }
                    break;
                }
            }

            i++;
        }
        dir = fields[0].value;
        keyPrefix = fields[1].value;
        certPrefix = fields[2].value;
        secmod = fields[3].value;
        noinit = fields[4].set;
    }

    /* if we've been asked to bypass, do so here */
    if (noinit) {
        return APR_SUCCESS;
    }

    /* sanity check - we can only initialise NSS once */
    if (NSS_IsInitialized()) {
        return APR_EREINIT;
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
        if (result) {
            /* Note: all memory must be owned by the caller, in case we're unloaded */
            apu_err_t *err = apr_pcalloc(pool, sizeof(apu_err_t));
            err->rc = PR_GetError();
            err->msg = apr_pstrdup(pool, PR_ErrorToName(s));
            err->reason = apr_pstrdup(pool, "Error during 'nss' initialisation");
            *result = err;
        }

        return APR_ECRYPT;
    }

    return APR_SUCCESS;
}

apr_status_t apr__crypto_nss_term(void)
{
    if (NSS_IsInitialized()) {
        SECStatus s = NSS_Shutdown();
        if (s != SECSuccess) {
            fprintf(stderr, "NSS failed to shutdown, possible leak: %d: %s",
                PR_GetError(), PR_ErrorToName(s));
            return APR_EINIT;
        }
    }
    return APR_SUCCESS;
}

#endif /* APU_HAVE_NSS */


#if APU_HAVE_COMMONCRYPTO

const char *apr__crypto_commoncrypto_version(void)
{
    return NULL;
}

apr_status_t apr__crypto_commoncrypto_init(const char *params,
                                           const apu_err_t **result,
                                           apr_pool_t *pool)
{
    return APR_SUCCESS;
}

apr_status_t apr__crypto_commoncrypto_term(void)
{
    return APR_SUCCESS;
}

#endif /* APU_HAVE_COMMONCRYPTO */


#if APU_HAVE_MSCAPI

const char *apr__crypto_mscapi_version(void)
{
    return NULL;
}

apr_status_t apr__crypto_mscapi_init(const char *params,
                                     const apu_err_t **result,
                                     apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}

apr_status_t apr__crypto_mscapi_term(void)
{
    return APR_ENOTIMPL;
}

#endif /* APU_HAVE_MSCAPI */


#if APU_HAVE_MSCNG

const char *apr__crypto_mscng_version(void)
{
    return NULL;
}

apr_status_t apr__crypto_mscng_init(const char *params,
                                    const apu_err_t **result,
                                    apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}

apr_status_t apr__crypto_mscng_term(void)
{
    return APR_ENOTIMPL;
}

#endif /* APU_HAVE_MSCNG */

#endif /* APU_HAVE_CRYPTO */
