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

/* ***APRDOC********************************************************
 * ap_status_t ap_set_local_port(ap_socket_t *socket, ap_uint32_t port)
 *    Assocaite a local port with a socket.
 * arg 1) The socket to set
 * arg 2) The local port this socket will be dealing with.
 * NOTE:  This does not bind the two together, it is just telling apr 
 *        that this socket is going to use this port if possible.  If
 *        the port is already used, we won't find out about it here.
 */
ap_status_t ap_set_local_port(struct socket_t *sock, ap_uint32_t port)
{
    sock->local_addr->sin_port = htons((short)port);
    return APR_SUCCESS;
}



/* ***APRDOC********************************************************
 * ap_status_t ap_set_remote_port(ap_socket_t *socket, ap_uint32_t port)
 *    Assocaite a remote port with a socket.
 * arg 1) The socket to enquire about.
 * arg 2) The local port this socket will be dealing with.
 * NOTE:  This does not make a connection to the remote port, it is just 
 *        telling apr which port ap_connect() should attempt to connect to.
 */
ap_status_t ap_set_remote_port(struct socket_t *sock, ap_uint32_t port)
{
    sock->remote_addr->sin_port = htons((short)port);
    return APR_SUCCESS;
}



/* ***APRDOC********************************************************
 * ap_status_t ap_get_local_port(ap_uint32_t *port, ap_socket_t *socket)
 *    Return the local port with a socket.
 * arg 1) The local port this socket is associated with.
 * arg 2) The socket to enquire about.
 */
ap_status_t ap_get_local_port(ap_uint32_t *port, struct socket_t *sock)
{
    *port = ntohs(sock->local_addr->sin_port);
    return APR_SUCCESS;
}



/* ***APRDOC********************************************************
 * ap_status_t ap_get_remote_port(ap_uint32_t *port, ap_socket_t *socket)
 *    Return the remote port associated with a socket.
 * arg 1) The remote port this socket is associated with.
 * arg 2) The socket to enquire about.
 */
ap_status_t ap_get_remote_port(ap_uint32_t *port, struct socket_t *sock)
{
    *port = ntohs(sock->remote_addr->sin_port);
    return APR_SUCCESS;
}



/* ***APRDOC********************************************************
 * ap_status_t ap_set_local_ipaddr(ap_socket_t *socket, cont char *addr)
 *    Assocaite a local socket addr with an apr socket.
 * arg 1) The socket to use 
 * arg 2) The IP address to attach to the socket.
 *        Use APR_ANYADDR to use any IP addr on the machine.
 * NOTE:  This does not bind the two together, it is just telling apr 
 *        that this socket is going to use this address if possible. 
 */
ap_status_t ap_set_local_ipaddr(struct socket_t *sock, const char *addr)
{
    u_long ipaddr;
    
    if (!strcmp(addr, APR_ANYADDR)) {
        sock->local_addr->sin_addr.s_addr = htonl(INADDR_ANY);
        return APR_SUCCESS;
    }
    
    ipaddr = inet_addr(addr);
    
    if (ipaddr == -1) {
        return errno;
    }
    
    sock->local_addr->sin_addr.s_addr = ipaddr;
    return APR_SUCCESS;
}



/* ***APRDOC********************************************************
 * ap_status_t ap_set_remote_ipaddr(ap_socket_t *socket, cont char *addr)
 *    Assocaite a remote socket addr with an apr socket.
 * arg 1) The socket to use 
 * arg 2) The IP address to attach to the socket.
 * NOTE:  This does not make a connection to the remote address, it is just
 *        telling apr which address ap_connect() should attempt to connect to.
 */
ap_status_t ap_set_remote_ipaddr(struct socket_t *sock, const char *addr)
{
    u_long ipaddr;
    
    if (!strcmp(addr, APR_ANYADDR)) {
        sock->remote_addr->sin_addr.s_addr = htonl(INADDR_ANY);
        return APR_SUCCESS;
    }
    
    ipaddr = inet_addr(addr);
    
    if (ipaddr == (u_long)-1) {
        return errno;
    }
    
    sock->remote_addr->sin_addr.s_addr = ipaddr;
    return APR_SUCCESS;
}



/* ***APRDOC********************************************************
 * ap_status_t ap_get_local_ipaddr(char **addr, const ap_socket_t *socket)
 *    Return the local IP address associated with an apr socket.
 * arg 1) The local IP address associated with the socket.
 * arg 2) The socket to use 
 */
ap_status_t ap_get_local_ipaddr(char **addr, const struct socket_t *sock)
{
    *addr = ap_pstrdup(sock->cntxt, inet_ntoa(sock->local_addr->sin_addr));
    return APR_SUCCESS;
}



/* ***APRDOC********************************************************
 * ap_status_t ap_get_remote_ipaddr(char **addr, const ap_socket_t *socket)
 *    Return the remote IP address associated with an apr socket.
 * arg 1) The remote IP address associated with the socket.
 * arg 2) The socket to use 
 */
ap_status_t ap_get_remote_ipaddr(char **addr, const struct socket_t *sock)
{
    *addr = ap_pstrdup(sock->cntxt, inet_ntoa(sock->remote_addr->sin_addr));
    return APR_SUCCESS;
}



#if APR_HAVE_NETINET_IN_H
/* ***APRDOC********************************************************
 * ap_status_t ap_get_local_name(struct sockaddr_in **name, const ap_socket_t *socket)
 *    Return the local socket name as a BSD style struct sockaddr_in.
 * arg 1) The local name associated with the socket.
 * arg 2) The socket to use 
 */
ap_status_t ap_get_local_name(struct sockaddr_in **name, const struct socket_t *sock)
{
    *name = sock->local_addr;
    return APR_SUCCESS;
}



/* ***APRDOC********************************************************
 * ap_status_t ap_get_remote_name(struct sockaddr_in **name, const ap_socket_t *socket)
 *    Return the remote socket name as a BSD style struct sockaddr_in.
 * arg 1) The remote name associated with the socket.
 * arg 2) The socket to use 
 */
ap_status_t ap_get_remote_name(struct sockaddr_in **name, const struct socket_t *sock)
{
    *name = sock->remote_addr;
    return APR_SUCCESS;
}
#endif
