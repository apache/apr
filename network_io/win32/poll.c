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
#include "apr_network_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <errno.h>
#include <windows.h>
#include <time.h>


ap_status_t ap_setup_poll(ap_pollfd_t **new, ap_int32_t num, ap_pool_t *cont)
{
    (*new) = (ap_pollfd_t *)ap_palloc(cont, sizeof(ap_pollfd_t) * num);
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont;
    (*new)->read = (fd_set *)ap_palloc(cont, sizeof(fd_set));
    (*new)->write = (fd_set *)ap_palloc(cont, sizeof(fd_set));
    (*new)->except = (fd_set *)ap_palloc(cont, sizeof(fd_set));
    FD_ZERO((*new)->read);
    (*new)->numread = 0;
    FD_ZERO((*new)->write);
    (*new)->numwrite = 0;
    FD_ZERO((*new)->except);
    (*new)->numexcept = 0;
    return APR_SUCCESS;
}

ap_status_t ap_add_poll_socket(ap_pollfd_t *aprset, 
			       ap_socket_t *sock, ap_int16_t event)
{
    if (event & APR_POLLIN) {
        FD_SET(sock->sock, aprset->read);
        aprset->numread++;
    }
    if (event & APR_POLLPRI) {
        FD_SET(sock->sock, aprset->read);
        aprset->numexcept++;
    }
    if (event & APR_POLLOUT) {
        FD_SET(sock->sock, aprset->write);
        aprset->numwrite++;
    }
    return APR_SUCCESS;
}

ap_status_t ap_poll(ap_pollfd_t *aprset, ap_int32_t *nsds, ap_int32_t timeout)
{
    int rv;
    struct timeval *thetime;
    fd_set *newread = NULL;
    fd_set *newwrite = NULL;
    fd_set *newexcept = NULL;

    if (timeout == -1) {
        thetime = NULL;
    }
    else {
        /* Convert milli-seconds into seconds and micro-seconds. */
        thetime = (struct timeval *)ap_palloc(aprset->cntxt, sizeof(struct timeval));
        thetime->tv_sec = timeout;
        thetime->tv_usec = 0;
    }

    if (aprset->numread != 0) {
        newread = aprset->read;
    }
    if (aprset->numwrite != 0) {
        newwrite = aprset->write;
    }
    if (aprset->numexcept != 0) {
        newexcept = aprset->except;
    }

    if (newread == NULL && newwrite == NULL && newexcept == NULL) {
        Sleep(100); /* Should sleep  for timeout, but that will be fixed next */
        return APR_TIMEUP;
    }
    else {
        rv = select(500, newread, newwrite, newexcept, thetime);
    }

    (*nsds) = rv;    
    if ((*nsds) < 0) {
        rv = GetLastError();
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_get_revents(ap_int16_t *event, ap_socket_t *sock, ap_pollfd_t *aprset)
{
    ap_int16_t revents = 0;
    WSABUF data;
    int dummy;
    int flags = MSG_PEEK;

    /* We just want to PEEK at the data, so I am setting up a dummy WSABUF
     * variable here.
     */
    data.len = 256;
    data.buf = (char *)ap_palloc(aprset->cntxt, 256);

    if (FD_ISSET(sock->sock, aprset->read)) {
        revents |= APR_POLLIN;
        if (WSARecv(sock->sock, &data, 1, &dummy, &flags, NULL, 
                    NULL) == SOCKET_ERROR) {
            dummy = WSAGetLastError();
            switch (dummy) {
                case WSAECONNRESET:
                case WSAECONNABORTED:
                case WSAESHUTDOWN:
                case WSAENETRESET: {
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLHUP;
                    break;
                }
                case WSAENOTSOCK: {
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLNVAL;
                }
                default: {
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLERR;
                }
            }
        }
    }
    if (FD_ISSET(sock->sock, aprset->write)) {
        revents |= APR_POLLOUT;
    }
    /* I am assuming that the except is for out of band data, not a failed
     * connection on a non-blocking socket.  Might be a bad assumption, but
     * it works for now. rbb.
     */
    if (FD_ISSET(sock->sock, aprset->except)) {
        revents |= APR_POLLPRI;
    }

    (*event) = revents;
    return APR_SUCCESS;
}

ap_status_t ap_get_polldata(ap_pollfd_t *pollfd, char *key, void *data)
{
    if (pollfd != NULL) {
        return ap_get_userdata(data, key, pollfd->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOFILE;
    }
}

ap_status_t ap_set_polldata(ap_pollfd_t *pollfd, void *data, char *key,
                            ap_status_t (*cleanup) (void *))
{
    if (pollfd != NULL) {
        return ap_set_userdata(data, key, cleanup, pollfd->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOFILE;
    }
}

ap_status_t ap_remove_poll_socket(ap_pollfd_t *aprset, 
                                  ap_socket_t *sock, ap_int16_t events)
{
    if (events & APR_POLLIN) {
        FD_CLR(sock->sock, aprset->read);
        aprset->numread--;
    }
    if (events & APR_POLLPRI) {
        FD_CLR(sock->sock, aprset->read);
        aprset->numexcept--;
    }
    if (events & APR_POLLOUT) {
        FD_CLR(sock->sock, aprset->write);
        aprset->numwrite--;
    }
    return APR_SUCCESS;
}

ap_status_t ap_clear_poll_sockets(ap_pollfd_t *aprset, ap_int16_t events)
{
    if (events & APR_POLLIN) {
        FD_ZERO(aprset->read);
        aprset->numread = 0;
    }
    if (events & APR_POLLPRI) {
        FD_ZERO(aprset->read);
        aprset->numexcept = 0;
    }
    if (events & APR_POLLOUT) {
        FD_ZERO(aprset->write);
        aprset->numwrite = 0;
    }
    return APR_SUCCESS;
}
