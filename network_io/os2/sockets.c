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

static apr_status_t socket_cleanup(void *sock)
{
    apr_socket_t *thesocket = sock;

    if (thesocket->socketdes < 0) {
        return APR_EINVALSOCK;
    }

    if (soclose(thesocket->socketdes) == 0) {
        thesocket->socketdes = -1;
        return APR_SUCCESS;
    }
    else {
        return APR_OS2_STATUS(sock_errno());
    }
}

static void set_socket_vars(apr_socket_t *sock, int family)
{
    sock->local_addr->sa.sin.sin_family = family;
    sock->remote_addr->sa.sin.sin_family = family;

    if (family == AF_INET) {
        sock->local_addr->sa_len = sizeof(struct sockaddr_in);
        sock->local_addr->addr_str_len = 16;
        sock->local_addr->ipaddr_ptr = &(sock->local_addr->sa.sin.sin_addr);
        sock->local_addr->ipaddr_len = sizeof(struct in_addr);

        sock->remote_addr->sa_len = sizeof(struct sockaddr_in);
        sock->remote_addr->addr_str_len = 16;
        sock->remote_addr->ipaddr_ptr = &(sock->remote_addr->sa.sin.sin_addr);
        sock->remote_addr->ipaddr_len = sizeof(struct in_addr);
    }
#if APR_HAVE_IPV6
    else if (family == AF_INET6) {
        sock->local_addr->sa_len = sizeof(struct sockaddr_in6);
        sock->local_addr->addr_str_len = 46;
        sock->local_addr->ipaddr_ptr = &(sock->local_addr->sa.sin6.sin6_addr);
        sock->local_addr->ipaddr_len = sizeof(struct in6_addr);

        sock->remote_addr->sa_len = sizeof(struct sockaddr_in6);
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
    if ((*new)->local_addr == NULL || (*new)->remote_addr == NULL) {
        return APR_ENOMEM;
    }
 
    (*new)->socketdes = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#if APR_HAVE_IPV6
    if ((*new)->socketdes < 0 && ofamily == AF_UNSPEC) {
        family = AF_INET;
        (*new)->socketdes = socket(family, type, 0);
    }
#endif

    if ((*new)->socketdes < 0) {
        return APR_OS2_STATUS(sock_errno());
    }
    set_socket_vars(*new, family);

    (*new)->timeout = -1;
    (*new)->nonblock = FALSE;
    apr_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, apr_null_cleanup);
    return APR_SUCCESS;
} 

apr_status_t apr_create_tcp_socket(apr_socket_t **new, apr_pool_t *cont)
{
    return apr_create_socket(new, AF_INET, SOCK_STREAM, cont);
}

apr_status_t apr_shutdown(apr_socket_t *thesocket, apr_shutdown_how_e how)
{
    if (shutdown(thesocket->socketdes, how) == 0) {
        return APR_SUCCESS;
    }
    else {
        return APR_OS2_STATUS(sock_errno());
    }
}

apr_status_t apr_close_socket(apr_socket_t *thesocket)
{
    apr_kill_cleanup(thesocket->cntxt, thesocket, socket_cleanup);
    return socket_cleanup(thesocket);
}




apr_status_t apr_bind(apr_socket_t *sock)
{
    if (bind(sock->socketdes, 
             (struct sockaddr *)&sock->local_addr->sa,
             sock->local_addr->sa_len) == -1)
        return APR_OS2_STATUS(sock_errno());
    else
        return APR_SUCCESS;
}

apr_status_t apr_listen(apr_socket_t *sock, apr_int32_t backlog)
{
    if (listen(sock->socketdes, backlog) == -1)
        return APR_OS2_STATUS(sock_errno());
    else
        return APR_SUCCESS;
}

apr_status_t apr_accept(apr_socket_t **new, apr_socket_t *sock, apr_pool_t *connection_context)
{
    alloc_socket(new, connection_context);
    set_socket_vars(*new, sock->local_addr->sa.sin.sin_family);

    (*new)->timeout = -1;
    (*new)->nonblock = FALSE;

    (*new)->socketdes = accept(sock->socketdes, 
                               (struct sockaddr *)&(*new)->remote_addr->sa,
                               &(*new)->remote_addr->sa_len);

    if ((*new)->socketdes < 0) {
        return APR_OS2_STATUS(sock_errno());
    }

    apr_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, apr_null_cleanup);
    return APR_SUCCESS;
}

apr_status_t apr_connect(apr_socket_t *sock, apr_sockaddr_t *sa)
{
    if ((connect(sock->socketdes, (struct sockaddr *)&sa->sa.sin, 
                 sa->sa_len) < 0) &&
        (sock_errno() != SOCEINPROGRESS)) {
        return APR_OS2_STATUS(sock_errno());
    }
    else {
        int namelen = sizeof(sock->local_addr->sa.sin);
        getsockname(sock->socketdes, (struct sockaddr *)&sock->local_addr->sa.sin, 
                    &namelen);
        sock->remote_addr = sa;
        return APR_SUCCESS;
    }
}



apr_status_t apr_get_socketdata(void **data, const char *key,
                              apr_socket_t *socket)
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
    *thesock = sock->socketdes;
    return APR_SUCCESS;
}



apr_status_t apr_put_os_sock(apr_socket_t **sock, apr_os_sock_t *thesock, apr_pool_t *cont)
{
    if (cont == NULL) {
        return APR_ENOPOOL;
    }
    if ((*sock) == NULL) {
        alloc_socket(sock, cont);
        set_socket_vars(*sock, AF_INET);
    }
    (*sock)->socketdes = *thesock;
    return APR_SUCCESS;
}

