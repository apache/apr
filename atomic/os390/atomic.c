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

#include <stdlib.h>

apr_status_t apr_atomic_init(apr_pool_t *p)
{
    return APR_SUCCESS;
}

apr_uint32_t apr_atomic_add32(volatile apr_uint32_t *mem, apr_uint32_t val)
{
    apr_uint32_t old, new_val; 

    old = *mem;   /* old is automatically updated on cs failure */
    do {
        new_val = old + val;
    } while (__cs(&old, (cs_t *)mem, new_val));
    return old;
}

void apr_atomic_sub32(volatile apr_uint32_t *mem, apr_uint32_t val)
{
     apr_uint32_t old, new_val;

     old = *mem;   /* old is automatically updated on cs failure */
     do {
         new_val = old - val;
     } while (__cs(&old, (cs_t *)mem, new_val));
}

apr_uint32_t apr_atomic_inc32(volatile apr_uint32_t *mem)
{
    return apr_atomic_add32(mem, 1);
}

int apr_atomic_dec32(volatile apr_uint32_t *mem)
{
    apr_uint32_t old, new_val; 

    old = *mem;   /* old is automatically updated on cs failure */
    do {
        new_val = old - 1;
    } while (__cs(&old, (cs_t *)mem, new_val)); 

    return new_val != 0;
}

apr_uint32_t apr_atomic_read32(volatile apr_uint32_t *mem)
{
    return *mem;
}

void apr_atomic_set32(volatile apr_uint32_t *mem, apr_uint32_t val)
{
    *mem = val;
}

apr_uint32_t apr_atomic_cas32(volatile apr_uint32_t *mem, apr_uint32_t swap, 
                              apr_uint32_t cmp)
{
    apr_uint32_t old = cmp;
    
    __cs(&old, (cs_t *)mem, swap);
    return old; /* old is automatically updated from mem on cs failure */
}

apr_uint32_t apr_atomic_xchg32(volatile apr_uint32_t *mem, apr_uint32_t val)
{
    apr_uint32_t old, new_val; 

    old = *mem;   /* old is automatically updated on cs failure */
    do {
        new_val = val;
    } while (__cs(&old, (cs_t *)mem, new_val)); 

    return old;
}

