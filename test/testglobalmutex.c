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
#include "testutil.h"

static void launch_child(abts_case *tc, apr_lockmech_e mech,
                         apr_proc_t *proc, apr_pool_t *p)
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

    args[0] = "globalmutexchild" EXTENSION;
    args[1] = (const char*)apr_itoa(p, (int)mech);
    args[2] = NULL;
    rv = apr_proc_create(proc, "./globalmutexchild" EXTENSION, args, NULL,
            procattr, p);
    apr_assert_success(tc, "Couldn't launch program", rv);
}

static int wait_child(abts_case *tc, apr_proc_t *proc)
{
    int exitcode;
    apr_exit_why_e why;

    ABTS_ASSERT(tc, "Error waiting for child process",
            apr_proc_wait(proc, &exitcode, &why, APR_WAIT) == APR_CHILD_DONE);

    ABTS_ASSERT(tc, "child didn't terminate normally", why == APR_PROC_EXIT);
    return exitcode;
}

static void test_exclusive(abts_case *tc, void *data, apr_lockmech_e mech)
{
    apr_proc_t p1, p2, p3, p4;
    apr_status_t rv;
    apr_global_mutex_t *global_lock;
    int x = 0;
 
    rv = apr_global_mutex_create(&global_lock, LOCKNAME, mech, p);
    apr_assert_success(tc, "Error creating mutex", rv);

    launch_child(tc, mech, &p1, p);
    launch_child(tc, mech, &p2, p);
    launch_child(tc, mech, &p3, p);
    launch_child(tc, mech, &p4, p);
 
    x += wait_child(tc, &p1);
    x += wait_child(tc, &p2);
    x += wait_child(tc, &p3);
    x += wait_child(tc, &p4);

    ABTS_INT_EQUAL(tc, MAX_COUNTER, x);
}

static void test_exclusive_default(abts_case *tc, void *data)
{
    test_exclusive(tc, data, APR_LOCK_DEFAULT);
}

static void test_exclusive_posixsem(abts_case *tc, void *data)
{
    test_exclusive(tc, data, APR_LOCK_POSIXSEM);
}

static void test_exclusive_sysvsem(abts_case *tc, void *data)
{
    test_exclusive(tc, data, APR_LOCK_SYSVSEM);
}

static void test_exclusive_proc_pthread(abts_case *tc, void *data)
{
    test_exclusive(tc, data, APR_LOCK_PROC_PTHREAD);
}

static void test_exclusive_fcntl(abts_case *tc, void *data)
{
    test_exclusive(tc, data, APR_LOCK_FCNTL);
}

static void test_exclusive_flock(abts_case *tc, void *data)
{
    test_exclusive(tc, data, APR_LOCK_FLOCK);
}

abts_suite *testglobalmutex(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

    abts_run_test(suite, test_exclusive_default, NULL);
#if APR_HAS_POSIXSEM_SERIALIZE
    abts_run_test(suite, test_exclusive_posixsem, NULL);
#endif
#if APR_HAS_SYSVSEM_SERIALIZE
    abts_run_test(suite, test_exclusive_sysvsem, NULL);
#endif
#if APR_HAS_PROC_PTHREAD_SERIALIZE
    abts_run_test(suite, test_exclusive_proc_pthread, NULL);
#endif
#if APR_HAS_FCNTL_SERIALIZE
    abts_run_test(suite, test_exclusive_fcntl, NULL);
#endif
#if APR_HAS_FLOCK_SERIALIZE
    abts_run_test(suite, test_exclusive_flock, NULL);
#endif

    return suite;
}

