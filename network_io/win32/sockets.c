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
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include <string.h>

static apr_status_t socket_cleanup(void *sock)
{
    apr_socket_t *thesocket = sock;

    if (thesocket->sock != INVALID_SOCKET) {
        if (closesocket(thesocket->sock) == SOCKET_ERROR) {
            return apr_get_netos_error();
        }
        thesocket->sock = INVALID_SOCKET;
    }
    return APR_SUCCESS;
}

static void set_socket_vars(apr_socket_t *sock, int family)
{
    sock->local_addr->sa.sin.sin_family = family;
    sock->remote_addr->sa.sin.sin_family = family;

    if (family == AF_INET) {
        sock->local_addr->salen = sizeof(struct sockaddr_in);
        sock->local_addr->addr_str_len = 16;
        sock->local_addr->ipaddr_ptr = &(sock->local_addr->sa.sin.sin_addr);
        sock->local_addr->ipaddr_len = sizeof(struct in_addr);

        sock->remote_addr->salen = sizeof(struct sockaddr_in);
        sock->remote_addr->addr_str_len = 16;
        sock->remote_addr->ipaddr_ptr = &(sock->remote_addr->sa.sin.sin_addr);
        sock->remote_addr->ipaddr_len = sizeof(struct in_addr);
    }
#if APR_HAVE_IPV6
    else if (family == AF_INET6) {
        sock->local_addr->salen = sizeof(struct sockaddr_in6);
        sock->local_addr->addr_str_len = 46;
        sock->local_addr->ipaddr_ptr = &(sock->local_addr->sa.sin6.sin6_addr);
        sock->local_addr->ipaddr_len = sizeof(struct in6_addr);

        sock->remote_addr->salen = sizeof(struct sockaddr_in6);
        sock->remote_addr->addr_str_len = 46;
        sock->remote_addr->ipaddr_ptr = &(sock->remote_addr->sa.sin6.sin6_addr);
        sock->remote_addr->ipaddr_len = sizeof(struct in6_addr);
    }
#endif
}                                                                                                  
static void alloc_socket(apr_socket_t **new, apr_pool_t *p)
{
    *new = (apr_socket_t *)apr_pcalloc(p, sizeof(apr_socket_t));
    (*new)->cntxt = p;
    (*new)->local_addr = (apr_sockaddr_t *)apr_pcalloc((*new)->cntxt,
                                                       sizeof(apr_sockaddr_t));
    (*new)->remote_addr = (apr_sockaddr_t *)apr_pcalloc((*new)->cntxt,
                                                        sizeof(apr_sockaddr_t));
}

apr_status_t apr_create_socket(apr_socket_t **new, int ofamily, int type,
                               apr_pool_t *cont)
{
    int family = ofamily;

    if (family == AF_UNSPEC) {
#if APR_HAVE_IPV6
        family = AF_INET6;
#else
        family = AF_INET;
#endif
    }

    alloc_socket(new, cont);

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    if (((*new)->local_addr == NULL) || ((*new)->remote_addr == NULL)) {
        return APR_ENOMEM;
    }

    /* For right now, we are not using socket groups.  We may later.
     * No flags to use when creating a socket, so use 0 for that parameter as well.
     */
    (*new)->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#if APR_HAVE_IPV6
    if ((*new)->sock == INVALID_SOCKET && ofamily == AF_UNSPEC) {
        family = AF_INET;
        (*new)->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
#endif

    if ((*new)->sock == INVALID_SOCKET) {
        return apr_get_netos_error();
    }
    set_socket_vars(*new, AF_INET);

    (*new)->timeout = -1;
    (*new)->disconnected = 0;

    apr_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, apr_null_cleanup);

    return APR_SUCCESS;
} 

apr_status_t apr_shutdown(apr_socket_t *thesocket, apr_shutdown_how_e how)
{
    int winhow;

    switch (how) {
        case APR_SHUTDOWN_READ: {
            winhow = SD_RECEIVE;
            break;
        }
        case APR_SHUTDOWN_WRITE: {
            winhow = SD_SEND;
            break;
        }
        case APR_SHUTDOWN_READWRITE: {
            winhow = SD_BOTH;
            break;
        }
    }
    if (shutdown(thesocket->sock, winhow) == 0) {
        return APR_SUCCESS;
    }
    else {
        return apr_get_netos_error();
    }
}

apr_status_t apr_close_socket(apr_socket_t *thesocket)
{
    apr_kill_cleanup(thesocket->cntxt, thesocket, socket_cleanup);
    return socket_cleanup(thesocket);
}

apr_status_t apr_bind(apr_socket_t *sock, apr_sockaddr_t *sa)
{
    if (bind(sock->sock, 
             (struct sockaddr *)&sa->sa, 
             sa->salen) == -1) {
        return apr_get_netos_error();
    }
    else {
        sock->local_addr = sa;
        if (sock->local_addr->sa.sin.sin_port == 0) {
            sock->local_port_unknown = 1; /* ephemeral port */
        }
        return APR_SUCCESS;
    }
}

