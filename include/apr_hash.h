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
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

#ifndef APR_HASH_H
#define APR_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @package Hash Tables
 */

#include "apr_pools.h"

/*
 * When passing a key to apr_hash_set or apr_hash_get, this value can be
 * passed to indicate a string-valued key, and have apr_hash compute the
 * length automatically.
 *
 * Note: apr_hash will use strlen(key) for the length. The null-terminator
 *       is not included in the hash value (why throw a constant in?).
 *       Since the hash table merely references the provided key (rather
 *       than copying it), apr_hash_this() will return the null-term'd key.
 */
#define APR_HASH_KEY_STRING     (-1)

/*
 * Abstract type for scanning hash tables.
 */
typedef struct apr_hash_index_t apr_hash_index_t;

/**
 * Create a hash table within a pool.
 * @param pool The pool to allocate the hash table out of
 * @return The hash table just created
 * @deffunc apr_hash_t *apr_make_hash(apr_pool_t *pool)
 */
APR_DECLARE(apr_hash_t *) apr_make_hash(apr_pool_t *pool);

/**
 * Associate a value with a key in a hash table.
 * @param ht The hash table
 * @param key Pointer to the key
 * @param klen Length of the key. Can be APR_HASH_KEY_STRING.
 * @param val Value to associate with the key
 * @tip If the value is NULL the hash entry is deleted.
 * @deffunc void apr_hash_set(apr_hash_t *ht, const void *key, apr_size_t klen, const void *val)
 */
APR_DECLARE(void) apr_hash_set(apr_hash_t *ht, const void *key,
                              apr_ssize_t klen, const void *val);

/**
 * Look up the value associated with a key in a hash table.
 * @param ht The hash table
 * @param key Pointer to the key
 * @param klen Length of the key. Can be APR_HASH_KEY_STRING.
 * @return Returns NULL if the key is not present.
 * @deffunc void *apr_hash_get(apr_hash_t *ht, const void *key, apr_size_t klen)
 */
APR_DECLARE(void*) apr_hash_get(apr_hash_t *ht, const void *key,
                               apr_ssize_t klen);

/**
 * Start iterating over the entries in a hash table.
 * @param ht The hash table
 * @return a pointer to the iteration state, or NULL if there are no entries.
 * @tip Example:
 * <PRE>
 * 
 *     int sum_values(apr_hash_t *ht)
 *     {
 *         apr_hash_index_t *hi;
 * 	   void *val;
 * 	   int sum = 0;
 * 	   for (hi = apr_hash_first(ht); hi; hi = apr_hash_next(hi)) {
 * 	       apr_hash_this(hi, NULL, NULL, &val);
 * 	       sum += *(int *)val;
 * 	   }
 * 	   return sum;
 *     }
 * 
 * There is no restriction on adding or deleting hash entries during an
 * iteration (although the results may be unpredictable unless all you do
 * is delete the current entry) and multiple iterations can be in
 * progress at the same time.
 * </PRE>
 * @deffunc apr_hash_index_t * apr_hash_first(apr_hash_t *ht)
 */
APR_DECLARE(apr_hash_index_t *) apr_hash_first(apr_hash_t *ht);

/**
 * Continue iterating over the entries in a hash table.
 * @param hi The iteration state
 * @return a pointer to the updated iteration state.  NULL if there are no more  *         entries.
 * @deffunc apr_hash_index_t * apr_hash_next(apr_hash_index_t *hi)
 */
APR_DECLARE(apr_hash_index_t *) apr_hash_next(apr_hash_index_t *hi);

/**
 * Get the current entry's details from the iteration state.
 * @param hi The iteration state
 * @param key Return pointer for the pointer to the key.
 * @param klen Return pointer for the key length.
 * @param val Return pointer for the associated value.
 * @tip The return pointers should point to a variable that will be set to the
 *      corresponding data, or they may be NULL if the data isn't interesting.
 * @deffunc void apr_hash_this(apr_hash_index_t *hi, const void **key, apr_size_t *klen, void **val);
 */
APR_DECLARE(void) apr_hash_this(apr_hash_index_t *hi, const void **key, 
                               apr_size_t *klen, void **val);

#ifdef __cplusplus
}
#endif

#endif	/* !APR_HASH_H */
