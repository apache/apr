/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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

#include "apr.h"
#include "apr_private.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "fileio.h"

/* System headers required for thread/process library */
#if APR_HAVE_PTHREAD_H
#include <pthread.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if APR_HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
/* End System Headers */


#ifndef THREAD_PROC_H
#define THREAD_PROC_H

#define SHELL_PATH "/bin/sh"

#if APR_HAS_THREADS
struct apr_thread_t {
    apr_pool_t *cntxt;
    pthread_t *td;
};

struct apr_threadattr_t {
    apr_pool_t *cntxt;
    pthread_attr_t *attr;
};

struct apr_threadkey_t {
    apr_pool_t *cntxt;
    pthread_key_t key;
};
#endif

struct apr_procattr_t {
    apr_pool_t *cntxt;
    apr_file_t *parent_in;
    apr_file_t *child_in;
    apr_file_t *parent_out;
    apr_file_t *child_out;
    apr_file_t *parent_err;
    apr_file_t *child_err;
    char *currdir;
    apr_int32_t cmdtype;
    apr_int32_t detached;
#ifdef RLIMIT_CPU
    struct rlimit *limit_cpu;
#endif
#if defined (RLIMIT_DATA) || defined (RLIMIT_VMEM) || defined(RLIMIT_AS)
    struct rlimit *limit_mem;
#endif
#ifdef RLIMIT_NPROC
    struct rlimit *limit_nproc;
#endif
};

#endif  /* ! THREAD_PROC_H */

