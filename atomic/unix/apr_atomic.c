/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "apr.h"
#include "apr_atomic.h"
#include "apr_thread_mutex.h"

#if !defined(apr_atomic_init) && !defined(APR_OVERRIDE_ATOMIC_INIT)

#if APR_HAS_THREADS
#define NUM_ATOMIC_HASH 7
/* shift by 2 to get rid of alignment issues */
#define ATOMIC_HASH(x) (unsigned int)(((unsigned long)(x)>>2)%(unsigned int)NUM_ATOMIC_HASH)
static apr_thread_mutex_t **hash_mutex;
#endif /* APR_HAS_THREADS */

apr_status_t apr_atomic_init(apr_pool_t *p)
{
#if APR_HAS_THREADS
    int i;
    apr_status_t rv;
    hash_mutex = apr_palloc(p, sizeof(apr_thread_mutex_t*) * NUM_ATOMIC_HASH);

    for (i = 0; i < NUM_ATOMIC_HASH; i++) {
        rv = apr_thread_mutex_create(&(hash_mutex[i]),
                                     APR_THREAD_MUTEX_DEFAULT, p);
        if (rv != APR_SUCCESS) {
           return rv;
        }
    }
#endif /* APR_HAS_THREADS */
    return APR_SUCCESS;
}
#endif /*!defined(apr_atomic_init) && !defined(APR_OVERRIDE_ATOMIC_INIT) */

#if !defined(apr_atomic_add) && !defined(APR_OVERRIDE_ATOMIC_ADD)
void apr_atomic_add(volatile apr_atomic_t *mem, apr_uint32_t val) 
{
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    apr_uint32_t prev;
       
    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        *mem += val;
        apr_thread_mutex_unlock(lock);
    }
#else
    *mem += val;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_add) && !defined(APR_OVERRIDE_ATOMIC_ADD) */

#if !defined(apr_atomic_set) && !defined(APR_OVERRIDE_ATOMIC_SET)
void apr_atomic_set(volatile apr_atomic_t *mem, apr_uint32_t val) 
{
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    apr_uint32_t prev;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        *mem = val;
        apr_thread_mutex_unlock(lock);
    }
#else
    *mem = val;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_set) && !defined(APR_OVERRIDE_ATOMIC_SET) */

#if !defined(apr_atomic_inc) && !defined(APR_OVERRIDE_ATOMIC_INC)
void apr_atomic_inc(volatile apr_uint32_t *mem) 
{
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    apr_uint32_t prev;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        (*mem)++;
        apr_thread_mutex_unlock(lock);
    }
#else
    (*mem)++;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_inc) && !defined(APR_OVERRIDE_ATOMIC_INC) */

#if !defined(apr_atomic_dec) && !defined(APR_OVERRIDE_ATOMIC_DEC)
int apr_atomic_dec(volatile apr_atomic_t *mem) 
{
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    apr_uint32_t new;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        (*mem)--;
        new = *mem;
        apr_thread_mutex_unlock(lock);
        return new; 
    }
#else
    (*mem)--;
#endif /* APR_HAS_THREADS */
    return *mem; 
}
#endif /*!defined(apr_atomic_dec) && !defined(APR_OVERRIDE_ATOMIC_DEC) */

#if !defined(apr_atomic_cas) && !defined(APR_OVERRIDE_ATOMIC_CAS)
apr_uint32_t apr_atomic_cas(volatile apr_uint32_t *mem, long with, long cmp)
{
    long prev;
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *(long*)mem;
        if (prev == cmp) {
            *(long*)mem = with;
        }
        apr_thread_mutex_unlock(lock);
        return prev;
    }
    return *(long*)mem;
#else
    prev = *(long*)mem;
    if (prev == cmp) {
        *(long*)mem = with;
    }
    return prev;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_cas) && !defined(APR_OVERRIDE_ATOMIC_CAS) */

#if !defined(apr_atomic_casptr) && !defined(APR_OVERRIDE_ATOMIC_CASPTR)
void *apr_atomic_casptr(volatile void **mem, void *with, const void *cmp)
{
    void *prev;
#if APR_HAS_THREADS
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *(void **)mem;
        if (prev == cmp) {
            *mem = with;
        }
        apr_thread_mutex_unlock(lock);
        return prev;
    }
    return *(void **)mem;
#else
    prev = *(void **)mem;
    if (prev == cmp) {
        *mem = with;
    }
    return prev;
#endif /* APR_HAS_THREADS */
}
#endif /*!defined(apr_atomic_cas) && !defined(APR_OVERRIDE_ATOMIC_CAS) */
