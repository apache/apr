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

#include "apr.h"
#include "apr_strings.h"
#include "apr_general.h"
#include "apr_private.h"
#include "apr_lib.h"
#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#include "apr_want.h"

#ifdef HAVE_STDDEF_H
#include <stddef.h> /* NULL */
#endif

/** this is used to cache lengths in apr_pstrcat */
#define MAX_SAVED_LENGTHS  6

APR_DECLARE(char *) apr_pstrdup(apr_pool_t *a, const char *s)
{
    char *res;
    size_t len;

    if (s == NULL) {
        return NULL;
    }
    len = strlen(s) + 1;
    res = apr_palloc(a, len);
    memcpy(res, s, len);
    return res;
}

APR_DECLARE(char *) apr_pstrndup(apr_pool_t *a, const char *s, apr_size_t n)
{
    char *res;
    const char *end;

    if (s == NULL) {
        return NULL;
    }
    end = memchr(s, '\0', n);
    if (end != NULL)
        n = end - s;
    res = apr_palloc(a, n + 1);
    memcpy(res, s, n);
    res[n] = '\0';
    return res;
}

APR_DECLARE(char *) apr_pstrmemdup(apr_pool_t *a, const char *s, apr_size_t n)
{
    char *res;

    if (s == NULL) {
        return NULL;
    }
    res = apr_palloc(a, n + 1);
    memcpy(res, s, n);
    res[n] = '\0';
    return res;
}

APR_DECLARE(void *) apr_pmemdup(apr_pool_t *a, const void *m, apr_size_t n)
{
    void *res;

    if (m == NULL)
	return NULL;
    res = apr_palloc(a, n);
    memcpy(res, m, n);
    return res;
}

APR_DECLARE_NONSTD(char *) apr_pstrcat(apr_pool_t *a, ...)
{
    char *cp, *argp, *res;
    apr_size_t saved_lengths[MAX_SAVED_LENGTHS];
    int nargs = 0;

    /* Pass one --- find length of required string */

    apr_size_t len = 0;
    va_list adummy;

    va_start(adummy, a);

    while ((cp = va_arg(adummy, char *)) != NULL) {
        apr_size_t cplen = strlen(cp);
        if (nargs < MAX_SAVED_LENGTHS) {
            saved_lengths[nargs++] = cplen;
        }
        len += cplen;
    }

    va_end(adummy);

    /* Allocate the required string */

    res = (char *) apr_palloc(a, len + 1);
    cp = res;

    /* Pass two --- copy the argument strings into the result space */

    va_start(adummy, a);

    nargs = 0;
    while ((argp = va_arg(adummy, char *)) != NULL) {
        if (nargs < MAX_SAVED_LENGTHS) {
            len = saved_lengths[nargs++];
        }
        else {
            len = strlen(argp);
        }
 
        memcpy(cp, argp, len);
        cp += len;
    }

    va_end(adummy);

    /* Return the result string */

    *cp = '\0';

    return res;
}

#if (!APR_HAVE_MEMCHR)
void *memchr(const void *s, int c, size_t n)
{
    const char *cp;

    for (cp = s; n > 0; n--, cp++) {
        if (*cp == c)
            return (char *) cp; /* Casting away the const here */
    }

    return NULL;
}
#endif

APR_DECLARE(char *) apr_itoa(apr_pool_t *p, int n)
{
    const int BUFFER_SIZE = sizeof(int) * 3 + 2;
    char *buf = apr_palloc(p, BUFFER_SIZE);
    char *start = buf + BUFFER_SIZE - 1;
    int negative;
    if (n < 0) {
	negative = 1;
	n = -n;
    }
    else {
	negative = 0;
    }
    *start = 0;
    do {
	*--start = '0' + (n % 10);
	n /= 10;
    } while (n);
    if (negative) {
	*--start = '-';
    }
    return start;
}

APR_DECLARE(char *) apr_ltoa(apr_pool_t *p, long n)
{
    const int BUFFER_SIZE = sizeof(long) * 3 + 2;
    char *buf = apr_palloc(p, BUFFER_SIZE);
    char *start = buf + BUFFER_SIZE - 1;
    int negative;
    if (n < 0) {
	negative = 1;
	n = -n;
    }
    else {
	negative = 0;
    }
    *start = 0;
    do {
	*--start = '0' + (n % 10);
	n /= 10;
    } while (n);
    if (negative) {
	*--start = '-';
    }
    return start;
}

APR_DECLARE(char *) apr_off_t_toa(apr_pool_t *p, apr_off_t n)
{
    const int BUFFER_SIZE = sizeof(apr_off_t) * 3 + 2;
    char *buf = apr_palloc(p, BUFFER_SIZE);
    char *start = buf + BUFFER_SIZE - 1;
    int negative;
    if (n < 0) {
	negative = 1;
	n = -n;
    }
    else {
	negative = 0;
    }
    *start = 0;
    do {
	*--start = '0' + (char)(n % 10);
	n /= 10;
    } while (n);
    if (negative) {
	*--start = '-';
    }
    return start;
}

APR_DECLARE(char *) apr_strfsize(apr_off_t size, char *buf)
{
    const char ord[] = "KMTPE";
    const char *o = ord;
    int remain;

    if (size < 0) {
        return strcpy(buf, "  - ");
    }
    if (size < 973) {
        sprintf(buf, "%3d ", (int) size);
        return buf;
    }
    do {
        remain = (int)(size & 1023);
        size >>= 10;
        if (size >= 973) {
            ++o;
            continue;
        }
        if (size < 9 || (size == 9 && remain < 973)) {
            if ((remain = ((remain * 5) + 256) / 512) >= 10)
                ++size, remain = 0;
            sprintf(buf, "%d.%d%c", (int) size, remain, *o);
            return buf;
        }
        if (remain >= 512)
            ++size;
        sprintf(buf, "%3d%c", (int) size, *o);
        return buf;
    } while (1);
}

