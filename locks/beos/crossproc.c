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

#include "locks.h"

ap_status_t lock_inter_cleanup(void * data)
{
    ap_lock_t *lock = (ap_lock_t*)data;
    if (lock->curr_locked == 1) {
    	if (atomic_add(&lock->ben_interproc , -1) > 1){
    		release_sem (lock->sem_interproc);
    	}
    }
    return APR_SUCCESS;
}    

ap_status_t create_inter_lock(ap_lock_t *new)
{
    new->sem_interproc = (sem_id)ap_palloc(new->cntxt, sizeof(sem_id));
    new->ben_interproc = (int32)ap_palloc(new->cntxt, sizeof(int32));

    new->ben_interproc = 0;
    new->sem_interproc = create_sem(0, "ap_interproc");
    if (new->sem_interproc < B_NO_ERROR){
    	lock_inter_cleanup(new);
        return errno;
    }
    new->curr_locked = 0;
    ap_register_cleanup(new->cntxt, (void *)new, lock_inter_cleanup,
                        ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t lock_inter(ap_lock_t *lock)
{
	if (atomic_add(&lock->ben_interproc, 1) > 0){
		acquire_sem(lock->sem_interproc);
	} else {
		return errno;
	}
    lock->curr_locked = 1;
    return APR_SUCCESS;
}

ap_status_t unlock_inter(ap_lock_t *lock)
{
	if (atomic_add(&lock->ben_interproc, -1) > 1){
        release_sem(lock->sem_interproc);
        lock->curr_locked = 0;
    } else {
    	return errno;
    }
    return APR_SUCCESS;
}

ap_status_t destroy_inter_lock(ap_lock_t *lock)
{
    ap_status_t stat;
    if ((stat = lock_inter_cleanup(lock)) == APR_SUCCESS) {
        ap_kill_cleanup(lock->cntxt, lock, lock_inter_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}
