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
#include "apr_lib.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

apr_status_t apr_set_port(apr_sockaddr_t *sockaddr, apr_port_t port)
{
    /* XXX IPv6: assumes sin_port and sin6_port at same offset */
    sockaddr->sa.sin.sin_port = htons(port);
    return APR_SUCCESS;
}

/* XXX assumes IPv4... I don't think this function is needed anyway
 * since we have apr_getaddrinfo(), but we need to clean up Apache's 
 * listen.c a bit more first.
 */
apr_status_t apr_set_ipaddr(apr_sockaddr_t *sockaddr, const char *addr)
{
    u_long ipaddr;
    
    if (!strcmp(addr, APR_ANYADDR)) {
        sockaddr->sa.sin.sin_addr.s_addr = htonl(INADDR_ANY);
        return APR_SUCCESS;
    }
    
    ipaddr = inet_addr(addr);
    if (ipaddr == (u_long)-1) {
#ifdef WIN32
        return WSAEADDRNOTAVAIL;
#else
        return errno;
#endif
    }
    
    sockaddr->sa.sin.sin_addr.s_addr = ipaddr;
    return APR_SUCCESS;
}

apr_status_t apr_get_port(apr_port_t *port, apr_sockaddr_t *sockaddr)
{
    /* XXX IPv6 - assumes sin_port and sin6_port at same offset */
    *port = ntohs(sockaddr->sa.sin.sin_port);
    return APR_SUCCESS;
}

apr_status_t apr_get_ipaddr(char **addr, apr_sockaddr_t *sockaddr)
{
    *addr = apr_palloc(sockaddr->pool, sockaddr->addr_str_len);
    apr_inet_ntop(sockaddr->sa.sin.sin_family,
                  sockaddr->ipaddr_ptr,
                  *addr,
                  sockaddr->addr_str_len);
#if APR_HAVE_IPV6
    if (sockaddr->sa.sin.sin_family == AF_INET6 &&
        IN6_IS_ADDR_V4MAPPED(sockaddr->ipaddr_ptr)) {
        /* This is an IPv4-mapped IPv6 address; drop the leading
         * part of the address string so we're left with the familiar
         * IPv4 format.
         */
        *addr += strlen("::ffff:");
    }
#endif
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
    if (!he || he->h_addrtype != APR_INET || !he->h_addr_list[0])
        return (h_errno + APR_OS_START_SYSERR);

    *addr = *(struct in_addr*)he->h_addr_list[0];

    return APR_SUCCESS;
}

static void set_sockaddr_vars(apr_sockaddr_t *addr, int family)
{
    addr->sa.sin.sin_family = family;

    if (family == APR_INET) {
        addr->salen = sizeof(struct sockaddr_in);
        addr->addr_str_len = 16;
        addr->ipaddr_ptr = &(addr->sa.sin.sin_addr);
        addr->ipaddr_len = sizeof(struct in_addr);
    }
#if APR_HAVE_IPV6
    else if (family == APR_INET6) {
        addr->salen = sizeof(struct sockaddr_in6);
        addr->addr_str_len = 46;
        addr->ipaddr_ptr = &(addr->sa.sin6.sin6_addr);
        addr->ipaddr_len = sizeof(struct in6_addr);
    }
#endif
}

apr_status_t apr_get_sockaddr(apr_sockaddr_t **sa, apr_interface_e which, apr_socket_t *sock)
{
    if (which == APR_LOCAL) {
        if (sock->local_interface_unknown || sock->local_port_unknown) {
            apr_status_t rv = get_local_addr(sock);

            if (rv != APR_SUCCESS) {
                return rv;
            }
        }
        *sa = sock->local_addr;
    }
    else if (which == APR_REMOTE) {
        *sa = sock->remote_addr;
    }
    else {
        *sa = NULL;
        return APR_EINVAL;
    }
    return APR_SUCCESS;
}

