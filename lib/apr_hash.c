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

#include "apr_private.h"

#include "apr_general.h"
#include "apr_pools.h"

#include "apr_hash.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDLIB_H
#include <string.h>
#endif


/*
 * The internal form of a hash table.
 *
 * The table is an array indexed by the hash of the key; collisions
 * are resolved by hanging a linked list of hash entries off each
 * element of the array. Although this is a really simple design it
 * isn't too bad given that pools have a low allocation overhead.
 */

typedef struct ap_hash_entry_t ap_hash_entry_t;

struct ap_hash_entry_t {
    ap_hash_entry_t	*next;
    int			 hash;
    const void		*key;
    size_t		 klen;
    const void		*val;
};

/*
 * The size of the array is always a power of two. We use the maximum
 * index rather than the size so that we can use bitwise-AND for
 * modular arithmetic.
 * The count of hash entries may be greater depending on the chosen
 * collision rate.
 */
struct ap_hash_t {
    ap_pool_t		 *pool;
    ap_hash_entry_t	**array;
    size_t		  count, max;
};
#define INITIAL_MAX 15 /* tunable == 2^n - 1 */

/*
 * Data structure for iterating through a hash table.
 *
 * We keep a pointer to the next hash entry here to allow the current
 * hash entry to be freed or otherwise mangled between calls to
 * ap_hash_next().
 */
struct ap_hash_index_t {
    ap_hash_t		*ht;
    ap_hash_entry_t	*this, *next;
    int			 index;
};


/*
 * Hash creation functions.
 */

static ap_hash_entry_t **alloc_array(ap_hash_t *ht)
{
   return ap_pcalloc(ht->pool, sizeof(*ht->array) * (ht->max + 1));
}

APR_EXPORT(ap_hash_t *) ap_make_hash(ap_pool_t *pool)
{
    ap_hash_t *ht;
    ht = ap_palloc(pool, sizeof(ap_hash_t));
    ht->pool = pool;
    ht->count = 0;
    ht->max = INITIAL_MAX;
    ht->array = alloc_array(ht);
    return ht;
}


/*
 * Hash iteration functions.
 */

APR_EXPORT(ap_hash_index_t *) ap_hash_next(ap_hash_index_t *hi)
{
    hi->this = hi->next;
    while (!hi->this) {
	if (hi->index > hi->ht->max)
	    return NULL;
	hi->this = hi->ht->array[hi->index++];
    }
    hi->next = hi->this->next;
    return hi;
}

APR_EXPORT(ap_hash_index_t *) ap_hash_first(ap_hash_t *ht)
{
    ap_hash_index_t *hi;
    hi = ap_palloc(ht->pool, sizeof(*hi));
    hi->ht = ht;
    hi->index = 0;
    hi->this = NULL;
    hi->next = NULL;
    return ap_hash_next(hi);
}

APR_EXPORT(void) ap_hash_this(ap_hash_index_t *hi,
			      const void **key,
			      size_t *klen,
			      void **val)
{
    if (key)  *key  = hi->this->key;
    if (klen) *klen = hi->this->klen;
    if (val)  *val  = (void *)hi->this->val;
}


/*
 * Resizing a hash table
 */

static void resize_array(ap_hash_t *ht)
{
    ap_hash_index_t *hi;
    ap_hash_entry_t **new_array;
    int i;

    new_array = alloc_array(ht);
    for (hi = ap_hash_first(ht); hi; hi = ap_hash_next(hi)) {
	i = hi->this->hash & ht->max;
	hi->this->next = new_array[i];
	new_array[i] = hi->this;
    }	       
    ht->array = new_array;
}

/*
 * This is where we keep the details of the hash function and control
 * the maximum collision rate.
 *
 * If val is non-NULL it creates and initializes a new hash entry if
 * there isn't already one there; it returns an updatable pointer so
 * that hash entries can be removed.
 */

static ap_hash_entry_t **find_entry(ap_hash_t *ht,
				    const void *key,
				    size_t klen,
				    const void *val)
{
    ap_hash_entry_t **hep, *he;
    const unsigned char *p;
    int hash;
    int i;

    if (klen == 0)
	klen = strlen(key) + 1;

    /*
     * This hash function is used by perl 5; RSE attributes it to DJB.
     * (See Message-ID: <19991013131827.A17702@engelschall.com>
     * in the new-httpd archive for October 1999.)
     */
    hash = 0;
    for (p = key, i = klen; i; i--, p++)
	hash = hash * 33 + *p;
    
    /* scan linked list */
    for (hep = &ht->array[hash & ht->max], he = *hep;
	 he;
	 hep = &he->next, he = *hep) {
	if (he->hash == hash &&
	    he->klen == klen &&
	    memcmp(he->key, key, klen) == 0)
	    break;
    }
    if (he || !val)
	return hep;
    /* add a new entry for non-NULL values */
    he = ap_pcalloc(ht->pool, sizeof(*he));
    he->hash = hash;
    he->key  = key;
    he->klen = klen;
    he->val  = val;
    *hep = he;
    /* check that the collision rate isn't too high */
    if (++ht->count > ht->max) {
	ht->max = ht->max * 2 + 1;
	resize_array(ht);
    }
    return hep;
}

APR_EXPORT(void *) ap_hash_get(ap_hash_t *ht,
			       const void *key,
			       size_t klen)
{
    ap_hash_entry_t *he;
    he = *find_entry(ht, key, klen, NULL);
    if (he)
	return (void *)he->val;
    else
	return NULL;
}

APR_EXPORT(void) ap_hash_set(ap_hash_t *ht,
			     const void *key,
			     size_t klen,
			     const void *val)
{
    ap_hash_entry_t **hep;
    hep = find_entry(ht, key, klen, val);
    if (*hep && !val)
	/* delete entry */
	*hep = (*hep)->next;
}
