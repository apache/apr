/* ====================================================================
 * Copyright (c) 1996-1999 The Apache Group.  All rights reserved.
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
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "networkio.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_lib.h"
#include "fileio.h"
#include <time.h>

ap_status_t ap_send(struct socket_t *sock, const char *buf, ap_ssize_t *len)
{
    ap_ssize_t rv;
    WSABUF wsaData;
    int lasterror;
    DWORD dwBytes = 0;
    int timeout = sock->timeout * 1000; /* Need timeout in milliseconds */

    wsaData.len = *len;
    wsaData.buf = (char*) buf;

    rv = setsockopt(sock->sock, SOL_SOCKET, SO_SNDTIMEO, (char*) &timeout, 
                    sizeof(timeout));
    if (rv == SOCKET_ERROR) {
        return WSAGetLastError();
    }

    rv = WSASend(sock->sock, &wsaData, 1, &dwBytes, 0, NULL, NULL);
    if (rv == SOCKET_ERROR) {
        lasterror = WSAGetLastError();
        /* Test code: Remove before release */
        if (lasterror == WSAETIMEDOUT)
            printf("wsasend: Connection timed out\n");
        else
            printf("wsasend: connection failed. lasterror = %d\n", lasterror);            

        return lasterror;
    }

    *len = dwBytes;

    return APR_SUCCESS;
}

ap_status_t ap_recv(struct socket_t *sock, char *buf, ap_ssize_t *len) 
{
    ap_ssize_t rv;
    WSABUF wsaData;
    int lasterror;
    DWORD dwBytes = 0;
    DWORD flags = 0;
    int timeout = sock->timeout * 1000; /* Need timeout in milliseconds */

    wsaData.len = *len;
    wsaData.buf = (char*) buf;

    rv = setsockopt(sock->sock, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, 
                    sizeof(timeout));
    if (rv == SOCKET_ERROR) {
        return WSAGetLastError();
    }

    rv = WSARecv(sock->sock, &wsaData, 1, &dwBytes, &flags, NULL, NULL);
    if (rv == SOCKET_ERROR) {
        lasterror = WSAGetLastError();
        /* Test code: remove before release */
        if (lasterror == WSAETIMEDOUT)
            printf("wsarecv: Connection timed out\n");
        else
            printf("wsarecv: connection failed. lasterror = %d\n", lasterror);            
        return lasterror;
    }

    *len = dwBytes;
    return APR_SUCCESS;
}

ap_status_t ap_sendv(struct socket_t *sock, const struct iovec *vec,
                     ap_int32_t nvec, ap_int32_t *nbytes)
{
    ap_ssize_t rv;
    int i;
    int lasterror;
    DWORD dwBytes = 0;
    int timeout = sock->timeout * 1000; /* Need timeout in milliseconds */

    LPWSABUF pWsaData = (LPWSABUF) malloc(sizeof(WSABUF) * nvec);

    if (!pWsaData)
        return APR_ENOMEM;

    for (i = 0; i < nvec; i++) {
        pWsaData[i].buf = vec[i].iov_base;
        pWsaData[i].len = vec[i].iov_len;
    }

    rv = setsockopt(sock->sock, SOL_SOCKET, SO_SNDTIMEO, (char*) &timeout, 
                    sizeof(timeout));
    if (rv == SOCKET_ERROR) {
        lasterror = WSAGetLastError();
        /* Test code: remove before release */
        printf("win32_writev: setsockopt failed. errno = %d\n", lasterror);
        free(pWsaData);
        return lasterror;
    }

    rv = WSASend(sock->sock, pWsaData, nvec, &dwBytes, 0, NULL, NULL);
    if (rv == SOCKET_ERROR) {
        lasterror = WSAGetLastError();
        /* Test code: remove before release */
        if (lasterror == WSAETIMEDOUT)
            printf("wsasend: Connection timed out\n");
        else
            printf("wsasend: connection failed. lasterror = %d\n", lasterror);            

        free(pWsaData);
        return lasterror;
    }

    free(pWsaData);

    *nbytes = dwBytes;
    return APR_SUCCESS;
}
#if defined(HAVE_SENDFILE)
/*
 * ap_status_t ap_sendfile(ap_socket_t *, ap_file_t *, ap_hdtr_t *, 
 *                         ap_off_t *, ap_size_t *, ap_int32_t flags)
 *    Send a file from an open file descriptor to a socket, along with 
 *    optional headers and trailers
 * arg 1) The socket to which we're writing
 * arg 2) The open file from which to read
 * arg 3) A structure containing the headers and trailers to send
 * arg 4) Offset into the file where we should begin writing
 * arg 5) Number of bytes to send 
 * arg 6) OS-specific flags to pass to sendfile()
 */
