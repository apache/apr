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

/*
 * USAGE
 *
 * Start one process, no args, and place it into the background. Start a
 * second process with the "-r" switch to attempt a read on the file
 * created by the first process.
 *
 * $ ./testflock &
 * ...messages...
 * $ ./testflock -r
 * ...messages...
 *
 * The first process will sleep for 30 seconds while holding a lock. The
 * second process will attempt to grab it (non-blocking) and fail. It
 * will then grab it with a blocking scheme. When the first process' 30
 * seconds are up, it will exit (thus releasing its lock). The second
 * process will acquire the lock, then exit.
 */

#include "apr_pools.h"
#include "apr_file_io.h"
#include "apr_time.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "apr_strings.h"

#include <stdlib.h>
#include <stdio.h>

const char *testfile = "testfile.tmp";

static apr_pool_t *pool = NULL;

static void errmsg(const char *msg)
{
    if (pool != NULL)
        apr_pool_destroy(pool);
    fprintf(stderr, msg);
    exit(1);
}

static void errmsg2(const char *msg, apr_status_t rv)
{
    char *newmsg;
    char errstr[120];

    apr_strerror(rv, errstr, sizeof errstr);
    newmsg = apr_psprintf(pool, "%s: %s (%d)\n",
                          msg, errstr, rv);
    errmsg(newmsg);
    exit(1);
}

static void do_read(void)
{
    apr_file_t *file;
    apr_status_t status;

    if (apr_file_open(&file, testfile, APR_WRITE,
                 APR_OS_DEFAULT, pool) != APR_SUCCESS)
        errmsg("Could not open test file.\n");
    printf("Test file opened.\n");

    status = apr_file_lock(file, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK);
    if (!APR_STATUS_IS_EAGAIN(status)) {
        char msg[200];
        errmsg(apr_psprintf(pool, "Expected APR_EAGAIN. Got %d: %s.\n",
                            status, apr_strerror(status, msg, sizeof(msg))));
    }
    printf("First attempt: we were properly locked out.\nWaiting for lock...");
    fflush(stdout);

    if (apr_file_lock(file, APR_FLOCK_EXCLUSIVE) != APR_SUCCESS)
        errmsg("Could not establish lock on test file.");
    printf(" got it.\n");

    (void) apr_file_close(file);
    printf("Exiting.\n");
}

static void do_write(void)
{
    apr_file_t *file;
    apr_status_t rv;

    if (apr_file_open(&file, testfile, APR_WRITE|APR_CREATE, APR_OS_DEFAULT,
                 pool) != APR_SUCCESS)
        errmsg("Could not create file.\n");
    printf("Test file created.\n");

    if ((rv = apr_file_lock(file, APR_FLOCK_EXCLUSIVE)) != APR_SUCCESS)
        errmsg2("Could not lock the file", rv);
    printf("Lock created.\nSleeping...");
    fflush(stdout);

    apr_sleep(apr_time_from_sec(30));

    (void) apr_file_close(file);
    printf(" done.\nExiting.\n");
}

int main(int argc, const char * const *argv)
{
    int reader = 0;
    apr_status_t status;
    char optchar;
    const char *optarg;
    apr_getopt_t *opt;

    if (apr_initialize() != APR_SUCCESS)
        errmsg("Could not initialize APR.\n");
    atexit(apr_terminate);

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS)
        errmsg("Could not create global pool.\n");

    if (apr_getopt_init(&opt, pool, argc, argv) != APR_SUCCESS)
        errmsg("Could not parse options.\n");

    while ((status = apr_getopt(opt, "rf:", &optchar, &optarg)) == APR_SUCCESS) {
        if (optchar == 'r')
            ++reader;
        else if (optchar == 'f')
            testfile = optarg;
    }
    if (status != APR_SUCCESS && status != APR_EOF) {
        char msgbuf[80];

        fprintf(stderr, "error: %s\n",
                apr_strerror(status, msgbuf, sizeof msgbuf));
        exit(1);
    }

    if (reader)
        do_read();
    else
        do_write();

    return 0;
}
