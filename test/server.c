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

#include <stdlib.h>
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"

#define STRLEN 15

int main(int argc, char * const argv[])
{
    apr_pool_t *context;
    apr_socket_t *sock;
    apr_socket_t *sock2;
    apr_ssize_t length;
    apr_int32_t rv;
    apr_pollfd_t *sdset;
    char datasend[STRLEN];
    char datarecv[STRLEN] = "Recv data test";
    const char *bind_to_ipaddr = NULL;
    char *local_ipaddr, *remote_ipaddr;
    apr_port_t local_port, remote_port;
    apr_sockaddr_t *localsa = NULL;
    apr_status_t stat;
    int family = APR_UNSPEC;
    char buf[128];
    apr_getopt_t *opt;
    const char *optarg;
    char optchar;

    fprintf(stdout, "Initializing.........");
    if (apr_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Something went wrong\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    atexit(apr_terminate);

    fprintf(stdout, "Creating context.......");
    if (apr_create_pool(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Could not create a context\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    if (apr_initopt(&opt, context, argc, argv)) {
        fprintf(stderr, "failed to initialize opts\n");
        exit(-1);
    }

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
        stat = apr_getaddrinfo(&localsa, bind_to_ipaddr, APR_UNSPEC, 8021, 0,
                               context);
        if (stat != APR_SUCCESS) {
            fprintf(stderr,
                    "Couldn't build the socket address correctly: %s\n",
                    apr_strerror(stat, buf, sizeof buf));
            exit(-1);
        }
        family = localsa->sa.sin.sin_family;
    }

    fprintf(stdout, "\tServer:  Creating new socket.......");
    if (apr_create_socket(&sock, family, SOCK_STREAM, context) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't create socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Setting socket option NONBLOCK.......");
    if (apr_setsocketopt(sock, APR_SO_NONBLOCK, 1) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Couldn't set socket option\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Setting socket option REUSEADDR.......");
    if (apr_setsocketopt(sock, APR_SO_REUSEADDR, 1) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Couldn't set socket option\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    if (!localsa) {
        apr_set_port(sock, APR_LOCAL, 8021);
        apr_get_sockaddr(&localsa, APR_LOCAL, sock);
    }

    fprintf(stdout, "\tServer:  Binding socket to port.......");
    if ((stat = apr_bind(sock, localsa)) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Could not bind: %s\n",
                apr_strerror(stat, buf, sizeof buf));
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tServer:  Listening to socket.......");
    if (apr_listen(sock, 5) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Could not listen\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Setting up socket for polling.......");
    apr_setup_poll(&sdset, 1, context);
    apr_add_poll_socket(sdset, sock, APR_POLLIN);
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tServer:  Beginning to poll for socket.......");
    rv = 1; 
    if (apr_poll(sdset, &rv, -1) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Select caused an error\n");
        exit(-1);
    }
    else if (rv == 0) {
        apr_close_socket(sock);
        fprintf(stderr, "I should not return until rv == 1\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Accepting a connection.......");
    if (apr_accept(&sock2, sock, context) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Could not accept connection.\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    apr_get_ipaddr(&remote_ipaddr, APR_REMOTE, sock2);
    apr_get_port(&remote_port, APR_REMOTE, sock2);
    apr_get_ipaddr(&local_ipaddr, APR_LOCAL, sock2);
    apr_get_port(&local_port, APR_LOCAL, sock2);
    fprintf(stdout, "\tServer socket: %s:%u -> %s:%u\n", local_ipaddr, local_port, remote_ipaddr, remote_port);

    length = STRLEN;
    fprintf(stdout, "\tServer:  Trying to recv data from socket.......");
    if (apr_recv(sock2, datasend, &length) != APR_SUCCESS) {
        apr_close_socket(sock);
        apr_close_socket(sock2);
        fprintf(stderr, "Problem recving data\n");
        exit(-1);
    }
    if (strcmp(datasend, "Send data test")) {
        apr_close_socket(sock);
        apr_close_socket(sock2);
        fprintf(stderr, "I did not receive the correct data %s\n", datarecv);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    length = STRLEN;
    fprintf(stdout, "\tServer:  Sending data over socket.......");
    if (apr_send(sock2, datarecv, &length) != APR_SUCCESS) {
        apr_close_socket(sock);
        apr_close_socket(sock2);
        fprintf(stderr, "Problem sending data\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tServer:  Shutting down accepted socket.......");
    if (apr_shutdown(sock2, APR_SHUTDOWN_READ) != APR_SUCCESS) {
        apr_close_socket(sock);
        apr_close_socket(sock2);
        fprintf(stderr, "Problem shutting down\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  closing duplicate socket.......");
    if (apr_close_socket(sock2) != APR_SUCCESS) {
        apr_close_socket(sock);
        fprintf(stderr, "Problem closing down\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tServer:  closing original socket.......");
    if (apr_close_socket(sock) != APR_SUCCESS) {
        fprintf(stderr, "Problem closing down\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    return 1;
}

