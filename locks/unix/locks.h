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
#ifndef LOCKS_H
#define LOCKS_H

#include "apr_lock.h"
#include "apr_file_io.h"

#if defined (USE_USLOCK_SERIALIZE)
#include <uslocks.h>
#elif defined (USE_SYSVSEM_SERIALIZE)
#include <sys/file.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#elif defined (USE_FLOCK_SERIALIZE)
#include <sys/file.h>
#include <stdio.h>
#elif defined (USE_FCNTL_SERIALIZE)
#include <stdio.h>
#include <fcntl.h>
#endif
#include <pthread.h>

#ifndef HAVE_UNION_SEMUN
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
    int curr_locked;
    char *fname;
#if defined (USE_SYSVSEM_SERIALIZE)
    int interproc;
    struct sembuf op_on;
    struct sembuf op_off;
#elif defined (USE_FCNTL_SERIALIZE) 
    int interproc;
    struct flock lock_it;
    struct flock unlock_it;
#elif defined (USE_PROC_PTHREAD_SERIALIZE)
    pthread_mutex_t *interproc;
#elif defined (USE_FLOCK_SERIALIZE)
    int interproc;
#else
    /* No Interprocess serialization.  Too bad. */
#endif 
#if defined (USE_PTHREAD_SERIALIZE)
    pthread_mutex_t *intraproc;
#endif
    /* At some point, we should do a type for both inter and intra process
     *  locking here.  Something like pthread_mutex with PTHREAD_PROCESS_SHARED
     */    
};

ap_status_t create_intra_lock(struct lock_t *new);
ap_status_t lock_intra(struct lock_t *lock);
ap_status_t unlock_intra(struct lock_t *lock);
ap_status_t destroy_intra_lock(struct lock_t *lock);

ap_status_t create_inter_lock(struct lock_t *new);
ap_status_t lock_inter(struct lock_t *lock);
ap_status_t unlock_inter(struct lock_t *lock);
ap_status_t destroy_inter_lock(struct lock_t *lock);

ap_status_t child_init_lock(ap_context_t *cont, char *fname,
			    struct lock_t **lock);

#endif  /* LOCKS_H */

