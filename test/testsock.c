/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "test_apr.h"
#include "testsock.h"
#include "apr_thread_proc.h"
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"

static void launch_child(CuTest *tc, apr_proc_t *proc, const char *arg1, apr_pool_t *p)
{
    apr_procattr_t *procattr;
    const char *args[3];
    apr_status_t rv;

    rv = apr_procattr_create(&procattr, p);
    apr_assert_success(tc, "Couldn't create procattr", rv);

    rv = apr_procattr_io_set(procattr, APR_NO_PIPE, APR_NO_PIPE,
            APR_NO_PIPE);
    apr_assert_success(tc, "Couldn't set io in procattr", rv);

    rv = apr_procattr_error_check_set(procattr, 1);
    apr_assert_success(tc, "Couldn't set error check in procattr", rv);

    args[0] = "sockchild" EXTENSION;
    args[1] = arg1;
    args[2] = NULL;
    rv = apr_proc_create(proc, "./sockchild" EXTENSION, args, NULL,
                         procattr, p);
    apr_assert_success(tc, "Couldn't launch program", rv);
}

static int wait_child(CuTest *tc, apr_proc_t *proc) 
{
    int exitcode;
    apr_exit_why_e why;

    CuAssert(tc, "Error waiting for child process",
            apr_proc_wait(proc, &exitcode, &why, APR_WAIT) == APR_CHILD_DONE);

    CuAssert(tc, "child terminated normally", why == APR_PROC_EXIT);
    return exitcode;
}

static void test_addr_info(CuTest *tc)
{
    apr_status_t rv;
    apr_sockaddr_t *sa;

    rv = apr_sockaddr_info_get(&sa, APR_LOCAL, APR_UNSPEC, 80, 0, p);
    apr_assert_success(tc, "Problem generating sockaddr", rv);

    rv = apr_sockaddr_info_get(&sa, "127.0.0.1", APR_UNSPEC, 80, 0, p);
    apr_assert_success(tc, "Problem generating sockaddr", rv);
    CuAssertStrEquals(tc, "127.0.0.1", sa->hostname);
}

static apr_socket_t *setup_socket(CuTest *tc)
{
    apr_status_t rv;
    apr_sockaddr_t *sa;
    apr_socket_t *sock;

    rv = apr_sockaddr_info_get(&sa, APR_LOCAL, APR_INET, 8021, 0, p);
    apr_assert_success(tc, "Problem generating sockaddr", rv);

    rv = apr_socket_create(&sock, sa->family, SOCK_STREAM, APR_PROTO_TCP, p);
    apr_assert_success(tc, "Problem creating socket", rv);
    
    rv = apr_socket_bind(sock, sa);
    apr_assert_success(tc, "Problem binding to port", rv);
    
    rv = apr_socket_listen(sock, 5);
    apr_assert_success(tc, "Problem listening on socket", rv);

    return sock;
}

static void test_create_bind_listen(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock = setup_socket(tc);
    
    rv = apr_socket_close(sock) ;
    apr_assert_success(tc, "Problem closing socket", rv);
}

static void test_send(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock;
    apr_socket_t *sock2;
    apr_proc_t proc;
    int protocol;
    apr_size_t length;

    sock = setup_socket(tc);

    launch_child(tc, &proc, "read", p);
    
    rv = apr_socket_accept(&sock2, sock, p);
    apr_assert_success(tc, "Problem with receiving connection", rv);

    apr_socket_protocol_get(sock2, &protocol);
    CuAssertIntEquals(tc, APR_PROTO_TCP, protocol);
    
    length = strlen(DATASTR);
    apr_socket_send(sock2, DATASTR, &length);

    /* Make sure that the client received the data we sent */
    CuAssertIntEquals(tc, strlen(DATASTR), wait_child(tc, &proc));

    rv = apr_socket_close(sock2);
    apr_assert_success(tc, "Problem closing connected socket", rv);
    rv = apr_socket_close(sock);
    apr_assert_success(tc, "Problem closing socket", rv);
}

static void test_recv(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock;
    apr_socket_t *sock2;
    apr_proc_t proc;
    int protocol;
    apr_size_t length = STRLEN;
    char datastr[STRLEN];
    
    sock = setup_socket(tc);

    launch_child(tc, &proc, "write", p);
    
    rv = apr_socket_accept(&sock2, sock, p);
    apr_assert_success(tc, "Problem with receiving connection", rv);

    apr_socket_protocol_get(sock2, &protocol);
    CuAssertIntEquals(tc, APR_PROTO_TCP, protocol);
    
    memset(datastr, 0, STRLEN);
    apr_socket_recv(sock2, datastr, &length);

    /* Make sure that the server received the data we sent */
    CuAssertStrEquals(tc, DATASTR, datastr);
    CuAssertIntEquals(tc, strlen(datastr), wait_child(tc, &proc));

    rv = apr_socket_close(sock2);
    apr_assert_success(tc, "Problem closing connected socket", rv);
    rv = apr_socket_close(sock);
    apr_assert_success(tc, "Problem closing socket", rv);
}

static void test_timeout(CuTest *tc)
{
    apr_status_t rv;
    apr_socket_t *sock;
    apr_socket_t *sock2;
    apr_proc_t proc;
    int protocol;
    int exit;
    
    sock = setup_socket(tc);

    launch_child(tc, &proc, "read", p);
    
    rv = apr_socket_accept(&sock2, sock, p);
    apr_assert_success(tc, "Problem with receiving connection", rv);

    apr_socket_protocol_get(sock2, &protocol);
    CuAssertIntEquals(tc, APR_PROTO_TCP, protocol);
    
    exit = wait_child(tc, &proc);    
    CuAssertIntEquals(tc, SOCKET_TIMEOUT, exit);

    /* We didn't write any data, so make sure the child program returns
     * an error.
     */
    rv = apr_socket_close(sock2);
    apr_assert_success(tc, "Problem closing connected socket", rv);
    rv = apr_socket_close(sock);
    apr_assert_success(tc, "Problem closing socket", rv);
}

CuSuite *testsock(void)
{
    CuSuite *suite = CuSuiteNew("Socket operations");

    SUITE_ADD_TEST(suite, test_addr_info);
    SUITE_ADD_TEST(suite, test_create_bind_listen);
    SUITE_ADD_TEST(suite, test_send);
    SUITE_ADD_TEST(suite, test_recv);
    SUITE_ADD_TEST(suite, test_timeout);

    return suite;
}

