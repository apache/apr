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
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

#ifndef APR_ATOMIC_H
#define APR_ATOMIC_H

/**
 * @file apr_atomic.h
 * @brief APR Atomic Operations
 */

#include "apr.h"
#include "apr_pools.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup apr_atomic Atomic Operations
 * @ingroup APR 
 * @{
 */

/**
 * this function is required on some platforms to initialize the
 * atomic operation's internal structures
 * @param p pool
 * @return APR_SUCCESS on successful completion
 */
APR_DECLARE(apr_status_t) apr_atomic_init(apr_pool_t *p);

/*
 * Atomic operations on 32-bit values
 * Note: Each of these functions internally implements a memory barrier
 * on platforms that require it
 */

/**
 * atomically read an apr_uint32_t from memory
 * @param mem the pointer
 */
APR_DECLARE(apr_uint32_t) apr_atomic_read32(volatile apr_uint32_t *mem);

/**
 * atomically set an apr_uint32_t in memory
 * @param mem pointer to the object
 */
APR_DECLARE(void) apr_atomic_set32(volatile apr_uint32_t *mem, apr_uint32_t val);

/**
 * atomically add 'val' to an apr_uint32_t
 * @param mem pointer to the object
 * @param val amount to add
 * @return old value pointed to by mem
 */
APR_DECLARE(apr_uint32_t) apr_atomic_add32(volatile apr_uint32_t *mem, apr_uint32_t val);

/**
 * atomically subtract 'val' from an apr_uint32_t
 * @param mem pointer to the object
 * @param val amount to subtract
 */
APR_DECLARE(void) apr_atomic_sub32(volatile apr_uint32_t *mem, apr_uint32_t val);

/**
 * atomically increment an apr_uint32_t by 1
 * @param mem pointer to the object
 * @return old value pointed to by mem
 */
APR_DECLARE(apr_uint32_t) apr_atomic_inc32(volatile apr_uint32_t *mem);

/**
 * atomically decrement an apr_uint32_t by 1
 * @param mem pointer to the atomic value
 * @return zero if the value becomes zero on decrement, otherwise non-zero
 */
APR_DECLARE(int) apr_atomic_dec32(volatile apr_uint32_t *mem);

/**
 * compare an apr_uint32_t's value with 'cmp'.
 * If they are the same swap the value with 'with'
 * @param mem pointer to the value
 * @param with what to swap it with
 * @param cmp the value to compare it to
 * @return the old value of *mem
 */
APR_DECLARE(apr_uint32_t) apr_atomic_cas32(volatile apr_uint32_t *mem, apr_uint32_t with,
                              apr_uint32_t cmp);

/**
 * exchange an apr_uint32_t's value with 'val'.
 * @param mem pointer to the value
 * @param val what to swap it with
 * @return the old value of *mem
 */
APR_DECLARE(apr_uint32_t) apr_atomic_xchg32(volatile apr_uint32_t *mem, apr_uint32_t val);

/**
 * compare the pointer's value with cmp.
 * If they are the same swap the value with 'with'
 * @param mem pointer to the pointer
 * @param with what to swap it with
 * @param cmp the value to compare it to
 * @return the old value of the pointer
 */
APR_DECLARE(void*) apr_atomic_casptr(volatile void **mem, void *with, const void *cmp);

/** @} */

#ifdef __cplusplus
}
#endif

#endif	/* !APR_ATOMIC_H */
