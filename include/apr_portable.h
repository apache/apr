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

/* This header file is where you should put ANY platform specific information.
 * This should be the only header file that programs need to include that 
 * actually has platform dependant code which refers to the .
 */
#ifndef APR_PORTABLE_H
#define APR_PORTABLE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "apr_general.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_lock.h"
#include "apr_time.h"

#if APR_HAVE_DIRENT_H
#include <dirent.h>
#endif
#if APR_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if APR_HAVE_PTHREAD_H
#include <pthread.h>
#endif
#if APR_HAVE_UNION_SEMUN
#include <sys/sem.h>
#endif

#ifdef WIN32
/* The primitives for Windows types */
typedef HANDLE                ap_os_file_t;
typedef HANDLE                ap_os_dir_t;
typedef SOCKET                ap_os_sock_t;
typedef HANDLE                ap_os_lock_t;
typedef HANDLE                ap_os_thread_t;
typedef PROCESS_INFORMATION   ap_os_proc_t;
typedef DWORD                 ap_os_threadkey_t; 
typedef FILETIME              ap_os_imp_time_t;
typedef SYSTEMTIME            ap_os_exp_time_t;

#elif defined(OS2)
#define INCL_DOS
#include <os2.h>
typedef HFILE                 ap_os_file_t;
typedef HDIR                  ap_os_dir_t;
typedef int                   ap_os_sock_t;
typedef HMTX                  ap_os_lock_t;
typedef TID                   ap_os_thread_t;
typedef PID                   ap_os_proc_t;
typedef PULONG                ap_os_threadkey_t; 
typedef struct timeval        ap_os_imp_time_t;
typedef struct tm             ap_os_exp_time_t;

#elif defined(BEOS)
#include <kernel/OS.h>

struct os_lock_t {
	/* Inter proc */
	sem_id sem_interproc;
	int32  ben_interproc;
	/* Intra Proc */
	sem_id sem_intraproc;
	int32  ben_intraproc;
};

typedef int                   ap_os_file_t;
typedef DIR                   ap_os_dir_t;
typedef int                   ap_os_sock_t;
typedef struct os_lock_t      ap_os_lock_t;
typedef thread_id             ap_os_thread_t;
typedef thread_id             ap_os_proc_t;
typedef int                   ap_os_threadkey_t;
typedef struct timeval        ap_os_imp_time_t;
typedef struct tm             ap_os_exp_time_t;

#else
/* Any other OS should go above this one.  This is the lowest common
 * denominator typedefs for  all UNIX-like systems.  :)
 */

#ifdef NEED_UNION_SEMUN
/* it makes no sense, but this isn't defined on solaris */
union semun {
    long val;
    struct semid_ds *buf;
    ushort *array;
};
#endif

struct os_lock_t {
#if APR_USE_SYSVSEM_SERIALIZE
    int crossproc;
#elif APR_USE_FCNTL_SERIALIZE
    int crossproc;
#elif APR_USE_PROC_PTHREAD_SERIALIZE
    pthread_mutex_t *crossproc; 
#elif APR_USE_FLOCK_SERIALIZE
    int crossproc;
#else
    /* No Interprocess serialization, too bad. */
#endif
#if APR_HAS_THREADS
    /* If no threads, no need for thread locks */
#if APR_USE_PTHREAD_SERIALIZE
    pthread_mutex_t *intraproc;
#endif
#endif
};

typedef int                   ap_os_file_t;
typedef DIR                   ap_os_dir_t;
typedef int                   ap_os_sock_t;
typedef struct os_lock_t      ap_os_lock_t;
#if APR_HAS_THREADS && APR_HAVE_PTHREAD_H 
typedef pthread_t             ap_os_thread_t;
typedef pthread_key_t         ap_os_threadkey_t;
#endif
typedef pid_t                 ap_os_proc_t;
typedef struct timeval        ap_os_imp_time_t;
typedef struct tm             ap_os_exp_time_t;
#endif

ap_status_t ap_get_os_file(ap_os_file_t *, ap_file_t *);     
ap_status_t ap_get_os_dir(ap_os_dir_t **, ap_dir_t *);      
ap_status_t ap_get_os_sock(ap_os_sock_t *, ap_socket_t *);
ap_status_t ap_get_os_lock(ap_os_lock_t *, ap_lock_t *);     
ap_status_t ap_get_os_proc(ap_os_proc_t *, ap_proc_t *);     
ap_status_t ap_get_os_exp_time(ap_os_exp_time_t **, ap_exploded_time_t *);     
ap_status_t ap_get_os_imp_time(ap_os_imp_time_t **, ap_time_t *);     
#if APR_HAS_THREADS
ap_status_t ap_get_os_thread(ap_os_thread_t *, ap_thread_t *);
ap_status_t ap_get_os_threadkey(ap_os_threadkey_t *, ap_key_t *);
#endif

ap_status_t ap_put_os_file(ap_file_t **, ap_os_file_t *, ap_context_t *); 
ap_status_t ap_put_os_dir(ap_dir_t **, ap_os_dir_t *, ap_context_t *); 
ap_status_t ap_put_os_sock(ap_socket_t **, ap_os_sock_t *, ap_context_t *);
ap_status_t ap_put_os_lock(ap_lock_t **, ap_os_lock_t *, ap_context_t *); 
ap_status_t ap_put_os_proc(ap_proc_t **, ap_os_proc_t *, ap_context_t *); 
ap_status_t ap_put_os_imp_time(ap_time_t *, ap_os_imp_time_t **, ap_context_t *); 
ap_status_t ap_put_os_exp_time(ap_exploded_time_t *, ap_os_exp_time_t **, ap_context_t *); 
#if APR_HAS_THREADS
ap_status_t ap_put_os_thread(ap_thread_t **, ap_os_thread_t *, ap_context_t *);
ap_status_t ap_put_os_threadkey(ap_key_t **, ap_os_threadkey_t *, ap_context_t *);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_PORTABLE_H */
