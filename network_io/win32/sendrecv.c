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

/* MAX_SEGMENT_SIZE is the maximum amount of data that will be sent to a client
 * in one call of TransmitFile. This number must be small enough to give the 
 * slowest client time to receive the data before the socket timeout triggers.
 * The same problem can exist with apr_send(). In that case, we rely on the
 * application to adjust socket timeouts and max send segment sizes appropriately.
 * For example, Apache will in most cases call apr_send() with less than 8193 
 * bytes
 * of data.
 */
#define MAX_SEGMENT_SIZE 65536
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
        lasterror = apr_get_netos_error();
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
        lasterror = apr_get_netos_error();
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
        lasterror = apr_get_netos_error();
        free(pWsaData);
        return lasterror;
    }

    free(pWsaData);

    *nbytes = dwBytes;
    return APR_SUCCESS;
}
static void collapse_iovec(char **buf, int *len, struct iovec *iovec, int numvec, apr_pool_t *p)
{
    int ptr = 0;

    if (numvec == 1) {
        *buf = iovec[0].iov_base;
        *len = iovec[0].iov_len;
    }
    else {
        int i;
        for (i = 0; i < numvec; i++) {
            *len += iovec[i].iov_len;
        }

        *buf = apr_palloc(p, *len); /* Should this be a malloc? */

        for (i = 0; i < numvec; i++) {
            memcpy((char*)*buf + ptr, iovec[i].iov_base, iovec[i].iov_len);
            ptr += iovec[i].iov_len;
        }
    }
}
#if APR_HAS_SENDFILE
/*
 *#define WAIT_FOR_EVENT
 * Note: Waiting for the socket directly is much faster than creating a seperate
 * wait event. There are a couple of dangerous aspects to waiting directly 
 * for the socket. First, we should not wait on the socket if concurrent threads
 * can wait-on/signal the same socket. This shouldn't be happening with Apache since 
 * a socket is uniquely tied to a thread. This will change when we begin using 
 * async I/O with completion ports on the socket. 
 */

/*
 * apr_status_t apr_sendfile(apr_socket_t *, apr_file_t *, apr_hdtr_t *, 
 *                         apr_off_t *, apr_ssize_t *, apr_int32_t flags)
 *    Send a file from an open file descriptor to a socket, along with 
 *    optional headers and trailers
 * arg 1) The socket to which we're writing
 * arg 2) The open file from which to read
 * arg 3) A structure containing the headers and trailers to send
 * arg 4) Offset into the file where we should begin writing
 * arg 5) Number of bytes to send out of the file
 * arg 6) APR flags that are mapped to OS specific flags
 */
apr_status_t apr_sendfile(apr_socket_t * sock, apr_file_t * file,
                          apr_hdtr_t * hdtr, apr_off_t * offset, apr_ssize_t * len,
                          apr_int32_t flags) 
{
    apr_status_t status = APR_SUCCESS;
    apr_ssize_t rv;
    DWORD dwFlags = 0;
    DWORD nbytes;
    OVERLAPPED overlapped;
    TRANSMIT_FILE_BUFFERS tfb, *ptfb = NULL;
    int bytes_to_send;
    int ptr = 0;

    /* Must pass in a valid length */
    if (len == 0) {
        return APR_EINVAL;
    }
    bytes_to_send = *len;
    *len = 0;

    /* Initialize the overlapped structure */
    memset(&overlapped,'\0', sizeof(overlapped));
    if (offset && *offset) {
        overlapped.Offset = *offset;
    }
#ifdef WAIT_FOR_EVENT
    overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif

    /* TransmitFile can only send one header and one footer */
    memset(&tfb, '\0', sizeof (tfb));
    if (hdtr && hdtr->numheaders) {
        ptfb = &tfb;
        collapse_iovec((char **)&ptfb->Head, &ptfb->HeadLength, hdtr->headers, hdtr->numheaders, sock->cntxt);
    }

    /* If we have more than MAX_SEGMENT_SIZE headers to send, send them
     * in segments.
     */
    if (ptfb && ptfb->HeadLength) {
        while (ptfb->HeadLength >= MAX_SEGMENT_SIZE) {
            nbytes = MAX_SEGMENT_SIZE;
            rv = apr_send(sock, ptfb->Head, &nbytes);
            if (rv != APR_SUCCESS)
                return rv;
            (char*) ptfb->Head += nbytes;
            ptfb->HeadLength -= nbytes;
            *len += nbytes;
        }
    }

    while (bytes_to_send) {
        if (bytes_to_send > MAX_SEGMENT_SIZE) {
            nbytes = MAX_SEGMENT_SIZE;
        }
        else {
            /* Last call to TransmitFile() */
            nbytes = bytes_to_send;
            /* Send trailers on the last packet, even if the total size 
             * exceeds MAX_SEGMENT_SIZE...
             */
            if (hdtr && hdtr->numtrailers) {
                ptfb = &tfb;
                collapse_iovec((char**) &ptfb->Tail, &ptfb->TailLength, 
                               hdtr->trailers, hdtr->numtrailers, sock->cntxt);
            }
            /* Disconnect the socket after last send */
            if (flags & APR_SENDFILE_DISCONNECT_SOCKET) {
                dwFlags |= TF_REUSE_SOCKET;
                dwFlags |= TF_DISCONNECT;
            }
        }

        rv = TransmitFile(sock->sock,     /* socket */
                          file->filehand, /* open file descriptor of the file to be sent */
                          nbytes,         /* number of bytes to send. 0=send all */
                          0,              /* Number of bytes per send. 0=use default */
                          &overlapped,    /* OVERLAPPED structure */
                          ptfb,           /* header and trailer buffers */
                          dwFlags);       /* flags to control various aspects of TransmitFile */
        if (!rv) {
            status = apr_get_netos_error();
            if (status == APR_FROM_OS_ERROR(ERROR_IO_PENDING)) {
#ifdef WAIT_FOR_EVENT
                rv = WaitForSingleObject(overlapped.hEvent, 
                                         sock->timeout >= 0 ? sock->timeout : INFINITE);
#else
                rv = WaitForSingleObject((HANDLE) sock->sock, 
                                         sock->timeout >= 0 ? sock->timeout : INFINITE);
#endif
                if (rv == WAIT_OBJECT_0)
                    status = APR_SUCCESS;
                else if (rv == WAIT_TIMEOUT)
                    status = WAIT_TIMEOUT;
                else if (rv == WAIT_ABANDONED)
                    status = WAIT_ABANDONED;
                else
                    status = apr_get_os_error();
            }
        }
        if (status != APR_SUCCESS)
            break;

        /* Assume the headers have been sent */
        ptfb->HeadLength = 0;
        ptfb->Head = NULL;
        bytes_to_send -= nbytes;
        *len += nbytes;
        overlapped.Offset += nbytes;
    }


    if (status == APR_SUCCESS) {
        if (ptfb && ptfb->TailLength)
            *len += ptfb->TailLength;

        /* Mark the socket as disconnected, but do not close it.
         * Note: The application must have stored the socket prior to making
         * the call to apr_sendfile in order to either reuse it or close it.
         */
        if (flags & APR_SENDFILE_DISCONNECT_SOCKET) {
            sock->disconnected = 1;
            sock->sock = INVALID_SOCKET;
        }
    }

#ifdef WAIT_FOR_EVENT
    CloseHandle(overlapped.hEvent);
#endif
    return status;
}
#endif
