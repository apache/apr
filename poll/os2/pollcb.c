/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr.h"
#include "apr_poll.h"

struct apr_pollcb_t {
    apr_pool_t *pool;
    apr_pollset_t *pollset;
};



APR_DECLARE(apr_status_t) apr_pollcb_create(apr_pollcb_t **pollcb,
                                            apr_uint32_t size,
                                            apr_pool_t *p,
                                            apr_uint32_t flags)
{
    return apr_pollcb_create_ex(pollcb, size, p, flags, APR_POLLSET_DEFAULT);
}



APR_DECLARE(apr_status_t) apr_pollcb_create_ex(apr_pollcb_t **ret_pollcb,
                                               apr_uint32_t size,
                                               apr_pool_t *p,
                                               apr_uint32_t flags,
                                               apr_pollset_method_e method)
{
    apr_pollcb_t *pollcb = apr_palloc(p, sizeof(apr_pollcb_t));
    pollcb->pool = p;
    *ret_pollcb = pollcb;
    return apr_pollset_create_ex(&pollcb->pollset, size, p, flags, method);
}



APR_DECLARE(apr_status_t) apr_pollcb_add(apr_pollcb_t *pollcb,
                                         apr_pollfd_t *descriptor)
{
    return apr_pollset_add(pollcb->pollset, descriptor);
}



APR_DECLARE(apr_status_t) apr_pollcb_remove(apr_pollcb_t *pollcb,
                                            apr_pollfd_t *descriptor)
{
    return apr_pollset_remove(pollcb->pollset, descriptor);
}



APR_DECLARE(apr_status_t) apr_pollcb_poll(apr_pollcb_t *pollcb,
                                          apr_interval_time_t timeout,
                                          apr_pollcb_cb_t func,
                                          void *baton)
{
    int c;
    int num_signaled = 0;
    const apr_pollfd_t *signalled_descriptors;
    apr_status_t rc = apr_pollset_poll(pollcb->pollset, timeout, &num_signaled, &signalled_descriptors);

    for (c = 0; rc == APR_SUCCESS && c < num_signaled; c++) {
        rc = func(baton, signalled_descriptors + c);
    }

    return rc;
}



APR_DECLARE(const char *) apr_pollcb_method_name(apr_pollcb_t *pollcb)
{
    return "poll";
}



APR_DECLARE(apr_status_t) apr_pollcb_wakeup(apr_pollcb_t *pollcb)
{
    return apr_pollset_wakeup(pollcb->pollset);
}
