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

APR_DECLARE(apr_status_t) apr_poll(apr_pollfd_t *aprset, apr_int32_t num,
                      apr_int32_t *nsds, apr_interval_time_t timeout)
{
    int *pollset;
    int i;
    int num_read = 0, num_write = 0, num_except = 0, num_total;
    int pos_read, pos_write, pos_except;

    for (i = 0; i < num; i++) {
        if (aprset[i].desc_type == APR_POLL_SOCKET) {
            num_read += (aprset[i].reqevents & APR_POLLIN) != 0;
            num_write += (aprset[i].reqevents & APR_POLLOUT) != 0;
            num_except += (aprset[i].reqevents & APR_POLLPRI) != 0;
        }
    }

    num_total = num_read + num_write + num_except;
    pollset = alloca(sizeof(int) * num_total);
    memset(pollset, 0, sizeof(int) * num_total);

    pos_read = 0;
    pos_write = num_read;
    pos_except = pos_write + num_write;

    for (i = 0; i < num; i++) {
        if (aprset[i].desc_type == APR_POLL_SOCKET) {
            if (aprset[i].reqevents & APR_POLLIN) {
                pollset[pos_read++] = aprset[i].desc.s->socketdes;
            }

            if (aprset[i].reqevents & APR_POLLOUT) {
                pollset[pos_write++] = aprset[i].desc.s->socketdes;
            }

            if (aprset[i].reqevents & APR_POLLPRI) {
                pollset[pos_except++] = aprset[i].desc.s->socketdes;
            }

            aprset[i].rtnevents = 0;
        }
    }

    if (timeout > 0) {
        timeout /= 1000; /* convert microseconds to milliseconds */
    }

    i = select(pollset, num_read, num_write, num_except, timeout);
    (*nsds) = i;

    if ((*nsds) < 0) {
        return APR_FROM_OS_ERROR(sock_errno());
    }

    if ((*nsds) == 0) {
        return APR_TIMEUP;
    }

    pos_read = 0;
    pos_write = num_read;
    pos_except = pos_write + num_write;

    for (i = 0; i < num; i++) {
        if (aprset[i].desc_type == APR_POLL_SOCKET) {
            if (aprset[i].reqevents & APR_POLLIN) {
                if (pollset[pos_read++] > 0) {
                    aprset[i].rtnevents |= APR_POLLIN;
                }
            }

            if (aprset[i].reqevents & APR_POLLOUT) {
                if (pollset[pos_write++] > 0) {
                    aprset[i].rtnevents |= APR_POLLOUT;
                }
            }

            if (aprset[i].reqevents & APR_POLLPRI) {
                if (pollset[pos_except++] > 0) {
                    aprset[i].rtnevents |= APR_POLLPRI;
                }
            }
        }
    }

    return APR_SUCCESS;
}
