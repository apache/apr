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

#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"

#define STRLEN 15

int main(int argc, char *argv[])
{
    ap_context_t *context;
    ap_socket_t *sock;
    ap_socket_t *sock2;
    ap_ssize_t length;
    ap_int32_t rv;
    ap_pollfd_t *sdset;
    char datasend[STRLEN];
    char datarecv[STRLEN] = "Recv data test";
    char *local_ipaddr, *remote_ipaddr;
    ap_uint32_t local_port, remote_port;

    fprintf(stdout, "Creating context.......");
    if (ap_create_context(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Could not create a context\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Creating new socket.......");
    if (ap_create_tcp_socket(&sock, context) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't create socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Setting socket option NONBLOCK.......");
    if (ap_setsocketopt(sock, APR_SO_NONBLOCK, 1) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Couldn't set socket option\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Setting port for socket.......");
    if (ap_set_local_port(sock, 8021) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Couldn't set the port correctly\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Binding socket to port.......");
    if (ap_bind(sock) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Could not bind\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tServer:  Listening to socket.......");
    if (ap_listen(sock, 8021) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Could not listen\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Setting up socket for polling.......");
    ap_setup_poll(&sdset, 1, context);
    ap_add_poll_socket(sdset, sock, APR_POLLIN);
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tServer:  Beginning to poll for socket.......");
    rv = 1; 
    if (ap_poll(sdset, &rv, -1) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Select caused an error\n");
        exit(-1);
    }
    else if (rv == 0) {
        ap_close_socket(sock);
        fprintf(stderr, "I should not return until rv == 1\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  Accepting a connection.......");
    if (ap_accept(&sock2, sock, context) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Could not accept connection.\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    ap_get_remote_ipaddr(&remote_ipaddr, sock2);
    ap_get_remote_port(&remote_port, sock2);
    ap_get_local_ipaddr(&local_ipaddr, sock2);
    ap_get_local_port(&local_port, sock2);
    fprintf(stdout, "\tServer socket: %s:%u -> %s:%u\n", local_ipaddr, local_port, remote_ipaddr, remote_port);

    length = STRLEN;
    fprintf(stdout, "\tServer:  Trying to recv data from socket.......");
    if (ap_recv(sock2, datasend, &length) != APR_SUCCESS) {
        ap_close_socket(sock);
        ap_close_socket(sock2);
        fprintf(stderr, "Problem recving data\n");
        exit(-1);
    }
    if (strcmp(datasend, "Send data test")) {
        ap_close_socket(sock);
        ap_close_socket(sock2);
        fprintf(stderr, "I did not receive the correct data %s\n", datarecv);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    length = STRLEN;
    fprintf(stdout, "\tServer:  Sending data over socket.......");
    if (ap_send(sock2, datarecv, &length) != APR_SUCCESS) {
        ap_close_socket(sock);
        ap_close_socket(sock2);
        fprintf(stderr, "Problem sending data\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tServer:  Shutting down accepte socket.......");
    if (ap_shutdown(sock2, APR_SHUTDOWN_READ) != APR_SUCCESS) {
        ap_close_socket(sock);
        ap_close_socket(sock2);
        fprintf(stderr, "Problem shutting down\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tServer:  closing duplicate socket.......");
    if (ap_close_socket(sock2) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Problem closing down\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tServer:  closing original socket.......");
    if (ap_close_socket(sock) != APR_SUCCESS) {
        fprintf(stderr, "Problem closing down\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    return 1;
}

