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

#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "test_apr.h"

static apr_socket_t *sock = NULL;

static void create_socket(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_socket_create(&sock, APR_INET, SOCK_STREAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
}

static void set_keepalive(CuTest *tc)
{
    apr_status_t rv;
    apr_int32_t ck;

    rv = apr_socket_opt_set(sock, APR_SO_KEEPALIVE, 1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_opt_get(sock, APR_SO_KEEPALIVE, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);
}

static void set_debug(CuTest *tc)
{
    apr_status_t rv1, rv2;
    apr_int32_t ck;
    
    /* On some platforms APR_SO_DEBUG can only be set as root; just test
     * for get/set consistency of this option. */
    rv1 = apr_socket_opt_set(sock, APR_SO_DEBUG, 1);
    rv2 = apr_socket_opt_get(sock, APR_SO_DEBUG, &ck);
    apr_assert_success(tc, "get SO_DEBUG option", rv2);
    if (APR_STATUS_IS_SUCCESS(rv1)) {
        CuAssertIntEquals(tc, 1, ck);
    } else {
        CuAssertIntEquals(tc, 0, ck);
    }
}

static void remove_keepalive(CuTest *tc)
{
    apr_status_t rv;
    apr_int32_t ck;

    rv = apr_socket_opt_get(sock, APR_SO_KEEPALIVE, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);

    rv = apr_socket_opt_set(sock, APR_SO_KEEPALIVE, 0);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_opt_get(sock, APR_SO_KEEPALIVE, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 0, ck);
}

static void corkable(CuTest *tc)
{
#if !APR_HAVE_CORKABLE_TCP
    CuNotImpl(tc, "TCP isn't corkable");
#else
    apr_status_t rv;
    apr_int32_t ck;

    rv = apr_socket_opt_set(sock, APR_TCP_NODELAY, 1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_opt_get(sock, APR_TCP_NODELAY, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);

    rv = apr_socket_opt_set(sock, APR_TCP_NOPUSH, 1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_opt_get(sock, APR_TCP_NOPUSH, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);

    rv = apr_socket_opt_get(sock, APR_TCP_NODELAY, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 0, ck);

    rv = apr_socket_opt_set(sock, APR_TCP_NOPUSH, 0);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    
    rv = apr_socket_opt_get(sock, APR_TCP_NODELAY, &ck);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, 1, ck);
#endif
}

static void close_socket(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_socket_close(sock);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

CuSuite *testsockopt(void)
{
    CuSuite *suite = CuSuiteNew("Socket Options");

    SUITE_ADD_TEST(suite, create_socket);
    SUITE_ADD_TEST(suite, set_keepalive);
    SUITE_ADD_TEST(suite, set_debug);
    SUITE_ADD_TEST(suite, remove_keepalive);
    SUITE_ADD_TEST(suite, corkable);
    SUITE_ADD_TEST(suite, close_socket);

    return suite;
}

