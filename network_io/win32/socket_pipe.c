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
#include "apr_portable.h"

static apr_status_t create_socket_pipe(SOCKET *rd, SOCKET *wr)
{
    FD_SET rs;
    SOCKET ls;
    struct timeval socktm;
    struct sockaddr_in pa;
    struct sockaddr_in la;
    struct sockaddr_in ca;
    int nrd;
    apr_status_t rv;
    int ll = sizeof(la);
    int lc = sizeof(ca);
    unsigned long bm = 1;
    char uid[8];
    char iid[8];

    *rd = INVALID_SOCKET;
    *wr = INVALID_SOCKET;

    /* Create the unique socket identifier
     * so that we know the connection originated
     * from us.
     */
    rv = apr_generate_random_bytes(uid, sizeof(uid));
    if (rv != APR_SUCCESS) {
        return rv;
    }

    if ((ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        return apr_get_netos_error();
    }

    pa.sin_family = AF_INET;
    pa.sin_port = 0;
    pa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(ls, (SOCKADDR *)&pa, sizeof(pa)) == SOCKET_ERROR) {
        rv = apr_get_netos_error();
        goto cleanup;
    }
    if (getsockname(ls, (SOCKADDR *)&la, &ll) == SOCKET_ERROR) {
        rv = apr_get_netos_error();
        goto cleanup;
    }
    if (listen(ls, 1) == SOCKET_ERROR) {
        rv = apr_get_netos_error();
        goto cleanup;
    }
    if ((*wr = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        rv = apr_get_netos_error();
        goto cleanup;
    }
    if (connect(*wr, (SOCKADDR *)&la, sizeof(la)) == SOCKET_ERROR) {
        rv = apr_get_netos_error();
        goto cleanup;
    }
    if (send(*wr, uid, sizeof(uid), 0) != sizeof(uid)) {
        if ((rv = apr_get_netos_error()) == 0) {
            rv = APR_EINVAL;
        }
        goto cleanup;
    }
    if (ioctlsocket(ls, FIONBIO, &bm) == SOCKET_ERROR) {
        rv = apr_get_netos_error();
        goto cleanup;
    }
    for (;;) {
        int ns;
        int nc = 0;
        /* Listening socket is nonblocking by now.
         * The accept should create the socket
         * immediatelly because we are connected already.
         * However on buys systems this can take a while
         * until winsock gets a chance to handle the events.
         */
        FD_ZERO(&rs);
        FD_SET(ls, &rs);

        socktm.tv_sec = 1;
        socktm.tv_usec = 0;
        if ((ns = select(0, &rs, NULL, NULL, &socktm)) == SOCKET_ERROR) {
            /* Accept still not signaled */
            Sleep(100);
            continue;
        }
        if (ns == 0) {
            /* No connections in the last second */
            continue;
        }
        if ((*rd = accept(ls, (SOCKADDR *)&ca, &lc)) == INVALID_SOCKET) {
            rv = apr_get_netos_error();
            goto cleanup;
        }
        /* Verify the connection by reading/waiting for the identification */
        bm = 0;
        if (ioctlsocket(*rd, FIONBIO, &bm) == SOCKET_ERROR) {
            rv = apr_get_netos_error();
            goto cleanup;
        }
        nrd = recv(*rd, iid, sizeof(iid), 0);
        if (nrd == SOCKET_ERROR) {
            rv = apr_get_netos_error();
            goto cleanup;
        }
        if (nrd == (int)sizeof(uid) && memcmp(iid, uid, sizeof(uid)) == 0) {
            /* Got the right identifier, return. */
            break;
        }
        closesocket(*rd);
    }
    /* We don't need the listening socket any more */
    closesocket(ls);
    return 0;

cleanup:
    /* Don't leak resources */
    closesocket(ls);
    if (*rd != INVALID_SOCKET)
        closesocket(*rd);
    if (*wr != INVALID_SOCKET)
        closesocket(*wr);

    *rd = INVALID_SOCKET;
    *wr = INVALID_SOCKET;
    return rv;
}

static apr_status_t socket_pipe_cleanup(void *thefile)
{
    apr_socket_t *file = thefile;
    if (file->socketdes != INVALID_SOCKET) {
        shutdown(file->socketdes, SD_BOTH);
        closesocket(file->socketdes);
        file->socketdes = INVALID_SOCKET;
    }
    return APR_SUCCESS;
}

apr_status_t apr_socket_pipe_create(apr_socket_t **in,
    apr_socket_t **out,
    apr_pool_t *p)
{
    apr_status_t rv;
    SOCKET rd;
    SOCKET wr;

    *in = NULL;
    *out = NULL;

    if ((rv = create_socket_pipe(&rd, &wr)) != APR_SUCCESS) {
        return rv;
    }
    apr_os_sock_put(in, &rd, p);
    apr_os_sock_put(out, &wr, p);

    /* read end of the pipe is non-blocking */
    apr_socket_timeout_set(*in, 0);

    apr_pool_cleanup_register(p, (void *)(*in), socket_pipe_cleanup,
        apr_pool_cleanup_null);
    apr_pool_cleanup_register(p, (void *)(*out), socket_pipe_cleanup,
        apr_pool_cleanup_null);

    return rv;
}

apr_status_t apr_socket_pipe_close(apr_socket_t *socket)
{
    apr_status_t stat;
    if ((stat = socket_pipe_cleanup(socket)) == APR_SUCCESS) {
        apr_pool_cleanup_kill(socket->pool, socket, socket_pipe_cleanup);

        return APR_SUCCESS;
    }
    return stat;
}
