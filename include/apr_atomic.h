/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

#ifndef APR_ATOMIC_H
#define APR_ATOMIC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file apr_atomic.h
 * @brief APR Atomic Operations
 */
/**
 * @defgroup APR_Atomic Atomic operations
 * @ingroup APR
 * @{
 */

/* easiest way to get these documented for the moment */
#if defined(DOXYGEN)
/**
 * structure for holding a atomic value.
 * this number >only< has a 24 bit size on some platforms
 */
typedef apr_atomic_t;

/**
 * @param pool 
 * this function is required on some platforms to initiliaze the
 * atomic operation's internal structures
 * returns APR_SUCCESS on successfull completion
 */
apr_status_t apr_atomic_init(apr_pool_t *p);
/**
 * read the value stored in a atomic variable
 * @param the pointer
 * @warning on certain platforms (linux) this number is not
 * stored directly in the pointer. in others it is 
 */
apr_uint32_t apr_atomic_read(volatile apr_atomic_t *mem);
/**
 * set the value for atomic.
 * @param the pointer
 * @param the value
 */
void apr_atomic_set(volatile apr_atomic_t *mem, apr_uint32_t val);
/**
 * Add 'val' to the atomic variable
 * @param mem pointer to the atomic value
 * @param val the addition
 */
void apr_atomic_add(volatile apr_atomic_t *mem, apr_uint32_t val);

/**
 * increment the atomic variable by 1
 * @param mem pointer to the atomic value
 */
void apr_atomic_inc(volatile apr_atomic_t *mem);

/**
 * decrement the atomic variable by 1
 * @param mem pointer to the atomic value
 */
void apr_atomic_dec(volatile apr_atomic_t *mem);

/**
 * compare the atomic's value with cmp.
 * If they are the same swap the value with 'with'
 * @param mem pointer to the atomic value
 * @param with what to swap it with
 * @param the value to compare it to
 * @return the old value of the atomic
 * @warning do not mix apr_atomic's with the CAS function.
 * on some platforms they may be implemented by different mechanisms
 */
apr_uint32_t apr_atomic_cas(volatile apr_uint32_t *mem,long with,long cmp);
#else /* !DOXYGEN */

#ifdef WIN32

#define apr_atomic_t LONG;

#define apr_atomic_add(mem, val)     InterlockedExchangeAdd(mem,val)
#define apr_atomic_dec(mem)          InterlockedDecrement(mem)
#define apr_atomic_inc(mem)          InterlockedIncrement(mem)
#define apr_atomic_set(mem, val)     InterlockedExchange(mem, val)
#define apr_atomic_read(mem)         *mem
#define apr_atomic_cas(mem,with,cmp) InterlockedCompareExchange(mem,with,cmp)
#define apr_atomic_init(pool)        APR_SUCCESS

#elif defined(__linux)

#include <asm/atomic.h>
#include <asm/system.h>
#define apr_atomic_t atomic_t

#define apr_atomic_add(mem, val)     atomic_add(val,mem)
#define apr_atomic_dec(mem)          atomic_dec(mem)
#define apr_atomic_inc(mem)          atomic_inc(mem)
#define apr_atomic_set(mem, val)     atomic_set(mem, val)
#define apr_atomic_read(mem)         atomic_read(mem)
#if defined(cmpxchg)
#define apr_atomic_init(pool)        APR_SUCCESS
#define apr_atomic_cas(mem,with,cmp) cmpxchg(mem,cmp,with)
#else
#define APR_ATOMIC_NEED_CAS_DEFAULT 1
#endif

#elif defined(__FreeBSD__) && (__FreeBSD__ >= 4)
#include <machine/atomic.h>

#define apr_atomic_t apr_uint32_t
#define apr_atomic_add(mem, val)     atomic_add_int(mem,val)
#define apr_atomic_dec(mem)          atomic_subtract_int(mem,1)
#define apr_atomic_inc(mem)          atomic_add_int(mem,1)
#define apr_atomic_set(mem, val)     atomic_set_int(mem, val)
#define apr_atomic_read(mem)         *mem
#define apr_atomic_init(pool)        APR_SUCCESS

#define APR_ATOMIC_NEED_CAS_DEFAULT 1

#elif defined(__sparc__)
#define apr_atomic_t apr_uint32_t
#define apr_atomic_read(p)  *p

#define apr_atomic_add(mem, val)     apr_atomic_add_sparc(mem,val)
#define apr_atomic_dec(mem)          apr_atomic_sub_sparc(mem,1)
#define apr_atomic_inc(mem)          apr_atomic_add_sparc(mem,1)
#define apr_atomic_cas(mem,val,cond) apr_atomic_cas_sparc(mem,val,cond)
#define apr_atomic_casptr(mem,val,cond) apr_atomic_casptr_sparc(mem,val,cond)
#define apr_atomic_set(mem, val)     *mem= val
#define apr_atomic_init(pool)        APR_SUCCESS

apr_uint32_t apr_atomic_add_sparc( volatile apr_atomic_t* mem, apr_uint32_t add);
apr_uint32_t apr_atomic_sub_sparc( volatile apr_atomic_t* mem, apr_uint32_t sub);
long apr_atomic_cas_sparc(volatile apr_atomic_t *mem,long with,long cmp);

#else
#if APR_HAS_THREADS

#define apr_atomic_t apr_uint32_t
#define apr_atomic_read(p)  *p
apr_status_t apr_atomic_init(apr_pool_t *p);
void apr_atomic_set(volatile apr_atomic_t *mem, apr_uint32_t val);
void apr_atomic_add(volatile apr_atomic_t *mem, apr_uint32_t val);
void apr_atomic_inc(volatile apr_atomic_t *mem);
void apr_atomic_dec(volatile apr_atomic_t *mem);

#define APR_ATOMIC_NEED_DEFAULT 1
#define APR_ATOMIC_NEED_CAS_DEFAULT 1

#endif /* APR_HAS_THREADS */

#endif /* !defined(WIN32) && !defined(__linux) */

#if defined(APR_ATOMIC_NEED_CAS_DEFAULT)
apr_status_t apr_atomic_init(apr_pool_t *p);
apr_uint32_t apr_atomic_cas(volatile apr_uint32_t *mem,long with,long cmp);
#endif

#endif /* DOXYGEN */
#ifdef __cplusplus
}
#endif

#endif	/* !APR_ATOMIC_H */
