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
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_portable.h"
#include  <kernel/OS.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#ifndef THREAD_PROC_H
#define THREAD_PROC_H

#define SHELL_PATH "/bin/sh"

#define PTHREAD_CANCEL_AYNCHRONOUS  CANCEL_ASYNCH; 
#define PTHREAD_CANCEL_DEFERRED     CANCEL_DEFER; 
                                   
#define PTHREAD_CANCEL_ENABLE       CANCEL_ENABLE; 
#define PTHREAD_CANCEL_DISABLE      CANCEL_DISABLE; 

#define BEOS_MAX_DATAKEYS	128

struct thread_t {
    ap_context_t *cntxt;
    thread_id td;
};

struct threadattr_t {
    ap_context_t *cntxt;
    int32 attr;
    int detached;
    int joinable;
};

struct threadkey_t {
    ap_context_t *cntxt;
	int32  key;
};

struct beos_private_data {
	const void ** data;
	int count;
	volatile thread_id  td;
};

struct beos_key {
	int  assigned;
	int  count;
	sem_id  lock;
	int32  ben_lock;
	void (* destructor) ();
};

struct procattr_t {
    ap_context_t *cntxt;
    ap_file_t *parent_in;
    ap_file_t *child_in;
    ap_file_t *parent_out;
    ap_file_t *child_out;
    ap_file_t *parent_err;
    ap_file_t *child_err;
    char *currdir;
    ap_int32_t cmdtype;
    ap_int32_t detached;
};

struct proc_t {
    ap_context_t *cntxt;
    pid_t pid;
    thread_id tid;
    struct procattr_t *attr;
};

/* we need a structure to pass off to the thread that will run any
 * new process we create */

#endif  /* ! THREAD_PROC_H */

