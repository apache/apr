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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "apr.h"
#include "apr_general.h"
#include "apr_proc_mutex.h"
#include "apr_global_mutex.h"
#include "apr_thread_proc.h"

#if !APR_HAS_THREADS
int main(void)
{
    printf("This test requires APR thread support.\n");
    return 0;
}

#else /* APR_HAS_THREADS */

static apr_thread_mutex_t *thread_mutex;
static apr_proc_mutex_t *proc_mutex;
static apr_global_mutex_t *global_mutex;
static apr_pool_t *p;
static volatile int counter;
typedef enum {TEST_GLOBAL, TEST_PROC} test_mode_e;

static void lock_init(apr_lockmech_e mech, test_mode_e test_mode)
{
    if (test_mode == TEST_PROC) {
        assert(apr_proc_mutex_create(&proc_mutex,
                                     NULL,
                                     mech,
                                     p) == APR_SUCCESS);
    }
    else {
        assert(apr_global_mutex_create(&global_mutex,
                                       NULL,
                                       mech,
                                       p) == APR_SUCCESS);
    }
}

static void lock_destroy(test_mode_e test_mode)
{
    if (test_mode == TEST_PROC) {
        assert(apr_proc_mutex_destroy(proc_mutex) == APR_SUCCESS);
    }
    else {
        assert(apr_global_mutex_destroy(global_mutex) == APR_SUCCESS);
    }
}

static void lock_grab(test_mode_e test_mode)
{
    if (test_mode == TEST_PROC) {
        assert(apr_proc_mutex_lock(proc_mutex) == APR_SUCCESS);
    }
    else {
        assert(apr_global_mutex_lock(global_mutex) == APR_SUCCESS);
    }
}

static void lock_release(test_mode_e test_mode)
{
    if (test_mode == TEST_PROC) {
        assert(apr_proc_mutex_unlock(proc_mutex) == APR_SUCCESS);
    }
    else {
        assert(apr_global_mutex_unlock(global_mutex) == APR_SUCCESS);
    }
}

static void * APR_THREAD_FUNC eachThread(apr_thread_t *id, void *p)
{
    test_mode_e test_mode = (test_mode_e)p;

    lock_grab(test_mode);
    ++counter;
    assert(apr_thread_mutex_lock(thread_mutex) == APR_SUCCESS);
    assert(apr_thread_mutex_unlock(thread_mutex) == APR_SUCCESS);
    lock_release(test_mode);
    return NULL;
}

static void test_mech_mode(apr_lockmech_e mech, const char *mech_name,
                           test_mode_e test_mode)
{
  apr_thread_t *threads[20];
  int numThreads = 5;
  int i;
  apr_status_t rv;

  printf("Trying %s mutexes with mechanism `%s'...\n",
         test_mode == TEST_GLOBAL ? "global" : "proc", mech_name);

  assert(numThreads <= sizeof(threads) / sizeof(threads[0]));

  assert(apr_pool_create(&p, NULL) == APR_SUCCESS);

  assert(apr_thread_mutex_create(&thread_mutex, 0, p) == APR_SUCCESS);
  assert(apr_thread_mutex_lock(thread_mutex) == APR_SUCCESS);
  
  lock_init(mech, test_mode);

  counter = 0;

  i = 0;
  while (i < numThreads)
  {
    rv = apr_thread_create(&threads[i],
                           NULL,
                           eachThread,
                           (void *)test_mode,
                           p);
    if (rv != APR_SUCCESS) {
      fprintf(stderr, "apr_thread_create->%d\n", rv);
      exit(1);
    }
    ++i;
  }

  apr_sleep(apr_time_from_sec(5));

  if (test_mode == TEST_PROC) {
      printf("  Mutex mechanism `%s' is %sglobal in scope on this platform.\n",
             mech_name, counter == 1 ? "" : "not ");
  }
  else {
      if (counter != 1) {
          fprintf(stderr, "\n!!!apr_global_mutex operations are broken on this "
                  "platform for mutex mechanism `%s'!\n"
                  "They don't block out threads within the same process.\n",
                  mech_name);
          fprintf(stderr, "counter value: %d\n", counter);
          exit(1);
      }
      else {
          printf("  no problems encountered...\n");
      }
  }
  
  assert(apr_thread_mutex_unlock(thread_mutex) == APR_SUCCESS);

  i = 0;
  while (i < numThreads)
  {
    apr_status_t ignored;

    rv = apr_thread_join(&ignored,
                         threads[i]);
    assert(rv == APR_SUCCESS);
    ++i;
  }

  lock_destroy(test_mode);
  apr_thread_mutex_destroy(thread_mutex);
  apr_pool_destroy(p);
}

static void test_mech(apr_lockmech_e mech, const char *mech_name)
{
    test_mech_mode(mech, mech_name, TEST_PROC);
    test_mech_mode(mech, mech_name, TEST_GLOBAL);
}

int main(void)
{
    struct {
        apr_lockmech_e mech;
        const char *mech_name;
    } lockmechs[] = {
        {APR_LOCK_DEFAULT, "default"}
#if APR_HAS_FLOCK_SERIALIZE
        ,{APR_LOCK_FLOCK, "flock"}
#endif
#if APR_HAS_SYSVSEM_SERIALIZE
        ,{APR_LOCK_SYSVSEM, "sysvsem"}
#endif
#if APR_HAS_POSIXSEM_SERIALIZE
        ,{APR_LOCK_POSIXSEM, "posix"}
#endif
#if APR_HAS_FCNTL_SERIALIZE
        ,{APR_LOCK_FCNTL, "fcntl"}
#endif
#if APR_HAS_PROC_PTHREAD_SERIALIZE
        ,{APR_LOCK_PROC_PTHREAD, "proc_pthread"}
#endif
    };
    int i;
        
    assert(apr_initialize() == APR_SUCCESS);

    for (i = 0; i < sizeof(lockmechs) / sizeof(lockmechs[0]); i++) {
        test_mech(lockmechs[i].mech, lockmechs[i].mech_name);
    }
    
    apr_terminate();
    return 0;
}

#endif /* APR_HAS_THREADS */
