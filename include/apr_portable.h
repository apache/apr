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

#include "apr_general.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_lock.h"
#include "apr_time.h"
#ifdef HAVE_DIR_H
#include <dir.h>
#endif
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
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
typedef SYSTEMTIME            ap_os_time_t;
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
#if defined USE_SYSVSEM_SERIALIZE
    int crossproc;
    struct sembuf op_on;
    struct sembuf op_off;
#elif defined (USE_FCNTL_SERIALIZE)
    int crossproc;
    struct flock lock_it;
    struct flock unlock_it;
#elif defined (USE_PROC_PTHREAD_SERIALIZE)
    pthread_mutex_t *crossproc; 
#elif defined (USE_FLOCK_SERIALIZE)
    int crossproc;
#else
    /* No Interprocess serialization, too bad. */
#endif
#if defined (USE_PTHREAD_SERIALIZE)
    pthread_mutex_t *intraproc;
#endif
};

typedef int                   ap_os_file_t;
typedef DIR                   ap_os_dir_t;
typedef int                   ap_os_sock_t;
typedef struct os_lock_t      ap_os_lock_t;
typedef pthread_t             ap_os_thread_t;
typedef pid_t                 ap_os_proc_t;
typedef pthread_key_t         ap_os_threadkey_t;
typedef struct timeval        ap_os_time_t;
#endif

ap_status_t ap_get_os_file(ap_file_t *, ap_os_file_t *);     
ap_status_t ap_get_os_dir(ap_dir_t *, ap_os_dir_t *);      
ap_status_t ap_get_os_sock(ap_socket_t *, ap_os_sock_t *);
ap_status_t ap_get_os_lock(ap_lock_t *, ap_os_lock_t *);     
ap_status_t ap_get_os_thread(ap_thread_t *, ap_os_thread_t *);
ap_status_t ap_get_os_proc(ap_proc_t *, ap_os_proc_t *);     
ap_status_t ap_get_os_time(ap_time_t *, ap_os_time_t **);     
ap_status_t ap_get_os_threadkey(ap_key_t *, ap_os_threadkey_t *);

ap_status_t ap_put_os_file(ap_context_t *, ap_file_t **, ap_os_file_t *); 
ap_status_t ap_put_os_dir(ap_context_t *, ap_dir_t **, ap_os_dir_t *); 
ap_status_t ap_put_os_sock(ap_context_t *, ap_socket_t **, ap_os_sock_t *);
ap_status_t ap_put_os_lock(ap_context_t *, ap_lock_t **, ap_os_lock_t *); 
ap_status_t ap_put_os_thread(ap_context_t *, ap_thread_t **, ap_os_thread_t *);
ap_status_t ap_put_os_proc(ap_context_t *, ap_proc_t **, ap_os_proc_t *); 
ap_status_t ap_put_os_time(ap_context_t *, ap_time_t **, ap_os_time_t *); 
ap_status_t ap_put_os_threadkey(ap_context_t *, ap_key_t **, ap_os_threadkey_t *);

