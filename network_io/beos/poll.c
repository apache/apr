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

#include "apr_private.h"
#if BEOS_BONE /* BONE uses the unix code - woohoo */
#include "../unix/poll.c"
#else
#include "networkio.h"

/*  BeOS R4 doesn't have a poll function, but R5 will have */
/*  so for the time being we try our best with an implementaion that */
/*  uses select.  However, select on beos isn't that hot either, so */
/*  until R5 we have to live with a less than perfect implementation */

/*  Apparently those sneaky people at Be included support for write in */
/*  select for R4.5 of BeOS.  So here we use code that uses the write */
/*  bits. */
    
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
    FD_ZERO((*new)->write);
    FD_ZERO((*new)->except);
    (*new)->highsock = -1;
    return APR_SUCCESS;
}

ap_status_t ap_add_poll_socket(ap_pollfd_t *aprset,
                               ap_socket_t *sock, ap_int16_t event)
{
    if (event & APR_POLLIN) {
        FD_SET(sock->socketdes, aprset->read);
    }
    if (event & APR_POLLPRI) {
        FD_SET(sock->socketdes, aprset->read);
    }
    if (event & APR_POLLOUT) {
        FD_SET(sock->socketdes, aprset->write);
    }
    if (sock->socketdes > aprset->highsock) {
        aprset->highsock = sock->socketdes;
    }
    return APR_SUCCESS;
}

ap_status_t ap_poll(ap_pollfd_t *aprset, ap_int32_t *nsds, 
		    ap_interval_time_t timeout)
{
    int rv;
    struct timeval tv, *tvptr;

    if (timeout < 0) {
        tvptr = NULL;
    }
    else {
        tv.tv_sec = timeout / AP_USEC_PER_SEC;
        tv.tv_usec = timeout % AP_USEC_PER_SEC;
	tvptr = &tv;
    }

    rv = select(aprset->highsock + 1, aprset->read, aprset->write, 
                    NULL, tvptr);

    (*nsds) = rv;
    if ((*nsds) == 0) {
        return APR_TIMEUP;
    }
    if ((*nsds) < 0) {
        return APR_EEXIST;
    }
    return APR_SUCCESS;
}

ap_status_t ap_get_revents(ap_int16_t *event, ap_socket_t *sock, ap_pollfd_t *aprset)
{
    ap_int16_t revents = 0;
    char data[256];
    int dummy = 256;

    if (FD_ISSET(sock->socketdes, aprset->read)) {
        revents |= APR_POLLIN;
        if (recv(sock->socketdes, &data, 0, 0) == -1) {
            switch (errno) {
                case ECONNRESET:
                case ECONNABORTED:
                case ESHUTDOWN:
                case ENETRESET: {
                    revents ^= APR_POLLIN;
                    revents |= APR_POLLHUP;
                    break;
                }
                case ENOTSOCK: {
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
    if (FD_ISSET(sock->socketdes, aprset->write)) {
        revents |= APR_POLLOUT;
    }

    /* Still no support for execpt bits in BeOS R4.5 so for the time being */
    /* we can't check this.  Hopefully the error checking above will allow */
    /* sufficient errors to be recognised to cover this. */

    /*if (FD_ISSET(sock->socketdes, aprset->except)) {
        revents |= APR_POLLPRI;
    }*/

    (*event) = revents;
    return APR_SUCCESS;
}

ap_status_t ap_remove_poll_socket(ap_pollfd_t *aprset, ap_socket_t *sock)
{
    FD_CLR(sock->socketdes, aprset->read);
    FD_CLR(sock->socketdes, aprset->read);
    FD_CLR(sock->socketdes, aprset->write);
    return APR_SUCCESS;
}

ap_status_t ap_clear_poll_sockets(ap_pollfd_t *aprset, ap_int16_t event)
{
    if (event & APR_POLLIN) {
        FD_ZERO(aprset->read);
    }
    if (event & APR_POLLPRI) {
        FD_ZERO(aprset->read);
    }
    if (event & APR_POLLOUT) {
        FD_ZERO(aprset->write);
    }
    aprset->highsock = 0;
    return APR_SUCCESS;
}

ap_status_t ap_get_polldata(ap_pollfd_t *pollfd, const char *key, void *data)
{
    if (pollfd != NULL) {
        return ap_get_userdata(data, key, pollfd->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOFILE;
    }
}

ap_status_t ap_set_polldata(ap_pollfd_t *pollfd, void *data, const char *key,
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
#endif

