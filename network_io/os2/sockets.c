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
#include "apr_lib.h"
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netdb.h>

ap_status_t socket_cleanup(void *sock)
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

ap_status_t ap_create_tcp_socket(struct socket_t **new, ap_context_t *cont)
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

    (*new)->addr->sin_family = AF_INET;

    (*new)->addr_len = sizeof(*(*new)->addr);
    
    if ((*new)->socketdes < 0) {
        return errno;
    }
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
    ap_kill_cleanup(thesocket->cntxt, thesocket, socket_cleanup);
    return socket_cleanup(thesocket);
}

ap_status_t ap_setport(struct socket_t *sock, ap_uint32_t port)
{
    sock->addr->sin_port = htons((short)port);
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
    
    ap_register_cleanup((*new)->cntxt, (void *)(*new), 
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
    if (!hp)  {
        if (h_errno == TRY_AGAIN) {
            return EAGAIN;
        }
        return h_errno;
    }
    
    memcpy((char *)&sock->addr->sin_addr, hp->h_addr_list[0], hp->h_length);

    sock->addr->sin_family = AF_INET;
   
    sock->addr_len = sizeof(*sock->addr);

    if ((connect(sock->socketdes, (const struct sockaddr *)sock->addr, sock->addr_len) < 0) &&
        (errno != EINPROGRESS)) {
        return errno;
    }
    else {
        return APR_SUCCESS;
    }
}

