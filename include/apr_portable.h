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

/* This header file is where you should put ANY platform specific information.
 * This should be the only header file that programs need to include that 
 * actually has platform dependant code which refers to the .
 */
#ifndef APR_PORTABLE_H
#define APR_PORTABLE_H

/**
 * @package APR portability Routines
 */

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
#include "apr_dso.h"

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

struct ap_os_lock_t {
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
typedef struct ap_os_lock_t      ap_os_lock_t;
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

struct ap_os_lock_t {
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
typedef struct ap_os_lock_t      ap_os_lock_t;
#if APR_HAS_THREADS && APR_HAVE_PTHREAD_H 
typedef pthread_t             ap_os_thread_t;
typedef pthread_key_t         ap_os_threadkey_t;
#endif
typedef pid_t                 ap_os_proc_t;
typedef struct timeval        ap_os_imp_time_t;
typedef struct tm             ap_os_exp_time_t;
#endif

/**
 * convert the file from apr type to os specific type.
 * @param thefile The os specific file we are converting to
 * @param file The apr file to convert.
 * @tip On Unix, it is only possible to get a file descriptor from 
 *      an apr file type.
 */
ap_status_t ap_get_os_file(ap_os_file_t *thefile, ap_file_t *file);     

/**
 * convert the dir from apr type to os specific type.
 * @param thedir The os specific dir we are converting to
 * @param dir The apr dir to convert.
 */   
ap_status_t ap_get_os_dir(ap_os_dir_t **thedir, ap_dir_t *dir);      

/**
 * Convert the socket from an apr type to an OS specific socket
 * @param thesock The socket to convert.
 * @param sock The os specifc equivelant of the apr socket..
 */
ap_status_t ap_get_os_sock(ap_os_sock_t *thesock, ap_socket_t *sock);

/**
 * Convert the lock from os specific type to apr type
 * @param oslock The os specific lock we are converting to.
 * @param lock The apr lock to convert.
 */
ap_status_t ap_get_os_lock(ap_os_lock_t *oslock, ap_lock_t *lock);     

/**
 * Get the exploded time in the platforms native format.
 * @param ostime the native time format
 * @param aprtime the time to convert
 */
ap_status_t ap_get_os_exp_time(ap_os_exp_time_t **ostime, ap_exploded_time_t *aprtime);

/**
 * Get the imploded time in the platforms native format.
 * @param ostime the native time format
 * @param aprtimethe time to convert
 */
ap_status_t ap_get_os_imp_time(ap_os_imp_time_t **ostime, ap_time_t *aprtime);

#if APR_HAS_THREADS
/**
 * convert the thread to os specific type from apr type.
 * @param thethd The apr thread to convert
 * @param thd The os specific thread we are converting to
 */
ap_status_t ap_get_os_thread(ap_os_thread_t **thethd, ap_thread_t *thd);

/**
 * convert the thread private memory key to os specific type from an apr type.
 * @param thekey The apr handle we are converting from.
 * @param key The os specific handle we are converting to.
 */
ap_status_t ap_get_os_threadkey(ap_os_threadkey_t *thekey, ap_threadkey_t *key);
#endif

/**
 * convert the file from os specific type to apr type.
 * @param file The apr file we are converting to.
 * @param thefile The os specific file to convert
 * @param cont The pool to use if it is needed.
 * @tip On Unix, it is only possible to put a file descriptor into
 *      an apr file type.
 */
ap_status_t ap_put_os_file(ap_file_t **file, ap_os_file_t *thefile, 
                           ap_pool_t *cont); 

/**
 * convert the dir from os specific type to apr type.
 * @param dir The apr dir we are converting to.
 * @param thedir The os specific dir to convert
 * @param cont The pool to use when creating to apr directory.
 */
ap_status_t ap_put_os_dir(ap_dir_t **dir, ap_os_dir_t *thedir, 
                          ap_pool_t *cont); 

/**
 * Convert a socket from the os specific type to the apr type
 * @param sock The pool to use.
 * @param thesock The socket to convert to.
 * @param cont The socket we are converting to an apr type.
 */
ap_status_t ap_put_os_sock(ap_socket_t **sock, ap_os_sock_t *thesock, 
                           ap_pool_t *cont);

/**
 * Convert the lock from os specific type to apr type
 * @param lock The apr lock we are converting to.
 * @param thelock The os specific lock to convert.
 * @param cont The pool to use if it is needed.
 */
ap_status_t ap_put_os_lock(ap_lock_t **lock, ap_os_lock_t *thelock, 
                           ap_pool_t *cont); 

/**
 * Put the imploded time in the APR format.
 * @param aprtime the APR time format
 * @param ostime the time to convert
 * @param cont the pool to use if necessary
 */
ap_status_t ap_put_os_imp_time(ap_time_t *aprtime, ap_os_imp_time_t **ostime, ap_pool_t *cont); 

/**
 * Put the exploded time in the APR format.
 * @param aprtime the APR time format
 * @param ostime the time to convert
 * @param cont the pool to use if necessary
 */
ap_status_t ap_put_os_exp_time(ap_exploded_time_t *aprtime, ap_os_exp_time_t **ostime, ap_pool_t *cont); 

#if APR_HAS_THREADS
/**
 * convert the thread from os specific type to apr type.
 * @param thd The apr thread we are converting to.
 * @param thethd The os specific thread to convert
 * @param cont The pool to use if it is needed.
 */
ap_status_t ap_put_os_thread(ap_thread_t **thd, ap_os_thread_t *thethd, 
                             ap_pool_t *cont);

/**
 * convert the thread private memory key from os specific type to apr type.
 * @param key The apr handle we are converting to.
 * @param thekey The os specific handle to convert
 * @param cont The pool to use if it is needed.
 */
ap_status_t ap_put_os_threadkey(ap_threadkey_t **key, ap_os_threadkey_t *thekey, 
                                ap_pool_t *cont);
#endif

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_PORTABLE_H */
