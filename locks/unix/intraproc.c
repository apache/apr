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

#include "locks.h"

#if APR_HAS_THREADS

#if defined(USE_PTHREAD_SERIALIZE)  

static ap_status_t lock_intra_cleanup(void *data)
{
    ap_lock_t *lock = (ap_lock_t *) data;
    return pthread_mutex_unlock(lock->intraproc);
}    

ap_status_t ap_unix_create_intra_lock(ap_lock_t *new)
{
    ap_status_t stat;
    pthread_mutexattr_t mattr;

    new->intraproc = (pthread_mutex_t *)ap_palloc(new->cntxt, 
                              sizeof(pthread_mutex_t));
    if (new->intraproc == NULL ) {
        return errno;
    }
    if ((stat = pthread_mutexattr_init(&mattr))) {
        lock_intra_cleanup(new);
        return stat;
    }

    if ((stat = pthread_mutex_init(new->intraproc, &mattr))) {
        lock_intra_cleanup(new);
        return stat;
    }

    if ((stat = pthread_mutexattr_destroy(&mattr))) {
        lock_intra_cleanup(new);
        return stat;
    }

    new->curr_locked = 0;
    ap_register_cleanup(new->cntxt, (void *)new, lock_intra_cleanup,
                        ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_unix_lock_intra(ap_lock_t *lock)
{
    return pthread_mutex_lock(lock->intraproc);
}

ap_status_t ap_unix_unlock_intra(ap_lock_t *lock)
{
    ap_status_t status;

    status = pthread_mutex_unlock(lock->intraproc);
    return status;
}

ap_status_t ap_unix_destroy_intra_lock(ap_lock_t *lock)
{
    ap_status_t stat;
    if ((stat = lock_intra_cleanup(lock)) == APR_SUCCESS) {
        ap_kill_cleanup(lock->cntxt, lock, lock_intra_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}
#endif
#endif
