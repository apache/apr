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

#include "beos/locks.h"

apr_status_t lock_inter_cleanup(void * data)
{
    apr_lock_t *lock = (apr_lock_t*)data;
    if (lock->curr_locked == 1) {
    	if (atomic_add(&lock->ben_interproc , -1) > 1){
            release_sem (lock->sem_interproc);
    	}
    }
    delete_sem(lock->sem_interproc);
    return APR_SUCCESS;
}    

apr_status_t create_inter_lock(apr_lock_t *new)
{
    int32 stat;
    
    new->sem_interproc = (sem_id)apr_palloc(new->cntxt, sizeof(sem_id));
    new->ben_interproc = (int32)apr_palloc(new->cntxt, sizeof(int32));

    if ((stat = create_sem(0, "apr_interproc")) < B_NO_ERROR) {
        lock_inter_cleanup(new);
        return stat;
    }
    new->ben_interproc = 0;
    new->curr_locked = 0;
    new->sem_interproc = stat;
    apr_register_cleanup(new->cntxt, (void *)new, lock_inter_cleanup,
                        apr_null_cleanup);
    return APR_SUCCESS;
}

apr_status_t lock_inter(apr_lock_t *lock)
{
    int32 stat;
    
	if (atomic_add(&lock->ben_interproc, 1) > 0){
		if ((stat = acquire_sem(lock->sem_interproc)) != B_NO_ERROR){
		    atomic_add(&lock->ben_interproc, -1);
		    return stat;
		}
	}
    lock->curr_locked = 1;
    return APR_SUCCESS;
}

apr_status_t unlock_inter(apr_lock_t *lock)
{
    int32 stat;
    
	if (atomic_add(&lock->ben_interproc, -1) > 1){
        if ((stat = release_sem(lock->sem_interproc)) != B_NO_ERROR) {
            atomic_add(&lock->ben_interproc, 1);
            return stat;
        }
    }
    lock->curr_locked = 0;
    return APR_SUCCESS;
}

apr_status_t destroy_inter_lock(apr_lock_t *lock)
{
    apr_status_t stat;
    if ((stat = lock_inter_cleanup(lock)) == APR_SUCCESS) {
        apr_kill_cleanup(lock->cntxt, lock, lock_inter_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

apr_status_t child_init_lock(apr_lock_t **lock, apr_pool_t *cont, const char *fname)
{
    return APR_SUCCESS;
}