ap_status_t ap_sendfile(ap_socket_t * sock, ap_file_t * file,
        		ap_hdtr_t * hdtr, ap_off_t * offset, ap_size_t * len,
        		ap_int32_t flags) 
{
    ap_ssize_t rv;
#ifdef OVERLAPPED
    OVERLAPPED overlapped;
#endif
    TRANSMIT_FILE_BUFFERS tfb, *ptfb = NULL;
    int i, lasterror, ptr = 0;
    DWORD dwFlags = 0;
    int timeout = sock->timeout * 1000; /* Need timeout in milliseconds */

    rv = setsockopt(sock->sock, SOL_SOCKET, SO_SNDTIMEO,
               (char*) &timeout, sizeof(timeout));
#if 0
    if (flags | APR_SENDFILE_KEEP_SOCKET)
        dwFlags |= TF_REUSE_SOCKET;
    if (flags | APR_SENDFILE_CLOSE_SOCKET)
        dwFlags |= TF_DISCONNECT;
#else
    dwFlags = 0;
#endif

    /* TransmitFile can only send one header and one footer */
    memset(&tfb, '0', sizeof (tfb));
    if (hdtr->numheaders) {
        ptfb = &tfb;
        for (i = 0; i < hdtr->numheaders; i++) {
            tfb.HeadLength += hdtr->headers[i].iov_len;
        }

        tfb.Head = ap_palloc(sock->cntxt, tfb.HeadLength); /* Should this be a malloc? */

        for (i = 0; i < hdtr->numheaders; i++) {
            memcpy(&tfb.Head + ptr, hdtr->headers[i].iov_base,
                   hdtr->headers[i].iov_len);
            ptr += hdtr->headers[i].iov_len;
        }
    }
    if (hdtr->numtrailers) {
        ptfb = &tfb;
        for (i = 0; i < hdtr->numtrailers; i++) {
            tfb.TailLength += hdtr->headers[i].iov_len;
        }

        tfb.Tail = ap_palloc(sock->cntxt, tfb.TailLength); /* Should this be a malloc? */

        for (i = 0; i < hdtr->numtrailers; i++) {
            memcpy(&tfb.Tail + ptr, hdtr->trailers[i].iov_base,
                   hdtr->trailers[i].iov_len);
            ptr += hdtr->trailers[i].iov_len;
        }


    }
#ifdef OVERLAPPED
    memset(&overlapped,'0', sizeof(overlapped));
#endif
    rv = TransmitFile(sock->sock,     /* socket */
                      file->filehand, /* open file descriptor of the file to be sent */
                      *len,           /* number of bytes to send. 0=send all */
                      0,              /* Number of bytes per send. 0=use default */
#ifdef OVERLAPPED
                      &overlapped,
#else
                      NULL,           /* OVERLAPPED structure */
#endif
                      ptfb,           /* header and trailer buffers */
                      dwFlags);       /* flags to control various aspects of TransmitFile */
    if (!rv) {
        lasterror = WSAGetLastError();
        printf("TransmitFile failed with error %d\n", lasterror);
        return lasterror;
    }

    return APR_SUCCESS;
}
#endif
