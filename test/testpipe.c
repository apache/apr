/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include <stdlib.h>

#include "test_apr.h"
#include "apr_file_io.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_thread_proc.h"
#include "apr_strings.h"

static apr_file_t *readp = NULL;
static apr_file_t *writep = NULL;

static void create_pipe(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_file_pipe_create(&readp, &writep, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, readp);
    CuAssertPtrNotNull(tc, writep);
}   

static void close_pipe(CuTest *tc)
{
    apr_status_t rv;
    apr_size_t nbytes = 256;
    char buf[256];

    rv = apr_file_close(readp);
    rv = apr_file_close(writep);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_read(readp, buf, &nbytes);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_EBADF(rv));
}   

static void set_timeout(CuTest *tc)
{
    apr_status_t rv;
    apr_file_t *readp = NULL;
    apr_file_t *writep = NULL;
    apr_interval_time_t timeout;

    rv = apr_file_pipe_create(&readp, &writep, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, readp);
    CuAssertPtrNotNull(tc, writep);

    rv = apr_file_pipe_timeout_get(readp, &timeout);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, -1, timeout);

    rv = apr_file_pipe_timeout_set(readp, apr_time_from_sec(1));
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_pipe_timeout_get(readp, &timeout);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, apr_time_from_sec(1), timeout);
}

static void read_write(CuTest *tc)
{
    apr_status_t rv;
    char *buf;
    apr_size_t nbytes;
    
    nbytes = strlen("this is a test");
    buf = (char *)apr_palloc(p, nbytes + 1);

    rv = apr_file_pipe_create(&readp, &writep, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, readp);
    CuAssertPtrNotNull(tc, writep);

    rv = apr_file_pipe_timeout_set(readp, apr_time_from_sec(1));
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_read(readp, buf, &nbytes);
    CuAssertIntEquals(tc, 1, APR_STATUS_IS_TIMEUP(rv));
    CuAssertIntEquals(tc, 0, nbytes);
}

static void read_write_notimeout(CuTest *tc)
{
    apr_status_t rv;
    char *buf = "this is a test";
    char *input;
    apr_size_t nbytes;
    
    nbytes = strlen("this is a test");

    rv = apr_file_pipe_create(&readp, &writep, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, readp);
    CuAssertPtrNotNull(tc, writep);

    rv = apr_file_write(writep, buf, &nbytes);
    CuAssertIntEquals(tc, strlen("this is a test"), nbytes);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    nbytes = 256;
    input = apr_pcalloc(p, nbytes + 1);
    rv = apr_file_read(readp, input, &nbytes);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertIntEquals(tc, strlen("this is a test"), nbytes);
    CuAssertStrEquals(tc, "this is a test", input);
}

/* XXX FIXME */
#ifdef WIN32
#define EXTENSION ".exe"
#elif NETWARE
#define EXTENSION ".nlm"
#else
#define EXTENSION
#endif

static void test_pipe_writefull(CuTest *tc)
{
    int iterations = 1000;
    int i;
    int bytes_per_iteration = 8000;
    char *buf = (char *)malloc(bytes_per_iteration);
    char responsebuf[128];
    apr_size_t nbytes;
    int bytes_processed;
    apr_proc_t proc = {0};
    apr_procattr_t *procattr;
    const char *args[2];
    apr_status_t rv;
    
    rv = apr_procattr_create(&procattr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_io_set(procattr, APR_CHILD_BLOCK, APR_CHILD_BLOCK,
                             APR_CHILD_BLOCK);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_error_check_set(procattr, 1);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    args[0] = "readchild" EXTENSION;
    args[1] = NULL;
    rv = apr_proc_create(&proc, "./readchild" EXTENSION, args, NULL, procattr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_pipe_timeout_set(proc.in, apr_time_from_sec(10));
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_pipe_timeout_set(proc.out, apr_time_from_sec(10));
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    i = iterations;
    do {
        rv = apr_file_write_full(proc.in, buf, bytes_per_iteration, NULL);
        CuAssertIntEquals(tc, APR_SUCCESS, rv);
    } while (--i);

    free(buf);

    rv = apr_file_close(proc.in);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    
    nbytes = sizeof(responsebuf);
    rv = apr_file_read(proc.out, responsebuf, &nbytes);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    bytes_processed = (int)apr_strtoi64(responsebuf, NULL, 10);
    CuAssertIntEquals(tc, iterations * bytes_per_iteration, bytes_processed);
}

CuSuite *testpipe(void)
{
    CuSuite *suite = CuSuiteNew("Pipes");

    SUITE_ADD_TEST(suite, create_pipe);
    SUITE_ADD_TEST(suite, close_pipe);
    SUITE_ADD_TEST(suite, set_timeout);
    SUITE_ADD_TEST(suite, read_write);
    SUITE_ADD_TEST(suite, read_write_notimeout);
    SUITE_ADD_TEST(suite, test_pipe_writefull);

    return suite;
}

