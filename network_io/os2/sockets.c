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
#include "os2calls.h"

ap_status_t socket_cleanup(void *sock)
{
    struct socket_t *thesocket = sock;
    if (soclose(thesocket->socketdes) == 0) {
        thesocket->socketdes = -1;
        return APR_SUCCESS;
    }
    else {
        return os2errno(sock_errno());
    }
}

ap_status_t ap_create_tcp_socket(struct socket_t **new, ap_context_t *cont)
{
    (*new) = (struct socket_t *)ap_palloc(cont, sizeof(struct socket_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont; 
    (*new)->local_addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt,
                         sizeof(struct sockaddr_in));
    (*new)->remote_addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt,
                         sizeof(struct sockaddr_in));

    if ((*new)->local_addr == NULL || (*new)->remote_addr == NULL) {
        return APR_ENOMEM;
    }
 
    (*new)->socketdes = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    (*new)->local_addr->sin_family = AF_INET;
    (*new)->remote_addr->sin_family = AF_INET;

    (*new)->addr_len = sizeof(*(*new)->local_addr);
    
    if ((*new)->socketdes < 0) {
        return os2errno(sock_errno());
    }
    (*new)->timeout = -1;
    (*new)->nonblock = FALSE;
    ap_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
} 

ap_status_t ap_shutdown(struct socket_t *thesocket, ap_shutdown_how_e how)
{
    if (shutdown(thesocket->socketdes, how) == 0) {
        return APR_SUCCESS;
    }
    else {
        return os2errno(sock_errno());
    }
}

ap_status_t ap_close_socket(struct socket_t *thesocket)
{
    ap_kill_cleanup(thesocket->cntxt, thesocket, socket_cleanup);
    return socket_cleanup(thesocket);
}




ap_status_t ap_bind(struct socket_t *sock)
{
    if (bind(sock->socketdes, (struct sockaddr *)sock->local_addr, sock->addr_len) == -1)
        return os2errno(sock_errno());
    else
        return APR_SUCCESS;
}

ap_status_t ap_listen(struct socket_t *sock, ap_int32_t backlog)
{
    if (listen(sock->socketdes, backlog) == -1)
        return os2errno(sock_errno());
    else
        return APR_SUCCESS;
}

ap_status_t ap_accept(struct socket_t **new, const struct socket_t *sock, struct context_t *connection_context)
{
    (*new) = (struct socket_t *)ap_palloc(connection_context, 
                            sizeof(struct socket_t));

    (*new)->cntxt = connection_context;
    (*new)->remote_addr = (struct sockaddr_in *)ap_palloc((*new)->cntxt,
                          sizeof(struct sockaddr_in));
    (*new)->local_addr = sock->local_addr;
    (*new)->addr_len = sizeof(struct sockaddr_in);

    (*new)->socketdes = accept(sock->socketdes, (struct sockaddr *)(*new)->remote_addr,
                        &(*new)->addr_len);

    if ((*new)->socketdes < 0) {
        return os2errno(sock_errno());
    }

    ap_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_connect(struct socket_t *sock, char *hostname)
{
    struct hostent *hp;

    if (hostname != NULL) {
        hp = gethostbyname(hostname);

        if ((sock->socketdes < 0) || (!sock->remote_addr)) {
            return APR_ENOTSOCK;
        }
        if (!hp)  {
            if (h_errno == TRY_AGAIN) {
                return EAGAIN;
            }
            return h_errno;
        }
    
        memcpy((char *)&sock->remote_addr->sin_addr, hp->h_addr_list[0], hp->h_length);
        sock->addr_len = sizeof(*sock->remote_addr);
    }

    if ((connect(sock->socketdes, (struct sockaddr *)sock->remote_addr, sock->addr_len) < 0) &&
        (sock_errno() != SOCEINPROGRESS)) {
        return os2errno(sock_errno());
    }
    else {
        int namelen = sizeof(*sock->local_addr);
        getsockname(sock->socketdes, (struct sockaddr *)sock->local_addr, &namelen);
        return APR_SUCCESS;
    }
}



ap_status_t ap_get_socketdata(void **data, char *key, struct socket_t *socket)
{
    if (socket != NULL) {
        return ap_get_userdata(data, key, socket->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOSOCKET;
    }
}



ap_status_t ap_set_socketdata(struct socket_t *socket, void *data, char *key,
                              ap_status_t (*cleanup) (void *))
{
    if (socket != NULL) {
        return ap_set_userdata(data, key, cleanup, socket->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOSOCKET;
    }
}

ap_status_t ap_get_os_sock(ap_os_sock_t *thesock, struct socket_t *sock)
{
    if (sock == NULL) {
        return APR_ENOSOCKET;
    }
    *thesock = sock->socketdes;
    return APR_SUCCESS;
}



ap_status_t ap_put_os_sock(struct socket_t **sock, ap_os_sock_t *thesock, ap_context_t *cont)
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