apr_status_t apr_listen(apr_socket_t *sock, apr_int32_t backlog)
{
    if (listen(sock->sock, backlog) == SOCKET_ERROR)
        return apr_get_netos_error();
    else
        return APR_SUCCESS;
}

apr_status_t apr_accept(apr_socket_t **new, apr_socket_t *sock, apr_pool_t *connection_context)
{
    alloc_socket(new, connection_context);
    set_socket_vars(*new, sock->local_addr->sa.sin.sin_family);

    (*new)->timeout = -1;   
    (*new)->disconnected = 0;

    (*new)->remote_addr->salen = sizeof((*new)->remote_addr->sa);
    (*new)->sock = accept(sock->sock, 
                          (struct sockaddr *)&(*new)->remote_addr->sa,
                          &(*new)->remote_addr->salen);

    if ((*new)->sock == INVALID_SOCKET) {
        return apr_get_netos_error();
    }
    *(*new)->local_addr = *sock->local_addr;
    /* fix up any pointers which are no longer valid */
    if (sock->local_addr->sa.sin.sin_family == AF_INET) {
        (*new)->local_addr->ipaddr_ptr = &(*new)->local_addr->sa.sin.sin_addr;
    }
#if APR_HAVE_IPV6
    else if (sock->local_addr->sa.sin.sin_family == AF_INET6) {
        (*new)->local_addr->ipaddr_ptr = &(*new)->local_addr->sa.sin6.sin6_addr;
    }
#endif

    if (sock->local_port_unknown) {
        /* not likely for a listening socket, but theoretically possible :) */
        (*new)->local_port_unknown = 1;
    }

    if (sock->local_interface_unknown ||
        sock->local_addr->sa.sin.sin_addr.s_addr == 0) {
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

apr_status_t apr_connect(apr_socket_t *sock, apr_sockaddr_t *sa)
{
    apr_status_t lasterror;
    fd_set temp;

    if ((sock->sock == INVALID_SOCKET) || (!sock->local_addr)) {
        return APR_ENOTSOCK;
    }

    if (connect(sock->sock, (const struct sockaddr *)&sa->sa.sin,
                sa->salen) == SOCKET_ERROR) {
        lasterror = apr_get_netos_error();
        if (lasterror != APR_FROM_OS_ERROR(WSAEWOULDBLOCK)) {
            return lasterror;
        }
        /* wait for the connect to complete */
        FD_ZERO(&temp);
        FD_SET(sock->sock, &temp);
        if (select(sock->sock+1, NULL, &temp, NULL, NULL) == SOCKET_ERROR) {
            return apr_get_netos_error();
        }
    }
    /* connect was OK .. amazing */
    sock->remote_addr = sa;
    if (sock->local_addr->sa.sin.sin_port == 0) {
        sock->local_port_unknown = 1;
    }
    if (sock->local_addr->sa.sin.sin_addr.s_addr == 0) {
        /* must be using free-range port */
        sock->local_interface_unknown = 1;
    }
    return APR_SUCCESS;
}

apr_status_t apr_get_socketdata(void **data, const char *key, apr_socket_t *socket)
{
    return apr_get_userdata(data, key, socket->cntxt);
}

apr_status_t apr_set_socketdata(apr_socket_t *socket, void *data, const char *key, 
                              apr_status_t (*cleanup) (void *))
{
    return apr_set_userdata(data, key, cleanup, socket->cntxt);
}

apr_status_t apr_get_os_sock(apr_os_sock_t *thesock, apr_socket_t *sock)
{
    *thesock = sock->sock;
    return APR_SUCCESS;
}

apr_status_t apr_make_os_sock(apr_socket_t **apr_sock, 
                              apr_os_sock_info_t *os_sock_info, 
                              apr_pool_t *cont)
{
    alloc_socket(apr_sock, cont);
    set_socket_vars(*apr_sock, os_sock_info->family);
    (*apr_sock)->timeout = -1;
    (*apr_sock)->disconnected = 0;
    (*apr_sock)->sock = *os_sock_info->os_sock;
    if (os_sock_info->local) {
        memcpy(&(*apr_sock)->local_addr->sa.sin, 
               os_sock_info->local, 
               (*apr_sock)->local_addr->salen);
    }
    else {
        (*apr_sock)->local_port_unknown = (*apr_sock)->local_interface_unknown = 1;
    }
    if (os_sock_info->remote) {
        memcpy(&(*apr_sock)->remote_addr->sa.sin, 
               os_sock_info->remote,
               (*apr_sock)->remote_addr->salen);
    }
        
    apr_register_cleanup((*apr_sock)->cntxt, (void *)(*apr_sock), 
                        socket_cleanup, apr_null_cleanup);

    return APR_SUCCESS;
}

apr_status_t apr_put_os_sock(apr_socket_t **sock, apr_os_sock_t *thesock, 
                           apr_pool_t *cont)
{
    if ((*sock) == NULL) {
        alloc_socket(sock, cont);
        set_socket_vars(*sock, AF_INET);
        (*sock)->timeout = -1;
        (*sock)->disconnected = 0;
    }
    (*sock)->local_port_unknown = (*sock)->local_interface_unknown = 1;
    (*sock)->sock = *thesock;
    return APR_SUCCESS;
}
