/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
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

#include "apr_time.h"
#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "test_apr.h"


static void do_sleep(int howlong)
{
    apr_time_t then, now, diff;
    apr_time_t interval = apr_time_from_sec(howlong);

    printf("    I'm about to go to sleep!\n");

    then = apr_time_now();
    apr_sleep(interval);
    now = apr_time_now();

    diff = now - then;

    printf("%-60s","    Just woken up, checking how long I've been asleep");
    if (diff < interval * 0.99 || diff > interval * 1.01) {
        printf("Failed!\n\t(actual: %" APR_TIME_T_FMT
               ", wanted: %" APR_TIME_T_FMT")\n", diff, interval);
    } else {
        printf("OK\n");
    }
}

#if APR_HAS_THREADS
static void * APR_THREAD_FUNC time_a_thread(apr_thread_t *thd, void *data)
{
    do_sleep(15);

    return NULL;
}
#define OUTPUT_LINES 8
#else
#define OUTPUT_LINES 2
#endif /* APR_HAS_THREADS */

int main(void)
{
    apr_pool_t *p;
#if APR_HAS_THREADS
    apr_thread_t *t1, *t2, *t3;
    apr_status_t rv;
#endif

    apr_initialize();

    printf("Testing apr_sleep()\n===================\n\n");

    STD_TEST_NEQ("Creating a pool to use", apr_pool_create(&p, NULL))

#if APR_HAS_THREADS
    printf("\nI will now start 3 threads, each of which should sleep for\n"
           "15 seconds, then exit.\n");
#endif

    printf("The main app will sleep for 45 seconds then wake up.\n");
    printf("All tests will check how long they've been in the land of nod.\n\n");
    printf("If all is working you should see %d lines below within 45 seconds.\n\n",
           OUTPUT_LINES);

#if APR_HAS_THREADS
    rv = apr_thread_create(&t1, NULL, time_a_thread, NULL, p);
    rv = apr_thread_create(&t2, NULL, time_a_thread, NULL, p);
    rv = apr_thread_create(&t3, NULL, time_a_thread, NULL, p);
#endif

    do_sleep(45);

    return 0;
}    