apr_status_t apr_parse_addr_port(char **addr,
                                 char **scope_id,
                                 apr_port_t *port,
                                 const char *str,
                                 apr_pool_t *p)
{
    const char *ch, *lastchar;
    int big_port;
    apr_size_t addrlen;

    *addr = NULL;         /* assume not specified */
    *scope_id = NULL;     /* assume not specified */
    *port = 0;            /* assume not specified */

    /* First handle the optional port number.  That may be all that
     * is specified in the string.
     */
    ch = lastchar = str + strlen(str) - 1;
    while (ch >= str && apr_isdigit(*ch)) {
        --ch;
    }

    if (ch < str) {       /* Entire string is the port. */
        big_port = atoi(str);
        if (big_port < 1 || big_port > 65535) {
            return APR_EINVAL;
        }
        *port = big_port;
        return APR_SUCCESS;
    }

    if (*ch == ':' && ch < lastchar) { /* host and port number specified */
        if (ch == str) {               /* string starts with ':' -- bad */
            return APR_EINVAL;
        }
        big_port = atoi(ch + 1);
        if (big_port < 1 || big_port > 65535) {
            return APR_EINVAL;
        }
        *port = big_port;
        lastchar = ch - 1;
    }

    /* now handle the hostname */
    addrlen = lastchar - str + 1;

#if APR_HAVE_IPV6 /* XXX don't require this; would need to pass char[] for ipaddr and always define APR_INET6 */
    if (*str == '[') {
        const char *end_bracket = memchr(str, ']', addrlen);
        struct in6_addr ipaddr;
        const char *scope_delim;

        if (!end_bracket || end_bracket != lastchar) {
            *port = 0;
            return APR_EINVAL;
        }

        /* handle scope id; this is the only context where it is allowed */
        scope_delim = memchr(str, '%', addrlen);
        if (scope_delim) {
            if (scope_delim == end_bracket - 1) { /* '%' without scope identifier */
                *port = 0;
                return APR_EINVAL;
            }
            addrlen = scope_delim - str - 1;
            *scope_id = apr_palloc(p, end_bracket - scope_delim);
            memcpy(*scope_id, scope_delim + 1, end_bracket - scope_delim - 1);
            (*scope_id)[end_bracket - scope_delim - 1] = '\0';
        }
        else {
            addrlen = addrlen - 2; /* minus 2 for '[' and ']' */
        }

        *addr = apr_palloc(p, addrlen + 1);
        memcpy(*addr,
               str + 1,
               addrlen);
        (*addr)[addrlen] = '\0';
        if (apr_inet_pton(AF_INET6, *addr, &ipaddr) != 1) {
            *addr = NULL;
            *scope_id = NULL;
            *port = 0;
            return APR_EINVAL;
        }
    }
    else 
#endif
    {
        /* XXX If '%' is not a valid char in a DNS name, we *could* check 
         *     for bogus scope ids first.
         */
        *addr = apr_palloc(p, addrlen + 1);
        memcpy(*addr, str, addrlen);
        (*addr)[addrlen] = '\0';
    }
    return APR_SUCCESS;
}

