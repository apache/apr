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
 */

#include "apr_private.h"
#include "apr_general.h"
#include "wchar.h"
#include "fileio.h"
#include "assert.h"

extern int main(int argc, char **argv, char **env);

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

    newarr = malloc(arg * sizeof(char *));

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
    ele = elements = malloc(elesize * sizeof(char));

    for (arg = 0; arg < args; ++arg) {
        size_t len = (size_t)newarr[arg];
        size_t newlen = elesize;

        newarr[arg] = ele;
        (void)apr_conv_ucs2_to_utf8(arr[arg], &len,
                                    newarr[arg], &elesize);

        newlen -= elesize;
        ele += newlen;
        assert(elesize);
    }

    newarr[arg] = NULL;
    *ele = '\0';

    /* Return to the free store if the heap realloc is the least bit optimized
     */
    ele = realloc(elements, ele - elements);

    if (ele != elements) {
        size_t diff = ele - elements;
        for (arg = 0; arg < args; ++arg) {
            newarr[arg] += diff;
        }
    }

    *retarr = newarr;
    return args;
}

int wmain(int argc, wchar_t **wargv, wchar_t **wenv)
{
    char **argv;
    char **env;
    int dupenv;

    (void)wastrtoastr(&argv, wargv, argc);

    _wenviron = wenv;
    dupenv = wastrtoastr(&env, wenv, -1);

    _environ = malloc((dupenv + 1) * sizeof (char *));
    memcpy(_environ, env, (dupenv + 1) * sizeof (char *));

    return main(argc, argv, env);
}
