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

#include "apr.h"
#include "apr_poll.h"
#include "apr_arch_networkio.h"
#include "apr_arch_file_io.h"
#if HAVE_POLL_H
#include <poll.h>
#endif
#if HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif

APR_DECLARE(apr_status_t) apr_poll_setup(apr_pollfd_t **new, apr_int32_t num, apr_pool_t *cont)
{
    (*new) = (apr_pollfd_t *)apr_pcalloc(cont, sizeof(apr_pollfd_t) * (num + 1));
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)[num].desc_type = APR_POLL_LASTDESC;
    (*new)[0].p = cont;
    return APR_SUCCESS;
}

static apr_pollfd_t *find_poll_sock(apr_pollfd_t *aprset, apr_socket_t *sock)
{
    apr_pollfd_t *curr = aprset;
    
    while (curr->desc.s != sock) {
        if (curr->desc_type == APR_POLL_LASTDESC) {
            return NULL;
        }
        curr++;
    }

    return curr;
}

APR_DECLARE(apr_status_t) apr_poll_socket_add(apr_pollfd_t *aprset, 
			       apr_socket_t *sock, apr_int16_t event)
{
    apr_pollfd_t *curr = aprset;
    
    while (curr->desc_type != APR_NO_DESC) {
        if (curr->desc_type == APR_POLL_LASTDESC) {
            return APR_ENOMEM;
        }
        curr++;
    }
    curr->desc.s = sock;
    curr->desc_type = APR_POLL_SOCKET;
    curr->reqevents = event;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_revents_get(apr_int16_t *event, apr_socket_t *sock, apr_pollfd_t *aprset)
{
    apr_pollfd_t *curr = find_poll_sock(aprset, sock);
    if (curr == NULL) {
        return APR_NOTFOUND;
    }

    (*event) = curr->rtnevents;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_mask(apr_pollfd_t *aprset, 
                                  apr_socket_t *sock, apr_int16_t events)
{
    apr_pollfd_t *curr = find_poll_sock(aprset, sock);
    if (curr == NULL) {
        return APR_NOTFOUND;
    }
    
    if (curr->reqevents & events) {
        curr->reqevents ^= events;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_remove(apr_pollfd_t *aprset, apr_socket_t *sock)
{
    apr_pollfd_t *match = NULL;
    apr_pollfd_t *curr;

    for (curr = aprset; (curr->desc_type != APR_POLL_LASTDESC) &&
             (curr->desc_type != APR_NO_DESC); curr++) {
        if (curr->desc.s == sock) {
            match = curr;
        }
    }
    if (match == NULL) {
        return APR_NOTFOUND;
    }

    /* Remove this entry by swapping the last entry into its place.
     * This ensures that the non-APR_NO_DESC entries are all at the
     * start of the array, so that apr_poll() doesn't have to worry
     * about invalid entries in the middle of the pollset.
     */
    curr--;
    if (curr != match) {
        *match = *curr;
    }
    curr->desc_type = APR_NO_DESC;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_poll_socket_clear(apr_pollfd_t *aprset, apr_int16_t events)
{
    apr_pollfd_t *curr = aprset;

    while (curr->desc_type != APR_POLL_LASTDESC) {
        if (curr->reqevents & events) {
            curr->reqevents &= ~events;
        }
        curr++;
    }
    return APR_SUCCESS;
}

#if APR_FILES_AS_SOCKETS
/* I'm not sure if this needs to return an apr_status_t or not, but
 * for right now, we'll leave it this way, and change it later if
 * necessary.
 */
APR_DECLARE(apr_status_t) apr_socket_from_file(apr_socket_t **newsock, apr_file_t *file)
{
    (*newsock) = apr_pcalloc(file->pool, sizeof(**newsock));
    (*newsock)->socketdes = file->filedes;
    (*newsock)->cntxt = file->pool;
    (*newsock)->timeout = file->timeout;
    return APR_SUCCESS;
}
#endif
