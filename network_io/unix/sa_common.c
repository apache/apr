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

/* sa_common.c
 * 
 * this file has common code that manipulates socket information
 *
 * It's intended to be included in sockaddr.c for every platform and
 * so should NOT have any headers included in it.
 *
 * Feature defines are OK, but there should not be any code in this file
 * that differs between platforms.
 */

#include "apr.h"

apr_status_t apr_set_port(apr_socket_t *sock, apr_interface_e which, 
                         apr_port_t port)
{
        /* XXX IPv6 */
    if (which == APR_LOCAL)
        sock->local_addr->sa.sin.sin_port = htons(port);
    else if (which == APR_REMOTE)
        sock->remote_addr->sa.sin.sin_port = htons(port);
    else
        return APR_EINVAL;
    return APR_SUCCESS;
}

apr_status_t apr_get_port(apr_port_t *port, apr_interface_e which, apr_socket_t *sock)
{
    if (which == APR_LOCAL)
    {
        if (sock->local_port_unknown) {
            apr_status_t rv = get_local_addr(sock);

            if (rv != APR_SUCCESS) {
                return rv;
            }
        }

        /* XXX IPv6 */
        *port = ntohs(sock->local_addr->sa.sin.sin_port);
    } else if (which == APR_REMOTE)
        *port = ntohs(sock->remote_addr->sa.sin.sin_port);
    else
        return APR_EINVAL;
    return APR_SUCCESS;
}

apr_status_t apr_get_ipaddr(char **addr, apr_interface_e which, apr_socket_t *sock)
{
    if (which == APR_LOCAL){
        if (sock->local_interface_unknown) {
            apr_status_t rv = get_local_addr(sock);

            if (rv != APR_SUCCESS) {
                return rv;
            }
        }
        /* XXX IPv6 */
        *addr = apr_pstrdup(sock->cntxt, inet_ntoa(sock->local_addr->sa.sin.sin_addr));
    } else if (which == APR_REMOTE)
        /* XXX IPv6 */
        *addr = apr_pstrdup(sock->cntxt, inet_ntoa(sock->remote_addr->sa.sin.sin_addr));
    else 
        return APR_EINVAL;

    return APR_SUCCESS;
}

apr_status_t apr_get_inaddr(apr_in_addr_t *addr, char *hostname)
{
    struct hostent *he;
    
    if (strcmp(hostname,"*") == 0){
        addr->s_addr = htonl(INADDR_ANY);
        return APR_SUCCESS;
    }
    if ((addr->s_addr = apr_inet_addr(hostname)) != APR_INADDR_NONE)
        return APR_SUCCESS;

    /* hmmm, it's not a numeric IP address so we need to look it up :( */
    he = gethostbyname(hostname);
    if (!he || he->h_addrtype != AF_INET || !he->h_addr_list[0])
        return (h_errno + APR_OS_START_SYSERR);

    *addr = *(struct in_addr*)he->h_addr_list[0];

    return APR_SUCCESS;
}

apr_status_t apr_get_socket_inaddr(apr_in_addr_t *addr, apr_interface_e which,
                        apr_socket_t *sock)
{
    if (which == APR_LOCAL){
        if (sock->local_interface_unknown) {
            apr_status_t rv = get_local_addr(sock);

            if (rv != APR_SUCCESS){
                return rv;
            }
        }

        /* XXX IPv6 */
        *addr = *(apr_in_addr_t *)&sock->local_addr->sa.sin.sin_addr;
    } else if (which == APR_REMOTE) {
        /* XXX IPv6 */
        *addr = *(apr_in_addr_t *)&sock->remote_addr->sa.sin.sin_addr;
    } else {
        return APR_EINVAL;
    }
    return APR_SUCCESS;
}

