/*
 * This is work is derived from material Copyright RSA Data Security, Inc.
 *
 * The RSA copyright statement and Licence for that original material is
 * included below. This is followed by the Apache copyright statement and
 * licence for the modifications made to that material.
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
 */

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

#ifndef APR_MD5_H
#define APR_MD5_H

#include "apr_lib.h"
#include "apr_xlate.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @package APR MD5 Library
 */

#define MD5_DIGESTSIZE 16

/* UINT4 defines a four byte word */
typedef unsigned int UINT4;

/* MD5 context. */
typedef struct {
    UINT4 state[4];		/* state (ABCD) */
    UINT4 count[2];		/* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];	/* input buffer */
#if APR_HAS_XLATE
    ap_xlate_t *xlate;          /* translation handle */
#endif
} ap_md5_ctx_t;

/**
 * MD5 Initialize.  Begins an MD5 operation, writing a new context.
 * @param context The MD5 context to initialize.
 * @deffunc ap_status_t ap_MD5Init(ap_md5_ctx_t *context)
 */
APR_EXPORT(ap_status_t) ap_MD5Init(ap_md5_ctx_t *context);

/**
 * MD5 translation setup.  Provides the APR translation handle to be used 
 * for translating the content before calculating the digest.
 * @param context The MD5 content to set the translation for.
 * @param xlate The translation handle to use for this MD5 context 
 * @deffunc ap_status_t ap_MD5SetXlate(ap_md5_ctx_t *context, ap_xlate_t *xlate)
 */
#if APR_HAS_XLATE
APR_EXPORT(ap_status_t) ap_MD5SetXlate(ap_md5_ctx_t *context, ap_xlate_t *xlate);
#else
#define ap_MD5SetXlate(context, xlate) APR_ENOTIMPL
#endif

/**
 * MD5 block update operation.  Continue an MD5 message-digest operation, 
 * processing another message block, and updating the context.
 * @param context The MD5 content to update.
 * @param input next message block to update
 * @param inputLen The length of the next message block
 * @deffunc ap_status_t ap_MD5Update(ap_md5_ctx_t *context, const unsigned char *input, unsigned int inputLen)
 */
APR_EXPORT(ap_status_t) ap_MD5Update(ap_md5_ctx_t *context,
                                     const unsigned char *input,
                                     unsigned int inputLen);

/**
 * MD5 finalization.  Ends an MD5 message-digest operation, writing the 
 * message digest and zeroing the context
 * @param digest The final MD5 digest
 * @param context The MD5 content we are finalizing.
 * @deffunc ap_status_t ap_MD5Final(unsigned char digest[MD5_DIGESTSIZE], ap_md5_ctx_t *context)
 */
APR_EXPORT(ap_status_t) ap_MD5Final(unsigned char digest[MD5_DIGESTSIZE],
                                    ap_md5_ctx_t *context);

/**
 * Encode a password using an MD5 algorithm
 * @param password The password to encode
 * @param salt The salt to use for the encoding
 * @param result The string to store the encoded password in
 * @param nbytes The length of the string
 * @deffunc ap_status_t ap_MD5Encode(const char *password, const char *salt, char *result, size_t nbytes)
 */
APR_EXPORT(ap_status_t) ap_MD5Encode(const char *password, const char *salt,
                                     char *result, size_t nbytes);

#ifdef __cplusplus
}
#endif

#endif	/* !APR_MD5_H */
