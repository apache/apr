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

#if defined(BEOS) && !defined(BEOS_BONE)
    if (closesocket(thesocket->socketdes) == 0) {
#else
    if (close(thesocket->socketdes) == 0) {
#endif
        thesocket->socketdes = -1;
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

static void set_socket_vars(apr_socket_t *sock, int family)
{
    sock->local_addr->sa.sin.sin_family = family;
    sock->remote_addr->sa.sin.sin_family = family;

    if (family == APR_INET) {
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
    else if (family == APR_INET6) {
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

    if (family == APR_UNSPEC) {
#if APR_HAVE_IPV6
        family = APR_INET6;
#else
        family = APR_INET;
#endif
    }

    alloc_socket(new, cont);

    if ((*new)->local_addr == NULL || (*new)->remote_addr == NULL) {
        return APR_ENOMEM;
    }

    (*new)->socketdes = socket(family, type, 0);

#if APR_HAVE_IPV6
    if ((*new)->socketdes < 0 && ofamily == APR_UNSPEC) {
        family = APR_INET;
        (*new)->socketdes = socket(family, type, 0);
    }
#endif

    if ((*new)->socketdes < 0) {
        return errno;
    }
    set_socket_vars(*new, family);

    (*new)->timeout = -1;
    apr_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, apr_null_cleanup);
    return APR_SUCCESS;
} 

apr_status_t apr_shutdown(apr_socket_t *thesocket, apr_shutdown_how_e how)
{
    return (shutdown(thesocket->socketdes, how) == -1) ? errno : APR_SUCCESS;
}

apr_status_t apr_close_socket(apr_socket_t *thesocket)
{
    apr_kill_cleanup(thesocket->cntxt, thesocket, socket_cleanup);
    return socket_cleanup(thesocket);
}

apr_status_t apr_bind(apr_socket_t *sock, apr_sockaddr_t *sa)
{
    if (bind(sock->socketdes, 
             (struct sockaddr *)&sa->sa, sa->salen) == -1) {
        return errno;
    }
    else {
        sock->local_addr = sa;
        /* XXX IPv6 - this assumes sin_port and sin6_port at same offset */
        if (sock->local_addr->sa.sin.sin_port == 0) { /* no need for ntohs() when comparing w/ 0 */
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
    alloc_socket(new, connection_context);
    set_socket_vars(*new, sock->local_addr->sa.sin.sin_family);

#ifndef HAVE_POLL
    (*new)->connected = 1;
#endif
    (*new)->timeout = -1;
    
    (*new)->remote_addr->salen = sizeof((*new)->remote_addr->sa);
    (*new)->socketdes = accept(sock->socketdes, 
                               (struct sockaddr *)&(*new)->remote_addr->sa,
                               &(*new)->remote_addr->salen);

    if ((*new)->socketdes < 0) {
        return errno;
    }
    *(*new)->local_addr = *sock->local_addr;

    if (sock->local_port_unknown) {
        /* not likely for a listening socket, but theoretically possible :) */
        (*new)->local_port_unknown = 1;
    }

    if (sock->local_interface_unknown ||
        /* XXX IPv6 issue */
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
    if ((sock->socketdes < 0) || (!sock->remote_addr)) {
        return APR_ENOTSOCK;
    }

    if ((connect(sock->socketdes,
                 (const struct sockaddr *)&sa->sa.sin,
                 sa->salen) < 0) &&
        (errno != EINPROGRESS)) {
        return errno;
    }
    else {
        sock->remote_addr = sa;
        /* XXX IPv6 assumes sin_port and sin6_port at same offset */
        if (sock->local_addr->sa.sin.sin_port == 0) {
            /* connect() got us an ephemeral port */
            sock->local_port_unknown = 1;
        }
        /* XXX IPv6 to be handled better later... */
        if (
#if APR_HAVE_IPV6
            sock->local_addr->sa.sin.sin_family == APR_INET6 ||
#endif
            sock->local_addr->sa.sin.sin_addr.s_addr == 0) {
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
        alloc_socket(sock, cont);
        /* XXX IPv6 figure out the family here! */
        set_socket_vars(*sock, APR_INET);
        (*sock)->timeout = -1;
    }
    (*sock)->local_port_unknown = (*sock)->local_interface_unknown = 1;
    (*sock)->socketdes = *thesock;
    return APR_SUCCESS;
}

