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

#include "apr_errno.h"
#include <sys/types.h>
#include <sys/socket.h>

extern int (*apr_os2_socket)(int, int, int);
extern int (*apr_os2_select)(int *, int, int, int, long);
extern int (*apr_os2_sock_errno)();
extern int (*apr_os2_accept)(int, struct sockaddr *, int *);
extern int (*apr_os2_bind)(int, struct sockaddr *, int);
extern int (*apr_os2_connect)(int, struct sockaddr *, int);
extern int (*apr_os2_getpeername)(int, struct sockaddr *, int *);
extern int (*apr_os2_getsockname)(int, struct sockaddr *, int *);
extern int (*apr_os2_getsockopt)(int, int, int, char *, int *);
extern int (*apr_os2_ioctl)(int, int, caddr_t, int);
extern int (*apr_os2_listen)(int, int);
extern int (*apr_os2_recv)(int, char *, int, int);
extern int (*apr_os2_send)(int, const char *, int, int);
extern int (*apr_os2_setsockopt)(int, int, int, char *, int);
extern int (*apr_os2_shutdown)(int, int);
extern int (*apr_os2_soclose)(int);
extern int (*apr_os2_writev)(int, struct iovec *, int);
extern int (*apr_os2_sendto)(int, const char *, int, int, const struct sockaddr *, int);
extern int (*apr_os2_recvfrom)(int, char *, int, int, struct sockaddr *, int *);

#define socket apr_os2_socket
#define select apr_os2_select
#define sock_errno apr_os2_sock_errno
#define accept apr_os2_accept
#define bind apr_os2_bind
#define connect apr_os2_connect
#define getpeername apr_os2_getpeername
#define getsockname apr_os2_getsockname
#define getsockopt apr_os2_getsockopt
#define ioctl apr_os2_ioctl
#define listen apr_os2_listen
#define recv apr_os2_recv
#define send apr_os2_send
#define setsockopt apr_os2_setsockopt
#define shutdown apr_os2_shutdown
#define soclose apr_os2_soclose
#define writev apr_os2_writev
#define sendto apr_os2_sendto
#define recvfrom apr_os2_recvfrom
