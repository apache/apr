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

/* Usage Notes:
 *
 *   this module, and the misc/win32/utf8.c modules must be 
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
#include "apr_arch_file_io.h"
#include "assert.h"
#include "apr_private.h"
#include "apr_arch_misc.h"

/* This symbol is _private_, although it must be exported.
 */

extern int main(int argc, const char **argv, const char **env);

int wmain(int argc, const wchar_t **wargv, const wchar_t **wenv)
{
    char **argv;
    char **env;
    int dupenv;

    (void)apr_wastrtoastr(&argv, wargv, argc);

    dupenv = apr_wastrtoastr(&env, wenv, -1);

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
        free((wchar_t **)wenv);
    }

    apr_app_init_complete = 1;

    return main(argc, argv, env);
}
