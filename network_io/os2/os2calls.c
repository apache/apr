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
#include "apr_network_io.h"
#include "apr_portable.h"
#include "apr_general.h"
#include "apr_lib.h"

static int os2_socket_init(int, int ,int);

int (*apr_os2_socket)(int, int, int) = os2_socket_init;
int (*apr_os2_select)(int *, int, int, int, long) = NULL;
int (*apr_os2_sock_errno)() = NULL;
int (*apr_os2_accept)(int, struct sockaddr *, int *) = NULL;
int (*apr_os2_bind)(int, struct sockaddr *, int) = NULL;
int (*apr_os2_connect)(int, struct sockaddr *, int) = NULL;
int (*apr_os2_getpeername)(int, struct sockaddr *, int *) = NULL;
int (*apr_os2_getsockname)(int, struct sockaddr *, int *) = NULL;
int (*apr_os2_getsockopt)(int, int, int, char *, int *) = NULL;
int (*apr_os2_ioctl)(int, int, caddr_t, int) = NULL;
int (*apr_os2_listen)(int, int) = NULL;
int (*apr_os2_recv)(int, char *, int, int) = NULL;
int (*apr_os2_send)(int, const char *, int, int) = NULL;
int (*apr_os2_setsockopt)(int, int, int, char *, int) = NULL;
int (*apr_os2_shutdown)(int, int) = NULL;
int (*apr_os2_soclose)(int) = NULL;
int (*apr_os2_writev)(int, struct iovec *, int) = NULL;
int (*apr_os2_sendto)(int, const char *, int, int, const struct sockaddr *, int);
int (*apr_os2_recvfrom)(int, char *, int, int, struct sockaddr *, int *);

static HMODULE hSO32DLL;

static int os2_fn_link()
{
    DosEnterCritSec(); /* Stop two threads doing this at the same time */

    if (apr_os2_socket == os2_socket_init) {
        ULONG rc;
        char errorstr[200];

        rc = DosLoadModule(errorstr, sizeof(errorstr), "SO32DLL", &hSO32DLL);

        if (rc)
            return APR_OS2_STATUS(rc);

        rc = DosQueryProcAddr(hSO32DLL, 0, "SOCKET", &apr_os2_socket);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "SELECT", &apr_os2_select);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "SOCK_ERRNO", &apr_os2_sock_errno);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "ACCEPT", &apr_os2_accept);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "BIND", &apr_os2_bind);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "CONNECT", &apr_os2_connect);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "GETPEERNAME", &apr_os2_getpeername);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "GETSOCKNAME", &apr_os2_getsockname);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "GETSOCKOPT", &apr_os2_getsockopt);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "IOCTL", &apr_os2_ioctl);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "LISTEN", &apr_os2_listen);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "RECV", &apr_os2_recv);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "SEND", &apr_os2_send);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "SETSOCKOPT", &apr_os2_setsockopt);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "SHUTDOWN", &apr_os2_shutdown);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "SOCLOSE", &apr_os2_soclose);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "WRITEV", &apr_os2_writev);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "SENDTO", &apr_os2_sendto);

        if (!rc)
            rc = DosQueryProcAddr(hSO32DLL, 0, "RECVFROM", &apr_os2_recvfrom);

        if (rc)
            return APR_OS2_STATUS(rc);
    }

    DosExitCritSec();
    return APR_SUCCESS;
}



static int os2_socket_init(int domain, int type, int protocol)
{
    int rc = os2_fn_link();
    if (rc == APR_SUCCESS)
        return apr_os2_socket(domain, type, protocol);
    return rc;
}
