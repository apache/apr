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

static ap_status_t socket_cleanup(void *sock)
{
    ap_socket_t *thesocket = sock;
    if (close(thesocket->socketdes) == 0) {
        thesocket->socketdes = -1;
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

ap_status_t ap_create_tcp_socket(ap_socket_t **new, ap_pool_t *cont)
{
    (*new) = (ap_socket_t *)ap_pcalloc(cont, sizeof(ap_socket_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont; 
    (*new)->local_addr = (struct sockaddr_in *)ap_pcalloc((*new)->cntxt,
                         sizeof(struct sockaddr_in));
    (*new)->remote_addr = (struct sockaddr_in *)ap_pcalloc((*new)->cntxt,
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
    ap_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
} 

ap_status_t ap_shutdown(ap_socket_t *thesocket, ap_shutdown_how_e how)
{
    return (shutdown(thesocket->socketdes, how) == -1) ? errno : APR_SUCCESS;
}

ap_status_t ap_close_socket(ap_socket_t *thesocket)
{
    ap_kill_cleanup(thesocket->cntxt, thesocket, socket_cleanup);
    return socket_cleanup(thesocket);
}

ap_status_t ap_bind(ap_socket_t *sock)
{
    if (bind(sock->socketdes, (struct sockaddr *)sock->local_addr, sock->addr_len) == -1)
        return errno;
    else
        return APR_SUCCESS;
}

ap_status_t ap_listen(ap_socket_t *sock, ap_int32_t backlog)
{
    if (listen(sock->socketdes, backlog) == -1)
        return errno;
    else
        return APR_SUCCESS;
}

ap_status_t ap_accept(ap_socket_t **new, const ap_socket_t *sock, ap_pool_t *connection_context)
{
    (*new) = (ap_socket_t *)ap_pcalloc(connection_context, 
                            sizeof(ap_socket_t));

    (*new)->cntxt = connection_context;
    (*new)->local_addr = (struct sockaddr_in *)ap_pcalloc((*new)->cntxt, 
                 sizeof(struct sockaddr_in));

    (*new)->remote_addr = (struct sockaddr_in *)ap_pcalloc((*new)->cntxt, 
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

    if (getsockname((*new)->socketdes, (struct sockaddr *)(*new)->local_addr, 
                    &((*new)->addr_len)) < 0) {
	return errno;
    }

    ap_register_cleanup((*new)->cntxt, (void *)(*new), 
                        socket_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_connect(ap_socket_t *sock, char *hostname)
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
        socklen_t namelen = sizeof(*sock->local_addr);
        getsockname(sock->socketdes, (struct sockaddr *)sock->local_addr, &namelen);
#ifndef HAVE_POLL
	sock->connected=1;
#endif
        return APR_SUCCESS;
    }
}

ap_status_t ap_get_socketdata(void **data, char *key, ap_socket_t *sock)
{
    if (sock != NULL) {
        return ap_get_userdata(data, key, sock->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOSOCKET;
    }
}

ap_status_t ap_set_socketdata(ap_socket_t *sock, void *data, char *key,
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

ap_status_t ap_get_os_sock(ap_os_sock_t *thesock, ap_socket_t *sock)
{
    if (sock == NULL) {
        return APR_ENOSOCKET;
    }
    *thesock = sock->socketdes;
    return APR_SUCCESS;
}

ap_status_t ap_put_os_sock(ap_socket_t **sock, ap_os_sock_t *thesock, 
                           ap_pool_t *cont)
{
    if (cont == NULL) {
        return APR_ENOPOOL;
    }
    if ((*sock) == NULL) {
        (*sock) = (ap_socket_t *)ap_pcalloc(cont, sizeof(ap_socket_t));
        (*sock)->cntxt = cont;
        (*sock)->local_addr = (struct sockaddr_in *)ap_pcalloc((*sock)->cntxt,
                             sizeof(struct sockaddr_in));
        (*sock)->remote_addr = (struct sockaddr_in *)ap_pcalloc((*sock)->cntxt,
                              sizeof(struct sockaddr_in));

        if ((*sock)->local_addr == NULL || (*sock)->remote_addr == NULL) {
            return APR_ENOMEM;
        }
     
        (*sock)->addr_len = sizeof(*(*sock)->local_addr);
        (*sock)->timeout = -1;
        if (getsockname(*thesock, (struct sockaddr *)(*sock)->local_addr, 
                        &((*sock)->addr_len)) < 0) {
            return errno;
        }
    }
    (*sock)->socketdes = *thesock;
    return APR_SUCCESS;
}

