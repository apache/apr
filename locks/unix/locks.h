/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
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
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */
#ifndef LOCKS_H
#define LOCKS_H

#include "apr_config.h"
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
#if HAVE_SYS_TYPES_H
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
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if APR_HAS_THREADS
#if HAVE_PTHREAD_H
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

struct lock_t {
    ap_context_t *cntxt;
    ap_locktype_e type;
    ap_lockscope_e scope;
    int curr_locked;
    char *fname;
#if USE_SYSVSEM_SERIALIZE
    int interproc;
#elif USE_FCNTL_SERIALIZE
    int interproc;
#elif USE_PROC_PTHREAD_SERIALIZE
    pthread_mutex_t *interproc;
#elif USE_FLOCK_SERIALIZE
    int interproc;
#else
    /* No Interprocess serialization.  Too bad. */
#endif 
#if APR_HAS_THREADS
    /* APR doesn't have threads, no sense in having an thread lock mechanism.
     */
#if USE_PTHREAD_SERIALIZE
    pthread_mutex_t *intraproc;
#endif
#endif
    /* At some point, we should do a scope for both inter and intra process
     *  locking here.  Something like pthread_mutex with PTHREAD_PROCESS_SHARED
     */    
};

#if APR_HAS_THREADS
ap_status_t create_intra_lock(struct lock_t *new);
ap_status_t lock_intra(struct lock_t *lock);
ap_status_t unlock_intra(struct lock_t *lock);
ap_status_t destroy_intra_lock(struct lock_t *lock);
#endif

void setup_lock();
ap_status_t create_inter_lock(struct lock_t *new);
ap_status_t lock_inter(struct lock_t *lock);
ap_status_t unlock_inter(struct lock_t *lock);
ap_status_t destroy_inter_lock(struct lock_t *lock);

ap_status_t child_init_lock(struct lock_t **lock, ap_context_t *cont, 
                            char *fname);

#endif  /* LOCKS_H */

