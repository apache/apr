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

#include "networkio.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_lib.h"
#include "fileio.h"
#include <time.h>

apr_status_t apr_send(apr_socket_t *sock, const char *buf, apr_ssize_t *len)
{
    apr_ssize_t rv;
    WSABUF wsaData;
    int lasterror;
    DWORD dwBytes = 0;

    wsaData.len = *len;
    wsaData.buf = (char*) buf;

    rv = WSASend(sock->sock, &wsaData, 1, &dwBytes, 0, NULL, NULL);
    if (rv == SOCKET_ERROR) {
        lasterror = WSAGetLastError();
        return lasterror;
    }

    *len = dwBytes;

    return APR_SUCCESS;
}

apr_status_t apr_recv(apr_socket_t *sock, char *buf, apr_ssize_t *len) 
{
    apr_ssize_t rv;
    WSABUF wsaData;
    int lasterror;
    DWORD dwBytes = 0;
    DWORD flags = 0;

    wsaData.len = *len;
    wsaData.buf = (char*) buf;

    rv = WSARecv(sock->sock, &wsaData, 1, &dwBytes, &flags, NULL, NULL);
    if (rv == SOCKET_ERROR) {
        lasterror = WSAGetLastError();
        *len = 0;
        return lasterror;
    }

    *len = dwBytes;
    return APR_SUCCESS;

}

apr_status_t apr_sendv(apr_socket_t *sock, const struct iovec *vec,
                     apr_int32_t nvec, apr_int32_t *nbytes)
{
    apr_ssize_t rv;
    int i;
    int lasterror;
    DWORD dwBytes = 0;

    LPWSABUF pWsaData = (LPWSABUF) malloc(sizeof(WSABUF) * nvec);

    if (!pWsaData)
        return APR_ENOMEM;

    for (i = 0; i < nvec; i++) {
        pWsaData[i].buf = vec[i].iov_base;
        pWsaData[i].len = vec[i].iov_len;
    }

    rv = WSASend(sock->sock, pWsaData, nvec, &dwBytes, 0, NULL, NULL);
    if (rv == SOCKET_ERROR) {
        lasterror = WSAGetLastError();
        free(pWsaData);
        return lasterror;
    }

    free(pWsaData);

    *nbytes = dwBytes;
    return APR_SUCCESS;
}
#if APR_HAS_SENDFILE
/*
 * apr_status_t ap_sendfile(apr_socket_t *, apr_file_t *, apr_hdtr_t *, 
 *                         apr_off_t *, apr_size_t *, apr_int32_t flags)
 *    Send a file from an open file descriptor to a socket, along with 
 *    optional headers and trailers
 * arg 1) The socket to which we're writing
 * arg 2) The open file from which to read
 * arg 3) A structure containing the headers and trailers to send
 * arg 4) Offset into the file where we should begin writing
 * arg 5) Number of bytes to send 
 * arg 6) APR flags that are mapped to OS specific flags
 */
