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

#ifndef NETWORK_IO_H
#define NETWORK_IO_H

#include "apr_private.h"
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_arch_os2calls.h"
#if APR_HAVE_NETDB_H
#include <netdb.h>
#endif

typedef struct sock_userdata_t sock_userdata_t;
struct sock_userdata_t {
    sock_userdata_t *next;
    const char *key;
    void *data;
};

struct apr_socket_t {
    apr_pool_t *cntxt;
    int socketdes;
    int type;
    int protocol;
    apr_sockaddr_t *local_addr;
    apr_sockaddr_t *remote_addr;
    apr_interval_time_t timeout;
    int nonblock;
    int local_port_unknown;
    int local_interface_unknown;
    int remote_addr_unknown;
    apr_int32_t netmask;
    apr_int32_t inherit;
    sock_userdata_t *userdata;
};

/* Error codes returned from sock_errno() */
#define SOCBASEERR              10000
#define SOCEPERM                (SOCBASEERR+1)             /* Not owner */
#define SOCESRCH                (SOCBASEERR+3)             /* No such process */
#define SOCEINTR                (SOCBASEERR+4)             /* Interrupted system call */
#define SOCENXIO                (SOCBASEERR+6)             /* No such device or address */
#define SOCEBADF                (SOCBASEERR+9)             /* Bad file number */
#define SOCEACCES               (SOCBASEERR+13)            /* Permission denied */
#define SOCEFAULT               (SOCBASEERR+14)            /* Bad address */
#define SOCEINVAL               (SOCBASEERR+22)            /* Invalid argument */
#define SOCEMFILE               (SOCBASEERR+24)            /* Too many open files */
#define SOCEPIPE                (SOCBASEERR+32)            /* Broken pipe */
#define SOCEOS2ERR              (SOCBASEERR+100)            /* OS/2 Error */

const char *apr_inet_ntop(int af, const void *src, char *dst, apr_size_t size);
int apr_inet_pton(int af, const char *src, void *dst);
void apr_sockaddr_vars_set(apr_sockaddr_t *, int, apr_port_t);

#endif  /* ! NETWORK_IO_H */

