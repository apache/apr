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

#include "apr_arch_file_io.h"
#include "apr_arch_networkio.h"
#include "apr_errno.h"
#include "apr_support.h"

static apr_status_t wait_for_file(apr_file_t *f, int for_read)
{
    int rc;

    if (!f->pipe) {
        /* No support for waiting on a regular file */
        return APR_ENOTIMPL;
    }

    rc = DosWaitEventSem(f->pipeSem, f->timeout >= 0 ? f->timeout / 1000 : SEM_INDEFINITE_WAIT);

    if (rc == ERROR_TIMEOUT) {
        return APR_TIMEUP;
    }

    return APR_FROM_OS_ERROR(rc);
}



static apr_status_t wait_for_socket(apr_socket_t *s, int for_read)
{
    int pollsocket = s->socketdes;
    int wait_rc = select(&pollsocket, for_read != 0, for_read == 0, 0, s->timeout / 1000);

    if (wait_rc == 0) {
        return APR_TIMEUP;
    }
    else if (wait_rc < 0) {
        return APR_FROM_OS_ERROR(sock_errno());
    }

    return APR_SUCCESS;
}



apr_status_t apr_wait_for_io_or_timeout(apr_file_t *f, apr_socket_t *s,
                                        int for_read)
{
    if (f) {
        return wait_for_file(f, for_read);
    }
    else if (s) {
        return wait_for_socket(s, for_read);
    }

    /* Didn't specify a file or socket */
    return APR_EINVAL;
}
