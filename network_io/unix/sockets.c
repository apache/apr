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

#include "networkio.h"
#include "apr_portable.h"

static apr_status_t socket_cleanup(void *sock)
{
    apr_socket_t *thesocket = sock;
    if (close(thesocket->socketdes) == 0) {
        thesocket->socketdes = -1;
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

apr_status_t apr_create_tcp_socket(apr_socket_t **new, apr_pool_t *cont)
{
    (*new) = (apr_socket_t *)apr_pcalloc(cont, sizeof(apr_socket_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont; 
    (*new)->local_addr = (struct sockaddr_in *)apr_pcalloc((*new)->cntxt,
                         sizeof(struct sockaddr_in));
    (*new)->remote_addr = (struct sockaddr_in *)apr_pcalloc((*new)->cntxt,
                          sizeof(struct sockaddr_in));

    if ((*new)->local_addr == NULL || (*new)->remote_addr == NULL) {
        return APR_ENOMEM;
    }
 
    (*new)->socketdes = socket(AF_INET ,SOCK_STREAM, IPPROTO_TCP);

    (*new)->local_addr->sin_family = AF_INET;
    (*new)->remote_addr->sin_family = AF_INET;

    (*new)->addr_len = sizeof(*(*new)->local_addr);
    
    if ((*new)->socketdes < 0) {
        return errno;
    }
    (*new)->timeout = -1;
    apr_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, apr_null_cleanup);
    return APR_SUCCESS;
} 

apr_status_t apr_shutdown(apr_socket_t *thesocket, ap_shutdown_how_e how)
{
    return (shutdown(thesocket->socketdes, how) == -1) ? errno : APR_SUCCESS;
}

apr_status_t apr_close_socket(apr_socket_t *thesocket)
{
    apr_kill_cleanup(thesocket->cntxt, thesocket, socket_cleanup);
    return socket_cleanup(thesocket);
}

apr_status_t apr_bind(apr_socket_t *sock)
{
    if (bind(sock->socketdes, (struct sockaddr *)sock->local_addr, sock->addr_len) == -1)
        return errno;
    else {
        if (sock->local_addr->sin_port == 0) { /* no need for ntohs() when comparing w/ 0 */
            sock->local_port_unknown = 1; /* kernel got us an ephemeral port */
        }
        return APR_SUCCESS;
    }
}

apr_status_t apr_listen(apr_socket_t *sock, apr_int32_t backlog)
{
    if (listen(sock->socketdes, backlog) == -1)
        return errno;
    else
        return APR_SUCCESS;
}

apr_status_t apr_accept(apr_socket_t **new, apr_socket_t *sock, apr_pool_t *connection_context)
{
    (*new) = (apr_socket_t *)apr_pcalloc(connection_context, 
                            sizeof(apr_socket_t));

    (*new)->cntxt = connection_context;
    (*new)->local_addr = (struct sockaddr_in *)apr_pcalloc((*new)->cntxt, 
                 sizeof(struct sockaddr_in));

    (*new)->remote_addr = (struct sockaddr_in *)apr_pcalloc((*new)->cntxt, 
                 sizeof(struct sockaddr_in));
    (*new)->addr_len = sizeof(struct sockaddr_in);
#ifndef HAVE_POLL
    (*new)->connected = 1;
#endif
    (*new)->timeout = -1;
    
    (*new)->socketdes = accept(sock->socketdes, (struct sockaddr *)(*new)->remote_addr,
                        &(*new)->addr_len);

    if ((*new)->socketdes < 0) {
        return errno;
    }

    *(*new)->local_addr = *sock->local_addr;

    if (sock->local_port_unknown) {
        /* not likely for a listening socket, but theoretically possible :) */
        (*new)->local_port_unknown = 1;
    }

    if (sock->local_interface_unknown ||
        sock->local_addr->sin_addr.s_addr == 0) {
        /* If the interface address inside the listening socket's local_addr wasn't 
         * up-to-date, we don't know local interface of the connected socket either.
         *
         * If the listening socket was not bound to a specific interface, we
         * don't know the local_addr of the connected socket.
         */
        (*new)->local_interface_unknown = 1;
    }

    apr_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, apr_null_cleanup);
    return APR_SUCCESS;
}

apr_status_t apr_connect(apr_socket_t *sock, char *hostname)
{
    struct hostent *hp;

    if ((sock->socketdes < 0) || (!sock->remote_addr)) {
        return APR_ENOTSOCK;
    }
    if (hostname != NULL) {
#ifndef GETHOSTBYNAME_HANDLES_NAS
        if (*hostname >= '0' && *hostname <= '9' &&
            strspn(hostname, "0123456789.") == strlen(hostname)) {
            sock->remote_addr->sin_addr.s_addr = inet_addr(hostname);
            sock->addr_len = sizeof(*sock->remote_addr);
        }
        else {
#endif
        hp = gethostbyname(hostname);

        if (!hp)  {
            return (h_errno + APR_OS_START_SYSERR);
        }
    
        memcpy((char *)&sock->remote_addr->sin_addr, hp->h_addr_list[0], 
               hp->h_length);

        sock->addr_len = sizeof(*sock->remote_addr);
#ifndef GETHOSTBYNAME_HANDLES_NAS
        }
#endif
    }

    if ((connect(sock->socketdes, (const struct sockaddr *)sock->remote_addr,
        sock->addr_len) < 0) && (errno != EINPROGRESS)) {
        return errno;
    }
    else {
        if (sock->local_addr->sin_port == 0) {
            /* connect() got us an ephemeral port */
            sock->local_port_unknown = 1;
        }
        if (sock->local_addr->sin_addr.s_addr == 0) {
            /* not bound to specific local interface; connect() had to assign
             * one for the socket
             */
            sock->local_interface_unknown = 1;
        }
#ifndef HAVE_POLL
	sock->connected=1;
#endif
        return APR_SUCCESS;
    }
}

apr_status_t apr_get_socketdata(void **data, const char *key, apr_socket_t *sock)
{
    return apr_get_userdata(data, key, sock->cntxt);
}

apr_status_t apr_set_socketdata(apr_socket_t *sock, void *data, const char *key,
                              apr_status_t (*cleanup) (void *))
{
    return apr_set_userdata(data, key, cleanup, sock->cntxt);
}

apr_status_t apr_get_os_sock(apr_os_sock_t *thesock, apr_socket_t *sock)
{
    *thesock = sock->socketdes;
    return APR_SUCCESS;
}

apr_status_t apr_put_os_sock(apr_socket_t **sock, apr_os_sock_t *thesock, 
                           apr_pool_t *cont)
{
    if ((*sock) == NULL) {
        (*sock) = (apr_socket_t *)apr_pcalloc(cont, sizeof(apr_socket_t));
        (*sock)->cntxt = cont;
        (*sock)->local_addr = (struct sockaddr_in *)apr_pcalloc((*sock)->cntxt,
                             sizeof(struct sockaddr_in));
        (*sock)->remote_addr = (struct sockaddr_in *)apr_pcalloc((*sock)->cntxt,
                              sizeof(struct sockaddr_in));

        if ((*sock)->local_addr == NULL || (*sock)->remote_addr == NULL) {
            return APR_ENOMEM;
        }
     
        (*sock)->addr_len = sizeof(*(*sock)->local_addr);
        (*sock)->timeout = -1;
    }
    (*sock)->local_port_unknown = (*sock)->local_interface_unknown = 1;
    (*sock)->socketdes = *thesock;
    return APR_SUCCESS;
}

