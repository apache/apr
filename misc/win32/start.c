/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include "apr_private.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_signal.h"
#include "ShellAPI.h"

#include "apr_arch_misc.h"       /* for WSAHighByte / WSALowByte */
#include "wchar.h"
#include "apr_arch_file_io.h"
#include "crtdbg.h"
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

    newarr = _malloc_dbg((args + 1) * sizeof(char *),
                         _CRT_BLOCK, __FILE__, __LINE__);

    /* This is a safe max allocation, we will realloc after
     * processing and return the excess to the free store.
     * 3 ucs bytes hold any single wchar_t value (16 bits)
     * 4 ucs bytes will hold a wchar_t pair value (20 bits)
     */
    newlen = totlen = wsize * 3 + 1;
    newarr[0] = _malloc_dbg(newlen * sizeof(char), 
                            _CRT_BLOCK, __FILE__, __LINE__);

    (void)apr_conv_ucs2_to_utf8(arrsz, &wsize,
                                newarr[0], &newlen);

    assert(newlen && !wsize);
    /* Return to the free store if the heap realloc is the least bit optimized
     */
    newarr[0] = _realloc_dbg(newarr[0], totlen - newlen, 
                             _CRT_BLOCK, __FILE__, __LINE__);

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
            *env = _malloc_dbg((dupenv + 1) * sizeof (char *), 
                               _CRT_BLOCK, __FILE__, __LINE__ );
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

APR_DECLARE_NONSTD(void) apr_terminate(void)
{
    initialized--;
    if (initialized) {
        return;
    }
    apr_pool_terminate();
    
    WSACleanup();

    TlsFree(tls_apr_thread);
}

APR_DECLARE(void) apr_terminate2(void)
{
    apr_terminate();
}
