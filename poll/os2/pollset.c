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



struct apr_pollset_t {
    apr_pool_t *pool;
    apr_uint32_t nelts;
    apr_uint32_t nalloc;
    int *pollset;
    int num_read;
    int num_write;
    int num_except;
    int num_total;
    apr_pollfd_t *query_set;
    apr_pollfd_t *result_set;
};



APR_DECLARE(apr_status_t) apr_pollset_create(apr_pollset_t **pollset,
                                             apr_uint32_t size,
                                             apr_pool_t *p,
                                             apr_uint32_t flags)
{
    *pollset = apr_palloc(p, sizeof(**pollset));
    (*pollset)->pool = p;
    (*pollset)->nelts = 0;
    (*pollset)->nalloc = size;
    (*pollset)->pollset = apr_palloc(p, size * sizeof(int) * 3);
    (*pollset)->query_set = apr_palloc(p, size * sizeof(apr_pollfd_t));
    (*pollset)->result_set = apr_palloc(p, size * sizeof(apr_pollfd_t));
    (*pollset)->num_read = -1;
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_pollset_destroy(apr_pollset_t *pollset)
{
    /* A no-op function for now.  If we later implement /dev/poll
     * support, we'll need to close the /dev/poll fd here
     */
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_pollset_add(apr_pollset_t *pollset,
                                          const apr_pollfd_t *descriptor)
{
    if (pollset->nelts == pollset->nalloc) {
        return APR_ENOMEM;
    }

    pollset->query_set[pollset->nelts] = *descriptor;

    if (descriptor->desc_type != APR_POLL_SOCKET) {
        return APR_EBADF;
    }

    pollset->nelts++;
    pollset->num_read = -1;
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_pollset_remove(apr_pollset_t *pollset,
                                             const apr_pollfd_t *descriptor)
{
    apr_uint32_t i;

    for (i = 0; i < pollset->nelts; i++) {
        if (descriptor->desc.s == pollset->query_set[i].desc.s) {
            /* Found an instance of the fd: remove this and any other copies */
            apr_uint32_t dst = i;
            apr_uint32_t old_nelts = pollset->nelts;
            pollset->nelts--;

            for (i++; i < old_nelts; i++) {
                if (descriptor->desc.s == pollset->query_set[i].desc.s) {
                    pollset->nelts--;
                }
                else {
                    pollset->pollset[dst] = pollset->pollset[i];
                    pollset->query_set[dst] = pollset->query_set[i];
                    dst++;
                }
            }

            pollset->num_read = -1;
            return APR_SUCCESS;
        }
    }

    return APR_NOTFOUND;
}



static void make_pollset(apr_pollset_t *pollset)
{
    int i;
    int pos = 0;

    pollset->num_read = 0;
    pollset->num_write = 0;
    pollset->num_except = 0;

    for (i = 0; i < pollset->nelts; i++) {
        if (pollset->query_set[i].reqevents & APR_POLLIN) {
            pollset->pollset[pos++] = pollset->query_set[i].desc.s->socketdes;
            pollset->num_read++;
        }
    }

    for (i = 0; i < pollset->nelts; i++) {
        if (pollset->query_set[i].reqevents & APR_POLLOUT) {
            pollset->pollset[pos++] = pollset->query_set[i].desc.s->socketdes;
            pollset->num_write++;
        }
    }

    for (i = 0; i < pollset->nelts; i++) {
        if (pollset->query_set[i].reqevents & APR_POLLPRI) {
            pollset->pollset[pos++] = pollset->query_set[i].desc.s->socketdes;
            pollset->num_except++;
        }
    }

    pollset->num_total = pollset->num_read + pollset->num_write + pollset->num_except;
}



APR_DECLARE(apr_status_t) apr_pollset_poll(apr_pollset_t *pollset,
                                           apr_interval_time_t timeout,
                                           apr_int32_t *num,
                                           const apr_pollfd_t **descriptors)
{
    int rv;
    apr_uint32_t i;
    int *pollresult;
    int read_pos, write_pos, except_pos;

    if (pollset->num_read < 0) {
        make_pollset(pollset);
    }

    pollresult = alloca(sizeof(int) * pollset->num_total);
    memcpy(pollresult, pollset->pollset, sizeof(int) * pollset->num_total);
    (*num) = 0;

    if (timeout > 0) {
        timeout /= 1000;
    }

    rv = select(pollresult, pollset->num_read, pollset->num_write, pollset->num_except, timeout);

    if (rv < 0) {
        return APR_FROM_OS_ERROR(sock_errno());
    }

    if (rv == 0) {
        return APR_TIMEUP;
    }

    read_pos = 0;
    write_pos = pollset->num_read;
    except_pos = pollset->num_read + pollset->num_write;

    for (i = 0; i < pollset->nelts; i++) {
        int rtnevents = 0;

        if (pollset->query_set[i].reqevents & APR_POLLIN) {
            if (pollresult[read_pos++] != -1) {
                rtnevents |= APR_POLLIN;
            }
        }

        if (pollset->query_set[i].reqevents & APR_POLLOUT) {
            if (pollresult[write_pos++] != -1) {
                rtnevents |= APR_POLLOUT;
            }
        }

        if (pollset->query_set[i].reqevents & APR_POLLPRI) {
            if (pollresult[except_pos++] != -1) {
                rtnevents |= APR_POLLPRI;
            }
        }

        if (rtnevents) {
            pollset->result_set[*num] = pollset->query_set[i];
            pollset->result_set[*num].rtnevents = rtnevents;
            (*num)++;
        }
    }

    *descriptors = pollset->result_set;
    return APR_SUCCESS;
}
