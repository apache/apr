/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "apr_arch_networkio.h"
#include "apr_strings.h"


static apr_status_t soblock(int sd)
{
/* BeOS uses setsockopt at present for non blocking... */
#ifndef BEOS
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
#else
    int on = 0;
    if (setsockopt(sd, SOL_SOCKET, SO_NONBLOCK, &on, sizeof(int)) < 0)
        return errno;
#endif /* BEOS */
    return APR_SUCCESS;
}

static apr_status_t sononblock(int sd)
{
#ifndef BEOS
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
#else
    int on = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_NONBLOCK, &on, sizeof(int)) < 0)
        return errno;
#endif /* BEOS */
    return APR_SUCCESS;
}


apr_status_t apr_socket_timeout_set(apr_socket_t *sock, apr_interval_time_t t)
{
    apr_status_t stat;

    /* If our timeout is positive or zero and our last timeout was
     * negative, then we need to ensure that we are non-blocking.
     * Conversely, if our timeout is negative and we had a positive
     * or zero timeout, we must make sure our socket is blocking.
     * We want to avoid calling fcntl more than necessary on the socket,
     */
    if (t >= 0 && sock->timeout < 0) {
        if (apr_is_option_set(sock->netmask, APR_SO_NONBLOCK) != 1) {
            if ((stat = sononblock(sock->socketdes)) != APR_SUCCESS) {
                return stat;
            }
            apr_set_option(&sock->netmask, APR_SO_NONBLOCK, 1);
        }
    } 
    else if (t < 0 && sock->timeout >= 0) {
        if (apr_is_option_set(sock->netmask, APR_SO_NONBLOCK) != 0) { 
            if ((stat = soblock(sock->socketdes)) != APR_SUCCESS) { 
                return stat; 
            }
            apr_set_option(&sock->netmask, APR_SO_NONBLOCK, 0);
        } 
    }
    /* must disable the incomplete read support if we disable
     * a timeout
     */
    if (t <= 0) {
        sock->netmask &= ~APR_INCOMPLETE_READ;
    }
    sock->timeout = t; 
    apr_set_option(&sock->netmask, APR_SO_TIMEOUT, t > 0);
    return APR_SUCCESS;
}


apr_status_t apr_socket_opt_set(apr_socket_t *sock, 
                                apr_int32_t opt, apr_int32_t on)
{
    int one;
    apr_status_t rv;

    if (on)
        one = 1;
    else
        one = 0;
    switch(opt) {
    case APR_SO_KEEPALIVE:
#ifdef SO_KEEPALIVE
        if (on != apr_is_option_set(sock->netmask, APR_SO_KEEPALIVE)) {
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_KEEPALIVE, (void *)&one, sizeof(int)) == -1) {
                return errno;
            }
            apr_set_option(&sock->netmask,APR_SO_KEEPALIVE, on);
        }
#else
        return APR_ENOTIMPL;
#endif
        break;
    case APR_SO_DEBUG:
        if (on != apr_is_option_set(sock->netmask, APR_SO_DEBUG)) {
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_DEBUG, (void *)&one, sizeof(int)) == -1) {
                return errno;
            }
            apr_set_option(&sock->netmask, APR_SO_DEBUG, on);
        }
        break;
    case APR_SO_REUSEADDR:
        if (on != apr_is_option_set(sock->netmask, APR_SO_REUSEADDR)) {
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_REUSEADDR, (void *)&one, sizeof(int)) == -1) {
                return errno;
            }
            apr_set_option(&sock->netmask, APR_SO_REUSEADDR, on);
        }
        break;
    case APR_SO_SNDBUF:
#ifdef SO_SNDBUF
        if (apr_is_option_set(sock->netmask, APR_SO_SNDBUF) != on) {
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_SNDBUF, (void *)&on, sizeof(int)) == -1) {
                return errno;
            }
            apr_set_option(&sock->netmask, APR_SO_SNDBUF, on);
        }
#else
        return APR_ENOTIMPL;
#endif
        break;
    case APR_SO_RCVBUF:
