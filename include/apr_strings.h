/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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

/* Portions of this file are covered by */
/* -*- mode: c; c-file-style: "k&r" -*-

  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
  Copyright (C) 2000 by Martin Pool <mbp@humbug.org.au>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/


#include "apr.h"
#include "apr_lib.h"

#ifndef APR_STRINGS_H
#define APR_STRINGS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @package APR strings library
 */

/**
 * Do a natural order comparison of two strings.
 * @param  The first string to compare
 * @param  The second string to compare
 * @return Either <0, 0, or >0.  If the first string is less than the second
 *          this returns <0, if they are equivalent it returns 0, and if the
 *          first string is greater than second string it retuns >0.
 */
int apr_strnatcmp(char const *a, char const *b);

/**
 * Do a natural order comparison of two strings ignoring the case of the 
 * strings.
 * @param a The first string to compare
 * @param b The second string to compare
 * @return Either <0, 0, or >0.  If the first string is less than the second
 *         this returns <0, if they are equivalent it returns 0, and if the
 *         first string is greater than second string it retuns >0.
 */
int apr_strnatcasecmp(char const *a, char const *b);

/**
 * duplicate a string into memory allocated out of a pool
 * @param p The pool to allocate out of
 * @param s The string to allocate
 * @return The new string
 * @deffunc char *apr_pstrdup(apr_pool_t *p, const char *s)
 */
APR_DECLARE(char *) apr_pstrdup(apr_pool_t *p, const char *s);

/**
 * duplicate the first n characters of a string into memory allocated 
 * out of a pool; the new string will be '\0'-terminated
 * @param p The pool to allocate out of
 * @param s The string to allocate
 * @param n The number of characters to duplicate
 * @return The new string
 * @deffunc char *apr_pstrndup(apr_pool_t *p, const char *s, apr_size_t n)
 */
APR_DECLARE(char *) apr_pstrndup(apr_pool_t *p, const char *s, apr_size_t n);

/**
 * Concatenate multiple strings, allocating memory out a pool
 * @param p The pool to allocate out of
 * @param ... The strings to concatenate.  The final string must be NULL
 * @return The new string
 * @deffunc char *apr_pstrcat(apr_pool_t *p, ...)
 */
APR_DECLARE_NONSTD(char *) apr_pstrcat(apr_pool_t *p, ...);

/**
 * printf-style style printing routine.  The data is output to a string 
 * allocated from a pool
 * @param p The pool to allocate out of
 * @param fmt The format of the string
 * @param ap The arguments to use while printing the data
 * @return The new string
 * @deffunc char *apr_pvsprintf(apr_pool_t *p, const char *fmt, va_list ap)
 */
APR_DECLARE(char *) apr_pvsprintf(apr_pool_t *p, const char *fmt, va_list ap);

/**
 * printf-style style printing routine.  The data is output to a string 
 * allocated from a pool
 * @param p The pool to allocate out of
 * @param fmt The format of the string
 * @param ... The arguments to use while printing the data
 * @return The new string
 * @deffunc char *apr_psprintf(apr_pool_t *p, const char *fmt, ...)
 */
APR_DECLARE_NONSTD(char *) apr_psprintf(apr_pool_t *p, const char *fmt, ...);

/**
 * copy n characters from src to des>
 * @param dst The destination string
 * @param src The source string
 * @param dst_size The number of characters to copy
 * @tip  
 * <PRE>
 * We re-implement this function to implement these specific changes:
 *       1) strncpy() doesn't always null terminate and we want it to.
 *       2) strncpy() null fills, which is bogus, esp. when copy 8byte strings
 *          into 8k blocks.
 *       3) Instead of returning the pointer to the beginning of the
 *          destination string, we return a pointer to the terminating '\0'
 *          to allow us to check for truncation.
 * </PRE>
 * @deffunc char *apr_cpystrn(char *dst, const char *src, size_t dst_size)
 */
APR_DECLARE(char *) apr_cpystrn(char *dst, const char *src, size_t dst_size);

/**
 * Strip spaces from a string
 * @param dest The destination string.  It is okay to modify the string
 *             in place.  Namely dest == src
 * @param src The string to rid the spaces from.
 * @deffunc char *apr_collapse_spaces(char *dest, const char *src)
 */
APR_DECLARE(char *) apr_collapse_spaces(char *dest, const char *src);

/**
 * Convert the arguments to a program from one string to an array of 
 * strings term inated by a NULL
 * @param str The arguments to convert
 * @param argv_out Output location.  This is a pointer to an array of strings.
 * @param token_context Pool to use.
 * @deffunc apr_status_t apr_tokenize_to_argv(const char *arg_str, char ***argv_out, apr_pool_t *token_context);
 */
APR_DECLARE(apr_status_t) apr_tokenize_to_argv(const char *arg_str,
                                            char ***argv_out,
                                            apr_pool_t *token_context);

#ifdef __cplusplus
}
#endif

#endif  /* !APR_STRINGS_H */
