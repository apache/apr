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

#include "testflock.h"
#include "test_apr.h"
#include "apr_pools.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_general.h"
#include "apr_strings.h"

/* XXX I'm sure there has to be a better way to do this ... */
#ifdef WIN32
#define EXTENSION ".exe"
#elif NETWARE
#define EXTENSION ".nlm"
#else
#define EXTENSION
#endif


static int launch_reader(CuTest *tc)
{
    apr_proc_t proc = {0};
    apr_procattr_t *procattr;
    const char *args[2];
    apr_status_t rv;
    apr_exit_why_e why;
    int exitcode;

    rv = apr_procattr_create(&procattr, p);
    apr_assert_success(tc, "Couldn't create procattr", rv);

    rv = apr_procattr_io_set(procattr, APR_NO_PIPE, APR_NO_PIPE,
            APR_NO_PIPE);
    apr_assert_success(tc, "Couldn't set io in procattr", rv);

    rv = apr_procattr_error_check_set(procattr, 1);
    apr_assert_success(tc, "Couldn't set error check in procattr", rv);

    args[0] = "tryread" EXTENSION;
    args[1] = NULL;
    rv = apr_proc_create(&proc, "./tryread" EXTENSION, args, NULL, procattr, p);
    apr_assert_success(tc, "Couldn't launch program", rv);

    CuAssert(tc, "wait for child process",
            apr_proc_wait(&proc, &exitcode, &why, APR_WAIT) == APR_CHILD_DONE);

    CuAssert(tc, "child terminated normally", why == APR_PROC_EXIT);
    return exitcode;
}

static void test_withlock(CuTest *tc)
{
    apr_file_t *file;
    apr_status_t rv;
    int code;
    
    rv = apr_file_open(&file, TESTFILE, APR_WRITE|APR_CREATE, 
                       APR_OS_DEFAULT, p);
    apr_assert_success(tc, "Could not create file.", rv);
    CuAssertPtrNotNull(tc, file);

    rv = apr_file_lock(file, APR_FLOCK_EXCLUSIVE);
    apr_assert_success(tc, "Could not lock the file.", rv);
    CuAssertPtrNotNull(tc, file);

    code = launch_reader(tc);
    CuAssertIntEquals(tc, FAILED_READ, code);

    (void) apr_file_close(file);
}

static void test_withoutlock(CuTest *tc)
{
    int code;
    
    code = launch_reader(tc);
    CuAssertIntEquals(tc, SUCCESSFUL_READ, code);
}

static void remove_lockfile(CuTest *tc)
{
    apr_assert_success(tc, "Couldn't remove lock file.",
                       apr_file_remove(TESTFILE, p));
}
    
CuSuite *testflock(void)
{
    CuSuite *suite = CuSuiteNew("Flock");

    SUITE_ADD_TEST(suite, test_withlock);
    SUITE_ADD_TEST(suite, test_withoutlock);
    SUITE_ADD_TEST(suite, remove_lockfile);

    return suite;
}
