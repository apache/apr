/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
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

#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_xlate.h"

/* If no implementation is available, don't generate code here since
 * apr_xlate.h emitted macros which return APR_ENOTIMPL.
 */

#if APR_HAS_XLATE

#ifdef HAVE_STDDEF_H
#include <stddef.h> /* for NULL */
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

#ifdef APR_ICONV_INBUF_CONST
#define ICONV_INBUF_TYPE const char **
#else
#define ICONV_INBUF_TYPE char **
#endif

#ifndef min
#define min(x,y) ((x) <= (y) ? (x) : (y))
#endif

struct apr_xlate_t {
    apr_pool_t *pool;
    char *frompage;
    char *topage;
    char *sbcs_table;
#ifdef HAVE_ICONV
    iconv_t ich;
#endif
};

/* get_default_charset()
 *
 * simple heuristic to determine codepage of source code so that
 * literal strings (e.g., "GET /\r\n") in source code can be translated
 * properly
 *
 * If appropriate, a symbol can be set at configure time to determine
 * this.  On EBCDIC platforms, it will be important how the code was
 * unpacked.
 */

static const char *get_default_charset(void)
{
#ifdef __MVS__
#    ifdef __CODESET__
        return __CODESET__;
#    else
        return "IBM-1047";
#    endif
#endif

    if ('}' == 0xD0) {
        return "IBM-1047";
    }

    if ('{' == 0xFB) {
        return "EDF04";
    }

    if ('A' == 0xC1) {
        return "EBCDIC"; /* not useful */
    }

    if ('A' == 0x41) {
        return "ISO8859-1"; /* not necessarily true */
    }

    return "unknown";
}

/* get_locale_charset()
 *
 * If possible on this system, get the charset of the locale.  Otherwise,
 * defer to get_default_charset().
 */

static const char *get_locale_charset(void)
{
#if defined(HAVE_NL_LANGINFO) && defined(HAVE_CODESET)
    const char *charset;
    charset = nl_langinfo(CODESET);
    if (charset) {
        return charset;
    }
#endif
    return get_default_charset();
}

static const char *handle_special_names(const char *page)
{
    if (page == APR_DEFAULT_CHARSET) {
        return get_default_charset();
    }
    else if (page == APR_LOCALE_CHARSET) {
        return get_locale_charset();
    }
    else {
        return page;
    }
}

static apr_status_t apr_xlate_cleanup(void *convset)
{
#ifdef HAVE_ICONV
    apr_xlate_t *old = convset;

    if (old->ich != (iconv_t)-1) {
        if (iconv_close(old->ich)) {
            return errno;
        }
    }
#endif
    return APR_SUCCESS;
}

#ifdef HAVE_ICONV
static void check_sbcs(apr_xlate_t *convset)
{
    char inbuf[256], outbuf[256];
    char *inbufptr = inbuf;
    char *outbufptr = outbuf;
    apr_size_t inbytes_left, outbytes_left;
    int i;
    apr_size_t translated;

    for (i = 0; i < sizeof(inbuf); i++) {
        inbuf[i] = i;
    }

    inbytes_left = outbytes_left = sizeof(inbuf);
    translated = iconv(convset->ich, (ICONV_INBUF_TYPE)&inbufptr, 
                       &inbytes_left, &outbufptr, &outbytes_left);
    if (translated != (apr_size_t) -1 &&
        inbytes_left == 0 &&
        outbytes_left == 0) {
        /* hurray... this is simple translation; save the table,
         * close the iconv descriptor
         */
        
        convset->sbcs_table = apr_palloc(convset->pool, sizeof(outbuf));
        memcpy(convset->sbcs_table, outbuf, sizeof(outbuf));
        iconv_close(convset->ich);
        convset->ich = (iconv_t)-1;

        /* TODO: add the table to the cache */
    }
}
#endif /* HAVE_ICONV */

static void make_identity_table(apr_xlate_t *convset)
{
  int i;
  convset->sbcs_table = apr_palloc(convset->pool, 256);
  for (i = 0; i < 256; i++)
      convset->sbcs_table[i] = i;
}

