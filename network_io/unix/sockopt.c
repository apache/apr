/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
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
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "networkio.h"

static ap_status_t soblock(int sd)
{
    int fd_flags;

    fd_flags = fcntl(sd, F_GETFL, 0);
#if defined(O_NONBLOCK)
    fd_flags &= ~O_NONBLOCK;
#elif defined(O_NDELAY)
    fd_flags &= ~O_NDELAY;
#elif defined(FNDELAY)
    fd_flags &= ~O_FNDELAY;
#else
    /* XXXX: this breaks things, but an alternative isn't obvious...*/
    return -1;
#endif
    if (fcntl(sd, F_SETFL, fd_flags) == -1) {
        return errno;
    }
    return APR_SUCCESS;
}

static ap_status_t sononblock(int sd)
{
    int fd_flags;

    fd_flags = fcntl(sd, F_GETFL, 0);
#if defined(O_NONBLOCK)
    fd_flags |= O_NONBLOCK;
#elif defined(O_NDELAY)
    fd_flags |= O_NDELAY;
#elif defined(FNDELAY)
    fd_flags |= O_FNDELAY;
#else
    /* XXXX: this breaks things, but an alternative isn't obvious...*/
    return -1;
#endif
    if (fcntl(sd, F_SETFL, fd_flags) == -1) {
        return errno;
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_setsocketopt(ap_socket_t *sock, ap_int32_t opt, ap_int32_t on)
 *    Setup socket options for the specified socket 
 * arg 1) The socket to set up.
 * arg 2) The option we would like to configure.  One of:
 *            APR_SO_DEBUG      --  turn on debugging information 
 *            APR_SO_KEEPALIVE  --  keep connections active
 *            APR_SO_LINGER     --  lingers on close if data is present
 *            APR_SO_NONBLOCK   --  Turns blocking on/off for socket
 *            APR_SO_REUSEADDR  --  The rules used in validating addresses
 *                                  supplied to bind should allow reuse
 *                                  of local addresses.
 *            APR_SO_TIMEOUT    --  Set the timeout value in seconds.
 *                                  values < 0 mean wait forever.  0 means
 *                                  don't wait at all.
 *            APR_SO_SNDBUF     --  Set the SendBufferSize
 * arg 3) Are we turning the option on or off.
 */
ap_status_t ap_setsocketopt(struct socket_t *sock, ap_int32_t opt, ap_int32_t on)
{
    int one;
    struct linger li;
    ap_status_t stat;

    if (on)
        one = 1;
    else
        one = 0;

    if (opt & APR_SO_KEEPALIVE) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int)) == -1) {
            return errno;
        }
    }
    if (opt & APR_SO_DEBUG) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_DEBUG, (void *)&one, sizeof(int)) == -1) {
            return errno;
        }
    }
    if (opt & APR_SO_REUSEADDR) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int)) == -1) {
            return errno;
        }
    }
    if (opt & APR_SO_SNDBUF) {
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_SNDBUF, (void *)&on, sizeof(int)) == -1) {
            return errno;
        }
    }
    if (opt & APR_SO_NONBLOCK) {
        if (on) {
            if ((stat = soblock(sock->socketdes)) != APR_SUCCESS) 
                return stat;
        }
        else {
            if ((stat = sononblock(sock->socketdes)) != APR_SUCCESS)
                return stat;
        }
    }
    if (opt & APR_SO_LINGER) {
        li.l_onoff = on;
        li.l_linger = MAX_SECS_TO_LINGER;
        if (setsockopt(sock->socketdes, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger)) == -1) {
            return errno;
        }
    }
    if (opt & APR_SO_TIMEOUT) {
        sock->timeout = on;
        if ((stat = sononblock(sock->socketdes)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}         

/* ***APRDOC********************************************************
 * ap_status_t ap_gethostname(char *buf, ap_int32_t len, ap_context_t *cont)
 *    Get name of the current machine 
 * arg 1) A buffer to store the hostname in.
 * arg 2) The maximum length of the hostname that can be stored in the
 *        buffer provided. 
 * arg 3) The context to use.
 */
ap_status_t ap_gethostname(char *buf, ap_int32_t len, ap_context_t *cont)
{
    if (gethostname(buf, len) == -1)
        return errno;
    else
        return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_remote_hostname(char **name, ap_socket_t *sock)
 *    Get name of the machine we are currently connected to. 
 * arg 1) A buffer to store the hostname in.
 * arg 2) The socket to examine.
 */
ap_status_t ap_get_remote_hostname(char **name, struct socket_t *sock)
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

    /* XXX - Is this threadsafe? */
    return h_errno;
}


