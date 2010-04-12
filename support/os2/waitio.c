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

apr_status_t apr_wait_for_io_or_timeout(apr_file_t *f, apr_socket_t *s,
                                        int for_read)
{
    if (f) {
        return apr_file_pipe_wait(f, for_read ? APR_WAIT_READ : APR_WAIT_WRITE);
    }
    else if (s) {
        return apr_socket_wait(s, for_read ? APR_WAIT_READ : APR_WAIT_WRITE);
    }

    /* Didn't specify a file or socket */
    return APR_EINVAL;
}
