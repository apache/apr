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
#include "apr_portable.h"

ap_status_t ap_create_lock(ap_lock_t **lock, ap_locktype_e type, 
                           ap_lockscope_e scope, const char *fname, 
                           ap_pool_t *cont)
{
    ap_lock_t *new;
    ap_status_t stat;

    new = (ap_lock_t *)ap_pcalloc(cont, sizeof(ap_lock_t));

    new->cntxt = cont;
    new->type  = type;
    new->scope = scope;
#if (APR_USE_FCNTL_SERIALIZE) || (APR_USE_FLOCK_SERIALIZE)
    /* file-based serialization primitives */
    if (scope != APR_INTRAPROCESS) {
        if (fname != NULL) {
            new->fname = ap_pstrdup(cont, fname);
        }
    }
#endif

    if (scope != APR_CROSS_PROCESS) {
#if APR_HAS_THREADS
        if ((stat = ap_unix_create_intra_lock(new)) != APR_SUCCESS) {
            return stat;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (scope != APR_INTRAPROCESS) {
        if ((stat = ap_unix_create_inter_lock(new)) != APR_SUCCESS) {
            return stat;
        }
    }
    *lock = new;
    return APR_SUCCESS;
}

ap_status_t ap_lock(ap_lock_t *lock)
{
    ap_status_t stat;
    if (lock->scope != APR_CROSS_PROCESS) {
#if APR_HAS_THREADS
        if ((stat = ap_unix_lock_intra(lock)) != APR_SUCCESS) {
            return stat;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (lock->scope != APR_INTRAPROCESS) {
        if ((stat = ap_unix_lock_inter(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_unlock(ap_lock_t *lock)
{
    ap_status_t stat;

    if (lock->scope != APR_CROSS_PROCESS) {
#if APR_HAS_THREADS
        if ((stat = ap_unix_unlock_intra(lock)) != APR_SUCCESS) {
            return stat;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (lock->scope != APR_INTRAPROCESS) {
        if ((stat = ap_unix_unlock_inter(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_destroy_lock(ap_lock_t *lock)
{
    ap_status_t stat;
    if (lock->scope != APR_CROSS_PROCESS) {
#if APR_HAS_THREADS
        if ((stat = ap_unix_destroy_intra_lock(lock)) != APR_SUCCESS) {
            return stat;
        }
#else
        return APR_ENOTIMPL;
#endif
    }
    if (lock->scope != APR_INTRAPROCESS) {
        if ((stat = ap_unix_destroy_inter_lock(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_child_init_lock(ap_lock_t **lock, const char *fname, 
                               ap_pool_t *cont)
{
    ap_status_t stat;
    if ((*lock)->scope != APR_CROSS_PROCESS) {
        if ((stat = ap_unix_child_init_lock(lock, cont, fname)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_get_lockdata(ap_lock_t *lock, const char *key, void *data)
{
    return ap_get_userdata(data, key, lock->cntxt);
}

ap_status_t ap_set_lockdata(ap_lock_t *lock, void *data, const char *key,
                            ap_status_t (*cleanup) (void *))
{
    return ap_set_userdata(data, key, cleanup, lock->cntxt);
}

ap_status_t ap_get_os_lock(ap_os_lock_t *oslock, ap_lock_t *lock)
{
    oslock->crossproc = lock->interproc;
#if APR_HAS_THREADS
#if APR_USE_PTHREAD_SERIALIZE
    oslock->intraproc = lock->intraproc;
#endif
#endif

    return APR_SUCCESS;
}

ap_status_t ap_put_os_lock(ap_lock_t **lock, ap_os_lock_t *thelock, 
                           ap_pool_t *cont)
{
    if (cont == NULL) {
        return APR_ENOPOOL;
    }
    if ((*lock) == NULL) {
        (*lock) = (ap_lock_t *)ap_pcalloc(cont, sizeof(ap_lock_t));
        (*lock)->cntxt = cont;
    }
    (*lock)->interproc = thelock->crossproc;
#if APR_HAS_THREADS
#if (APR_USE_PTHREAD_SERIALIZE)
    (*lock)->intraproc = thelock->intraproc;
#endif
#endif
    return APR_SUCCESS;
}
    
