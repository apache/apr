/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
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

#include "apr_private.h"
#if BEOS_BONE /* BONE uses the unix code - woohoo */
#include "../unix/poll.c"
#else
#include "networkio.h"

/*  BeOS R4 doesn't have a poll function, but R5 will have 
 *  so for the time being we try our best with an implementaion that 
 *  uses select.  However, select on beos isn't that hot either, so 
 *  until R5 we have to live with a less than perfect implementation 
 *
 *  Apparently those sneaky people at Be included support for write in
 *  select for R4.5 of BeOS.  So here we use code that uses the write
 *  bits.
 */

static void ck_ncks(apr_pollfd_t *pfd)
{
    int i;

    pfd->ncks = 0;    
    for (i=pfd->lowsock;i <= pfd->highsock;i++) {
        if (FD_ISSET(i, pfd->read_set)) {
            pfd->ncks++;
        }
        if (FD_ISSET(i, pfd->write_set)) {
            pfd->ncks++;
        }
        if (FD_ISSET(i, pfd->except_set)) {
            pfd->ncks++;
        }
    }
}

APR_DECLARE(apr_status_t) apr_poll_setup(apr_pollfd_t **new, apr_int32_t num, apr_pool_t *cont)
{
    (*new) = (apr_pollfd_t *)apr_pcalloc(cont, sizeof(apr_pollfd_t) * num);

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    (*new)->read       = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->write      = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->except     = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->read_set   = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->write_set  = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    (*new)->except_set = (fd_set *)apr_palloc(cont, sizeof(fd_set));
    FD_ZERO((*new)->read);
    FD_ZERO((*new)->write);
    FD_ZERO((*new)->except);
    FD_ZERO((*new)->read_set);
    FD_ZERO((*new)->write_set);
    FD_ZERO((*new)->except_set);
    (*new)->highsock = -1;
    (*new)->ncks = 0;
    (*new)->lowsock = -1;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_add(apr_pollfd_t *aprset,
                                              apr_socket_t *sock, apr_int16_t event)
{
    if (event & APR_POLLIN) {
        FD_SET(sock->socketdes, aprset->read_set);
        aprset->ncks++;
    }
    if (event & APR_POLLPRI) {
        FD_SET(sock->socketdes, aprset->read_set);
        aprset->ncks++;
    }
    if (event & APR_POLLOUT) {
        FD_SET(sock->socketdes, aprset->write_set);
        aprset->ncks++;
    }
    if (sock->socketdes > aprset->highsock) {
        aprset->highsock = sock->socketdes;
    }
    if (sock->socketdes < aprset->lowsock || aprset->lowsock == -1) {
        aprset->lowsock = sock->socketdes;
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_mask(apr_pollfd_t *aprset, 
                                               apr_socket_t *sock, 
                                               apr_int16_t events)
{
    if (events & APR_POLLIN) {
        FD_CLR(sock->socketdes, aprset->read_set);
        aprset->ncks--;
    }
    if (events & APR_POLLPRI) {
        FD_CLR(sock->socketdes, aprset->except_set);
        aprset->ncks--;
    }
    if (events & APR_POLLOUT) {
        FD_CLR(sock->socketdes, aprset->write_set);
        aprset->ncks--;
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll(apr_pollfd_t *aprset, apr_int32_t *nsds, 
		                           apr_interval_time_t timeout)
{
    int rv;
    struct timeval tv, *tvptr;

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = timeout / APR_USEC_PER_SEC;
        tv.tv_usec = timeout % APR_USEC_PER_SEC;
        tvptr = &tv;
    }

    memcpy(aprset->read,   aprset->read_set,   sizeof(fd_set));
    memcpy(aprset->write,  aprset->write_set,  sizeof(fd_set));
    memcpy(aprset->except, aprset->except_set, sizeof(fd_set));

    rv = select(aprset->highsock + 1, aprset->read, aprset->write, 
                aprset->except, tvptr);

    /* Often we won't see anything beyond the first socket that's
     * available, so here we check for any more...  Without this we 
     * really do what we want to with this call - annoying isn't it?
     */
    if (rv > 0 && rv < aprset->ncks) {
        while (1) {
            fd_set ckr, ckw, cke;
            int i;
            FD_ZERO(&ckr);
            FD_ZERO(&ckw);
            FD_ZERO(&cke);
            for (i=aprset->lowsock;i <= aprset->highsock;i++) {
                if (FD_ISSET(i, aprset->read_set) &&
                   !FD_ISSET(i, aprset->read)) {
                    FD_SET(i, &ckr);
                }
                if (FD_ISSET(i, aprset->write_set) &&
                   !FD_ISSET(i, aprset->write)) {
                    FD_SET(i, &ckw);
                }
                if (FD_ISSET(i, aprset->except_set) &&
                   !FD_ISSET(i, aprset->except)) {
                    FD_SET(i, &cke);
                }
 
            }
            /* set these to zero for an immeadiate return */
            tv.tv_sec = 0;
            tv.tv_usec = 0;
            i = select(aprset->highsock + 1, &ckr, &ckw, &cke, tvptr);
            if (i > 0) {
                for (i=aprset->lowsock;i <= aprset->highsock;i++) {
                    if (FD_ISSET(i, &ckr)) {
                        FD_SET(i, aprset->read);
                        rv++;
                    }
                    if (FD_ISSET(i, &ckw)) {
                        FD_SET(i, aprset->write);
                        rv++;
                    }
                    if (FD_ISSET(i, &cke)) {
                        FD_SET(i, aprset->except);
                        rv++;
                    }
                }
            } else {
                break;
            }
        }
    }       

    (*nsds) = rv;
    if ((*nsds) == 0) {
        return APR_TIMEUP;
    }
    if ((*nsds) < 0) {
        return errno;
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_revents_get(apr_int16_t *event, apr_socket_t *sock, apr_pollfd_t *aprset)
{
    apr_int16_t revents = 0;
    char data[1];
    int flags = 0;

    if (FD_ISSET(sock->socketdes, aprset->read)) {
        revents |= APR_POLLIN;
        if (sock->connected
	    && recv(sock->socketdes, data, 0 /*sizeof data*/, flags) == -1) {
            switch (errno) {
                case ECONNRESET:
                case ECONNABORTED:
                case ESHUTDOWN:
                case ENETRESET:
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLHUP;
                    break;
                case ENOTSOCK:
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLNVAL;
                    break;
                default:
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLERR;
                    break;
            }
        }
    }
    if (FD_ISSET(sock->socketdes, aprset->write)) {
        revents |= APR_POLLOUT;
    }
    /* I am assuming that the except is for out of band data, not a failed
     * connection on a non-blocking socket.  Might be a bad assumption, but
     * it works for now. rbb.
     */
    if (FD_ISSET(sock->socketdes, aprset->except)) {
        revents |= APR_POLLPRI;
    }

    (*event) = revents;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_remove(apr_pollfd_t *aprset, apr_socket_t *sock)
{
    if (FD_ISSET(sock->socketdes, aprset->read_set))
        aprset->ncks--;
    FD_CLR(sock->socketdes, aprset->read_set);
    if (FD_ISSET(sock->socketdes, aprset->except_set))
        aprset->ncks--;
    FD_CLR(sock->socketdes, aprset->except_set);
    if (FD_ISSET(sock->socketdes, aprset->write_set))
        aprset->ncks--;
    FD_CLR(sock->socketdes, aprset->write_set);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_clear(apr_pollfd_t *aprset, apr_int16_t event)
{
    if (event & APR_POLLIN) {
        FD_ZERO(aprset->read_set);
    }
    if (event & APR_POLLPRI) {
        FD_ZERO(aprset->except_set);
    }
    if (event & APR_POLLOUT) {
        FD_ZERO(aprset->write_set);
    }
    aprset->highsock = 0;
    ck_ncks(aprset);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_data_get(apr_pollfd_t *pollfd, const char *key, void *data)
{
    return apr_pool_userdata_get(data, key, pollfd->cntxt);
}

APR_DECLARE(apr_status_t) apr_poll_data_set(apr_pollfd_t *pollfd, void *data, const char *key,
                                            apr_status_t (*cleanup) (void *))
{
    return apr_pool_userdata_set(data, key, cleanup, pollfd->cntxt);
}

#endif /* BEOS_BONE */
