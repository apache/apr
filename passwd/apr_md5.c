/*
 * This is work is derived from material Copyright RSA Data Security, Inc.
 *
 * The RSA copyright statement and Licence for that original material is
 * included below. This is followed by the Apache copyright statement and
 * licence for the modifications made to that material.
 */

/* MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
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

/*
 * The apr_MD5Encode() routine uses much code obtained from the FreeBSD 3.0
 * MD5 crypt() function, which is licenced as follows:
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dknet.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 */
#ifndef WIN32
#include "apr_private.h"
#endif
#include "apr_strings.h"
#include "apr_md5.h"
#include "apr_lib.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Constants for MD5Transform routine.
 */

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static void MD5Transform(UINT4 state[4], const unsigned char block[64]);
static void Encode(unsigned char *output, const UINT4 *input,
		   unsigned int len);
static void Decode(UINT4 *output, const unsigned char *input,
		   unsigned int len);

static unsigned char PADDING[64] =
{
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#ifdef CHARSET_EBCDIC
static apr_xlate_t *xlate_ebcdic_to_ascii; /* used in apr_MD5Encode() */
#endif

/* F, G, H and I are basic MD5 functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/* ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/* FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
   Rotation is separate from addition to prevent recomputation.
 */
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (UINT4)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

/* MD5 initialization. Begins an MD5 operation, writing a new context.
 */
APR_EXPORT(apr_status_t) apr_MD5Init(apr_md5_ctx_t *context)
{
    context->count[0] = context->count[1] = 0;
    /* Load magic initialization constants. */
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
#if APR_HAS_XLATE
    context->xlate = NULL;
#endif
    return APR_SUCCESS;
}

#if APR_HAS_XLATE
/* MD5 translation setup.  Provides the APR translation handle
 * to be used for translating the content before calculating the
 * digest.
 */
APR_EXPORT(apr_status_t) apr_MD5SetXlate(apr_md5_ctx_t *context, 
                                         apr_xlate_t *xlate)
{
    apr_status_t rv;
    int is_sb;

    /* TODO: remove the single-byte-only restriction from this code
     */
    rv = apr_xlate_get_sb(xlate, &is_sb);
    if (rv != APR_SUCCESS) {
        return rv;
    }
    if (!is_sb) {
        return APR_EINVAL;
    }
    context->xlate = xlate;
    return APR_SUCCESS;
}
#endif /* APR_HAS_XLATE */

/* MD5 block update operation. Continues an MD5 message-digest
   operation, processing another message block, and updating the
   context.
 */
APR_EXPORT(apr_status_t) apr_MD5Update(apr_md5_ctx_t *context,
                                     const unsigned char *input,
                                     unsigned int inputLen)
{
    unsigned int i, idx, partLen;
#if APR_HAS_XLATE
    apr_size_t inbytes_left, outbytes_left;
#endif

    /* Compute number of bytes mod 64 */
    idx = (unsigned int) ((context->count[0] >> 3) & 0x3F);

    /* Update number of bits */
    if ((context->count[0] += ((UINT4) inputLen << 3)) 
            < ((UINT4) inputLen << 3))
	context->count[1]++;
    context->count[1] += (UINT4) inputLen >> 29;

    partLen = 64 - idx;

    /* Transform as many times as possible. */
#if !APR_HAS_XLATE
    if (inputLen >= partLen) {
	memcpy(&context->buffer[idx], input, partLen);
	MD5Transform(context->state, context->buffer);

	for (i = partLen; i + 63 < inputLen; i += 64)
	    MD5Transform(context->state, &input[i]);

	idx = 0;
    }
    else
	i = 0;

    /* Buffer remaining input */
    memcpy(&context->buffer[idx], &input[i], inputLen - i);
#else /*APR_HAS_XLATE*/
    if (inputLen >= partLen) {
        if (context->xlate) {
            inbytes_left = outbytes_left = partLen;
            apr_xlate_conv_buffer(context->xlate, input, &inbytes_left,
                                 &context->buffer[idx],&outbytes_left);
        }
        else {
            memcpy(&context->buffer[idx], input, partLen);
        }
	MD5Transform(context->state, context->buffer);

	for (i = partLen; i + 63 < inputLen; i += 64) {
            if (context->xlate) {
                unsigned char inp_tmp[64];
                inbytes_left = outbytes_left = 64;
                apr_xlate_conv_buffer(context->xlate, &input[i], &inbytes_left,
                                     inp_tmp, &outbytes_left);
                MD5Transform(context->state, inp_tmp);
            }
            else {
                MD5Transform(context->state, &input[i]);
            }
	}

	idx = 0;
    }
    else
	i = 0;

    /* Buffer remaining input */
    if (context->xlate) {
        inbytes_left = outbytes_left = inputLen - i;
        apr_xlate_conv_buffer(context->xlate, &input[i], &inbytes_left,
                             &context->buffer[idx], &outbytes_left);
    }
    else {
        memcpy(&context->buffer[idx], &input[i], inputLen - i);
    }
#endif /*APR_HAS_XLATE*/
    return APR_SUCCESS;
}

/* MD5 finalization. Ends an MD5 message-digest operation, writing the
   the message digest and zeroizing the context.
 */
APR_EXPORT(apr_status_t) apr_MD5Final(unsigned char digest[MD5_DIGESTSIZE],
                                      apr_md5_ctx_t *context)
{
    unsigned char bits[8];
    unsigned int idx, padLen;

    /* Save number of bits */
    Encode(bits, context->count, 8);

#if APR_HAS_XLATE
    /* apr_MD5Update() should not translate for this final round. */
    context->xlate = NULL;
#endif /*APR_HAS_XLATE*/

    /* Pad out to 56 mod 64. */
    idx = (unsigned int) ((context->count[0] >> 3) & 0x3f);
    padLen = (idx < 56) ? (56 - idx) : (120 - idx);
    apr_MD5Update(context, PADDING, padLen);

    /* Append length (before padding) */
    apr_MD5Update(context, bits, 8);

    /* Store state in digest */
    Encode(digest, context->state, MD5_DIGESTSIZE);

    /* Zeroize sensitive information. */
    memset(context, 0, sizeof(*context));
    
    return APR_SUCCESS;
}

/* MD5 basic transformation. Transforms state based on block. */
static void MD5Transform(UINT4 state[4], const unsigned char block[64])
{
    UINT4 a = state[0], b = state[1], c = state[2], d = state[3],
          x[MD5_DIGESTSIZE];

    Decode(x, block, 64);

    /* Round 1 */
    FF(a, b, c, d, x[0], S11, 0xd76aa478);	/* 1 */
    FF(d, a, b, c, x[1], S12, 0xe8c7b756);	/* 2 */
    FF(c, d, a, b, x[2], S13, 0x242070db);	/* 3 */
    FF(b, c, d, a, x[3], S14, 0xc1bdceee);	/* 4 */
    FF(a, b, c, d, x[4], S11, 0xf57c0faf);	/* 5 */
    FF(d, a, b, c, x[5], S12, 0x4787c62a);	/* 6 */
    FF(c, d, a, b, x[6], S13, 0xa8304613);	/* 7 */
    FF(b, c, d, a, x[7], S14, 0xfd469501);	/* 8 */
    FF(a, b, c, d, x[8], S11, 0x698098d8);	/* 9 */
    FF(d, a, b, c, x[9], S12, 0x8b44f7af);	/* 10 */
    FF(c, d, a, b, x[10], S13, 0xffff5bb1);	/* 11 */
    FF(b, c, d, a, x[11], S14, 0x895cd7be);	/* 12 */
    FF(a, b, c, d, x[12], S11, 0x6b901122);	/* 13 */
    FF(d, a, b, c, x[13], S12, 0xfd987193);	/* 14 */
    FF(c, d, a, b, x[14], S13, 0xa679438e);	/* 15 */
    FF(b, c, d, a, x[15], S14, 0x49b40821);	/* 16 */

    /* Round 2 */
    GG(a, b, c, d, x[1], S21, 0xf61e2562);	/* 17 */
    GG(d, a, b, c, x[6], S22, 0xc040b340);	/* 18 */
    GG(c, d, a, b, x[11], S23, 0x265e5a51);	/* 19 */
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);	/* 20 */
    GG(a, b, c, d, x[5], S21, 0xd62f105d);	/* 21 */
    GG(d, a, b, c, x[10], S22, 0x2441453);	/* 22 */
    GG(c, d, a, b, x[15], S23, 0xd8a1e681);	/* 23 */
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);	/* 24 */
    GG(a, b, c, d, x[9], S21, 0x21e1cde6);	/* 25 */
    GG(d, a, b, c, x[14], S22, 0xc33707d6);	/* 26 */
    GG(c, d, a, b, x[3], S23, 0xf4d50d87);	/* 27 */
    GG(b, c, d, a, x[8], S24, 0x455a14ed);	/* 28 */
    GG(a, b, c, d, x[13], S21, 0xa9e3e905);	/* 29 */
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8);	/* 30 */
    GG(c, d, a, b, x[7], S23, 0x676f02d9);	/* 31 */
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);	/* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[5], S31, 0xfffa3942);	/* 33 */
    HH(d, a, b, c, x[8], S32, 0x8771f681);	/* 34 */
    HH(c, d, a, b, x[11], S33, 0x6d9d6122);	/* 35 */
    HH(b, c, d, a, x[14], S34, 0xfde5380c);	/* 36 */
    HH(a, b, c, d, x[1], S31, 0xa4beea44);	/* 37 */
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9);	/* 38 */
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60);	/* 39 */
    HH(b, c, d, a, x[10], S34, 0xbebfbc70);	/* 40 */
    HH(a, b, c, d, x[13], S31, 0x289b7ec6);	/* 41 */
    HH(d, a, b, c, x[0], S32, 0xeaa127fa);	/* 42 */
    HH(c, d, a, b, x[3], S33, 0xd4ef3085);	/* 43 */
    HH(b, c, d, a, x[6], S34, 0x4881d05);	/* 44 */
    HH(a, b, c, d, x[9], S31, 0xd9d4d039);	/* 45 */
    HH(d, a, b, c, x[12], S32, 0xe6db99e5);	/* 46 */
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8);	/* 47 */
    HH(b, c, d, a, x[2], S34, 0xc4ac5665);	/* 48 */

    /* Round 4 */
    II(a, b, c, d, x[0], S41, 0xf4292244);	/* 49 */
    II(d, a, b, c, x[7], S42, 0x432aff97);	/* 50 */
    II(c, d, a, b, x[14], S43, 0xab9423a7);	/* 51 */
    II(b, c, d, a, x[5], S44, 0xfc93a039);	/* 52 */
    II(a, b, c, d, x[12], S41, 0x655b59c3);	/* 53 */
    II(d, a, b, c, x[3], S42, 0x8f0ccc92);	/* 54 */
    II(c, d, a, b, x[10], S43, 0xffeff47d);	/* 55 */
    II(b, c, d, a, x[1], S44, 0x85845dd1);	/* 56 */
    II(a, b, c, d, x[8], S41, 0x6fa87e4f);	/* 57 */
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0);	/* 58 */
    II(c, d, a, b, x[6], S43, 0xa3014314);	/* 59 */
    II(b, c, d, a, x[13], S44, 0x4e0811a1);	/* 60 */
    II(a, b, c, d, x[4], S41, 0xf7537e82);	/* 61 */
    II(d, a, b, c, x[11], S42, 0xbd3af235);	/* 62 */
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb);	/* 63 */
    II(b, c, d, a, x[9], S44, 0xeb86d391);	/* 64 */

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    /* Zeroize sensitive information. */
    memset(x, 0, sizeof(x));
}

