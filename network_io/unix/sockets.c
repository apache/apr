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

#include "networkio.h"
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_portable.h"
#include "apr_lib.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static ap_status_t socket_cleanup(void *sock)
{
    struct socket_t *thesocket = sock;
    if (close(thesocket->socketdes) == 0) {
        thesocket->socketdes = -1;
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_create_tcp_socket(ap_context_t *, ap_socket_t **)
 *    Create a socket for tcp communication.
 * arg 1) The context to use
 * arg 2) The new socket that has been setup. 
 */
ap_status_t ap_create_tcp_socket(ap_context_t *cont, struct socket_t **new)
{
    (*new) = (struct socket_t *)ap_palloc(cont, sizeof(struct socket_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont; 
    (*new)->addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt,
                       sizeof(struct sockaddr_in));

    if ((*new)->addr == NULL) {
        return APR_ENOMEM;
    }
 
    (*new)->socketdes = socket(AF_INET ,SOCK_STREAM, IPPROTO_TCP);
    (*new)->remote_hostname = NULL;

    (*new)->addr->sin_family = AF_INET;

    (*new)->addr_len = sizeof(*(*new)->addr);
    
    if ((*new)->socketdes < 0) {
        return errno;
    }
    (*new)->timeout = -1;
    ap_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, NULL);
    return APR_SUCCESS;
} 

/* ***APRDOC********************************************************
 * ap_status_t ap_shutdown(ap_socket_t *, ap_shutdown_how_e)
 *    Shutdown either reading, writing, or both sides of a tcp socket.
 * arg 1) The socket to close 
 * arg 2) How to shutdown the socket.  One of:
 *            APR_SHUTDOWN_READ      -- no longer allow read requests
 *            APR_SHUTDOWN_WRITE     -- no longer allow write requests
 *            APR_SHUTDOWN_READWRITE -- no longer allow read or write requests 
 * NOTE:  This does not actually close the socket descriptor, it just
 *        controls which calls are still valid on the socket.
 */
ap_status_t ap_shutdown(struct socket_t *thesocket, ap_shutdown_how_e how)
{
    if (shutdown(thesocket->socketdes, how) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_close_socket(ap_socket_t *)
 *    Close a tcp socket.
 * arg 1) The socket to close 
 */
ap_status_t ap_close_socket(struct socket_t *thesocket)
{
    ap_kill_cleanup(thesocket->cntxt, thesocket, socket_cleanup);
    return socket_cleanup(thesocket);
}

/* ***APRDOC********************************************************
 * ap_status_t ap_setport(ap_socket_t *, ap_uint32_t)
 *    Assocaite a port with a socket.
 * arg 1) The socket use 
 * arg 2) The port this socket will be dealing with.
 * NOTE:  This does not bind the two together, it is just telling apr 
 *        that this socket is going to use this port if possible.  If
 *        the port is already used, we won't find out about it here.
 */
ap_status_t ap_setport(struct socket_t *sock, ap_uint32_t port)
{
    sock->addr->sin_port = htons((short)port);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_getport(ap_socket_t *, ap_uint32_t *)
 *    Return the port with a socket.
 * arg 1) The socket use 
 * arg 2) The port this socket will be dealing with.
 */
ap_status_t ap_getport(struct socket_t *sock, ap_uint32_t *port)
{
    *port = ntohs(sock->addr->sin_port);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_setipaddr(ap_socket_t *, cont char *addr)
 *    Assocaite a socket addr with an apr socket.
 * arg 1) The socket to use 
 * arg 2) The IP address to attach to the socket.
 *        Use APR_ANYADDR to use any IP addr on the machine.
 * NOTE:  This does not bind the two together, it is just telling apr 
 *        that this socket is going to use this address if possible. 
 */
ap_status_t ap_setipaddr(struct socket_t *sock, const char *addr)
{
    if (!strcmp(addr, APR_ANYADDR)) {
        sock->addr->sin_addr.s_addr = htonl(INADDR_ANY);
        return APR_SUCCESS;
    }
    if (inet_aton(addr, &sock->addr->sin_addr) == 0) {
        return errno;
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_getipaddr(char *addr, int len, const ap_socket_t *)
 *    Return the IP address associated with an apr socket.
 * arg 1) A buffer for the IP address associated with the socket.
 * arg 2) The total length of the buffer (including terminating NUL)
 * arg 3) The socket to use 
 */
ap_status_t ap_getipaddr(char *addr, ap_ssize_t len,
			 const struct socket_t *sock)
{
    char *temp = inet_ntoa(sock->addr->sin_addr);
    ap_cpystrn(addr,temp,len-1);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_bind(ap_socket_t *)
 *    Bind the socket to it's assocaited port
 * arg 1) The socket to bind 
 * NOTE:  This is where we will find out if there is any other process
 *        using the selected port.
 */
