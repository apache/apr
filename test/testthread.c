/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */
#include "apr_thread_proc.h"
#include "apr_lock.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "errno.h"
#include <stdio.h>
#ifdef BEOS
#include <unistd.h>
#endif


void * API_THREAD_FUNC thread_func1(void *data);
void * API_THREAD_FUNC thread_func2(void *data);
void * API_THREAD_FUNC thread_func3(void *data);
void * API_THREAD_FUNC thread_func4(void *data);


ap_lock_t *thread_lock;
ap_context_t *context;
int x = 0;

void * API_THREAD_FUNC thread_func1(void *data)
{
    int i;
    for (i = 0; i < 10000; i++) {
        ap_lock(thread_lock);
        x++;
        ap_unlock(thread_lock);
    }
    return NULL;
} 

void * API_THREAD_FUNC thread_func2(void *data)
{
    int i;
    for (i = 0; i < 10000; i++) {
        ap_lock(thread_lock);
        x++;
        ap_unlock(thread_lock);
    }
    return NULL;
} 

void * API_THREAD_FUNC thread_func3(void *data)
{
    int i;
    for (i = 0; i < 10000; i++) {
        ap_lock(thread_lock);
        x++;
        ap_unlock(thread_lock);
    }
    return NULL;
} 

void * API_THREAD_FUNC thread_func4(void *data)
{
    int i;
    for (i = 0; i < 10000; i++) {
        ap_lock(thread_lock);
        x++;
        ap_unlock(thread_lock);
    }
    return NULL;
} 

int main()
{
    ap_thread_t *t1;
    ap_thread_t *t2;
    ap_thread_t *t3;
    ap_thread_t *t4;
    ap_status_t s1;
    ap_status_t s2;
    ap_status_t s3;
    ap_status_t s4;

    ap_initialize();

    fprintf(stdout, "Initializing the context......."); 
    if (ap_create_context(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "could not initialize\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Initializing the lock......."); 
    s1 = ap_create_lock(&thread_lock, APR_MUTEX, APR_INTRAPROCESS, "lock.file", context); 
    if (s1 != APR_SUCCESS) {
        fprintf(stderr, "Could not create lock\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Starting all the threads......."); 
    s1 = ap_create_thread(&t1, NULL, thread_func1, NULL, context);
    s2 = ap_create_thread(&t2, NULL, thread_func2, NULL, context);
    s3 = ap_create_thread(&t3, NULL, thread_func3, NULL, context);
    s4 = ap_create_thread(&t4, NULL, thread_func4, NULL, context);
    if (s1 != APR_SUCCESS || s2 != APR_SUCCESS || 
        s3 != APR_SUCCESS || s4 != APR_SUCCESS) {
        fprintf(stderr, "Error starting thread\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Waiting for threads to exit.......");
    ap_thread_join(&s1, t1);
    ap_thread_join(&s2, t2);
    ap_thread_join(&s3, t3);
    ap_thread_join(&s4, t4);
    fprintf (stdout, "OK\n");   

    fprintf(stdout, "Checking if locks worked......."); 
    if (x != 40000) {
        fprintf(stderr, "The locks didn't work????  %d\n", x);
    }
    else {
        fprintf(stdout, "Everything is working!\n");
    }

    return 1;
}