apr_status_t apr_xlate_open(apr_xlate_t **convset, const char *topage,
                            const char *frompage, apr_pool_t *pool)
{
    apr_status_t status;
    apr_xlate_t *new;
    int found = 0;

    *convset = NULL;

    topage = handle_special_names(topage);
    frompage = handle_special_names(frompage);
    
    new = (apr_xlate_t *)apr_pcalloc(pool, sizeof(apr_xlate_t));
    if (!new) {
        return APR_ENOMEM;
    }

    new->pool = pool;
    new->topage = apr_pstrdup(pool, topage);
    new->frompage = apr_pstrdup(pool, frompage);
    if (!new->topage || !new->frompage) {
        return APR_ENOMEM;
    }

#ifdef TODO
    /* search cache of codepage pairs; we may be able to avoid the
     * expensive iconv_open()
     */

    set found to non-zero if found in the cache
#endif

    if ((! found) && (strcmp(topage, frompage) == 0)) {
        /* to and from are the same */
        found = 1;
	make_identity_table(new);
    }

#ifdef HAVE_ICONV
    if (!found) {
        new->ich = iconv_open(topage, frompage);
        if (new->ich == (iconv_t)-1) {
            return errno;
        }
        found = 1;
        check_sbcs(new);
    } else
        new->ich = (iconv_t)-1;
#endif /* HAVE_ICONV */

    if (found) {
        *convset = new;
        apr_pool_cleanup_register(pool, (void *)new, apr_xlate_cleanup,
                            apr_pool_cleanup_null);
        status = APR_SUCCESS;
    }
    else {
        status = EINVAL; /* same as what iconv() would return if it
                            couldn't handle the pair */
    }
    
    return status;
}

apr_status_t apr_xlate_sb_get(apr_xlate_t *convset, int *onoff)
{
    *onoff = convset->sbcs_table != NULL;
    return APR_SUCCESS;
} 

apr_status_t apr_xlate_conv_buffer(apr_xlate_t *convset, const char *inbuf,
                                   apr_size_t *inbytes_left, char *outbuf,
                                   apr_size_t *outbytes_left)
{
    apr_status_t status = APR_SUCCESS;
#ifdef HAVE_ICONV
    apr_size_t translated;

    if (convset->ich != (iconv_t)-1) {
        const char *inbufptr = inbuf;
        char *outbufptr = outbuf;
        
        translated = iconv(convset->ich, (ICONV_INBUF_TYPE)&inbufptr, 
                           inbytes_left, &outbufptr, outbytes_left);
        /* If everything went fine but we ran out of buffer, don't
         * report it as an error.  Caller needs to look at the two
         * bytes-left values anyway.
         *
         * There are three expected cases where rc is -1.  In each of
         * these cases, *inbytes_left != 0.
         * a) the non-error condition where we ran out of output
         *    buffer
         * b) the non-error condition where we ran out of input (i.e.,
         *    the last input character is incomplete)
         * c) the error condition where the input is invalid
         */
        if (translated == (apr_size_t)-1) {
            switch (errno) {
            case E2BIG:  /* out of space on output */
                status = 0; /* change table lookup code below if you
                               make this an error */
                break;
            case EINVAL: /* input character not complete (yet) */
                status = APR_INCOMPLETE;
                break;
            case EILSEQ: /* bad input byte */
                status = APR_EINVAL;
                break;
            default:
                status = errno;
            }
        }
    }
    else
#endif
    {
        int to_convert = min(*inbytes_left, *outbytes_left);
        int converted = to_convert;
        char *table = convset->sbcs_table;
        
        while (to_convert) {
            *outbuf = table[(unsigned char)*inbuf];
            ++outbuf;
            ++inbuf;
            --to_convert;
        }
        *inbytes_left -= converted;
        *outbytes_left -= converted;
    }

    return status;
}

apr_int32_t apr_xlate_conv_byte(apr_xlate_t *convset, unsigned char inchar)
{
    if (convset->sbcs_table) {
        return convset->sbcs_table[inchar];
    }
    else {
        return -1;
    }
}

apr_status_t apr_xlate_close(apr_xlate_t *convset)
{
    return apr_pool_cleanup_run(convset->pool, convset, apr_xlate_cleanup);
}

#else /* !APR_HAS_XLATE */

APR_DECLARE(apr_status_t) apr_xlate_open(apr_xlate_t **convset, 
                                         const char *topage,
                                         const char *frompage, 
                                         apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_xlate_sb_get(apr_xlate_t *convset, int *onoff)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_int32_t) apr_xlate_conv_byte(apr_xlate_t *convset, 
                                             unsigned char inchar)
{
    return (-1);
}

APR_DECLARE(apr_status_t) apr_xlate_conv_buffer(apr_xlate_t *convset, 
                                                const char *inbuf,
                                                apr_size_t *inbytes_left, 
                                                char *outbuf,
                                                apr_size_t *outbytes_left)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_xlate_close(apr_xlate_t *convset)
{
    return APR_ENOTIMPL;
}

#endif /* APR_HAS_XLATE */

/* Deprecated
 */
APR_DECLARE(apr_status_t) apr_xlate_get_sb(apr_xlate_t *convset, int *onoff)
{
    return apr_xlate_sb_get(convset, onoff);
}
