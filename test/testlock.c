/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_lock.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef BEOS
#include <unistd.h>
#endif

#if !APR_HAS_THREADS
int main(void)
{
    fprintf(stderr,
            "This program won't work on this platform because there is no "
            "support for threads.\n");
    return 0;
}
#else /* !APR_HAS_THREADS */

#define MAX_ITER 40000

void * APR_THREAD_FUNC thread_rw_func(void *data);
void * APR_THREAD_FUNC thread_func(void *data);

apr_file_t *in, *out, *err;
apr_lock_t *thread_rw_lock, *thread_lock;
apr_pool_t *context;
int i = 0, x = 0;

void * APR_THREAD_FUNC thread_rw_func(void *data)
{
    int exitLoop = 1;

    while (1)
    {
        apr_lock_acquire_rw(thread_rw_lock, APR_READER);
        if (i == MAX_ITER)
            exitLoop = 0;
        apr_lock_release(thread_rw_lock);

        if (!exitLoop)
            break;

        apr_lock_acquire_rw(thread_rw_lock, APR_WRITER);
        if (i != MAX_ITER)
        {
            i++;
            x++;
        }
        apr_lock_release(thread_rw_lock);
    }
    return NULL;
} 

void * APR_THREAD_FUNC thread_func(void *data)
{
    int exitLoop = 1;

    while (1)
    {
        apr_lock_acquire(thread_lock);
        if (i == MAX_ITER)
            exitLoop = 0;
        else 
        {
            i++;
            x++;
        }
        apr_lock_release(thread_lock);

        if (!exitLoop)
            break;
    }
    return NULL;
} 

int test_rw(void)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    apr_file_printf(out, "Initializing the rw lock.");
    s1 = apr_lock_create(&thread_rw_lock, APR_READWRITE, APR_INTRAPROCESS, 
                         "lock.file", context); 
    if (s1 != APR_SUCCESS) {
        apr_file_printf(err, "Could not create lock\n");
        return s1;
    }
    apr_file_printf(out, " OK\n");

    i = 0;
    x = 0;

    apr_file_printf(out, "Starting all the threads......."); 
    s1 = apr_thread_create(&t1, NULL, thread_rw_func, NULL, context);
    s2 = apr_thread_create(&t2, NULL, thread_rw_func, NULL, context);
    s3 = apr_thread_create(&t3, NULL, thread_rw_func, NULL, context);
    s4 = apr_thread_create(&t4, NULL, thread_rw_func, NULL, context);
    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS || 
        s3 != APR_SUCCESS || s4 != APR_SUCCESS) {
        apr_file_printf(err, "Error starting threads\n");
        return s1;
    }
    apr_file_printf(out, "OK\n");

    apr_file_printf(out, "Waiting for threads to exit.......");
    apr_thread_join(&s1, t1);
    apr_thread_join(&s2, t2);
    apr_thread_join(&s3, t3);
    apr_thread_join(&s4, t4);
    apr_file_printf(out, "OK\n");

    apr_file_printf(out, "OK\n");
    if (x != MAX_ITER) {
        apr_file_printf(err, "The locks didn't work????  %d\n", x);
    }
    else {
        apr_file_printf(out, "Everything is working!\n");
    }
    return APR_SUCCESS;
}

int test_exclusive(void)
{
    apr_thread_t *t1, *t2, *t3, *t4;
    apr_status_t s1, s2, s3, s4;

    apr_file_printf(out, "Initializing the lock.");
    s1 = apr_lock_create(&thread_lock, APR_MUTEX, APR_INTRAPROCESS, 
                         "lock.file", context); 

    if (s1 != APR_SUCCESS) {
        apr_file_printf(err, "Could not create lock\n");
        return s1;
    }
    apr_file_printf(out, " OK\n");

    i = 0;
    x = 0;

    apr_file_printf(out, "Starting all the threads......."); 
    s1 = apr_thread_create(&t1, NULL, thread_func, NULL, context);
    s2 = apr_thread_create(&t2, NULL, thread_func, NULL, context);
    s3 = apr_thread_create(&t3, NULL, thread_func, NULL, context);
    s4 = apr_thread_create(&t4, NULL, thread_func, NULL, context);
    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS || 
        s3 != APR_SUCCESS || s4 != APR_SUCCESS) {
        apr_file_printf(err, "Error starting threads\n");
        return s1;
    }
    apr_file_printf(out, "OK\n");

    apr_file_printf(out, "Waiting for threads to exit.......");
    apr_thread_join(&s1, t1);
    apr_thread_join(&s2, t2);
    apr_thread_join(&s3, t3);
    apr_thread_join(&s4, t4);
    apr_file_printf(out, "OK\n");

    apr_file_printf(out, "OK\n");
    if (x != MAX_ITER) {
        apr_file_printf(err, "The locks didn't work????  %d\n", x);
    }
    else {
        apr_file_printf(out, "Everything is working!\n");
    }
    return APR_SUCCESS;
}

int main(void)
{
    apr_initialize();
    atexit(apr_terminate);

    if (apr_pool_create(&context, NULL) != APR_SUCCESS)
        exit(-1);

    apr_file_open_stdin(&in, context); 
    apr_file_open_stdout(&out, context); 
    apr_file_open_stderr(&err, context); 

    apr_file_printf(out, "OK\n");

    if (test_rw() != APR_SUCCESS)
        exit(-2);

    if (test_exclusive() != APR_SUCCESS)
        exit(-3);

    return 1;
}

#endif /* !APR_HAS_THREADS */