#ifdef SO_RCVBUF
        if (apr_is_option_set(sock->netmask, APR_SO_RCVBUF) != on) {
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_RCVBUF, (void *)&on, sizeof(int)) == -1) {
                return errno;
            }
            apr_set_option(&sock->netmask, APR_SO_RCVBUF, on);
        }
#else
        return APR_ENOTIMPL;
#endif
        break;
    case APR_SO_NONBLOCK:
        if (apr_is_option_set(sock->netmask, APR_SO_NONBLOCK) != on) {
            if (on) {
                if ((rv = sononblock(sock->socketdes)) != APR_SUCCESS) 
                    return rv;
            }
            else {
                if ((rv = soblock(sock->socketdes)) != APR_SUCCESS)
                    return rv;
            }
            apr_set_option(&sock->netmask, APR_SO_NONBLOCK, on);
        }
        break;
    case APR_SO_LINGER:
#ifdef SO_LINGER
        if (apr_is_option_set(sock->netmask, APR_SO_LINGER) != on) {
            struct linger li;
            li.l_onoff = on;
            li.l_linger = APR_MAX_SECS_TO_LINGER;
            if (setsockopt(sock->socketdes, SOL_SOCKET, SO_LINGER, (char *) &li, sizeof(struct linger)) == -1) {
                return errno;
            }
            apr_set_option(&sock->netmask, APR_SO_LINGER, on);
        }
#else
        return APR_ENOTIMPL;
#endif
        break;
    case APR_SO_TIMEOUT:
        /* XXX: To be deprecated */
        return apr_socket_timeout_set(sock, on);
        break;
    case APR_TCP_NODELAY:
#if defined(TCP_NODELAY)
        if (apr_is_option_set(sock->netmask, APR_TCP_NODELAY) != on) {
            int optlevel = IPPROTO_TCP;
            int optname = TCP_NODELAY;

#if APR_HAVE_SCTP
            if (sock->protocol == IPPROTO_SCTP) {
                optlevel = IPPROTO_SCTP;
                optname = SCTP_NODELAY;
            }
#endif
            if (setsockopt(sock->socketdes, optlevel, optname, (void *)&on, sizeof(int)) == -1) {
                return errno;
            }
            apr_set_option(&sock->netmask, APR_TCP_NODELAY, on);
        }
#else
        /* BeOS pre-BONE has TCP_NODELAY set by default.
         * As it can't be turned off we might as well check if they're asking
         * for it to be turned on!
         */
#ifdef BEOS
        if (on == 1)
            return APR_SUCCESS;
        else
#endif
        return APR_ENOTIMPL;
#endif
        break;
    case APR_TCP_NOPUSH:
#if APR_TCP_NOPUSH_FLAG
        if (apr_is_option_set(sock->netmask, APR_TCP_NOPUSH) != on) {
            int optlevel = IPPROTO_TCP;
            int optname = TCP_NODELAY;

#if APR_HAVE_SCTP
            if (sock->protocol == IPPROTO_SCTP) {
                optlevel = IPPROTO_SCTP;
                optname = SCTP_NODELAY;
            }
#endif
            /* OK we're going to change some settings here... */
            /* TCP_NODELAY is mutually exclusive, so do we have it set? */
            if (apr_is_option_set(sock->netmask, APR_TCP_NODELAY) == 1 && on) {
                /* If we want to set NOPUSH then if we have the TCP_NODELAY
                 * flag set we need to switch it off...
                 */
                int tmpflag = 0;
                if (setsockopt(sock->socketdes, optlevel, optname,
                               (void*)&tmpflag, sizeof(int)) == -1) {
                    return errno;
                }
                apr_set_option(&sock->netmask, APR_RESET_NODELAY, 1);
                apr_set_option(&sock->netmask, APR_TCP_NODELAY, 0);
            } else if (on) {
                apr_set_option(&sock->netmask, APR_RESET_NODELAY, 0);
            }
            /* OK, now we can just set the TCP_NOPUSH flag accordingly...*/
            if (setsockopt(sock->socketdes, IPPROTO_TCP, APR_TCP_NOPUSH_FLAG,
                           (void*)&on, sizeof(int)) == -1) {
                return errno;
            }
            apr_set_option(&sock->netmask, APR_TCP_NOPUSH, on);
            if (!on && apr_is_option_set(sock->netmask, APR_RESET_NODELAY)) {
                int tmpflag = 1;
                if (setsockopt(sock->socketdes, optlevel, optname,
                               (void*)&tmpflag, sizeof(int)) == -1) {
                    return errno;
                }
                apr_set_option(&sock->netmask, APR_RESET_NODELAY,0);
                apr_set_option(&sock->netmask, APR_TCP_NODELAY, 1);
            }
        }
