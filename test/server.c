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

#define APR_TEST_PREFIX "server: "

#include "aprtest.h"
#include <stdlib.h>
#include "apr_network_io.h"
#include "apr_getopt.h"
#include "apr_poll.h"

#define STRLEN 15

int main(int argc, const char * const argv[])
{
    apr_pool_t *context;
    apr_status_t rv;
    apr_socket_t *sock;
    apr_socket_t *sock2;
    apr_size_t length;
    apr_int32_t pollres;
    apr_pollfd_t *sdset;
    char datasend[STRLEN];
    char datarecv[STRLEN] = "Recv data test";
    const char *bind_to_ipaddr = NULL;
    char *local_ipaddr, *remote_ipaddr;
    apr_port_t local_port, remote_port;
    apr_sockaddr_t *localsa = NULL, *remotesa;
    apr_status_t stat;
    int family = APR_UNSPEC;
    int protocol;
    apr_getopt_t *opt;
    const char *optarg;
    char optchar;

    APR_TEST_INITIALIZE(rv, context);

    APR_TEST_SUCCESS(rv, "Preparing getopt", 
                     apr_getopt_init(&opt, context, argc, argv))
    
    while ((stat = apr_getopt(opt, "i:", &optchar, &optarg)) == APR_SUCCESS) {
        switch(optchar) {
        case 'i':
            bind_to_ipaddr = optarg;
            break;
        }
    }
    if (stat != APR_EOF) {
        fprintf(stderr,
                "usage: %s [-i local-interface-address]\n",
                argv[0]);
        exit(-1);
    }

    if (bind_to_ipaddr) {
        /* First, parse/resolve ipaddr so we know what address family of
         * socket we need.  We'll use the returned sockaddr later when
         * we bind.
         */
        APR_TEST_SUCCESS(rv, "Preparing sockaddr", 
            apr_sockaddr_info_get(&localsa, bind_to_ipaddr, APR_UNSPEC, 8021, 0, context))
        family = localsa->family;
    }

    APR_TEST_SUCCESS(rv, "Creating new socket", 
        apr_socket_create_ex(&sock, family, SOCK_STREAM, APR_PROTO_TCP, context))

    APR_TEST_SUCCESS(rv, "Setting option APR_SO_NONBLOCK",
        apr_socket_opt_set(sock, APR_SO_NONBLOCK, 1))

    APR_TEST_SUCCESS(rv, "Setting option APR_SO_REUSEADDR",
        apr_socket_opt_set(sock, APR_SO_REUSEADDR, 1))

    if (!localsa) {
        apr_socket_addr_get(&localsa, APR_LOCAL, sock);
        apr_sockaddr_port_set(localsa, 8021);
    }

    APR_TEST_SUCCESS(rv, "Binding socket to port",
        apr_socket_bind(sock, localsa))
    
    APR_TEST_SUCCESS(rv, "Listening to socket",
        apr_socket_listen(sock, 5))
    
    APR_TEST_BEGIN(rv, "Setting up for polling",
        apr_poll_setup(&sdset, 1, context))
    APR_TEST_END(rv, 
        apr_poll_socket_add(sdset, sock, APR_POLLIN))
    
    pollres = 1; 
    APR_TEST_BEGIN(rv, "Polling for socket",
        apr_poll(sdset, 1, &pollres, -1))

    if (pollres == 0) {
        fprintf(stdout, "Failed\n");
        apr_socket_close(sock);
        fprintf(stderr, "Error: Unrecognized poll result, "
                "expected 1, received %d\n", pollres);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    APR_TEST_SUCCESS(rv, "Accepting a connection",
        apr_socket_accept(&sock2, sock, context))

    apr_socket_protocol_get(sock2, &protocol);
    if (protocol != APR_PROTO_TCP) {
        fprintf(stderr, "Error: protocol not conveyed from listening socket "
                "to connected socket!\n");
        exit(1);
    }
    apr_socket_addr_get(&remotesa, APR_REMOTE, sock2);
    apr_sockaddr_ip_get(&remote_ipaddr, remotesa);
    apr_sockaddr_port_get(&remote_port, remotesa);
    apr_socket_addr_get(&localsa, APR_LOCAL, sock2);
    apr_sockaddr_ip_get(&local_ipaddr, localsa);
    apr_sockaddr_port_get(&local_port, localsa);
    fprintf(stdout, "Server socket: %s:%u -> %s:%u\n", local_ipaddr, 
            local_port, remote_ipaddr, remote_port);

    length = STRLEN;
    APR_TEST_BEGIN(rv, "Receiving data from socket",
        apr_socket_recv(sock2, datasend, &length))

    if (strcmp(datasend, "Send data test")) {
        fprintf(stdout, "Failed\n");
        apr_socket_close(sock);
        apr_socket_close(sock2);
        fprintf(stderr, "Error: Unrecognized response;\n"
                "Expected: \"Send data test\"\n"
                "Received: \"%s\"\n", datarecv);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    length = STRLEN;
    APR_TEST_SUCCESS(rv, "Sending data over socket",
        apr_socket_send(sock2, datarecv, &length))
    
    APR_TEST_SUCCESS(rv, "Shutting down accepted socket",
        apr_socket_shutdown(sock2, APR_SHUTDOWN_READ))

    APR_TEST_SUCCESS(rv, "Closing duplicate socket",
        apr_socket_close(sock2))
    
    APR_TEST_SUCCESS(rv, "Closing original socket",
        apr_socket_close(sock))

    return 0;
}

