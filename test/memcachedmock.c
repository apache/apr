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

#include <stdlib.h>
#include "apr_network_io.h"
#include "apr_pools.h"
#include "apr_pools.h"
#include "testmemcache.h"

#define MOCK_REPLY_DEFAULT "VERSION 1.5.22\r\n"

/* Number of connections to serve before exiting.
 * The test_connection_validation test needs 2 (one intentional, one
 * after the server closes the socket).  The version-response tests
 * need only 1 each.  Pass MOCK_NCONN on the command line after the
 * reply string to override; default is 2 for backward compatibility.
 */
#define MOCK_NCONN_DEFAULT 2

int main(int argc, char *argv[])
{
    apr_pool_t *p;
    apr_sockaddr_t *sa;
    apr_socket_t *server;
    apr_socket_t *server_connection;
    apr_status_t rv;
    apr_size_t length;
    int i, nconn, conn;
    const char *reply;

    reply = (argc >= 2) ? argv[1] : MOCK_REPLY_DEFAULT;
    nconn = (argc >= 3) ? atoi(argv[2]) : MOCK_NCONN_DEFAULT;

    apr_initialize();
    atexit(apr_terminate);
    apr_pool_create(&p, NULL);

    apr_sockaddr_info_get(&sa, MOCK_HOST, APR_UNSPEC, MOCK_PORT, 0, p);

    apr_socket_create(&server, sa->family, SOCK_STREAM, 0, p);

    apr_socket_opt_set(server, APR_SO_REUSEADDR, 1);

    apr_socket_timeout_set(server, 0);

    apr_socket_bind(server, sa);

    apr_socket_listen(server, 5);

    for (conn = 0; conn < nconn; conn++) {
        /* Do spin instead of a proper poll for sake of simplicity */
        for (i = 0; i < 4; i++) {

            rv = apr_socket_accept(&server_connection, server, p);
            if (rv == APR_SUCCESS) {
                break;
            }

            apr_sleep(apr_time_from_sec(1));
        }

        length = strlen(reply);
        apr_socket_send(server_connection, reply, &length);

        apr_socket_close(server_connection);
    }

    exit(0);
}
