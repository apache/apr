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

#include "apr_shm.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_proc_mutex.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_getopt.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include "test_apr.h"


#define MAX_ITER 4000
#define MAX_COUNTER (MAX_ITER * 4)

apr_proc_mutex_t *proc_lock;
apr_pool_t *pool;
int *x;

static int make_child(apr_proc_t **proc, apr_pool_t *p)
{
    int i = 0;
    *proc = apr_pcalloc(p, sizeof(**proc));

    /* slight delay to allow things to settle */
    apr_sleep (1);

    if (apr_proc_fork(*proc, p) == APR_INCHILD) {
        /* The parent process has setup all processes to call apr_terminate
         * at exit.  But, that means that all processes must also call
         * apr_initialize at startup.  You cannot have an unequal number
         * of apr_terminate and apr_initialize calls.  If you do, bad things
         * will happen.  In this case, the bad thing is that if the mutex
         * is a semaphore, it will be destroyed before all of the processes
         * die.  That means that the test will most likely fail.
         */
        apr_initialize();

        while (1) {
            apr_proc_mutex_lock(proc_lock); 
            if (i == MAX_ITER) {
                apr_proc_mutex_unlock(proc_lock); 
                exit(1);
            }
            i++;
            (*x)++;
            apr_proc_mutex_unlock(proc_lock); 
        }
        exit(1);
    }
    return APR_SUCCESS;
}

static apr_status_t test_exclusive(const char *lockname)
{
    apr_proc_t *p1, *p2, *p3, *p4;
    apr_status_t s1, s2, s3, s4;
 
    printf("Exclusive lock test\n");
    printf("%-60s", "    Initializing the lock");
    s1 = apr_proc_mutex_create(&proc_lock, lockname, APR_LOCK_DEFAULT, pool);
 
    if (s1 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");
 
    printf("%-60s", "    Starting all of the processes");
    fflush(stdout);
    s1 = make_child(&p1, pool);
    s2 = make_child(&p2, pool);
    s3 = make_child(&p3, pool);
    s4 = make_child(&p4, pool);
    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS ||
        s3 != APR_SUCCESS || s4 != APR_SUCCESS) {
        printf("Failed!\n");
        return s1;
    }
    printf("OK\n");
 
    printf("%-60s", "    Waiting for processes to exit");
    s1 = apr_proc_wait(p1, NULL, NULL, APR_WAIT);
    s2 = apr_proc_wait(p2, NULL, NULL, APR_WAIT);
    s3 = apr_proc_wait(p3, NULL, NULL, APR_WAIT);
    s4 = apr_proc_wait(p4, NULL, NULL, APR_WAIT);
    printf("OK\n");
 
    if ((*x) != MAX_COUNTER) {
        fprintf(stderr, "Locks don't appear to work!  x = %d instead of %d\n",
                (*x), MAX_COUNTER);
    }
    else {
        printf("Test passed\n");
    }
    return APR_SUCCESS;
}

int main(int argc, const char * const *argv)
{
    apr_status_t rv;
    char errmsg[200];
    const char *lockname = NULL;
    const char *shmname = "shm.file";
    apr_getopt_t *opt;
    char optchar;
    const char *optarg;
    apr_shm_t *shm;

    printf("APR Proc Mutex Test\n==============\n\n");
        
    apr_initialize();
    atexit(apr_terminate);

    if (apr_pool_create(&pool, NULL) != APR_SUCCESS)
        exit(-1);

    if ((rv = apr_getopt_init(&opt, pool, argc, argv)) != APR_SUCCESS) {
        fprintf(stderr, "Could not set up to parse options: [%d] %s\n",
                rv, apr_strerror(rv, errmsg, sizeof errmsg));
        exit(-1);
    }
        
    while ((rv = apr_getopt(opt, "f:", &optchar, &optarg)) == APR_SUCCESS) {
        if (optchar == 'f') {
            lockname = optarg;
        }
    }

    if (rv != APR_SUCCESS && rv != APR_EOF) {
        fprintf(stderr, "Could not parse options: [%d] %s\n",
                rv, apr_strerror(rv, errmsg, sizeof errmsg));
        exit(-1);
    }

    apr_shm_create(&shm, sizeof(int), shmname, pool);
    x = apr_shm_baseaddr_get(shm);

    if ((rv = test_exclusive(lockname)) != APR_SUCCESS) {
        fprintf(stderr,"Exclusive Lock test failed : [%d] %s\n",
                rv, apr_strerror(rv, (char*)errmsg, 200));
        exit(-2);
    }
    
    return 0;
}