#else
        return APR_ENOTIMPL;
#endif
        break;
    case APR_INCOMPLETE_READ:
        apr_set_option(&sock->netmask, APR_INCOMPLETE_READ, on);
        break;
    case APR_IPV6_V6ONLY:
#if APR_HAVE_IPV6 && defined(IPV6_V6ONLY)
        /* we don't know the initial setting of this option,
         * so don't check/set sock->netmask since that optimization
         * won't work
         */
        if (setsockopt(sock->socketdes, IPPROTO_IPV6, IPV6_V6ONLY,
                       (void *)&on, sizeof(int)) == -1) {
            return errno;
        }
#else
        return APR_ENOTIMPL;
#endif
        break;
    default:
        return APR_EINVAL;
    }

    return APR_SUCCESS; 
}         


apr_status_t apr_socket_timeout_get(apr_socket_t *sock, apr_interval_time_t *t)
{
    *t = sock->timeout;
    return APR_SUCCESS;
}


apr_status_t apr_socket_opt_get(apr_socket_t *sock, 
                                apr_int32_t opt, apr_int32_t *on)
{
    switch(opt) {
        case APR_SO_TIMEOUT:
            /* XXX: To be deprecated */
            *on = (apr_int32_t)sock->timeout;
            break;
        default:
            *on = apr_is_option_set(sock->netmask, opt);
    }
    return APR_SUCCESS;
}


apr_status_t apr_socket_atmark(apr_socket_t *sock, int *atmark)
{
/* In 1.0 we rely on compile failure to assure all platforms grabbed
 * the correct header file support for SIOCATMARK, but we don't want 
 * to fail the build of 0.9.  Keep things good for the released branch.
 */
#ifdef SIOCATMARK
    int oobmark;

    if (ioctl(sock->socketdes, SIOCATMARK, (void*) &oobmark) < 0)
        return apr_get_netos_error();

    *atmark = (oobmark != 0);

    return APR_SUCCESS;
#else
    return APR_ENOTIMPL;
#endif
}


/* deprecated */
apr_status_t apr_setsocketopt(apr_socket_t *sock,
                              apr_int32_t opt, apr_int32_t on)
{
    return apr_socket_opt_set(sock, opt, on);
}

apr_status_t apr_getsocketopt(apr_socket_t *sock,
                              apr_int32_t opt, apr_int32_t *on)
{
    return apr_socket_opt_get(sock, opt, on);
}
                                           

apr_status_t apr_gethostname(char *buf, apr_int32_t len, apr_pool_t *cont)
{
    if (gethostname(buf, len) == -1) {
        buf[0] = '\0';
        return errno;
    }
    else if (!memchr(buf, '\0', len)) { /* buffer too small */
        /* note... most platforms just truncate in this condition
         *         linux+glibc return an error
         */
        buf[0] = '\0';
        return APR_ENAMETOOLONG;
    }
    return APR_SUCCESS;
}

#if APR_HAS_SO_ACCEPTFILTER
apr_status_t apr_socket_accept_filter(apr_socket_t *sock, char *name, 
                                      char *args)
{
    struct accept_filter_arg af;
    strncpy(af.af_name, name, 16);
    strncpy(af.af_arg, args, 256 - 16);

    if ((setsockopt(sock->socketdes, SOL_SOCKET, SO_ACCEPTFILTER,
          &af, sizeof(af))) < 0) {
        return errno;
    }
    return APR_SUCCESS;
}
#endif
