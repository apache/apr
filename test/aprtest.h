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

#include "apr_errno.h"
#include "apr_general.h"
#include "apr_strings.h"

#ifndef APR_TEST_PREFIX
#define APR_TEST_PREFIX ""
#endif

#define APR_TEST_BEGIN(rv, desc, op) \
    fprintf(stdout, "%s%.*s ", APR_TEST_PREFIX desc,                  \
            strlen(desc) < 37 ? (int)(40 - strlen(desc)) : 3,         \
            "........................................");              \
    APR_TEST_MORE(rv, op)

#define APR_TEST_MORE(rv, op) \
    if ((rv = (op)) != APR_SUCCESS) {                                 \
        char msgbuf[256];                                             \
        fprintf (stdout, "Failed\n");                                 \
        fprintf (stderr, "Error (%d): %s\n%s", rv, #op,               \
                 apr_strerror(rv, msgbuf, sizeof(msgbuf)));           \
        exit(-1); }

#define APR_TEST_END(rv, op) \
    APR_TEST_MORE(rv, op)                                             \
    fprintf(stdout, "OK\n");

#define APR_TEST_SUCCESS(rv, desc, op) \
    APR_TEST_BEGIN(rv, desc, op)                                      \
    fprintf(stdout, "OK\n");

#define APR_TEST_INITIALIZE(rv, pool) \
    APR_TEST_SUCCESS(rv, "Initializing", apr_initialize());           \
    atexit(apr_terminate);                                            \
    APR_TEST_SUCCESS(rv, "Creating context",                          \
                     apr_pool_create(&pool, NULL));

