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

#include "testflock.h"
#include "apr_pools.h"
#include "apr_file_io.h"
#include "apr_general.h"

int main(int argc, const char * const *argv)
{
    apr_file_t *file;
    apr_status_t status;
    apr_pool_t *p;

    apr_initialize();
    apr_pool_create(&p, NULL);

    if (apr_file_open(&file, TESTFILE, APR_WRITE, APR_OS_DEFAULT, p) 
        != APR_SUCCESS) {
        
        exit(UNEXPECTED_ERROR);
    }
    status = apr_file_lock(file, APR_FLOCK_EXCLUSIVE | APR_FLOCK_NONBLOCK);
    if (status == APR_SUCCESS) {
        exit(SUCCESSFUL_READ);
    }
    if (APR_STATUS_IS_EAGAIN(status)) {
        exit(FAILED_READ);
    }
    exit(UNEXPECTED_ERROR);
}
