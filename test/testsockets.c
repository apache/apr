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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "test_apr.h"

#if APR_HAVE_IPV6
#define US "::1"
#else
#define US "127.0.0.1"
#endif

static void closeapr(void)
{
    apr_terminate();
}

static void close_sock(apr_socket_t *sock)
{
    STD_TEST_NEQ("    Closing socket", apr_socket_close(sock))
}

#define STRLEN 21

int main(void)
{
    apr_pool_t *pool;
    apr_socket_t *sock = NULL, *sock2 = NULL;
    apr_sockaddr_t *from;
    apr_sockaddr_t *to;
    apr_size_t len = 30;
    char sendbuf[STRLEN] = "APR_INET, SOCK_DGRAM";
    char recvbuf[80];
    char *ip_addr;
    apr_port_t fromport;
#if APR_HAVE_IPV6
    int family = APR_INET6;
#else
    int family = APR_INET;
#endif

    STD_TEST_NEQ("Initializing APR", apr_initialize())

    atexit(closeapr);
    STD_TEST_NEQ("Creating 1st pool", apr_pool_create(&pool, NULL))

    printf("Testing socket creation functions.\n");

    STD_TEST_NEQ("    Creating a TCP socket",
                 apr_socket_create(&sock, APR_INET, SOCK_STREAM, pool))
    close_sock(sock);

    STD_TEST_NEQ("    Creating UDP socket",
                 apr_socket_create(&sock, APR_INET, SOCK_DGRAM, pool))
    close_sock(sock);

#if APR_HAVE_IPV6
    STD_TEST_NEQ("    Creating an IPv6 TCP socket",
                 apr_socket_create(&sock, APR_INET6, SOCK_STREAM, pool))
    close_sock(sock);

    STD_TEST_NEQ("    Creating an IPv6 UDP socket",
                 apr_socket_create(&sock, APR_INET6, SOCK_DGRAM, pool))
    close_sock(sock);
#else
    printf("NO IPv6 support.\n");
#endif

    printf("Now trying sendto/recvfrom (simple tests only)\n");

    STD_TEST_NEQ("    Creating socket #1 for test",
                 apr_socket_create(&sock, family, SOCK_DGRAM, pool))
    STD_TEST_NEQ("    Creating socket #2 for test",
                 apr_socket_create(&sock2, family, SOCK_DGRAM, pool))

    apr_sockaddr_info_get(&to, US, APR_UNSPEC, 7772, 0, pool);
    apr_sockaddr_info_get(&from, US, APR_UNSPEC, 7771, 0, pool);

    STD_TEST_NEQ("    Binding socket #1", apr_bind(sock, to))
    STD_TEST_NEQ("    Binding socket #2", apr_bind(sock2, from))

    len = STRLEN;

    STD_TEST_NEQ("    Trying to sendto",
                 apr_sendto(sock2, to, 0, sendbuf, &len))
    len = 80;
    STD_TEST_NEQ("    Trying to recvfrom",
                 apr_recvfrom(from, sock, 0, recvbuf, &len))
    printf("\t\tGot back %d bytes [%s] from recvfrom\n", len, recvbuf);   
    apr_sockaddr_ip_get(&ip_addr, from);
    apr_sockaddr_port_get(&fromport, from);
    printf("\t\tData came from %s:%u\n", ip_addr, fromport);

    close_sock(sock);
    close_sock(sock2);

    return 1;
}
