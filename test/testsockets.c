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

#if APR_HAVE_IPV6
#define US "::1"
#define FAMILY APR_INET6
#else
#define US "127.0.0.1"
#define FAMILY APR_INET
#endif

#define STRLEN 21

static void tcp_socket(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock = NULL;

    rv = apr_socket_create(&sock, APR_INET, SOCK_STREAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
    apr_socket_close(sock);
}

static void udp_socket(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock = NULL;

    rv = apr_socket_create(&sock, APR_INET, SOCK_DGRAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
    apr_socket_close(sock);
}

static void tcp6_socket(CuTest *tc)
{
#if APR_HAVE_IPV6
    apr_status_t rv;
    apr_socket_t *sock = NULL;

    rv = apr_socket_create(&sock, APR_INET6, SOCK_STREAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
    apr_socket_close(sock);
#else
    CuNotImpl(tc, "IPv6");
#endif
}

static void udp6_socket(CuTest *tc)
{
#if APR_HAVE_IPV6
    apr_status_t rv;
    apr_socket_t *sock = NULL;

    rv = apr_socket_create(&sock, APR_INET6, SOCK_DGRAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, sock);
    apr_socket_close(sock);
#else
    CuNotImpl(tc, "IPv6");
#endif
}

static void sendto_receivefrom(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock = NULL;
    apr_socket_t *sock2 = NULL;
    char sendbuf[STRLEN] = "APR_INET, SOCK_DGRAM";
    char recvbuf[80];
    char *ip_addr;
    apr_port_t fromport;
    apr_sockaddr_t *from;
    apr_sockaddr_t *to;
    apr_size_t len = 30;

    rv = apr_socket_create(&sock, FAMILY, SOCK_DGRAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_socket_create(&sock2, FAMILY, SOCK_DGRAM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_sockaddr_info_get(&to, US, APR_UNSPEC, 7772, 0, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_sockaddr_info_get(&from, US, APR_UNSPEC, 7771, 0, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_socket_bind(sock, to);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_socket_bind(sock2, from);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    len = STRLEN;
    rv = apr_socket_sendto(sock2, to, 0, sendbuf, &len);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, STRLEN, len);

    len = 80;
    rv = apr_socket_recvfrom(from, sock, 0, recvbuf, &len);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, STRLEN, len);
    CuAssertStrEquals(tc, "APR_INET, SOCK_DGRAM", recvbuf);

    apr_sockaddr_ip_get(&ip_addr, from);
    apr_sockaddr_port_get(&fromport, from);
    CuAssertStrEquals(tc, US, ip_addr);
    CuAssertIntEquals(tc, 7771, fromport);

    apr_socket_close(sock);
    apr_socket_close(sock2);
}

CuSuite *testsockets(void)
{
    CuSuite *suite = CuSuiteNew("Socket Creation");

    SUITE_ADD_TEST(suite, tcp_socket);
    SUITE_ADD_TEST(suite, udp_socket);

    SUITE_ADD_TEST(suite, tcp6_socket);
    SUITE_ADD_TEST(suite, udp6_socket);

    SUITE_ADD_TEST(suite, sendto_receivefrom);
    return suite;
}

