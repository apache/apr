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

#include "testglobalmutex.h"
#include "apr_thread_proc.h"
#include "apr_global_mutex.h"
#include "apr_errno.h"
#include "test_apr.h"

static void launch_child(CuTest *tc, apr_proc_t *proc, apr_pool_t *p)
{
    apr_procattr_t *procattr;
    const char *args[2];
    apr_status_t rv;

    rv = apr_procattr_create(&procattr, p);
    apr_assert_success(tc, "Couldn't create procattr", rv);

    rv = apr_procattr_io_set(procattr, APR_NO_PIPE, APR_NO_PIPE,
            APR_NO_PIPE);
    apr_assert_success(tc, "Couldn't set io in procattr", rv);

    rv = apr_procattr_error_check_set(procattr, 1);
    apr_assert_success(tc, "Couldn't set error check in procattr", rv);

    args[0] = "globalmutexchild" EXTENSION;
    args[2] = NULL;
    rv = apr_proc_create(proc, "./globalmutexchild" EXTENSION, args, NULL,
            procattr, p);
    apr_assert_success(tc, "Couldn't launch program", rv);
}

static int wait_child(CuTest *tc, apr_proc_t *proc)
{
    int exitcode;
    apr_exit_why_e why;

    CuAssert(tc, "Error waiting for child process",
            apr_proc_wait(proc, &exitcode, &why, APR_WAIT) == APR_CHILD_DONE);

    CuAssert(tc, "child didn't terminate normally", why == APR_PROC_EXIT);
    return exitcode;
}

static void test_exclusive(CuTest *tc)
{
    apr_proc_t p1, p2, p3, p4;
    apr_status_t rv;
    apr_global_mutex_t *global_lock;
    int x = 0;
 
    rv = apr_global_mutex_create(&global_lock, LOCKNAME, APR_LOCK_DEFAULT, p);
    apr_assert_success(tc, "Error creating mutex", rv);


    launch_child(tc, &p1, p);
    launch_child(tc, &p2, p);
    launch_child(tc, &p3, p);
    launch_child(tc, &p4, p);
 
    x += wait_child(tc, &p1);
    x += wait_child(tc, &p2);
    x += wait_child(tc, &p3);
    x += wait_child(tc, &p4);

    CuAssertIntEquals(tc, MAX_COUNTER, x);
}

CuSuite *testglobalmutex(void)
{
    CuSuite *suite = CuSuiteNew("Global Mutex");

    SUITE_ADD_TEST(suite, test_exclusive);

    return suite;
}

