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

#include "apr_arch_misc.h"
#include "apr_arch_file_io.h"
#include <crtdbg.h>
#include <assert.h>

/* This module is the source of -static- helper functions that are
 * entirely internal to apr.  If the fn is exported - it does not
 * belong here.
 *
 * Namespace decoration is still required to protect us from symbol
 * clashes in static linkages.
 */


/* Shared by apr_app.c and start.c 
 *
 * An internal apr function to convert an array of strings (either
 * a counted or NULL terminated list, such as an argv[argc] or env[]
 * list respectively) from wide Unicode strings to narrow utf-8 strings.
 * These are allocated from the MSVCRT's _CRT_BLOCK to trick the system
 * into trusting our store.
 */
int apr_wastrtoastr(char const * const * *retarr, 
                    wchar_t const * const *arr, int args)
{
    apr_size_t elesize = 0;
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
        elesize += (apr_size_t)newarr[arg];
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
        apr_size_t len = (apr_size_t)newarr[arg];
        apr_size_t newlen = elesize;

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
        apr_size_t diff = ele - elements;
        for (arg = 0; arg < args; ++arg) {
            newarr[arg] += diff;
        }
    }

    *retarr = newarr;
    return args;
}
