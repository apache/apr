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

#ifndef UTF8_H
#define UTF8_H

#include "apr.h"
#include "apr_lib.h"
#include "apr_errno.h"

/* If we ever support anything more exciting than char... this could move.
 */
typedef apr_uint16_t apr_wchar_t;

/**
 * An APR internal function for fast utf-8 octet-encoded Unicode conversion
 * to the ucs-2 wide Unicode format.  This function is used for filename and 
 * other resource conversions for platforms providing native Unicode support.
 *
 * @tip Only the errors APR_EINVAL and APR_INCOMPLETE may occur, the former
 * when the character code is invalid (in or out of context) and the later
 * when more characters were expected, but insufficient characters remain.
 */
APR_DECLARE(apr_status_t) apr_conv_utf8_to_ucs2(const char *in, 
                                                apr_size_t *inbytes,
                                                apr_wchar_t *out, 
                                                apr_size_t *outwords);

/**
 * An APR internal function for fast ucs-2 wide Unicode format conversion to 
 * the utf-8 octet-encoded Unicode.  This function is used for filename and 
 * other resource conversions for platforms providing native Unicode support.
 *
 * @tip Only the errors APR_EINVAL and APR_INCOMPLETE may occur, the former
 * when the character code is invalid (in or out of context) and the later
 * when more words were expected, but insufficient words remain.
 */
APR_DECLARE(apr_status_t) apr_conv_ucs2_to_utf8(const apr_wchar_t *in, 
                                                apr_size_t *inwords,
                                                char *out, 
                                                apr_size_t *outbytes);

#endif /* def UTF8_H */
