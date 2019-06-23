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
#include "apr_tables.h"
#include "apr_dso.h"
#include "apr_strings.h"
#include "apr_hash.h"
#include "apr_file_io.h"
#include "apr_env.h"
#include "apr_atomic.h"
#include "apr_version.h"

#include "apu_internal.h"

#if APR_HAVE_MODULAR_DSO

#if APR_HAS_THREADS
static apr_thread_mutex_t* mutex = NULL;
#endif
static apr_hash_t *dsos = NULL;
static apr_uint32_t in_init = 0, initialised = 0;

#if APR_HAS_THREADS
apr_status_t apu_dso_mutex_lock()
{
    return apr_thread_mutex_lock(mutex);
}
apr_status_t apu_dso_mutex_unlock()
{
    return apr_thread_mutex_unlock(mutex);
}
#else
apr_status_t apu_dso_mutex_lock() {
    return APR_SUCCESS;
}
apr_status_t apu_dso_mutex_unlock() {
    return APR_SUCCESS;
}
#endif

static apr_status_t apu_dso_term(void *ptr)
{
    if (apr_atomic_inc32(&in_init)) {
        while (apr_atomic_read32(&in_init) > 1); /* wait until we get fully inited */
    }

    /* Reference count - cleanup when last reference is cleaned up */
    if (!apr_atomic_dec32(&initialised)) {
        apr_pool_t *global = apr_hash_pool_get(dsos);

        apr_pool_destroy(global);

        /* set statics to NULL so init can work again */
        dsos = NULL;
#if APR_HAS_THREADS
        mutex = NULL;
#endif
    }

    apr_atomic_dec32(&in_init);

    /* Everything else we need is handled by cleanups registered
     * when we created mutexes and loaded DSOs
     */
    return APR_SUCCESS;
}

apr_status_t apu_dso_init(apr_pool_t *pool)
{
    apr_status_t ret = APR_SUCCESS;
    apr_pool_t *parent;

    if (apr_atomic_inc32(&in_init)) {
        while (apr_atomic_read32(&in_init) > 1); /* wait until we get fully inited */
    }

    /* Reference count increment... */
    if (!apr_atomic_inc32(&initialised)) {
        apr_pool_t *global;

        apr_pool_create_unmanaged(&global);
        dsos = apr_hash_make(global);

#if APR_HAS_THREADS
        ret = apr_thread_mutex_create(&mutex, APR_THREAD_MUTEX_DEFAULT, global);
        /* This already registers a pool cleanup */
#endif

    }

    /* Top level pool scope, need process-scope lifetime */
    for (parent = apr_pool_parent_get(pool);
         parent && parent != pool;
         parent = apr_pool_parent_get(pool))
        pool = parent;

    /* ... to be decremented on cleanup */
    apr_pool_cleanup_register(pool, NULL, apu_dso_term,
                              apr_pool_cleanup_null);

    apr_atomic_dec32(&in_init);

    return ret;
}

struct dso_entry {
    apr_dso_handle_t *handle;
    apr_dso_handle_sym_t *sym;
};

apr_status_t apu_dso_load(apr_dso_handle_t **dlhandleptr,
                          apr_dso_handle_sym_t *dsoptr,
                          const char *module,
                          const char *modsym,
                          apr_pool_t *pool)
{
    apr_dso_handle_t *dlhandle = NULL;
    char *pathlist;
    char path[APR_PATH_MAX + 1];
    apr_array_header_t *paths;
    apr_pool_t *global;
    apr_status_t rv = APR_EDSOOPEN;
    struct dso_entry *entry;
    char *eos = NULL;
    int i;

    entry = apr_hash_get(dsos, module, APR_HASH_KEY_STRING);
    if (entry) {
        *dlhandleptr = entry->handle;
        *dsoptr = entry->sym;
        return APR_EINIT;
    }

    /* The driver DSO must have exactly the same lifetime as the
     * drivers hash table; ignore the passed-in pool */
    global = apr_hash_pool_get(dsos);

    /* Retrieve our path search list or prepare for a single search */
    if ((apr_env_get(&pathlist, APR_DSOPATH, pool) != APR_SUCCESS)
          || (apr_filepath_list_split(&paths, pathlist, pool) != APR_SUCCESS))
        paths = apr_array_make(pool, 1, sizeof(char*));

#if defined(APR_DSO_LIBDIR)
    /* Always search our prefix path, but on some platforms such as
     * win32 this may be left undefined
     */
    (*((char **)apr_array_push(paths))) = APR_DSO_LIBDIR;
#endif

    for (i = 0; i < paths->nelts; ++i)
    {
#if defined(WIN32)
        /* Use win32 dso search semantics and attempt to
         * load the relative lib on the first pass.
         */
        if (!eos) {
            eos = path;
            --i;
        }
        else
#endif
        {
            eos = apr_cpystrn(path, ((char**)paths->elts)[i], sizeof(path));
            if ((eos > path) && (eos - path < sizeof(path) - 1))
                *(eos++) = '/';
        }
        apr_cpystrn(eos, module, sizeof(path) - (eos - path));

        rv = apr_dso_load(&dlhandle, path, global);
        if (dlhandleptr) {
            *dlhandleptr = dlhandle;
        }
        if (rv == APR_SUCCESS) { /* APR_EDSOOPEN */
            break;
        }
#if defined(APR_DSO_LIBDIR)
        else if (i < paths->nelts - 1) {
#else
        else {   /* No APR_DSO_LIBDIR to skip */
#endif
             /* try with apr-APR_MAJOR_VERSION appended */
            eos = apr_cpystrn(eos,
                              "apr-" APR_STRINGIFY(APR_MAJOR_VERSION) "/",
                              sizeof(path) - (eos - path));

            apr_cpystrn(eos, module, sizeof(path) - (eos - path));

            rv = apr_dso_load(&dlhandle, path, global);
            if (dlhandleptr) {
                *dlhandleptr = dlhandle;
            }
            if (rv == APR_SUCCESS) { /* APR_EDSOOPEN */
                break;
            }
        }
    }

    if (rv != APR_SUCCESS) /* APR_ESYMNOTFOUND */
        return rv;

    rv = apr_dso_sym(dsoptr, dlhandle, modsym);
    if (rv != APR_SUCCESS) { /* APR_ESYMNOTFOUND */
        apr_dso_unload(dlhandle);
    }
    else {
        module = apr_pstrdup(global, module);
        entry = apr_palloc(global, sizeof(*entry));
        entry->handle = dlhandle;
        entry->sym = *dsoptr;
        apr_hash_set(dsos, module, APR_HASH_KEY_STRING, entry);
    }
    return rv;
}

#endif /* APR_DSO_BUILD */

