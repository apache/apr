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

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include "networkio.h"
#include "apr_network_io.h"
#include "apr_general.h"

ap_status_t ap_setsocketopt(struct socket_t *sock, ap_int32_t opt, ap_int32_t on)
{
    int one;
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
    	if (setsockopt(sock->socketdes, SOL_SOCKET, SO_NONBLOCK, &one, sizeof(one)) == -1){
    		return errno;
    	}
    } 
    return APR_SUCCESS;
}         

ap_status_t ap_gethostname(ap_context_t *cont, char * buf, int len)
{
	if (gethostname(buf, len) == -1){
		return errno;
	} else {
		return APR_SUCCESS;
	}
}

ap_status_t ap_get_remote_hostname(char **name, struct socket_t *sock)
{
    (*name) = (char*)ap_pstrdup(sock->cntxt, sock->remote_hostname);
    if (*name) {
        return APR_SUCCESS;
    }
    return APR_ENOMEM;
}
