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

#include "apr_private.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_signal.h"
#include "apr_hash.h"
#include "ShellAPI.h"

#include "apr_arch_misc.h"       /* for WSAHighByte / WSALowByte */
#include "wchar.h"
#include "apr_arch_file_io.h"
#include "apr_arch_threadproc.h"
#include "assert.h"

/* This symbol is _private_, although it must be exported.
 */
int APR_DECLARE_DATA apr_app_init_complete = 0;

/* Used by apr_app_initialize to reprocess the environment
 *
 * An internal apr function to convert a double-null terminated set
 * of single-null terminated strings from wide Unicode to narrow utf-8
 * as a list of strings.  These are allocated from the MSVCRT's
 * _CRT_BLOCK to trick the system into trusting our store.
 */
static int warrsztoastr(const char * const * *retarr,
                        const wchar_t * arrsz, int args)
{
    const apr_wchar_t *wch;
    apr_size_t totlen;
    apr_size_t newlen;
    apr_size_t wsize;
    char **newarr;
    int arg;

    if (args < 0) {
        for (args = 1, wch = arrsz; wch[0] || wch[1]; ++wch)
            if (!*wch)
                ++args;
    }
    wsize = 1 + wch - arrsz;

    newarr = apr_malloc_dbg((args + 1) * sizeof(char *),
                            __FILE__, __LINE__);

    /* This is a safe max allocation, we will realloc after
     * processing and return the excess to the free store.
     * 3 ucs bytes hold any single wchar_t value (16 bits)
     * 4 ucs bytes will hold a wchar_t pair value (20 bits)
     */
    newlen = totlen = wsize * 3 + 1;
    newarr[0] = apr_malloc_dbg(newlen * sizeof(char),
                               __FILE__, __LINE__);

    (void)apr_conv_ucs2_to_utf8(arrsz, &wsize,
                                newarr[0], &newlen);

    assert(newlen && !wsize);
    /* Return to the free store if the heap realloc is the least bit optimized
     */
    newarr[0] = apr_realloc_dbg(newarr[0], totlen - newlen,
                                __FILE__, __LINE__);

    for (arg = 1; arg < args; ++arg) {
        newarr[arg] = newarr[arg - 1] + 2;
        while (*(newarr[arg]++)) {
            /* continue */;
        }
    }

    newarr[arg] = NULL;

    *retarr = newarr;
    return args;
}

/* Reprocess the arguments to main() for a completely apr-ized application
 */

APR_DECLARE(apr_status_t) apr_app_initialize(int *argc,
                                             const char * const * *argv,
                                             const char * const * *env)
{
    apr_status_t rv = apr_initialize();

    if (rv != APR_SUCCESS) {
        return rv;
    }

#if APR_HAS_UNICODE_FS
    IF_WIN_OS_IS_UNICODE
    {
        apr_wchar_t **wstrs;
        apr_wchar_t *sysstr;
        int wstrc;
        int dupenv;

        if (apr_app_init_complete) {
            return rv;
        }

        apr_app_init_complete = 1;

        sysstr = GetCommandLineW();
        if (sysstr) {
            wstrs = CommandLineToArgvW(sysstr, &wstrc);
            if (wstrs) {
                *argc = apr_wastrtoastr(argv, wstrs, wstrc);
                GlobalFree(wstrs);
            }
        }

        sysstr = GetEnvironmentStringsW();
        dupenv = warrsztoastr(&_environ, sysstr, -1);

        if (env) {
            *env = apr_malloc_dbg((dupenv + 1) * sizeof (char *),
                                  __FILE__, __LINE__ );
            memcpy((void*)*env, _environ, (dupenv + 1) * sizeof (char *));
        }
        else {
        }

        FreeEnvironmentStringsW(sysstr);

        /* MSVCRT will attempt to maintain the wide environment calls
         * on _putenv(), which is bogus if we've passed a non-ascii
         * string to _putenv(), since they use MultiByteToWideChar
         * and breaking the implicit utf-8 assumption we've built.
         *
         * Reset _wenviron for good measure.
         */
        if (_wenviron) {
            apr_wchar_t **wenv = _wenviron;
            _wenviron = NULL;
            free(wenv);
        }

    }
#endif
    return rv;
}

static int initialized = 0;

/* Provide to win32/thread.c */
extern DWORD tls_apr_thread;

APR_DECLARE(apr_status_t) apr_initialize(void)
{
    apr_pool_t *pool;
    apr_status_t status;
    int iVersionRequested;
    WSADATA wsaData;
    int err;
    apr_oslevel_e osver;

    if (initialized++) {
        return APR_SUCCESS;
    }

    /* Initialize apr_os_level global */
    if (apr_get_oslevel(&osver) != APR_SUCCESS) {
        return APR_EEXIST;
    }

    tls_apr_thread = TlsAlloc();
    if ((status = apr_pool_initialize()) != APR_SUCCESS)
        return status;

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        return APR_ENOPOOL;
    }

    apr_pool_tag(pool, "apr_initialize");

#if defined(APR_DECLARE_EXPORT)
    /* Initialize threadpriv table */
    apr_tls_threadkeys = apr_hash_make(pool);
#endif

    iVersionRequested = MAKEWORD(WSAHighByte, WSALowByte);
    err = WSAStartup((WORD) iVersionRequested, &wsaData);
    if (err) {
        return err;
    }
    if (LOBYTE(wsaData.wVersion) != WSAHighByte ||
        HIBYTE(wsaData.wVersion) != WSALowByte) {
        WSACleanup();
        return APR_EEXIST;
    }

    apr_signal_init(pool);

    return APR_SUCCESS;
}

#if defined(APR_DECLARE_EXPORT)
typedef (apr_thredkey_destfn_t)(void *data);

static void threadkey_terminate()
{
    apr_hash_index_t *hi = apr_hash_first(NULL, apr_tls_threadkeys);

    for (; hi != NULL; hi = apr_hash_next(hi)) {
        LPDWORD key;
        apr_hash_this(hi, &key, NULL, NULL);
        TlsFree(*key);
    }
}

static void threadkey_detach()
{
    apr_hash_index_t *hi = apr_hash_first(NULL, apr_tls_threadkeys);

    for (; hi != NULL; hi = apr_hash_next(hi)) {
        apr_thredkey_destfn_t *dest = NULL;
        LPDWORD key;
        void *data;
        apr_hash_this(hi, &key, NULL, (void **)&dest);
        data = TlsGetValue(*key);
        if (data != NULL || GetLastError() == ERROR_SUCCESS) {
            /* NULL data is a valid TLS value if explicitly set
             * by the TlsSetValue
             */
            (*dest)(data);
        }
    }
}

BOOL APIENTRY DllMain(HINSTANCE instance,
                      DWORD  reason_for_call,
                      LPVOID lpReserved)
{
    switch (reason_for_call) {
        case DLL_PROCESS_ATTACH:
        break;
        case DLL_THREAD_ATTACH:
        break;
        case DLL_THREAD_DETACH:
            threadkey_detach();
        break;
        case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

#endif /* APR_DECLARE_EXPORT */

APR_DECLARE_NONSTD(void) apr_terminate(void)
{
    initialized--;
    if (initialized) {
        return;
    }
#if defined(APR_DECLARE_EXPORT)
    threadkey_terminate();
#endif
    apr_pool_terminate();

    WSACleanup();

    TlsFree(tls_apr_thread);
}

APR_DECLARE(void) apr_terminate2(void)
{
    apr_terminate();
}
