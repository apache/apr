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

#ifndef APR_XLATE_H
#define APR_XLATE_H

#include "apr.h"
#include "apr_general.h"
#include "apr_time.h"
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* APR_HAS_XLATE determines whether or not useful implementations of 
 * ap_xlate_open() et al are provided. 
 *
 * If APR_HAS_XLATE is zero, ap_xlate_open() et al will all return 
 * APR_ENOTIMPL at run-time.
 */

#if ! APR_HAS_XLATE

typedef void                         ap_xlate_t;

/* For platforms where we don't bother with translating between charsets,
 * these are macros which always return failure.
 */

#define ap_xlate_open(convset, topage, frompage, pool) APR_ENOTIMPL

#define ap_xlate_conv_buffer(convset, inbuf, inbytes_left, outbuf, \
                          outbytes_left) APR_ENOTIMPL

#define ap_xlate_conv_byte(convset, inchar) (-1)

/* The purpose of ap_xlate_conv_char is to translate one character
 * at a time.  This needs to be written carefully so that it works
 * with double-byte character sets. 
 */
#define ap_xlate_conv_char(convset, inchar, outchar) APR_ENOTIMPL

#define ap_xlate_close(convset) APR_ENOTIMPL

#else  /* ! APR_HAS_XLATE */

typedef struct ap_xlate_t            ap_xlate_t;

/*

=head1 ap_status_t ap_xlate_open(ap_xlate_t **convset, const char *topage, const char *frompage, ap_pool_t *pool)

B<Set up for converting text from one charset to another.>

    arg 1) The handle to be filled in by this function
    arg 2) The name of the target charset
    arg 3) The name of the source charset
    arg 4) The pool to use

B<NOTE>:  Specify APR_DEFAULT_CHARSET for one of the charset
          names to indicate the charset of the source code at
          compile time.  This is useful if there are literal
          strings in the source code which must be translated
          according to the charset of the source code.
          APR_DEFAULT_CHARSET is not useful if the source code
          of the caller was not encoded in the same charset as
          APR at compile time.

=cut
 */
ap_status_t ap_xlate_open(ap_xlate_t **convset, const char *topage, 
                       const char *frompage, ap_pool_t *pool);

#define APR_DEFAULT_CHARSET NULL

/*

=head1 ap_status_t ap_xlate_conv_buffer(ap_xlate_t *convset, const char *inbuf, ap_size_t *inbytes_left, char *outbuf, ap_size_t outbytes_left)

B<Convert a buffer of text from one codepage to another.>

    arg 1) The handle allocated by ap_xlate_open, specifying the parameters
           of conversion
    arg 2) The address of the source buffer
    arg 3) Input: the amount of input data to be translated
           Output: the amount of input data not yet translated    
    arg 4) The address of the destination buffer
    arg 5) Input: the size of the output buffer
           Output: the amount of the output buffer not yet used

=cut
 */
ap_status_t ap_xlate_conv_buffer(ap_xlate_t *convset, const char *inbuf, 
                              ap_size_t *inbytes_left, char *outbuf,
                              ap_size_t *outbytes_left);

/* The purpose of ap_xlate_conv_char is to translate one character
 * at a time.  This needs to be written carefully so that it works
 * with double-byte character sets. 
 */
ap_status_t ap_xlate_conv_char(ap_xlate_t *convset, char inchar, char outchar);

/*

=head1 ap_int32_t ap_xlate_conv_byte(ap_xlate_t *convset, unsigned char inchar)

B<Convert a single-byte character from one charset to another.>

    arg 1) The handle allocated by ap_xlate_open, specifying the parameters
           of conversion
    arg 2) The single-byte character to convert.

B<NOTE>: This only works when converting between single-byte character sets.
         -1 will be returned if the conversion can't be performed.

=cut
*/
ap_int32_t ap_xlate_conv_byte(ap_xlate_t *convset, unsigned char inchar);

/*

=head1 ap_status_t ap_xlate_close(ap_xlate_t *convset)

B<Close a codepage translation handle.>

    arg 1) The codepage translation handle to close

=cut
 */
ap_status_t ap_xlate_close(ap_xlate_t *convset);

#endif  /* ! APR_HAS_XLATE */

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_XLATE_H */
