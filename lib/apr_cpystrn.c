/* ====================================================================
 * Copyright (c) 1995-2000 The Apache Software Foundation.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */
#ifndef WIN32
#include "apr_config.h"
#else
#include "apr_winconfig.h"
#endif
#include "apr_lib.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif

/*
 * Apache's "replacement" for the strncpy() function. We roll our
 * own to implement these specific changes:
 *   (1) strncpy() doesn't always null terminate and we want it to.
 *   (2) strncpy() null fills, which is bogus, esp. when copy 8byte
 *       strings into 8k blocks.
 *   (3) Instead of returning the pointer to the beginning of
 *       the destination string, we return a pointer to the
 *       terminating '\0' to allow us to "check" for truncation
 *
 * ap_cpystrn() follows the same call structure as strncpy().
 */

API_EXPORT(char *) ap_cpystrn(char *dst, const char *src, size_t dst_size)
{

    char *d, *end;

    if (dst_size == 0) {
        return (dst);
    }

    d = dst;
    end = dst + dst_size - 1;

    for (; d < end; ++d, ++src) {
	if (!(*d = *src)) {
	    return (d);
	}
    }

    *d = '\0';	/* always null terminate */

    return (d);
}


/*
 * This function provides a way to parse a generic argument string
 * into a standard argv[] form of argument list. It respects the 
 * usual "whitespace" and quoteing rules. In the future this could
 * be expanded to include support for the ap_call_exec command line
 * string processing (including converting '+' to ' ' and doing the 
 * url processing. It does not currently support this function.
 *
 *    token_context: Context from which pool allocations will occur.
 *    arg_str:       Input argument string for conversion to argv[].
 *    argv_out:      Output location. This is a pointer to an array
 *                   of pointers to strings (ie. &(char *argv[]).
 *                   This value will be allocated from the contexts
 *                   pool and filled in with copies of the tokens
 *                   found during parsing of the arg_str. 
 */
API_EXPORT(int) ap_tokenize_to_argv(ap_context_t *token_context,
                                    char *arg_str, char ***argv_out)
{
    char *cp, *tmpCnt;
    int isquoted, numargs = 0, rc = APR_SUCCESS;

#define SKIP_WHITESPACE(cp) \
    for ( ; *cp == ' ' || *cp == '\t'; ) { \
        cp++; \
    };

#define CHECK_QUOTATION(cp,isquoted) \
    isquoted = 0; \
    if (*cp == '"') { \
        isquoted = 1; \
        cp++; \
    }

/* DETERMINE_NEXTSTRING:
 * At exit, cp will point to one of the following:  NULL, SPACE, TAB or QUOTE.
 * NULL implies the argument string has been fully traversed.
 */
#define DETERMINE_NEXTSTRING(cp,isquoted) \
    for ( ; *cp != '\0'; cp++) { \
        if (   (isquoted    && (*cp     == ' ' || *cp     == '\t')) \
            || (*cp == '\\' && (*(cp+1) == ' ' || *(cp+1) == '\t'))) { \
            cp++; \
            continue; \
        } \
        if (   (!isquoted && (*cp == ' ' || *cp == '\t')) \
            || (isquoted  && *cp == '"')                  ) { \
            break; \
        } \
    }

    cp = arg_str;
    SKIP_WHITESPACE(cp);
    tmpCnt = cp;

    /* This is ugly and expensive, but if anyone wants to figure a
     * way to support any number of args without counting and 
     * allocating, please go ahead and change the code.
     */
    while (*tmpCnt != '\0') {
        CHECK_QUOTATION(tmpCnt, isquoted);
        DETERMINE_NEXTSTRING(tmpCnt, isquoted);
        numargs++;
        SKIP_WHITESPACE(tmpCnt);
    }

    *argv_out = ap_palloc(token_context, numargs*sizeof(char*));
    if (*argv_out == NULL) {
        return (APR_ENOMEM);
    }

    /*  determine first argument */
    numargs = 0;
    while (*cp != '\0') {
        CHECK_QUOTATION(cp, isquoted);
        tmpCnt = cp;
        DETERMINE_NEXTSTRING(cp, isquoted);
        if (*cp == '\0') {
            (*argv_out)[numargs] = ap_pstrdup(token_context, tmpCnt);
            numargs++;
            (*argv_out)[numargs] = '\0';
            break;
        }
        else {
            *cp++ = '\0';
            (*argv_out)[numargs] = ap_pstrdup(token_context, tmpCnt);
            numargs++;
        }
        
        SKIP_WHITESPACE(cp);
    }

    return(rc);
}

/* Filename_of_pathname returns the final element of the pathname.
 * Using the current platform's filename syntax.
 *   "/foo/bar/gum" -> "gum"
 *   "/foo/bar/gum/" -> ""
 *   "gum" -> "gum"
 *   "wi\\n32\\stuff" -> "stuff
 */

const char *ap_filename_of_pathname(const char *pathname)
{
#ifdef WIN32
    const char path_separator = '\\';
#else
    const char path_separator = '/';
#endif
    const char *s = strrchr(pathname, path_separator);

    return s ? ++s : pathname;
}

