/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "errno.h"

#define STRLEN 15

int main(int argc, char *argv[])
{
    ap_context_t *context;
    ap_socket_t *sock;
    ap_ssize_t length;
    ap_status_t stat;
    char datasend[STRLEN] = "Send data test";
    char datarecv[STRLEN];

    fprintf(stdout, "Creating context.......");
    if (ap_create_context(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Something went wrong\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Creating new socket.......");
    if (ap_create_tcp_socket(&sock, context) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't create socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Setting socket option NONBLOCK.......");
    if (ap_setsocketopt(sock, APR_SO_NONBLOCK, 1) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Couldn't set socket option\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Setting port for socket.......");
    if (ap_setport(sock, 8021) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Couldn't set the port correctly\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");           

    fprintf(stdout, "\tClient:  Connecting to socket.......");
do {
    stat = ap_connect(sock, "127.0.0.1");
} while (stat == APR_ECONNREFUSED);
    if (stat != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Could not connect  %d\n", stat);
        fflush(stderr);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Trying to send data over socket.......");
    length = STRLEN;
    if (ap_send(sock, datasend, &length) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Problem sending data\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
   
    length = STRLEN; 
    fprintf(stdout, "\tClient:  Trying to receive data over socket.......");
    if (ap_recv(sock, datarecv, &length) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Problem receiving data\n");
        exit(-1);
    }
    if (strcmp(datarecv, "Recv data test")) {
        ap_close_socket(sock);
        fprintf(stderr, "I did not receive the correct data %s\n", datarecv);
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Shutting down socket.......");
    if (ap_shutdown(sock, APR_SHUTDOWN_WRITE) != APR_SUCCESS) {
        ap_close_socket(sock);
        fprintf(stderr, "Could not shutdown socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tClient:  Closing down socket.......");
    if (ap_close_socket(sock) != APR_SUCCESS) {
        fprintf(stderr, "Could not shutdown socket\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    return 1;
}