/* Encodes input (UINT4) into output (unsigned char). Assumes len is
   a multiple of 4.
 */
static void Encode(unsigned char *output, const UINT4 *input, unsigned int len)
{
    unsigned int i, j;
    UINT4 k;

    for (i = 0, j = 0; j < len; i++, j += 4) {
	k = input[i];
	output[j] = (unsigned char) (k & 0xff);
	output[j + 1] = (unsigned char) ((k >> 8) & 0xff);
	output[j + 2] = (unsigned char) ((k >> 16) & 0xff);
	output[j + 3] = (unsigned char) ((k >> 24) & 0xff);
    }
}

/* Decodes input (unsigned char) into output (UINT4). Assumes len is
 * a multiple of 4.
 */
static void Decode(UINT4 *output, const unsigned char *input, unsigned int len)
{
    unsigned int i, j;

    for (i = 0, j = 0; j < len; i++, j += 4)
	output[i] = ((UINT4) input[j]) | (((UINT4) input[j + 1]) << 8) |
	    (((UINT4) input[j + 2]) << 16) | (((UINT4) input[j + 3]) << 24);
}

#ifdef CHARSET_EBCDIC
APR_EXPORT(apr_status_t) apr_MD5InitEBCDIC(apr_xlate_t *xlate)
{
    xlate_ebcdic_to_ascii = xlate;
    return APR_SUCCESS;
}
#endif

