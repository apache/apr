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
#include "../unix/sockopt.c"
#else
#include "networkio.h"

static int setnonblocking(int on, int sock)
{
    return setsockopt(sock, SOL_SOCKET, SO_NONBLOCK,
        &on, sizeof(on));
}

ap_status_t ap_setsocketopt(ap_socket_t *sock, ap_int32_t opt, ap_int32_t on)
{
    int one;
    int rv;
    if (on){
        one = 1;
    }else {
        one = 0;
	}
	if (opt & APR_SO_SNDBUF) 
	    return APR_ENOTIMPL;

    if (opt & APR_SO_DEBUG) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_DEBUG, &one, sizeof(one)) == -1) {
            return errno;
        }
    }
    if (opt & APR_SO_REUSEADDR) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
            return errno;
        }
    }
    if (opt & APR_SO_NONBLOCK) {
    	if ((rv = setnonblocking (one, sock->socketdes)) < 0) {
            return errno;
    	}
    } 
    if (opt & APR_SO_TIMEOUT) { 
        if (on > 0 && sock->timeout < 0)
            /* we should be in non-blocking mode right now... */
            one = 1;
        else if (on < 0 && sock->timeout >= 0)
            /* they've set a timeout so we should be blocking... */
            one = 0;
        else if (on == 0)
            /* we need to be in nonblocking or this will hang the server */
            one = 1;
            
        if ((rv = setnonblocking (one, sock->socketdes)) < 0)
            return rv; 
         
        sock->timeout = on; 
    } 
    return APR_SUCCESS;
}         

ap_status_t ap_getsocketopt(ap_socket_t *sock, ap_int32_t opt, ap_int32_t *on)
{
    switch(opt) {
    case APR_SO_TIMEOUT:
        *on = sock->timeout;
        break;
    default:
        return APR_EINVAL;
    }
    return APR_SUCCESS;
}

ap_status_t ap_gethostname(char * buf, int len, ap_pool_t *cont)
{
	if (gethostname(buf, len) == -1){
		return errno;
	} else {
		return APR_SUCCESS;
	}
}

ap_status_t ap_get_remote_hostname(char **name, ap_socket_t *sock)
{
    struct hostent *hptr;
    
    hptr = gethostbyaddr((char *)&(sock->remote_addr->sin_addr), 
                         sizeof(struct in_addr), AF_INET);
    if (hptr != NULL) {
        *name = ap_pstrdup(sock->cntxt, hptr->h_name);
        if (*name) {
            return APR_SUCCESS;
        }
        return APR_ENOMEM;
    }

    /* XXX - Is this threadsafe? - manoj */
    /* on BeOS h_errno is a global... */
    return h_errno;
}
#endif