/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_network_io.h"
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void closeapr(void)
{
    apr_terminate();
}

static int make_socket(apr_socket_t **sock, apr_sockaddr_t **sa, apr_port_t port,
                apr_pool_t *p)
{
    if (apr_sockaddr_info_get(sa, "127.0.0.1", APR_UNSPEC, port, 0, p)
        != APR_SUCCESS){
        printf("couldn't create control socket information, shutting down");
        return 1;
    }
    if (apr_socket_create(sock, (*sa)->sa.sin.sin_family, SOCK_DGRAM, p)
        != APR_SUCCESS){
        printf("couldn't create UDP socket, shutting down");
        return 1;
    }
    if (apr_bind((*sock), (*sa)) != APR_SUCCESS){
        printf("couldn't bind UDP socket!");
        return 1;
    }
    return 0;
}

static int check_sockets(apr_pollfd_t *pollset, apr_socket_t **sockarray)
{
    int i = 0;
    printf("\tSocket 0\tSocket 1\tSocket 2\n\t");
    for (i = 0;i < 3;i++){
        apr_int16_t event;
        if (apr_poll_revents_get(&event, sockarray[i], pollset) != APR_SUCCESS){
            printf("Failed!\n");
            exit (-1);
        }
        if (event & APR_POLLIN){
            printf ("POLLIN!\t\t");
        } else {
            printf ("No wait\t\t");
        }
    }
    printf("\n");
    return 0;
}

static void send_msg(apr_socket_t **sockarray, apr_sockaddr_t **sas, int which)
{
    apr_size_t len = 5;
    apr_status_t rv;
    char errmsg[120];

    printf("\tSending message to socket %d............", which);
    if ((rv = apr_sendto(sockarray[which], sas[which], 0, "hello", &len)) != APR_SUCCESS){
        apr_strerror(rv, errmsg, sizeof errmsg);
        printf("Failed! %s\n", errmsg);
        exit(-1);
    }
    printf("OK\n");
}

static void recv_msg(apr_socket_t **sockarray, int which, apr_pool_t *p)
{
    apr_size_t buflen = 5;
    char *buffer = apr_pcalloc(p, sizeof(char) * buflen);
    apr_sockaddr_t *recsa;
    apr_status_t rv;
    char errmsg[120];

    apr_sockaddr_info_get(&recsa, "127.0.0.1", APR_UNSPEC, 7770, 0, p);

    printf("\tTrying to get message from socket %d....", which);
    if ((rv = apr_recvfrom(recsa, sockarray[which], 0, buffer, &buflen))
        != APR_SUCCESS){
        apr_strerror(rv, errmsg, sizeof errmsg);
        printf("Failed! %s\n", errmsg);
        exit (-1);
    }
    printf("OK\n");
}

int main(void)
{
    apr_pool_t *context;
    apr_socket_t *s[3];
    apr_sockaddr_t *sa[3];
    apr_pollfd_t *pollset;
    int i = 0, srv = 0;
    
    fprintf (stdout,"APR Poll Test\n*************\n\n");
    
    printf("Initializing...................................");
    if (apr_initialize() != APR_SUCCESS) {
        printf("Failed.\n");
        exit(-1);
    }
    printf("OK\n");
    atexit(closeapr);

    printf("Creating context...............................");    
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        printf("Failed.\n");
        exit(-1);
    }
    printf("OK\n");
    
    printf("\tCreating the sockets I'll use..........");
    for (i = 0; i < 3; i++){
        if (make_socket(&s[i], &sa[i], 7770 + i, context) != 0){
            exit(-1);
        }
    }
    printf("OK\n");
       
    printf ("\tSetting up the pollset I'll use........");
    if (apr_poll_setup(&pollset, 3, context) != APR_SUCCESS){
        printf("Couldn't create a pollset!\n");
        exit (-1);
    }
    for (i = 0; i < 3;i++){
        if (apr_poll_socket_add(pollset, s[i], APR_POLLIN) != APR_SUCCESS){
            printf("Failed to add socket %d\n", i);
            exit (-1);
        }
    }
    printf("OK\n");
    printf("Starting Tests\n");

    apr_poll(pollset, &srv, 10);
    check_sockets(pollset, s);
    
    send_msg(s, sa, 2);

    apr_poll(pollset, &srv, 10); 
    check_sockets(pollset, s);

    recv_msg(s, 2, context);
    send_msg(s, sa, 1);

    apr_poll(pollset, &srv, 10); 
    check_sockets(pollset, s);

    send_msg(s, sa, 2);

    apr_poll(pollset, &srv, 10); 
    check_sockets(pollset, s);
     
    recv_msg(s, 1, context);
    send_msg(s, sa, 0);
    
    apr_poll(pollset, &srv, 10); 
    check_sockets(pollset, s);
        
    printf("Tests completed.\n");
    printf("\tClosing sockets........................");
    for (i = 0; i < 3; i++){
        if (apr_socket_close(s[i]) != APR_SUCCESS){
            printf("Failed!\n");
            exit(-1);
        }
    }
    printf ("OK\n");
    return 1;
}