ap_status_t ap_bind(struct socket_t *sock)
{
    sock->addr->sin_addr.s_addr = INADDR_ANY;
    if (bind(sock->socketdes, (struct sockaddr *)sock->addr, sock->addr_len) == -1)
        return errno;
    else
        return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_listen(ap_socket_t *, ap_int32_t)
 *    Listen to a bound socketi for connections. 
 * arg 1) The socket to listen on 
 * arg 2) The number of outstanding connections allowed in the sockets
 *        listen queue.  If this value is less than zero, the listen
 *        queue size is set to zero.  
 */
ap_status_t ap_listen(struct socket_t *sock, ap_int32_t backlog)
{
    if (listen(sock->socketdes, backlog) == -1)
        return errno;
    else
        return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_accept(ap_socket_t *, ap_socket_t **)
 *    Accept a new connection request
 * arg 1) The socket we are listening on 
 * arg 2) A copy of the socket that is connected to the socket that
 *        made the connection request.  This is the socket which should
 *        be used for all future communication.
 */
ap_status_t ap_accept(const struct socket_t *sock, struct socket_t **new)
{
    struct hostent *hptr;
    
    (*new) = (struct socket_t *)ap_palloc(sock->cntxt, 
                            sizeof(struct socket_t));

    (*new)->cntxt = sock->cntxt;
    (*new)->addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt, 
                 sizeof(struct sockaddr_in));
    (*new)->addr_len = sizeof(struct sockaddr_in);

    (*new)->socketdes = accept(sock->socketdes, (struct sockaddr *)(*new)->addr,
                        &(*new)->addr_len);

    if ((*new)->socketdes < 0) {
        return errno;
    }
    
    hptr = gethostbyaddr((char *)&(*new)->addr->sin_addr, 
                         sizeof(struct in_addr), AF_INET);
    if (hptr != NULL) {
        (*new)->remote_hostname = strdup(hptr->h_name);
    }
    
    ap_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, NULL);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_connect(ap_socket_t *, char *)
 *    Issue a connection request to a socket either on the same machine
 *    or a different one. 
 * arg 1) The socket we wish to use for our side of the connection 
 * arg 2) The hostname of the machine we wish to connect to.  If NULL,
 *        APR assumes that the sockaddr_in in the apr_socket is completely
 *        filled out.
 */
ap_status_t ap_connect(struct socket_t *sock, char *hostname)
{
    struct hostent *hp;

    if (hostname == NULL) {
        hp = gethostbyname(hostname);

        if ((sock->socketdes < 0) || (!sock->addr)) {
            return APR_ENOTSOCK;
        }
        if (!hp)  {
            if (h_errno == TRY_AGAIN) {
                return EAGAIN;
            }
            return h_errno;
        }
    
        memcpy((char *)&sock->addr->sin_addr, hp->h_addr_list[0], hp->h_length);

        sock->addr_len = sizeof(*sock->addr);
    }

    if ((connect(sock->socketdes, (const struct sockaddr *)sock->addr, sock->addr_len) < 0) &&
        (errno != EINPROGRESS)) {
        return errno;
    }
    else {
        sock->remote_hostname = strdup(hostname);
        return APR_SUCCESS;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_socketdata(ap_socket_t *, char *, void *)
 *    Return the context associated with the current socket.
 * arg 1) The currently open socket.
 * arg 2) The user data associated with the socket.
 */
ap_status_t ap_get_socketdata(struct socket_t *sock, char *key, void *data)
{
    if (sock != NULL) {
        return ap_get_userdata(sock->cntxt, key, &data);
    }
    else {
        data = NULL;
        return APR_ENOSOCKET;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_socketdata(ap_socket_t *, void *, char *,
                                 ap_status_t (*cleanup) (void *))
 *    Set the context associated with the current socket.
 * arg 1) The currently open socket.
 * arg 2) The user data to associate with the socket.
 */
ap_status_t ap_set_socketdata(struct socket_t *sock, void *data, char *key,
                              ap_status_t (*cleanup) (void *))
{
    if (sock != NULL) {
        return ap_set_userdata(sock->cntxt, data, key, cleanup);
    }
    else {
        data = NULL;
        return APR_ENOSOCKET;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_sock(ap_socket_t *, ap_os_sock_t *)
 *    Convert the socket from an apr type to an OS specific socket
 * arg 1) The socket to convert.
 * arg 2) The os specifc equivelant of the apr socket..
 */
ap_status_t ap_get_os_sock(struct socket_t *sock, ap_os_sock_t *thesock)
{
    if (sock == NULL) {
        return APR_ENOSOCKET;
    }
    *thesock = sock->socketdes;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_sock(ap_context_t *, ap_socket_t **, ap_os_socket_t *)
 *    Convert a socket from the os specific type to the apr type
 * arg 1) The context to use.
 * arg 2) The socket to convert to.
 * arg 3) The socket we are converting to an apr type.
 */
ap_status_t ap_put_os_sock(ap_context_t *cont, struct socket_t **sock,
                            ap_os_sock_t *thesock)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if ((*sock) == NULL) {
        (*sock) = (struct socket_t *)ap_palloc(cont, sizeof(struct socket_t));
        (*sock)->cntxt = cont;
    }
    (*sock)->socketdes = *thesock;
    return APR_SUCCESS;
}