/*
 * Define the Magic String prefix that identifies a password as being
 * hashed using our algorithm.
 */
static const char *apr1_id = "$apr1$";

/*
 * The following MD5 password encryption code was largely borrowed from
 * the FreeBSD 3.0 /usr/src/lib/libcrypt/crypt.c file, which is
 * licenced as stated at the top of this file.
 */

static void to64(char *s, unsigned long v, int n)
{
    static unsigned char itoa64[] =         /* 0 ... 63 => ASCII - 64 */
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    while (--n >= 0) {
	*s++ = itoa64[v&0x3f];
	v >>= 6;
    }
}

APR_EXPORT(apr_status_t) apr_MD5Encode(const char *pw, const char *salt,
                             char *result, size_t nbytes)
{
    /*
     * Minimum size is 8 bytes for salt, plus 1 for the trailing NUL,
     * plus 4 for the '$' separators, plus the password hash itself.
     * Let's leave a goodly amount of leeway.
     */

    char passwd[120], *p;
    const char *sp, *ep;
    unsigned char final[MD5_DIGESTSIZE];
    int sl, pl, i;
    apr_md5_ctx_t ctx, ctx1;
    unsigned long l;

    /* 
     * Refine the salt first.  It's possible we were given an already-hashed
     * string as the salt argument, so extract the actual salt value from it
     * if so.  Otherwise just use the string up to the first '$' as the salt.
     */
    sp = salt;

    /*
     * If it starts with the magic string, then skip that.
     */
    if (!strncmp(sp, apr1_id, strlen(apr1_id))) {
	sp += strlen(apr1_id);
    }

    /*
     * It stops at the first '$' or 8 chars, whichever comes first
     */
    for (ep = sp; (*ep != '\0') && (*ep != '$') && (ep < (sp + 8)); ep++) {
	continue;
    }

    /*
     * Get the length of the true salt
     */
    sl = ep - sp;

    /*
     * 'Time to make the doughnuts..'
     */
    apr_MD5Init(&ctx);
#ifdef CHARSET_EBCDIC
    apr_MD5SetXlate(&ctx, xlate_ebcdic_to_ascii);
#endif
    
    /*
     * The password first, since that is what is most unknown
     */
    apr_MD5Update(&ctx, (unsigned char *)pw, strlen(pw));

    /*
     * Then our magic string
     */
    apr_MD5Update(&ctx, (unsigned char *)apr1_id, strlen(apr1_id));

    /*
     * Then the raw salt
     */
    apr_MD5Update(&ctx, (unsigned char *)sp, sl);

    /*
     * Then just as many characters of the MD5(pw, salt, pw)
     */
    apr_MD5Init(&ctx1);
    apr_MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));
    apr_MD5Update(&ctx1, (unsigned char *)sp, sl);
    apr_MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));
    apr_MD5Final(final, &ctx1);
    for (pl = strlen(pw); pl > 0; pl -= MD5_DIGESTSIZE) {
        apr_MD5Update(&ctx, final, 
                      (pl > MD5_DIGESTSIZE) ? MD5_DIGESTSIZE : pl);
    }

    /*
     * Don't leave anything around in vm they could use.
     */
    memset(final, 0, sizeof(final));

    /*
     * Then something really weird...
     */
    for (i = strlen(pw); i != 0; i >>= 1) {
	if (i & 1) {
	    apr_MD5Update(&ctx, final, 1);
	}
	else {
	    apr_MD5Update(&ctx, (unsigned char *)pw, 1);
	}
    }

    /*
     * Now make the output string.  We know our limitations, so we
     * can use the string routines without bounds checking.
     */
    strcpy(passwd, apr1_id);
    strncat(passwd, sp, sl);
    strcat(passwd, "$");

    apr_MD5Final(final, &ctx);

    /*
     * And now, just to make sure things don't run too fast..
     * On a 60 Mhz Pentium this takes 34 msec, so you would
     * need 30 seconds to build a 1000 entry dictionary...
     */
    for (i = 0; i < 1000; i++) {
	apr_MD5Init(&ctx1);
	if (i & 1) {
	    apr_MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));
	}
	else {
	    apr_MD5Update(&ctx1, final, MD5_DIGESTSIZE);
	}
	if (i % 3) {
	    apr_MD5Update(&ctx1, (unsigned char *)sp, sl);
	}

	if (i % 7) {
	    apr_MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));
	}

	if (i & 1) {
	    apr_MD5Update(&ctx1, final, MD5_DIGESTSIZE);
	}
	else {
	    apr_MD5Update(&ctx1, (unsigned char *)pw, strlen(pw));
	}
	apr_MD5Final(final,&ctx1);
    }

    p = passwd + strlen(passwd);

    l = (final[ 0]<<16) | (final[ 6]<<8) | final[12]; to64(p, l, 4); p += 4;
    l = (final[ 1]<<16) | (final[ 7]<<8) | final[13]; to64(p, l, 4); p += 4;
    l = (final[ 2]<<16) | (final[ 8]<<8) | final[14]; to64(p, l, 4); p += 4;
    l = (final[ 3]<<16) | (final[ 9]<<8) | final[15]; to64(p, l, 4); p += 4;
    l = (final[ 4]<<16) | (final[10]<<8) | final[ 5]; to64(p, l, 4); p += 4;
    l =                    final[11]                ; to64(p, l, 2); p += 2;
    *p = '\0';

    /*
     * Don't leave anything around in vm they could use.
     */
    memset(final, 0, sizeof(final));

    apr_cpystrn(result, passwd, nbytes - 1);
    return APR_SUCCESS;
}

/*
 * Validate a plaintext password against a smashed one.  Use either
 * crypt() (if available) or apr_MD5Encode(), depending upon the format
 * of the smashed input password.  Return APR_SUCCESS if they match, or
 * APR_EMISMATCH if they don't.
 */

APR_EXPORT(apr_status_t) apr_validate_password(const char *passwd, 
                                               const char *hash)
{
    char sample[120];
#if !defined(WIN32) && !defined(BEOS)
    char *crypt_pw;
#endif
    if (!strncmp(hash, apr1_id, strlen(apr1_id))) {
	/*
	 * The hash was created using our custom algorithm.
	 */
	apr_MD5Encode(passwd, hash, sample, sizeof(sample));
    }
    else {
	/*
	 * It's not our algorithm, so feed it to crypt() if possible.
	 */
#if defined(WIN32) || defined(BEOS)
	apr_cpystrn(sample, passwd, sizeof(sample) - 1);
#else
	crypt_pw = crypt(passwd, hash);
	apr_cpystrn(sample, crypt_pw, sizeof(sample) - 1);
#endif
    }
    return (strcmp(sample, hash) == 0) ? APR_SUCCESS : APR_EMISMATCH;
}
