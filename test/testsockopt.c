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
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

static void failure(apr_socket_t *sock)
{
    apr_socket_close(sock);
    printf("Failed!\n");
    exit(-1);
}

static void failureno(apr_socket_t *sock)
{
    apr_socket_close(sock);
    printf("No!\n");
    exit(-1);
}

int main(void)
{
    apr_pool_t *context;
    apr_pool_t *cont2;
    apr_socket_t *sock = NULL;
    apr_status_t stat = 0;
    apr_int32_t ck;

    if (apr_initialize() != APR_SUCCESS) {
        fprintf(stderr, "Couldn't initialize.");
        exit(-1);
    }
    atexit(apr_terminate);
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate context.");
        exit(-1);
    }
    if (apr_pool_create(&cont2, context) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate context.");
        exit(-1);
    }

    printf("Testing socket option functions.\n");

    printf("\tCreating socket..........................");
    if ((stat = apr_socket_create(&sock, APR_INET, SOCK_STREAM, context))
         != APR_SUCCESS){
        printf("Failed to create a socket!\n");
        exit(-1);
    }
    printf("OK\n");

    printf ("\tTrying to set APR_SO_KEEPALIVE...........");
    if (apr_socket_opt_set(sock, APR_SO_KEEPALIVE, 1) != APR_SUCCESS){
        apr_socket_close(sock);
        printf("Failed!\n");
        exit (-1);
    }
    printf ("OK\n");

    printf("\tChecking if we recorded it...............");
    if (apr_socket_opt_get(sock, APR_SO_KEEPALIVE, &ck) != APR_SUCCESS){
        apr_socket_close(sock);
        fprintf(stderr,"Failed\n");
        exit(-1);
    }
    if (ck != 1){ 
        apr_socket_close(sock);
        printf("No (%d)\n", ck);
        exit(-1);
    }
    printf("Yes\n");

    printf("\tTrying to set APR_SO_DEBUG...............");
    if (apr_socket_opt_set(sock, APR_SO_DEBUG, 1) != APR_SUCCESS){
        printf("Failed (ignored)\n");
    }
    else {
        printf ("OK\n");

        printf("\tChecking if we recorded it...............");
        if (apr_socket_opt_get(sock, APR_SO_DEBUG, &ck) != APR_SUCCESS){
            apr_socket_close(sock);
            printf("Failed!\n");
            exit (-1);
        }
        if (ck != 1){
            printf ("No (%d)\n", ck);
            apr_socket_close(sock);
            exit (-1);
        }
        printf ("Yes\n");
    }

    printf ("\tTrying to remove APR_SO_KEEPALIVE........");
    if (apr_socket_opt_set(sock, APR_SO_KEEPALIVE, 0) != APR_SUCCESS){
        apr_socket_close(sock);
        printf("Failed!\n");
        exit (-1);
    }
    printf ("OK\n");

    printf ("\tDid we record the removal................");
    if (apr_socket_opt_get(sock, APR_SO_KEEPALIVE, &ck) != APR_SUCCESS){
        apr_socket_close(sock);
        printf("Didn't get value!\n");
        exit(-1);
    }
    if (ck != 0){
        failureno(sock);
    }
    printf ("Yes\n");

#if APR_HAVE_CORKABLE_TCP
    printf ("\tTesting APR_TCP_NOPUSH!\n");
    printf("\t\tSetting APR_TCP_NODELAY..........");
    if (apr_socket_opt_set(sock, APR_TCP_NODELAY, 1) != APR_SUCCESS){
        failure(sock);
    }
    printf("OK\n");
    printf("\t\tSetting APR_TCP_NOPUSH...........");
    if (apr_socket_opt_set(sock, APR_TCP_NOPUSH, 1) != APR_SUCCESS){
        failure(sock);
    }
    printf("OK\n");
    printf("\t\tChecking on APR_TCP_NODELAY......");
    if (apr_socket_opt_get(sock, APR_TCP_NODELAY, &ck) != APR_SUCCESS){
        failure(sock);
    }
    if (ck != 0){
        failureno(sock);
    }
    printf("Yes (not set)\n");
    printf("\t\tUnsetting APR_TCP_NOPUSH.........");
    if (apr_socket_opt_set(sock, APR_TCP_NOPUSH, 0) != APR_SUCCESS){
        failure(sock);
    }
    printf("OK\n");
    
    printf("\t\tChecking on APR_TCP_NODELAY......");
    if (apr_socket_opt_get(sock, APR_TCP_NODELAY, &ck) != APR_SUCCESS){
        failure(sock);
    }
    if (ck != 1){
        failureno(sock);
    }
    printf("Yes (set)\n");

    printf ("\tSeems OK!\n");
#endif

    printf("\tTrying to close the socket...............");
    if ((stat = apr_socket_close(sock)) != APR_SUCCESS){
        printf("Failed to close the socket!\n");
        exit(-1);
    }
    printf("OK\n");

    return 0;
}
