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

#include "apr_private.h"
#if BEOS_BONE /* BONE uses the unix code - woohoo */
#include "../unix/sockaddr.c"
#else
#include "networkio.h"

static apr_status_t get_local_addr(apr_socket_t *sock)
{
    apr_socklen_t namelen = sizeof(*sock->local_addr);

    if (getsockname(sock->socketdes, (struct sockaddr *)sock->local_addr,
                    &namelen) < 0) {
        return errno;
    }
    else {
        sock->local_port_unknown = sock->local_interface_unknown = 0;
        return APR_SUCCESS;
    }
}

/* Include this here so we already have get_local_addr... */
#include "../unix/sa_common.c"

apr_status_t apr_set_ipaddr(apr_socket_t *sock, apr_interface_e which, const char *addr)
{
    u_long ipaddr;
    struct sockaddr_in *ptr;

    if (which == APR_LOCAL)
        ptr = sock->local_addr;
    else if (which == APR_REMOTE)
        ptr = sock->remote_addr;
    else 
        return APR_EINVAL;
    
    if (!strcmp(addr, APR_ANYADDR)) {
        ptr->sin_addr.s_addr = htonl(INADDR_ANY);
        return APR_SUCCESS;
    }
    
    ipaddr = inet_addr(addr);
    
    if (ipaddr == -1) {
        return errno;
    }
    
    ptr->sin_addr.s_addr = ipaddr;
    return APR_SUCCESS;
}

apr_status_t get_local_name(struct sockaddr_in **name, apr_socket_t *sock)
{
    if (!sock) {
        return APR_EBADF;
    }

    *name = sock->local_addr;
    return APR_SUCCESS;
}

apr_status_t apr_get_remote_name(struct sockaddr_in **name, apr_socket_t *sock)
{
    if (!sock) {
        return APR_EBADF;
    }

    *name = sock->remote_addr;
    return APR_SUCCESS;
}
#endif
