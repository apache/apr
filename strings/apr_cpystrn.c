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

#include "apr.h"
#include "apr_strings.h"
#include "apr_private.h"
#include "apr_lib.h"

#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_CTYPE_H
#include <ctype.h>
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
 * apr_cpystrn() follows the same call structure as strncpy().
 */

APR_DECLARE(char *) apr_cpystrn(char *dst, const char *src, apr_size_t dst_size)
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
 * be expanded to include support for the apr_call_exec command line
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
APR_DECLARE(apr_status_t) apr_tokenize_to_argv(const char *arg_str, 
                                            char ***argv_out,
                                            apr_pool_t *token_context)
{
    const char *cp;
    const char *ct;
    char *cleaned, *dirty;
    int escaped;
    int isquoted, numargs = 0, argnum;

#define SKIP_WHITESPACE(cp) \
    for ( ; *cp == ' ' || *cp == '\t'; ) { \
        cp++; \
    };

#define CHECK_QUOTATION(cp,isquoted) \
    isquoted = 0; \
    if (*cp == '"') { \
        isquoted = 1; \
        cp++; \
    } \
    else if (*cp == '\'') { \
        isquoted = 2; \
        cp++; \
    }

/* DETERMINE_NEXTSTRING:
 * At exit, cp will point to one of the following:  NULL, SPACE, TAB or QUOTE.
 * NULL implies the argument string has been fully traversed.
 */
#define DETERMINE_NEXTSTRING(cp,isquoted) \
    for ( ; *cp != '\0'; cp++) { \
        if (   (isquoted    && (*cp     == ' ' || *cp     == '\t')) \
            || (*cp == '\\' && (*(cp+1) == ' ' || *(cp+1) == '\t' || \
                                *(cp+1) == '"' || *(cp+1) == '\''))) { \
            cp++; \
            continue; \
        } \
        if (   (!isquoted && (*cp == ' ' || *cp == '\t')) \
            || (isquoted == 1 && *cp == '"') \
            || (isquoted == 2 && *cp == '\'')                 ) { \
            break; \
        } \
    }
 
/* REMOVE_ESCAPE_CHARS:
 * Compresses the arg string to remove all of the '\' escape chars.
 * The final argv strings should not have any extra escape chars in it.
 */
#define REMOVE_ESCAPE_CHARS(cleaned, dirty, escaped) \
    escaped = 0; \
    while(*dirty) { \
        if (!escaped && *dirty == '\\') { \
            escaped = 1; \
        } \
        else { \
            escaped = 0; \
            *cleaned++ = *dirty; \
        } \
        ++dirty; \
    } \
    *cleaned = 0;        /* last line of macro... */

    cp = arg_str;
    SKIP_WHITESPACE(cp);
    ct = cp;

    /* This is ugly and expensive, but if anyone wants to figure a
     * way to support any number of args without counting and 
     * allocating, please go ahead and change the code.
     *
     * Must account for the trailing NULL arg.
     */
    numargs = 1;
    while (*ct != '\0') {
        CHECK_QUOTATION(ct, isquoted);
        DETERMINE_NEXTSTRING(ct, isquoted);
        if (*ct != '\0') {
            ct++;
        }
        numargs++;
        SKIP_WHITESPACE(ct);
    }
    *argv_out = apr_palloc(token_context, numargs * sizeof(char*));

    /*  determine first argument */
    for (argnum = 0; argnum < (numargs-1); argnum++) {
        CHECK_QUOTATION(cp, isquoted);
        ct = cp;
        DETERMINE_NEXTSTRING(cp, isquoted);
        cp++;
        (*argv_out)[argnum] = apr_palloc(token_context, cp - ct);
        apr_cpystrn((*argv_out)[argnum], ct, cp - ct);
        cleaned = dirty = (*argv_out)[argnum];
        REMOVE_ESCAPE_CHARS(cleaned, dirty, escaped);
        SKIP_WHITESPACE(cp);
    }
    (*argv_out)[argnum] = NULL;

    return APR_SUCCESS;
}

/* Filepath_name_get returns the final element of the pathname.
 * Using the current platform's filename syntax.
 *   "/foo/bar/gum" -> "gum"
 *   "/foo/bar/gum/" -> ""
 *   "gum" -> "gum"
 *   "wi\\n32\\stuff" -> "stuff
 *
 * Corrected Win32 to accept "a/b\\stuff", "a:stuff"
 */

APR_DECLARE(const char *) apr_filepath_name_get(const char *pathname)
{
    const char path_separator = '/';
    const char *s = strrchr(pathname, path_separator);

#ifdef WIN32
    const char path_separator_win = '\\';
    const char drive_separator_win = ':';
    const char *s2 = strrchr(pathname, path_separator_win);

    if (s2 > s) s = s2;

    if (!s) s = strrchr(pathname, drive_separator_win);
#endif

    return s ? ++s : pathname;
}

/* deprecated */
APR_DECLARE(const char *) apr_filename_of_pathname(const char *pathname)
{
        return apr_filepath_name_get(pathname);
}

/* length of dest assumed >= length of src
 * collapse in place (src == dest) is legal.
 * returns terminating null ptr to dest string.
 */
APR_DECLARE(char *) apr_collapse_spaces(char *dest, const char *src)
{
    while (*src) {
        if (!apr_isspace(*src)) 
            *dest++ = *src;
        ++src;
    }
    *dest = 0;
    return (dest);
}

#if !APR_HAVE_STRDUP
char *strdup(const char *str)
{
    char *sdup;
    size_t len = strlen(str) + 1;

    sdup = (char *) malloc(len);
    memcpy(sdup, str, len);

    return sdup;
}
#endif

/* The following two routines were donated for SVR4 by Andreas Vogel */
#if (!APR_HAVE_STRCASECMP && !APR_HAVE_STRICMP)
int strcasecmp(const char *a, const char *b)
{
    const char *p = a;
    const char *q = b;
    for (p = a, q = b; *p && *q; p++, q++) {
        int diff = apr_tolower(*p) - apr_tolower(*q);
        if (diff)
            return diff;
    }
    if (*p)
        return 1;               /* p was longer than q */
    if (*q)
        return -1;              /* p was shorter than q */
    return 0;                   /* Exact match */
}

#endif

#if (!APR_HAVE_STRNCASECMP && !APR_HAVE_STRNICMP)
int strncasecmp(const char *a, const char *b, size_t n)
{
    const char *p = a;
    const char *q = b;

    for (p = a, q = b; /*NOTHING */ ; p++, q++) {
        int diff;
        if (p == a + n)
            return 0;           /*   Match up to n characters */
        if (!(*p && *q))
            return *p - *q;
        diff = apr_tolower(*p) - apr_tolower(*q);
        if (diff)
            return diff;
    }
    /*NOTREACHED */
}
#endif

/* The following routine was donated for UTS21 by dwd@bell-labs.com */
#if (!APR_HAVE_STRSTR)
char *strstr(char *s1, char *s2)
{
    char *p1, *p2;
    if (*s2 == '\0') {
        /* an empty s2 */
        return(s1);
    }
    while((s1 = strchr(s1, *s2)) != NULL) {
        /* found first character of s2, see if the rest matches */
        p1 = s1;
        p2 = s2;
        while (*++p1 == *++p2) {
            if (*p1 == '\0') {
                /* both strings ended together */
                return(s1);
            }
        }
        if (*p2 == '\0') {
            /* second string ended, a match */
            break;
        }
        /* didn't find a match here, try starting at next character in s1 */
        s1++;
    }
    return(s1);
}
#endif