apr_status_t apr_getaddrinfo(apr_sockaddr_t **sa, const char *hostname, 
                             apr_int32_t family, apr_port_t port,
                             apr_int32_t flags, apr_pool_t *p)
{
    (*sa) = (apr_sockaddr_t *)apr_pcalloc(p, sizeof(apr_sockaddr_t));
    if ((*sa) == NULL)
        return APR_ENOMEM;
    (*sa)->pool = p;
    (*sa)->hostname = apr_pstrdup(p, hostname);

#if defined(HAVE_GETADDRINFO) && APR_HAVE_IPV6
    if (hostname != NULL) {
        struct addrinfo hints, *ai;
        int error;
        char num[8];

        memset(&hints, 0, sizeof(hints));
        hints.ai_flags = AI_CANONNAME;
        hints.ai_family = family;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;
        apr_snprintf(num, sizeof(num), "%d", port);
        error = getaddrinfo(hostname, num, &hints, &ai);
        if (error) {
            if (error == EAI_SYSTEM) {
                return errno;
            }
            else {
                /* XXX fixme!
                 * no current way to represent this with APR's error
                 * scheme... note that glibc uses negative values for these
                 * numbers, perhaps so they don't conflict with h_errno
                 * values...  Tru64 uses positive values which conflict
                 * with h_errno values
                 */
                return error + APR_OS_START_SYSERR;
            }
        }
        (*sa)->sa.sin.sin_family = ai->ai_family;
        memcpy(&(*sa)->sa, ai->ai_addr, ai->ai_addrlen);
    }
    else {
        if (family == APR_UNSPEC) {
            (*sa)->sa.sin.sin_family = APR_INET;
        }
        else {
            (*sa)->sa.sin.sin_family = family;
        }
    }
    set_sockaddr_vars(*sa, (*sa)->sa.sin.sin_family);
#else
    if (family == APR_UNSPEC) {
        (*sa)->sa.sin.sin_family = APR_INET; /* we don't yet support IPv6 here */
    }
    else {
        (*sa)->sa.sin.sin_family = family;
    }
    set_sockaddr_vars(*sa, (*sa)->sa.sin.sin_family);
    if (hostname != NULL) {
        struct hostent *hp;

#ifndef GETHOSTBYNAME_HANDLES_NAS
        if (*hostname >= '0' && *hostname <= '9' &&
            strspn(hostname, "0123456789.") == strlen(hostname)) {
            (*sa)->sa.sin.sin_addr.s_addr = inet_addr(hostname);
            (*sa)->salen = sizeof(struct sockaddr_in);
        }
        else {
#endif
        hp = gethostbyname(hostname);

        if (!hp)  {
#ifdef WIN32
            apr_get_netos_error();
#else
            return (h_errno + APR_OS_START_SYSERR);
#endif
        }

        memcpy((char *)&(*sa)->sa.sin.sin_addr, hp->h_addr_list[0],
               hp->h_length);
        (*sa)->salen = sizeof(struct sockaddr_in);
        (*sa)->ipaddr_len = hp->h_length;

#ifndef GETHOSTBYNAME_HANDLES_NAS
        }
#endif
    }
#endif
    /* XXX IPv6: assumes sin_port and sin6_port at same offset */
    (*sa)->sa.sin.sin_port = htons(port);
    return APR_SUCCESS;
}

apr_status_t apr_getnameinfo(char **hostname, apr_sockaddr_t *sockaddr, 
                             apr_int32_t flags)
{
#if defined(HAVE_GETNAMEINFO) && APR_HAVE_IPV6
    int rc;
#if defined(NI_MAXHOST)
    char tmphostname[NI_MAXHOST];
#else
    char tmphostname[256];
#endif

    h_errno = 0; /* don't know if it is portable for getnameinfo() to set h_errno */
    rc = getnameinfo((const struct sockaddr *)&sockaddr->sa, sockaddr->salen,
                     tmphostname, sizeof(tmphostname), NULL, 0,
                     /* flags != 0 ? flags : */  NI_NAMEREQD);
    if (rc != 0) {
        *hostname = NULL;
        /* XXX I have no idea if this is okay.  I don't see any info
         * about getnameinfo() returning anything other than good or bad.
         */
        if (h_errno) {
            return h_errno + APR_OS_START_SYSERR;
        }
        else {
            return APR_NOTFOUND;
        }
    }
    *hostname = sockaddr->hostname = apr_pstrdup(sockaddr->pool, 
                                                 tmphostname);
    return APR_SUCCESS;
#else
    struct hostent *hptr;

    hptr = gethostbyaddr((char *)&sockaddr->sa.sin.sin_addr, 
                         sizeof(struct in_addr), 
                         AF_INET);
    if (hptr) {
        *hostname = sockaddr->hostname = apr_pstrdup(sockaddr->pool, hptr->h_name);
        return APR_SUCCESS;
    }
    *hostname = NULL;
#if defined(WIN32)
    return apr_get_netos_error();
#elif defined(OS2)
    return h_errno;
#else
    return h_errno + APR_OS_START_SYSERR;
#endif
#endif
}

apr_status_t apr_getservbyname(apr_sockaddr_t *sockaddr, const char *servname)
{
    struct servent *se;

    if (servname == NULL)
        return APR_EINVAL;

    if ((se = getservbyname(servname, NULL)) != NULL){
        sockaddr->port = htons(se->s_port);
        sockaddr->servname = apr_pstrdup(sockaddr->pool, servname);
        sockaddr->sa.sin.sin_port = se->s_port;
        return APR_SUCCESS;
    }
    return errno;
}