apr_status_t ap_sendfile(apr_socket_t * sock, apr_file_t * file,
        		apr_hdtr_t * hdtr, apr_off_t * offset, apr_size_t * len,
        		apr_int32_t flags) 
{
/*
 *#define WAIT_FOR_EVENT
 * Note: Waiting for the socket directly is much faster than creating a seperate
 * wait event. There are a couple of dangerous aspects to waiting directly 
 * for the socket. First, we should not wait on the socket if concurrent threads
 * can wait-on/signal the same socket. This shouldn't be happening with Apache since 
 * a socket is uniquely tied to a thread. This will change when we begin using 
 * async I/O with completion ports on the socket. Second, I am not sure how the
 * socket timeout code will work. I am hoping the socket will be signaled if the
 * setsockopt timeout expires. Need to verify this...
 */
    apr_ssize_t rv;
    OVERLAPPED overlapped;
    TRANSMIT_FILE_BUFFERS tfb, *ptfb = NULL;
    int i, ptr = 0;
    int lasterror = APR_SUCCESS;
    DWORD dwFlags = 0;

    /* Map APR flags to OS specific flags */
    if (flags & APR_SENDFILE_DISCONNECT_SOCKET) {
        dwFlags |= TF_REUSE_SOCKET;
        dwFlags |= TF_DISCONNECT;
    }

    /* TransmitFile can only send one header and one footer */
    memset(&tfb, '\0', sizeof (tfb));
    if (hdtr && hdtr->numheaders) {
        ptfb = &tfb;
        if (hdtr->numheaders == 1) {
            ptfb->Head = hdtr->headers[0].iov_base;
            ptfb->HeadLength = hdtr->headers[0].iov_len;
        }
        else {
            /* Need to collapse all the header fragments into one buffer */
            for (i = 0; i < hdtr->numheaders; i++) {
                ptfb->HeadLength += hdtr->headers[i].iov_len;
            }
            ptfb->Head = apr_palloc(sock->cntxt, ptfb->HeadLength); /* Should this be a malloc? */

            for (i = 0; i < hdtr->numheaders; i++) {
                memcpy((char*)ptfb->Head + ptr, hdtr->headers[i].iov_base,
                       hdtr->headers[i].iov_len);
                ptr += hdtr->headers[i].iov_len;
            }
        }
    }
    if (hdtr && hdtr->numtrailers) {
        ptfb = &tfb;
        if (hdtr->numtrailers == 1) {
            ptfb->Tail = hdtr->trailers[0].iov_base;
            ptfb->TailLength = hdtr->trailers[0].iov_len;
        }
        else {
            /* Need to collapse all the trailer fragments into one buffer */
            for (i = 0; i < hdtr->numtrailers; i++) {
                ptfb->TailLength += hdtr->headers[i].iov_len;
            }

            ptfb->Tail = apr_palloc(sock->cntxt, ptfb->TailLength); /* Should this be a malloc? */

            for (i = 0; i < hdtr->numtrailers; i++) {
                memcpy((char*)ptfb->Tail + ptr, hdtr->trailers[i].iov_base,
                       hdtr->trailers[i].iov_len);
                ptr += hdtr->trailers[i].iov_len;
            }
        }
    }
    /* Initialize the overlapped structure */
    memset(&overlapped,'\0', sizeof(overlapped));
    if (offset && *offset) {
        overlapped.Offset = *offset;
    }
#ifdef WAIT_FOR_EVENT
    overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
 
    rv = TransmitFile(sock->sock,     /* socket */
                      file->filehand, /* open file descriptor of the file to be sent */
                      *len,           /* number of bytes to send. 0=send all */
                      0,              /* Number of bytes per send. 0=use default */
                      &overlapped,    /* OVERLAPPED structure */
                      ptfb,           /* header and trailer buffers */
                      dwFlags);       /* flags to control various aspects of TransmitFile */
    if (!rv) {
        lasterror = WSAGetLastError();
        if (lasterror == ERROR_IO_PENDING) {
#ifdef WAIT_FOR_EVENT
            rv = WaitForSingleObject(overlapped.hEvent, 
                                     sock->timeout >= 0 ? sock->timeout / 1000 : INFINITE);
#else
            rv = WaitForSingleObject((HANDLE) sock->sock, 
                                     sock->timeout >= 0 ? sock->timeout / 1000 : INFINITE);
#endif
            if (rv == WAIT_OBJECT_0)
                lasterror = APR_SUCCESS;
            else if (rv == WAIT_TIMEOUT)
                lasterror = WAIT_TIMEOUT;
            else if (rv == WAIT_ABANDONED)
                lasterror = WAIT_ABANDONED;
            else
                lasterror = GetLastError();
        }
    }

    /* Mark the socket as disconnected, but do not close it.
     * Note: The application must have stored the socket prior to making
     * the call to ap_sendfile in order to either reuse it or close it.
     */
    if ((lasterror == APR_SUCCESS) && (flags & APR_SENDFILE_DISCONNECT_SOCKET)) {
        sock->disconnected = 1;
        sock->sock = INVALID_SOCKET;
    }

#ifdef WAIT_FOR_EVENT
    CloseHandle(overlapped.hEvent);
#endif
    return lasterror;
}
#endif
