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

#include "apr_arch_networkio.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_support.h"
#include "apr_lib.h"
#include <sys/time.h>

static apr_status_t wait_socket_ready(apr_socket_t *sock, int readwrite)
{
    int pollsocket = sock->socketdes;
    int wait_rc = select(&pollsocket, readwrite == 0, readwrite == 1, 0, sock->timeout / 1000);

    if (wait_rc == 0) {
        return APR_TIMEUP;
    }
    else if (wait_rc < 0) {
        return APR_FROM_OS_ERROR(sock_errno());
    }

    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_socket_sendto(apr_socket_t *sock,
                                            apr_sockaddr_t *where,
                                            apr_int32_t flags, const char *buf,
                                            apr_size_t *len)
{
    apr_ssize_t rv;
    int serrno;

    do {
        rv = sendto(sock->socketdes, buf, (*len), flags, 
                    (struct sockaddr*)&where->sa,
                    where->salen);

        if (rv == -1) {
            serrno = sock_errno();

            if (serrno == SOCEWOULDBLOCK && sock->timeout != 0) {
                apr_status_t wait_status = wait_socket_ready(sock, 1);

                if (wait_status != APR_SUCCESS) {
                    *len = 0;
                    return wait_status;
                }
            }
            else if (serrno != SOCEINTR) {
                *len = 0;
                return APR_FROM_OS_ERROR(serrno);
            }
        }
        else {
            *len = rv;
            return APR_SUCCESS;
        }
    } while (1);
}



APR_DECLARE(apr_status_t) apr_socket_recvfrom(apr_sockaddr_t *from,
                                              apr_socket_t *sock,
                                              apr_int32_t flags, char *buf,
                                              apr_size_t *len)
{
    apr_ssize_t rv;
    int serrno;

    do {
        from->salen = sizeof(from->sa);
        rv = recvfrom(sock->socketdes, buf, (*len), flags,
                      (struct sockaddr*)&from->sa,
                      &from->salen);

        if (rv == -1) {
            serrno = sock_errno();

            if (serrno == SOCEWOULDBLOCK && sock->timeout != 0) {
                apr_status_t wait_status = wait_socket_ready(sock, 0);

                if (wait_status != APR_SUCCESS) {
                    *len = 0;
                    return wait_status;
                }
            }
            else if (serrno != SOCEINTR) {
                *len = 0;
                return APR_FROM_OS_ERROR(serrno);
            }
        }
        else {
            *len = rv;
            apr_sockaddr_vars_set(from, from->sa.sin.sin_family, ntohs(from->sa.sin.sin_port));
            return (rv == 0 && sock->type == SOCK_STREAM) ? APR_EOF : APR_SUCCESS;
        }
    } while (1);
}
