/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

/* Usage Notes:
 *
 *   this module, and the i18n/unix/ucs2_utf8.c modules must be 
 *   compiled APR_EXPORT_STATIC and linked to an application with
 *   the /entry:wmainCRTStartup flag.  This module becomes the true
 *   wmain entry point, and passes utf-8 reformatted argv and env
 *   arrays to the application's main function.
 *
 *   This module is only compatible with Unicode-only executables.
 *   Mixed (Win9x backwards compatible) binaries should refer instead
 *   to the apr_startup.c module.
 *
 *   _dbg_malloc/realloc is used in place of the usual API, in order
 *   to convince the MSVCRT that they created these entities.  If we
 *   do not create them as _CRT_BLOCK entities, the crt will fault
 *   on an assert.  We are not worrying about the crt's locks here, 
 *   since we are single threaded [so far].
 */

#include "apr_general.h"
#include "ShellAPI.h"
#include "crtdbg.h"
#include "wchar.h"
#include "fileio.h"
#include "assert.h"
#include "apr_private.h"

static int wastrtoastr(char ***retarr, wchar_t **arr, int args)
{
    size_t elesize = 0;
    char **newarr;
    char *elements;
    char *ele;
    int arg;

    if (args < 0) {
        for (args = 0; arr[args]; ++args)
            ;
    }

    newarr = _malloc_dbg((args + 1) * sizeof(char *),
                         _CRT_BLOCK, __FILE__, __LINE__);

    for (arg = 0; arg < args; ++arg) {
        newarr[arg] = (void*)(wcslen(arr[arg]) + 1);
        elesize += (size_t)newarr[arg];
    }

    /* This is a safe max allocation, we will realloc after
     * processing and return the excess to the free store.
     * 3 ucs bytes hold any single wchar_t value (16 bits)
     * 4 ucs bytes will hold a wchar_t pair value (20 bits)
     */
    elesize = elesize * 3 + 1;
    ele = elements = _malloc_dbg(elesize * sizeof(char), 
                                 _CRT_BLOCK, __FILE__, __LINE__);

    for (arg = 0; arg < args; ++arg) {
        size_t len = (size_t)newarr[arg];
        size_t newlen = elesize;

        newarr[arg] = ele;
        (void)apr_conv_ucs2_to_utf8(arr[arg], &len,
                                    newarr[arg], &elesize);

        newlen -= elesize;
        ele += newlen;
        assert(elesize && (len == 0));
    }

    newarr[arg] = NULL;
    *(ele++) = '\0';

    /* Return to the free store if the heap realloc is the least bit optimized
     */
    ele = _realloc_dbg(elements, ele - elements, 
                       _CRT_BLOCK, __FILE__, __LINE__);

    if (ele != elements) {
        size_t diff = ele - elements;
        for (arg = 0; arg < args; ++arg) {
            newarr[arg] += diff;
        }
    }

    *retarr = newarr;
    return args;
}

#ifdef APR_APP

extern int main(int argc, char **argv, char **env);

int wmain(int argc, wchar_t **wargv, wchar_t **wenv)
{
    char **argv;
    char **env;
    int dupenv;

    (void)wastrtoastr(&argv, wargv, argc);

    dupenv = wastrtoastr(&env, wenv, -1);

    _environ = _malloc_dbg((dupenv + 1) * sizeof (char *), 
                           _CRT_BLOCK, __FILE__, __LINE__ );
    memcpy(_environ, env, (dupenv + 1) * sizeof (char *));

    /* MSVCRT will attempt to maintain the wide environment calls
     * on _putenv(), which is bogus if we've passed a non-ascii
     * string to _putenv(), since they use MultiByteToWideChar
     * and breaking the implicit utf-8 assumption we've built.
     *
     * Reset _wenviron for good measure.
     */
    if (_wenviron) {
        wenv = _wenviron;
        _wenviron = NULL;
        free(wenv);
    }

    return main(argc, argv, env);
}

#else

static int warrsztoastr(char ***retarr, wchar_t *arrsz, int args)
{
    apr_wchar_t *wch;
    size_t totlen;
    size_t newlen;
    size_t wsize;
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
            ;
        }
    }

    newarr[arg] = NULL;

    *retarr = newarr;
    return args;
}

/* Reprocess the arguments to main() for a completely apr-ized application
 */

APR_DECLARE(apr_status_t) apr_main(int *argc, char ***argv, char ***env)
{
#if APR_HAS_UNICODE_FS
    IF_WIN_OS_IS_UNICODE
    {
        apr_wchar_t **wstrs;
        apr_wchar_t *sysstr;
        int wstrc;

        sysstr = GetCommandLineW();
        if (sysstr) {
            wstrs = CommandLineToArgvW(sysstr, &wstrc);
            if (wstrs) {
                *argc = wastrtoastr(argv, wstrs, wstrc);
                GlobalFree(wstrs);
            }
        }

        sysstr = GetEnvironmentStringsW();
    }
#endif
    return APR_SUCCESS;
}

#endif