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

#ifndef LOCKS_H
#define LOCKS_H

#include "apr.h"
#include "apr_private.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_lock.h"

/* System headers required by Locks library */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_USLOCKS_H
#include <uslocks.h>
#endif
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif
#if HAVE_SYS_SEM_H
#include <sys/sem.h>
#endif
#if HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if APR_HAS_THREADS
#if APR_HAVE_PTHREAD_H
#include <pthread.h>
#endif
#endif
/* End System Headers */

#if !APR_HAVE_UNION_SEMUN && APR_USE_SYSVSEM_SERIALIZE
/* it makes no sense, but this isn't defined on solaris */
union semun {
    long val;
    struct semid_ds *buf;
    ushort *array;
};
#endif

struct ap_lock_t {
    ap_pool_t *cntxt;
    ap_locktype_e type;
    ap_lockscope_e scope;
    int curr_locked;
    char *fname;
#if APR_USE_SYSVSEM_SERIALIZE
    int interproc;
#elif APR_USE_FCNTL_SERIALIZE
    int interproc;
#elif APR_USE_PROC_PTHREAD_SERIALIZE
    pthread_mutex_t *interproc;
#elif APR_USE_FLOCK_SERIALIZE
    int interproc;
#else
    /* No Interprocess serialization.  Too bad. */
#endif 
#if APR_HAS_THREADS
    /* APR doesn't have threads, no sense in having an thread lock mechanism.
     */
#if APR_USE_PTHREAD_SERIALIZE
    pthread_mutex_t *intraproc;
#endif
#endif
    /* At some point, we should do a scope for both inter and intra process
     *  locking here.  Something like pthread_mutex with PTHREAD_PROCESS_SHARED
     */    
};

#if APR_HAS_THREADS
ap_status_t ap_unix_create_intra_lock(struct ap_lock_t *new);
ap_status_t ap_unix_lock_intra(struct ap_lock_t *lock);
ap_status_t ap_unix_unlock_intra(struct ap_lock_t *lock);
ap_status_t ap_unix_destroy_intra_lock(struct ap_lock_t *lock);
#endif

void ap_unix_setup_lock(void);
ap_status_t ap_unix_create_inter_lock(struct ap_lock_t *new);
ap_status_t ap_unix_lock_inter(struct ap_lock_t *lock);
ap_status_t ap_unix_unlock_inter(struct ap_lock_t *lock);
ap_status_t ap_unix_destroy_inter_lock(struct ap_lock_t *lock);

ap_status_t ap_unix_child_init_lock(struct ap_lock_t **lock, ap_pool_t *cont, 
                            const char *fname);

#endif  /* LOCKS_H */

