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

#include "apr_arch_file_io.h"
#include "apr_arch_networkio.h"
#include "apr_poll.h"
#include "apr_errno.h"
#include "apr_support.h"

/* The only case where we don't use wait_for_io_or_timeout is on
 * pre-BONE BeOS, so this check should be sufficient and simpler */
#if !BEOS_R5
#define USE_WAIT_FOR_IO
#endif

#ifdef USE_WAIT_FOR_IO

apr_status_t apr_wait_for_io_or_timeout(apr_file_t *f, apr_socket_t *s,
                                        int for_read)
{
    apr_interval_time_t timeout;
    apr_pollfd_t pfd;
    int type = for_read ? APR_POLLIN : APR_POLLOUT;
    apr_pollset_t *pollset;
    apr_status_t status;

    /* TODO - timeout should be less each time through this loop */
    if (f) {
        pfd.desc_type = APR_POLL_FILE;
        pfd.desc.f = f;

        pollset = f->pollset;
        timeout = f->timeout;
    }
    else {
        pfd.desc_type = APR_POLL_SOCKET;
        pfd.desc.s = s;

        pollset = s->pollset;
        timeout = s->timeout;
    }
    pfd.reqevents = type;

    /* Remove the object if it was in the pollset, then add in the new
     * object with the correct reqevents value. Ignore the status result
     * on the remove, because it might not be in there (yet).
     */
    (void) apr_pollset_remove(pollset, &pfd);

    /* ### check status code */
    (void) apr_pollset_add(pollset, &pfd);

    do {
        int numdesc;
        const apr_pollfd_t *pdesc;

        status = apr_pollset_poll(pollset, timeout, &numdesc, &pdesc);

        if (numdesc == 1 && (pdesc[0].rtnevents & type) != 0) {
            return APR_SUCCESS;
        }
    } while (APR_STATUS_IS_EINTR(status));

    return status;
}

#endif /* USE_WAIT_FOR_IO */
