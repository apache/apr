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

#include <errno.h>
#include <string.h>
#include <socket.h>
#include <netdb.h>
#include "networkio.h"
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_portable.h"

ap_status_t socket_cleanup(void *sock)
{
    struct socket_t *thesocket = sock;
    if (closesocket(thesocket->socketdes) == 0) {
        thesocket->socketdes = -1;
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

ap_status_t ap_create_tcp_socket(struct socket_t **new, ap_context_t *cont)
{
    (*new) = (struct socket_t *)ap_palloc(cont,sizeof(struct socket_t));
    
    if ((*new) == NULL){
        return APR_ENOMEM;
    }
    
    (*new)->cntxt = cont;
	(*new)->addr = (struct sockaddr_in *) ap_palloc((*new)->cntxt,
	                    sizeof (struct sockaddr_in));
    if ((*new)->addr == NULL){
        return APR_ENOMEM;
    }
    
    (*new)->socketdes = socket(AF_INET ,SOCK_STREAM, 0);
	(*new)->remote_hostname=NULL;
	(*new)->addr->sin_family = AF_INET; 
    (*new)->addr_len = sizeof(*(*new)->addr);
	memset(&(*new)->addr->sin_zero, 0, sizeof((*new)->addr->sin_zero));

    if ((*new)->socketdes < 0) {
        return errno;
    }

    (*new)->timeout = -1;
    ap_register_cleanup((*new)->cntxt, (void *)(*new),
                            socket_cleanup, NULL);
    return APR_SUCCESS;
} 

ap_status_t ap_shutdown(struct socket_t *thesocket, ap_shutdown_how_e how)
{
    if (shutdown(thesocket->socketdes, how) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

ap_status_t ap_close_socket(struct socket_t *thesocket)
{
    ap_kill_cleanup(thesocket->cntxt,thesocket,socket_cleanup);
    return socket_cleanup(thesocket);
}

ap_status_t ap_setport(struct socket_t *sock, ap_uint32_t port) 
{ 
    sock->addr->sin_port = htons((short)port); 
    return APR_SUCCESS; 
} 

ap_status_t ap_getport(struct socket_t *sock, ap_uint32_t *port)
{
    *port = ntohs(sock->addr->sin_port);
    return APR_SUCCESS;
}

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

ap_status_t ap_getipaddr(char *addr, ap_ssize_t len,
			 const struct socket_t *sock)
{
    char *temp = inet_ntoa(sock->addr->sin_addr);
    ap_cpystrn(addr,temp,len-1);
    return APR_SUCCESS;
}

ap_status_t ap_bind(struct socket_t *sock) 
{ 
    sock->addr->sin_addr.s_addr = INADDR_ANY;
    if (bind(sock->socketdes, (struct sockaddr *)sock->addr, sock->addr_len) == -1) 
        return errno; 
    else 
        return APR_SUCCESS; 
} 
 
ap_status_t ap_listen(struct socket_t *sock, ap_int32_t backlog) 
{ 
    if (listen(sock->socketdes, backlog) == -1) 
        return errno; 
    else 
        return APR_SUCCESS; 
} 

ap_status_t ap_accept(struct socket_t **new, const struct socket_t *sock) 
{ 
	struct hostent *hptr;
	
	(*new) = (struct socket_t *)ap_palloc(sock->cntxt,
	                        sizeof(ap_socket_t)); 

    (*new)->cntxt = sock->cntxt;
    (*new)->addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt, 
                 sizeof(struct sockaddr_in));
    (*new)->addr_len = sizeof(struct sockaddr_in);

    (*new)->socketdes = accept(sock->socketdes, (struct sockaddr *)(*new)->addr,
                        &(*new)->addr_len);

	if ((*new)->socketdes <0){
		return errno;
	}

	hptr = gethostbyaddr((char*)&(*new)->addr->sin_addr, 
	                    sizeof(struct in_addr), AF_INET);
	if (hptr != NULL){
		(*new)->remote_hostname = strdup(hptr->h_name);
	}
	    
    ap_register_cleanup((*new)->cntxt, (void *)new, 
                        socket_cleanup, NULL);
    return APR_SUCCESS;
} 
 
ap_status_t ap_connect(struct socket_t *sock, char *hostname) 
{ 
    struct hostent *hp; 

    hp = gethostbyname(hostname); 
    if ((sock->socketdes < 0) || (!sock->addr)) { 
        return APR_ENOTSOCK; 
    } 

	memcpy((char *)&sock->addr->sin_addr, hp->h_addr , hp->h_length);

    sock->addr->sin_family = AF_INET;
     
    memset(sock->addr->sin_zero, 0, sizeof(sock->addr->sin_zero));
    
    sock->addr_len = sizeof(sock->addr);
    
    while ((connect(sock->socketdes, (const struct sockaddr *)sock->addr, sock->addr_len) < 0)){
    	if (errno != EALREADY && errno != EINPROGRESS)
            return errno; 
    }
     
    sock->remote_hostname = strdup(hostname);
    return APR_SUCCESS; 
} 

ap_status_t ap_get_socketdata(struct socket_t *sock, char *key, void *data)
{
    if (socket != NULL) {
        return ap_get_userdata(&data, key, sock->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOSOCKET;
    }
}

ap_status_t ap_set_socketdata(struct socket_t *sock, void *data, char *key,
                              ap_status_t (*cleanup) (void *))
{
    if (sock != NULL) {
        return ap_set_userdata(data, key, cleanup, sock->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOSOCKET;
    }
}

ap_status_t ap_get_os_sock(struct socket_t *sock, ap_os_sock_t *thesock)
{
    if (sock == NULL) {
        return APR_ENOSOCKET;
    }
    *thesock = sock->socketdes;
    return APR_SUCCESS;
}

ap_status_t ap_put_os_sock(struct socket_t **sock, ap_os_sock_t *thesock, 
                           ap_context_t *cont)
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
