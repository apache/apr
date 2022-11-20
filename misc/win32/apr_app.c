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

/* Usage Notes:
 *
 *   this module, and the misc/win32/utf8.c modules must be
 *   compiled APR_EXPORT_STATIC and linked to an application with
 *   the /entry:wmainCRTStartup flag (which this module kindly
 *   provides to the developer who links to libaprapp-1.lib).
 *   This module becomes the true wmain entry point, and passes
 *   utf-8 reformatted argv and env arrays to the application's
 *   main() function as if nothing happened.
 *
 *   This module is only compatible with Unicode operating systems.
 *   Mixed (Win9x backwards compatible) binaries should refer instead
 *   to the apr_startup.c module.
 *
 *   _dbg_malloc/realloc is used in place of the usual API, in order
 *   to convince the MSVCRT that it created these entities.  If we
 *   do not create them as _CRT_BLOCK entities, the crt will fault
 *   on an assert.  We are not worrying about the crt's locks here,
 *   since we are single threaded [so far].
 */

#include "apr_general.h"
#include "wchar.h"
#include "arch/win32/apr_arch_utf8.h"
#include "assert.h"
#include "apr_private.h"
#include "apr_arch_misc.h"

#pragma comment(linker,"/ENTRY:wmainCRTStartup")

extern int main(int argc, const char **argv, const char **env);

int wmain(int argc, const wchar_t **wargv, const wchar_t **wenv)
{
    char **argv;
    char **env;
    int envc;
    int i;

    (void)apr_wastrtoastr(&argv, wargv, argc);

    envc = 0;
    while (wenv[envc]) {
        envc++;
    }

    /* Initial environment stored as single heap block, but uses
     * separate heap entry for every environment variable
     * after first change.
     */
    env = apr_malloc_dbg((envc + 1) * sizeof(char *), __FILE__, __LINE__);

    for (i = 0; i < envc; i++) {
        apr_size_t wcount;
        apr_size_t envlen;

        wcount = wcslen(wenv[i]) + 1;
        envlen = (wcount - 1) * 3 + 1;

        env[i] = apr_malloc_dbg(envlen, __FILE__, __LINE__);

        (void)apr_conv_ucs2_to_utf8(wenv[i], &wcount, env[i], &envlen);
    }

    env[i] = NULL;

    _environ = env;

    /* MSVCRT will attempt to maintain the wide environment calls
     * on _putenv(), which is bogus if we've passed a non-ascii
     * string to _putenv(), since they use MultiByteToWideChar
     * and breaking the implicit utf-8 assumption we've built.
     *
     * Reset _wenviron for good measure.
     */
    if (_wenviron) {
        _wenviron = NULL;

        /* There is no need to free _wenviron because MSVCRT
         * automatically free 'initial' environment block.
         */
    }

    apr_app_init_complete = 1;

    return main(argc, argv, env);
}
