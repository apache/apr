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

/*
 * Resource allocation code... the code here is responsible for making
 * sure that nothing leaks.
 *
 * rst --- 4/95 --- 6/95
 */

#include "apr_private.h"

#include "apr_general.h"
#include "apr_pools.h"
#include "apr_tables.h"
#include "apr_strings.h"
#include "apr_lib.h"
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_STRINGS_H
#include <strings.h>
#endif

/*****************************************************************
 * This file contains array and apr_table_t functions only.
 */

/*****************************************************************
 *
 * The 'array' functions...
 */

static void make_array_core(apr_array_header_t *res, apr_pool_t *p,
			    int nelts, int elt_size, int clear)
{
    /*
     * Assure sanity if someone asks for
     * array of zero elts.
     */
    if (nelts < 1) {
        nelts = 1;
    }

    if (clear) {
        res->elts = apr_pcalloc(p, nelts * elt_size);
    }
    else {
        res->elts = apr_palloc(p, nelts * elt_size);
    }

    res->pool = p;
    res->elt_size = elt_size;
    res->nelts = 0;		/* No active elements yet... */
    res->nalloc = nelts;	/* ...but this many allocated */
}

APR_DECLARE(int) apr_is_empty_array(const apr_array_header_t *a)
{
    return ((a == NULL) || (a->nelts == 0));
}

APR_DECLARE(apr_array_header_t *) apr_array_make(apr_pool_t *p,
						int nelts, int elt_size)
{
    apr_array_header_t *res;

    res = (apr_array_header_t *) apr_palloc(p, sizeof(apr_array_header_t));
    make_array_core(res, p, nelts, elt_size, 1);
    return res;
}

APR_DECLARE(void *) apr_array_pop(apr_array_header_t *arr)
{
    if (apr_is_empty_array(arr)) {
        return NULL;
    }
   
    return arr->elts + (arr->elt_size * (--arr->nelts));
}

APR_DECLARE(void *) apr_array_push(apr_array_header_t *arr)
{
    if (arr->nelts == arr->nalloc) {
        int new_size = (arr->nalloc <= 0) ? 1 : arr->nalloc * 2;
        char *new_data;

        new_data = apr_palloc(arr->pool, arr->elt_size * new_size);

        memcpy(new_data, arr->elts, arr->nalloc * arr->elt_size);
        memset(new_data + arr->nalloc * arr->elt_size, 0,
               arr->elt_size * (new_size - arr->nalloc));
        arr->elts = new_data;
        arr->nalloc = new_size;
    }

    ++arr->nelts;
    return arr->elts + (arr->elt_size * (arr->nelts - 1));
}

static void *apr_array_push_noclear(apr_array_header_t *arr)
{
    if (arr->nelts == arr->nalloc) {
        int new_size = (arr->nalloc <= 0) ? 1 : arr->nalloc * 2;
        char *new_data;

        new_data = apr_palloc(arr->pool, arr->elt_size * new_size);

        memcpy(new_data, arr->elts, arr->nalloc * arr->elt_size);
        arr->elts = new_data;
        arr->nalloc = new_size;
    }

    ++arr->nelts;
    return arr->elts + (arr->elt_size * (arr->nelts - 1));
}

APR_DECLARE(void) apr_array_cat(apr_array_header_t *dst,
			       const apr_array_header_t *src)
{
    int elt_size = dst->elt_size;

    if (dst->nelts + src->nelts > dst->nalloc) {
	int new_size = (dst->nalloc <= 0) ? 1 : dst->nalloc * 2;
	char *new_data;

	while (dst->nelts + src->nelts > new_size) {
	    new_size *= 2;
	}

	new_data = apr_pcalloc(dst->pool, elt_size * new_size);
	memcpy(new_data, dst->elts, dst->nalloc * elt_size);

	dst->elts = new_data;
	dst->nalloc = new_size;
    }

    memcpy(dst->elts + dst->nelts * elt_size, src->elts,
	   elt_size * src->nelts);
    dst->nelts += src->nelts;
}

APR_DECLARE(apr_array_header_t *) apr_array_copy(apr_pool_t *p,
						const apr_array_header_t *arr)
{
    apr_array_header_t *res =
        (apr_array_header_t *) apr_palloc(p, sizeof(apr_array_header_t));
    make_array_core(res, p, arr->nalloc, arr->elt_size, 0);

    memcpy(res->elts, arr->elts, arr->elt_size * arr->nelts);
    res->nelts = arr->nelts;
    memset(res->elts + res->elt_size * res->nelts, 0,
           res->elt_size * (res->nalloc - res->nelts));
    return res;
}

/* This cute function copies the array header *only*, but arranges
 * for the data section to be copied on the first push or arraycat.
 * It's useful when the elements of the array being copied are
 * read only, but new stuff *might* get added on the end; we have the
 * overhead of the full copy only where it is really needed.
 */

static APR_INLINE void copy_array_hdr_core(apr_array_header_t *res,
					   const apr_array_header_t *arr)
{
    res->elts = arr->elts;
    res->elt_size = arr->elt_size;
    res->nelts = arr->nelts;
    res->nalloc = arr->nelts;	/* Force overflow on push */
}

APR_DECLARE(apr_array_header_t *)
    apr_array_copy_hdr(apr_pool_t *p,
		       const apr_array_header_t *arr)
{
    apr_array_header_t *res;

    res = (apr_array_header_t *) apr_palloc(p, sizeof(apr_array_header_t));
    res->pool = p;
    copy_array_hdr_core(res, arr);
    return res;
}

/* The above is used here to avoid consing multiple new array bodies... */

APR_DECLARE(apr_array_header_t *)
    apr_array_append(apr_pool_t *p,
		      const apr_array_header_t *first,
		      const apr_array_header_t *second)
{
    apr_array_header_t *res = apr_array_copy_hdr(p, first);

    apr_array_cat(res, second);
    return res;
}

/* apr_array_pstrcat generates a new string from the apr_pool_t containing
 * the concatenated sequence of substrings referenced as elements within
 * the array.  The string will be empty if all substrings are empty or null,
 * or if there are no elements in the array.
 * If sep is non-NUL, it will be inserted between elements as a separator.
 */
APR_DECLARE(char *) apr_array_pstrcat(apr_pool_t *p,
				     const apr_array_header_t *arr,
				     const char sep)
{
    char *cp, *res, **strpp;
    apr_size_t len;
    int i;

    if (arr->nelts <= 0 || arr->elts == NULL) {    /* Empty table? */
        return (char *) apr_pcalloc(p, 1);
    }

    /* Pass one --- find length of required string */

    len = 0;
    for (i = 0, strpp = (char **) arr->elts; ; ++strpp) {
        if (strpp && *strpp != NULL) {
            len += strlen(*strpp);
        }
        if (++i >= arr->nelts) {
            break;
	}
        if (sep) {
            ++len;
	}
    }

    /* Allocate the required string */

    res = (char *) apr_palloc(p, len + 1);
    cp = res;

    /* Pass two --- copy the argument strings into the result space */

    for (i = 0, strpp = (char **) arr->elts; ; ++strpp) {
        if (strpp && *strpp != NULL) {
            len = strlen(*strpp);
            memcpy(cp, *strpp, len);
            cp += len;
        }
        if (++i >= arr->nelts) {
            break;
	}
        if (sep) {
            *cp++ = sep;
	}
    }

    *cp = '\0';

    /* Return the result string */

    return res;
}


/*****************************************************************
 *
 * The "table" functions.
 */

#if APR_CHARSET_EBCDIC
#define CASE_MASK 0xbfbfbfbf
#else
#define CASE_MASK 0xdfdfdfdf
#endif

#define TABLE_HASH_SIZE 32
#define TABLE_INDEX_MASK 0x1f
#define TABLE_HASH(key)  (TABLE_INDEX_MASK & *(unsigned char *)(key))
#define TABLE_INDEX_IS_INITIALIZED(t, i) ((t)->index_initialized & (1 << (i)))
#define TABLE_SET_INDEX_INITIALIZED(t, i) ((t)->index_initialized |= (1 << (i)))

/* Compute the "checksum" for a key, consisting of the first
 * 4 bytes, normalized for case-insensitivity and packed into
 * an int...this checksum allows us to do a single integer
 * comparison as a fast check to determine whether we can
 * skip a strcasecmp
 */
#define COMPUTE_KEY_CHECKSUM(key, checksum)    \
{                                              \
    const char *k = (key);                     \
    apr_uint32_t c = (apr_uint32_t)*k;         \
    (checksum) = c;                            \
    (checksum) <<= 8;                          \
    if (c) {                                   \
        c = (apr_uint32_t)*++k;                \
        checksum |= c;                         \
    }                                          \
    (checksum) <<= 8;                          \
    if (c) {                                   \
        c = (apr_uint32_t)*++k;                \
        checksum |= c;                         \
    }                                          \
    (checksum) <<= 8;                          \
    if (c) {                                   \
        c = (apr_uint32_t)*++k;                \
        checksum |= c;                         \
    }                                          \
    checksum &= CASE_MASK;                     \
}

/** The opaque string-content table type */
struct apr_table_t {
    /* This has to be first to promote backwards compatibility with
     * older modules which cast a apr_table_t * to an apr_array_header_t *...
     * they should use the apr_table_elts() function for most of the
     * cases they do this for.
     */
    /** The underlying array for the table */
    apr_array_header_t a;
#ifdef MAKE_TABLE_PROFILE
    /** Who created the array. */
    void *creator;
#endif
    /* An index to speed up table lookups.  The way this works is:
     *   - Take the requested key and compute its checksum
     *   - Hash the checksum into the index:
     *     - index_first[TABLE_HASH(checksum)] is the offset within
     *       the table of the first entry with that key checksum
     *     - index_last[TABLE_HASH(checksum)] is the offset within
     *       the table of the first entry with that key checksum
     *   - If (and only if) there is no entry in the table whose
     *     checksum hashes to index element i, then the i'th bit
     *     of index_initialized will be zero.  (Check this before
     *     trying to use index_first[i] or index_last[i]!)
     */
    apr_uint32_t index_initialized;
    int index_first[TABLE_HASH_SIZE];
    int index_last[TABLE_HASH_SIZE];
};

/*
 * NOTICE: if you tweak this you should look at is_empty_table() 
 * and table_elts() in alloc.h
 */
#ifdef MAKE_TABLE_PROFILE
static apr_table_entry_t *table_push(apr_table_t *t)
{
    if (t->a.nelts == t->a.nalloc) {
        return NULL;
    }
    return (apr_table_entry_t *) apr_array_push_noclear(&t->a);
}
#else /* MAKE_TABLE_PROFILE */
#define table_push(t)	((apr_table_entry_t *) apr_array_push_noclear(&(t)->a))
#endif /* MAKE_TABLE_PROFILE */

APR_DECLARE(const apr_array_header_t *) apr_table_elts(const apr_table_t *t)
{
    return (const apr_array_header_t *)t;
}

APR_DECLARE(int) apr_is_empty_table(const apr_table_t *t)
{
    return ((t == NULL) || (t->a.nelts == 0));
}

APR_DECLARE(apr_table_t *) apr_table_make(apr_pool_t *p, int nelts)
{
    apr_table_t *t = apr_palloc(p, sizeof(apr_table_t));

    make_array_core(&t->a, p, nelts, sizeof(apr_table_entry_t), 0);
#ifdef MAKE_TABLE_PROFILE
    t->creator = __builtin_return_address(0);
#endif
    t->index_initialized = 0;
    return t;
}

APR_DECLARE(apr_table_t *) apr_table_copy(apr_pool_t *p, const apr_table_t *t)
{
    apr_table_t *new = apr_palloc(p, sizeof(apr_table_t));

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that t->a.pool
     * have a life span at least as long as p
     */
    if (!apr_pool_is_ancestor(t->a.pool, p)) {
	fprintf(stderr, "copy_table: t's pool is not an ancestor of p\n");
	abort();
    }
#endif
    make_array_core(&new->a, p, t->a.nalloc, sizeof(apr_table_entry_t), 0);
    memcpy(new->a.elts, t->a.elts, t->a.nelts * sizeof(apr_table_entry_t));
    new->a.nelts = t->a.nelts;
    memcpy(new->index_first, t->index_first, sizeof(int) * TABLE_HASH_SIZE);
    memcpy(new->index_last, t->index_last, sizeof(int) * TABLE_HASH_SIZE);
    new->index_initialized = t->index_initialized;
    return new;
}

static void table_reindex(apr_table_t *t)
{
    int i;
    int hash;
    apr_table_entry_t *next_elt = (apr_table_entry_t *) t->a.elts;

    t->index_initialized = 0;
    for (i = 0; i < t->a.nelts; i++, next_elt++) {
        hash = TABLE_HASH(next_elt->key);
        t->index_last[hash] = i;
        if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
            t->index_first[hash] = i;
            TABLE_SET_INDEX_INITIALIZED(t, hash);
        }
    }
}

APR_DECLARE(void) apr_table_clear(apr_table_t *t)
{
    t->a.nelts = 0;
    t->index_initialized = 0;
}

APR_DECLARE(const char *) apr_table_get(const apr_table_t *t, const char *key)
{
    apr_table_entry_t *next_elt;
    apr_table_entry_t *end_elt;
    apr_uint32_t checksum;
    int hash;

    if (key == NULL) {
	return NULL;
    }

    hash = TABLE_HASH(key);
    if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
        return NULL;
    }
    COMPUTE_KEY_CHECKSUM(key, checksum);
    next_elt = ((apr_table_entry_t *) t->a.elts) + t->index_first[hash];;
    end_elt = ((apr_table_entry_t *) t->a.elts) + t->index_last[hash];

    for (; next_elt <= end_elt; next_elt++) {
	if ((checksum == next_elt->key_checksum) &&
            !strcasecmp(next_elt->key, key)) {
	    return next_elt->val;
	}
    }

    return NULL;
}

APR_DECLARE(void) apr_table_set(apr_table_t *t, const char *key,
                                const char *val)
{
    apr_table_entry_t *next_elt;
    apr_table_entry_t *end_elt;
    apr_table_entry_t *table_end;
    apr_uint32_t checksum;
    int hash;

    COMPUTE_KEY_CHECKSUM(key, checksum);
    hash = TABLE_HASH(key);
    if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
        t->index_first[hash] = t->a.nelts;
        TABLE_SET_INDEX_INITIALIZED(t, hash);
        goto add_new_elt;
    }
    next_elt = ((apr_table_entry_t *) t->a.elts) + t->index_first[hash];;
    end_elt = ((apr_table_entry_t *) t->a.elts) + t->index_last[hash];
    table_end =((apr_table_entry_t *) t->a.elts) + t->a.nelts;

    for (; next_elt <= end_elt; next_elt++) {
	if ((checksum == next_elt->key_checksum) &&
            !strcasecmp(next_elt->key, key)) {

            /* Found an existing entry with the same key, so overwrite it */

            int must_reindex = 0;
            apr_table_entry_t *dst_elt = NULL;

            next_elt->val = apr_pstrdup(t->a.pool, val);

            /* Remove any other instances of this key */
            for (next_elt++; next_elt <= end_elt; next_elt++) {
                if ((checksum == next_elt->key_checksum) &&
                    !strcasecmp(next_elt->key, key)) {
                    t->a.nelts--;
                    if (!dst_elt) {
                        dst_elt = next_elt;
                    }
                }
                else if (dst_elt) {
                    *dst_elt++ = *next_elt;
                    must_reindex = 1;
                }
            }

            /* If we've removed anything, shift over the remainder
             * of the table (note that the previous loop didn't
             * run to the end of the table, just to the last match
             * for the index)
             */
            if (dst_elt) {
                for (; next_elt < table_end; next_elt++) {
                    *dst_elt++ = *next_elt;
                }
                must_reindex = 1;
            }
            if (must_reindex) {
                table_reindex(t);
            }
            return;
        }
    }

add_new_elt:
    t->index_last[hash] = t->a.nelts;
    next_elt = (apr_table_entry_t *) table_push(t);
    next_elt->key = apr_pstrdup(t->a.pool, key);
    next_elt->val = apr_pstrdup(t->a.pool, val);
    next_elt->key_checksum = checksum;
}

APR_DECLARE(void) apr_table_setn(apr_table_t *t, const char *key,
                                 const char *val)
{
    apr_table_entry_t *next_elt;
    apr_table_entry_t *end_elt;
    apr_table_entry_t *table_end;
    apr_uint32_t checksum;
    int hash;

    COMPUTE_KEY_CHECKSUM(key, checksum);
    hash = TABLE_HASH(key);
    if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
        t->index_first[hash] = t->a.nelts;
        TABLE_SET_INDEX_INITIALIZED(t, hash);
        goto add_new_elt;
    }
    next_elt = ((apr_table_entry_t *) t->a.elts) + t->index_first[hash];;
    end_elt = ((apr_table_entry_t *) t->a.elts) + t->index_last[hash];
    table_end =((apr_table_entry_t *) t->a.elts) + t->a.nelts;

    for (; next_elt <= end_elt; next_elt++) {
	if ((checksum == next_elt->key_checksum) &&
            !strcasecmp(next_elt->key, key)) {

            /* Found an existing entry with the same key, so overwrite it */

            int must_reindex = 0;
            apr_table_entry_t *dst_elt = NULL;

            next_elt->val = (char *)val;

            /* Remove any other instances of this key */
            for (next_elt++; next_elt <= end_elt; next_elt++) {
                if ((checksum == next_elt->key_checksum) &&
                    !strcasecmp(next_elt->key, key)) {
                    t->a.nelts--;
                    if (!dst_elt) {
                        dst_elt = next_elt;
                    }
                }
                else if (dst_elt) {
                    *dst_elt++ = *next_elt;
                    must_reindex = 1;
                }
            }

            /* If we've removed anything, shift over the remainder
             * of the table (note that the previous loop didn't
             * run to the end of the table, just to the last match
             * for the index)
             */
            if (dst_elt) {
                for (; next_elt < table_end; next_elt++) {
                    *dst_elt++ = *next_elt;
                }
                must_reindex = 1;
            }
            if (must_reindex) {
                table_reindex(t);
            }
            return;
        }
    }

add_new_elt:
    t->index_last[hash] = t->a.nelts;
    next_elt = (apr_table_entry_t *) table_push(t);
    next_elt->key = (char *)key;
    next_elt->val = (char *)val;
    next_elt->key_checksum = checksum;
}

APR_DECLARE(void) apr_table_unset(apr_table_t *t, const char *key)
{
    apr_table_entry_t *next_elt;
    apr_table_entry_t *end_elt;
    apr_table_entry_t *dst_elt;
    apr_uint32_t checksum;
    int hash;
    int must_reindex;

    hash = TABLE_HASH(key);
    if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
        return;
    }
    COMPUTE_KEY_CHECKSUM(key, checksum);
    next_elt = ((apr_table_entry_t *) t->a.elts) + t->index_first[hash];
    end_elt = ((apr_table_entry_t *) t->a.elts) + t->index_last[hash];
    must_reindex = 0;
    for (; next_elt <= end_elt; next_elt++) {
	if ((checksum == next_elt->key_checksum) &&
            !strcasecmp(next_elt->key, key)) {

            /* Found a match: remove this entry, plus any additional
             * matches for the same key that might follow
             */
            apr_table_entry_t *table_end = ((apr_table_entry_t *) t->a.elts) +
                t->a.nelts;
            t->a.nelts--;
            dst_elt = next_elt;
            for (next_elt++; next_elt <= end_elt; next_elt++) {
                if ((checksum == next_elt->key_checksum) &&
                    !strcasecmp(next_elt->key, key)) {
                    t->a.nelts--;
                }
                else {
                    *dst_elt++ = *next_elt;
                }
            }

            /* Shift over the remainder of the table (note that
             * the previous loop didn't run to the end of the table,
             * just to the last match for the index)
             */
            for (; next_elt < table_end; next_elt++) {
                *dst_elt++ = *next_elt;
            }
            must_reindex = 1;
            break;
        }
    }
    if (must_reindex) {
        table_reindex(t);
    }
}

APR_DECLARE(void) apr_table_merge(apr_table_t *t, const char *key,
				 const char *val)
{
    apr_table_entry_t *next_elt;
    apr_table_entry_t *end_elt;
    apr_uint32_t checksum;
    int hash;

    COMPUTE_KEY_CHECKSUM(key, checksum);
    hash = TABLE_HASH(key);
    if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
        t->index_first[hash] = t->a.nelts;
        TABLE_SET_INDEX_INITIALIZED(t, hash);
        goto add_new_elt;
    }
    next_elt = ((apr_table_entry_t *) t->a.elts) + t->index_first[hash];
    end_elt = ((apr_table_entry_t *) t->a.elts) + t->index_last[hash];

    for (; next_elt <= end_elt; next_elt++) {
	if ((checksum == next_elt->key_checksum) &&
            !strcasecmp(next_elt->key, key)) {

            /* Found an existing entry with the same key, so merge with it */
	    next_elt->val = apr_pstrcat(t->a.pool, next_elt->val, ", ",
                                        val, NULL);
            return;
        }
    }

add_new_elt:
    t->index_last[hash] = t->a.nelts;
    next_elt = (apr_table_entry_t *) table_push(t);
    next_elt->key = apr_pstrdup(t->a.pool, key);
    next_elt->val = apr_pstrdup(t->a.pool, val);
    next_elt->key_checksum = checksum;
}

APR_DECLARE(void) apr_table_mergen(apr_table_t *t, const char *key,
				  const char *val)
{
    apr_table_entry_t *next_elt;
    apr_table_entry_t *end_elt;
    apr_uint32_t checksum;
    int hash;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_pool_find(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_pool_find(val), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    COMPUTE_KEY_CHECKSUM(key, checksum);
    hash = TABLE_HASH(key);
    if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
        t->index_first[hash] = t->a.nelts;
        TABLE_SET_INDEX_INITIALIZED(t, hash);
        goto add_new_elt;
    }
    next_elt = ((apr_table_entry_t *) t->a.elts) + t->index_first[hash];;
    end_elt = ((apr_table_entry_t *) t->a.elts) + t->index_last[hash];

    for (; next_elt <= end_elt; next_elt++) {
	if ((checksum == next_elt->key_checksum) &&
            !strcasecmp(next_elt->key, key)) {

            /* Found an existing entry with the same key, so merge with it */
	    next_elt->val = apr_pstrcat(t->a.pool, next_elt->val, ", ",
                                        val, NULL);
            return;
        }
    }

add_new_elt:
    t->index_last[hash] = t->a.nelts;
    next_elt = (apr_table_entry_t *) table_push(t);
    next_elt->key = (char *)key;
    next_elt->val = (char *)val;
    next_elt->key_checksum = checksum;
}

APR_DECLARE(void) apr_table_add(apr_table_t *t, const char *key,
			       const char *val)
{
    apr_table_entry_t *elts;
    apr_uint32_t checksum;
    int hash;

    hash = TABLE_HASH(key);
    t->index_last[hash] = t->a.nelts;
    if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
        t->index_first[hash] = t->a.nelts;
        TABLE_SET_INDEX_INITIALIZED(t, hash);
    }
    COMPUTE_KEY_CHECKSUM(key, checksum);
    elts = (apr_table_entry_t *) table_push(t);
    elts->key = apr_pstrdup(t->a.pool, key);
    elts->val = apr_pstrdup(t->a.pool, val);
    elts->key_checksum = checksum;
}

APR_DECLARE(void) apr_table_addn(apr_table_t *t, const char *key,
				const char *val)
{
    apr_table_entry_t *elts;
    apr_uint32_t checksum;
    int hash;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_pool_find(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_pool_find(val), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    hash = TABLE_HASH(key);
    t->index_last[hash] = t->a.nelts;
    if (!TABLE_INDEX_IS_INITIALIZED(t, hash)) {
        t->index_first[hash] = t->a.nelts;
        TABLE_SET_INDEX_INITIALIZED(t, hash);
    }
    COMPUTE_KEY_CHECKSUM(key, checksum);
    elts = (apr_table_entry_t *) table_push(t);
    elts->key = (char *)key;
    elts->val = (char *)val;
    elts->key_checksum = checksum;
}

APR_DECLARE(apr_table_t *) apr_table_overlay(apr_pool_t *p,
					     const apr_table_t *overlay,
					     const apr_table_t *base)
{
    apr_table_t *res;

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that
     * overlay->a.pool and base->a.pool have a life span at least
     * as long as p
     */
    if (!apr_pool_is_ancestor(overlay->a.pool, p)) {
	fprintf(stderr,
		"overlay_tables: overlay's pool is not an ancestor of p\n");
	abort();
    }
    if (!apr_pool_is_ancestor(base->a.pool, p)) {
	fprintf(stderr,
		"overlay_tables: base's pool is not an ancestor of p\n");
	abort();
    }
#endif

    res = apr_palloc(p, sizeof(apr_table_t));
    /* behave like append_arrays */
    res->a.pool = p;
    copy_array_hdr_core(&res->a, &overlay->a);
    apr_array_cat(&res->a, &base->a);
    table_reindex(res);
    return res;
}

/* And now for something completely abstract ...

 * For each key value given as a vararg:
 *   run the function pointed to as
 *     int comp(void *r, char *key, char *value);
 *   on each valid key-value pair in the apr_table_t t that matches the vararg key,
 *   or once for every valid key-value pair if the vararg list is empty,
 *   until the function returns false (0) or we finish the table.
 *
 * Note that we restart the traversal for each vararg, which means that
 * duplicate varargs will result in multiple executions of the function
 * for each matching key.  Note also that if the vararg list is empty,
 * only one traversal will be made and will cut short if comp returns 0.
 *
 * Note that the table_get and table_merge functions assume that each key in
 * the apr_table_t is unique (i.e., no multiple entries with the same key).  This
 * function does not make that assumption, since it (unfortunately) isn't
 * true for some of Apache's tables.
 *
 * Note that rec is simply passed-on to the comp function, so that the
 * caller can pass additional info for the task.
 *
 * ADDENDUM for apr_table_vdo():
 * 
 * The caching api will allow a user to walk the header values:
 *
 * apr_status_t apr_cache_el_header_walk(apr_cache_el *el, 
 *    int (*comp)(void *, const char *, const char *), void *rec, ...);
 *
 * So it can be ..., however from there I use a  callback that use a va_list:
 *
 * apr_status_t (*cache_el_header_walk)(apr_cache_el *el, 
 *    int (*comp)(void *, const char *, const char *), void *rec, va_list);
 *
 * To pass those ...'s on down to the actual module that will handle walking
 * their headers, in the file case this is actually just an apr_table - and
 * rather than reimplementing apr_table_do (which IMHO would be bad) I just
 * called it with the va_list. For mod_shmem_cache I don't need it since I
 * can't use apr_table's, but mod_file_cache should (though a good hash would
 * be better, but that's a different issue :). 
 *
 * So to make mod_file_cache easier to maintain, it's a good thing
 */
APR_DECLARE_NONSTD(int) apr_table_do(apr_table_do_callback_fn_t *comp,
                                     void *rec, const apr_table_t *t, ...)
{
    int rv;

    va_list vp;
    va_start(vp, t);
    rv = apr_table_vdo(comp, rec, t, vp);
    va_end(vp);

    return rv;
} 

/* XXX: do the semantics of this routine make any sense?  Right now,
 * if the caller passed in a non-empty va_list of keys to search for,
 * the "early termination" facility only terminates on *that* key; other
 * keys will continue to process.  Note that this only has any effect
 * at all if there are multiple entries in the table with the same key,
 * otherwise the called function can never effectively early-terminate
 * this function, as the zero return value is effectively ignored.
 *
 * Note also that this behavior is at odds with the behavior seen if an
 * empty va_list is passed in -- in that case, a zero return value terminates
 * the entire apr_table_vdo (which is what I think should happen in
 * both cases).
 *
 * If nobody objects soon, I'm going to change the order of the nested
 * loops in this function so that any zero return value from the (*comp)
 * function will cause a full termination of apr_table_vdo.  I'm hesitant
 * at the moment because these (funky) semantics have been around for a
 * very long time, and although Apache doesn't seem to use them at all,
 * some third-party vendor might.  I can only think of one possible reason
 * the existing semantics would make any sense, and it's very Apache-centric,
 * which is this: if (*comp) is looking for matches of a particular
 * substring in request headers (let's say it's looking for a particular
 * cookie name in the Set-Cookie headers), then maybe it wants to be
 * able to stop searching early as soon as it finds that one and move
 * on to the next key.  That's only an optimization of course, but changing
 * the behavior of this function would mean that any code that tried
 * to do that would stop working right.
 *
 * Sigh.  --JCW, 06/28/02
 */
APR_DECLARE(int) apr_table_vdo(apr_table_do_callback_fn_t *comp,
                               void *rec, const apr_table_t *t, va_list vp)
{
    char *argp;
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int vdorv = 1;

    argp = va_arg(vp, char *);
    do {
        int rv = 1, i;
        if (argp) {
            /* Scan for entries that match the next key */
            int hash = TABLE_HASH(argp);
            if (TABLE_INDEX_IS_INITIALIZED(t, hash)) {
                apr_uint32_t checksum;
                COMPUTE_KEY_CHECKSUM(argp, checksum);
                for (i = t->index_first[hash];
                     rv && (i <= t->index_last[hash]); ++i) {
                    if (elts[i].key && (checksum == elts[i].key_checksum) &&
                                        !strcasecmp(elts[i].key, argp)) {
                        rv = (*comp) (rec, elts[i].key, elts[i].val);
                    }
                }
            }
        }
        else {
            /* Scan the entire table */
            for (i = 0; rv && (i < t->a.nelts); ++i) {
                if (elts[i].key) {
                    rv = (*comp) (rec, elts[i].key, elts[i].val);
                }
            }
        }
        if (rv == 0) {
            vdorv = 0;
        }
    } while (argp && ((argp = va_arg(vp, char *)) != NULL));

    return vdorv;
}

/* During apr_table_overlap(), we build an overlap key for
 * each element in the two tables.
 */
#define RED 0
#define BLACK 1
typedef struct overlap_key {
    /* The table element */
    apr_table_entry_t *elt;

    /* 0 if from table 'a', 1 if from table 'b' */
    int level;

    /* Whether to omit this element when building the result table */
    int skip;

    /* overlap_keys can be assembled into a red-black tree */
    int black;
    struct overlap_key *tree_parent;
    struct overlap_key *tree_left;
    struct overlap_key *tree_right;
    int color;

    /* List of multiple values for this key */
    struct overlap_key *merge_next;
    struct overlap_key *merge_last;
} overlap_key;

/* Rotate a subtree of a red-black tree */
static void rotate_counterclockwise(overlap_key **root,
                                    overlap_key *rotate_node)
{
    overlap_key *child = rotate_node->tree_right;
    rotate_node->tree_right = child->tree_left;
    if (rotate_node->tree_right) {
        rotate_node->tree_right->tree_parent = rotate_node;
    }
    child->tree_parent = rotate_node->tree_parent;
    if (child->tree_parent == NULL) {
        *root = child;
    }
    else {
        if (rotate_node == rotate_node->tree_parent->tree_left) {
            rotate_node->tree_parent->tree_left = child;
        }
        else {
            rotate_node->tree_parent->tree_right = child;
        }
    }
    child->tree_left = rotate_node;
    rotate_node->tree_parent = child;
}

static void rotate_clockwise(overlap_key **root, overlap_key *rotate_node)
{
    overlap_key *child = rotate_node->tree_left;
    rotate_node->tree_left = child->tree_right;
    if (rotate_node->tree_left) {
        rotate_node->tree_left->tree_parent = rotate_node;
    }
    child->tree_parent = rotate_node->tree_parent;
    if (child->tree_parent == NULL) {
        *root = child;
    }
    else {
        if (rotate_node == rotate_node->tree_parent->tree_left) {
            rotate_node->tree_parent->tree_left = child;
        }
        else {
            rotate_node->tree_parent->tree_right = child;
        }
    }
    child->tree_right = rotate_node;
    rotate_node->tree_parent = child;
}


static void overlap_hash(overlap_key *elt,
                         overlap_key **hash_table, int nhash,
                         unsigned flags)
{
    /* Each bucket in the hash table holds a red-black tree
     * containing the overlap_keys that hash into that bucket
     */
    overlap_key **child = &(hash_table[elt->elt->key_checksum & (nhash - 1)]);
    overlap_key **root = child;
    overlap_key *parent = NULL;
    overlap_key *next;

    /* Look for the element in the tree */
    while ((next = *child) != NULL) {
        int direction = strcasecmp(elt->elt->key, next->elt->key);
        if (direction < 0) {
            parent = next;
            child = &(next->tree_left);
        }
        else if (direction > 0) {
            parent = next;
            child = &(next->tree_right);
        }
        else {
            /* This is the key we're looking for */
            if (flags == APR_OVERLAP_TABLES_MERGE) {
                /* Just link this node at the end of the list
                 * of values for the key.  It doesn't need to
                 * be linked into the tree, because the node at
                 * the head of this key's value list is in the
                 * tree already.
                 */
                elt->skip = 1;
                elt->merge_next = NULL;
                if (next->merge_last) {
                    next->merge_last->merge_next = elt;
                }
                else {
                    next->merge_next = elt;
                }
                next->merge_last = elt;
            }
            else {
                /* In the "set" case, don't bother storing
                 * this value in the tree if it's already
                 * there, except if the previous value was
                 * from table 'a' (level==0) and this value
                 * is from table 'b' (level==1)
                 */
                if (elt->level > next->level) {
                    elt->tree_left = next->tree_left;
                    if (next->tree_left) {
                        next->tree_left->tree_parent = elt;
                    }
                    elt->tree_right = next->tree_right;
                    if (next->tree_right) {
                        next->tree_right->tree_parent = elt;
                    }
                    elt->tree_parent = next->tree_parent;
                    elt->color = next->color;
                    (*child) = elt;
                    elt->merge_next = NULL;
                    elt->merge_last = NULL;
                    elt->skip = 0;
                    next->skip = 1;
                }
                else {
                    elt->skip = 1;
                }
            }
            return;
        }
    }

    /* The element wasn't in the tree, so add it */
    elt->tree_left = NULL;
    elt->tree_right = NULL;
    elt->tree_parent = parent;
    (*child) = elt;
    elt->merge_next = NULL;
    elt->merge_last = NULL;
    elt->skip = 0;
    elt->color = RED;

    /* Shuffle the nodes to maintain the red-black tree's balanced
     * shape property.  (This is what guarantees O(n*log(n)) worst-case
     * performance for apr_table_overlap().)
     */
    next = elt;
    while ((next->tree_parent) && (next->tree_parent->color == RED)) {
        /* Note: Root is always black, and red and black nodes
         * alternate on any path down the tree.  So if we're inside
         * this block, the grandparent node is non-NULL.
         */
        overlap_key *grandparent = next->tree_parent->tree_parent;
        if (next->tree_parent == grandparent->tree_left) {
            overlap_key *parent_sibling = grandparent->tree_right;
            if (parent_sibling && (parent_sibling->color == RED)) {
                next->tree_parent->color = BLACK;
                parent_sibling->color = BLACK;
                grandparent->color = RED;
                next = grandparent;
            }
            else {
                if (next == next->tree_parent->tree_right) {
                    next = next->tree_parent;
                    rotate_counterclockwise(root, next);
                }
                next->tree_parent->color = BLACK;
                next->tree_parent->tree_parent->color = RED;
                rotate_clockwise(root, next->tree_parent->tree_parent);
            }
        }
        else {
            overlap_key *parent_sibling = grandparent->tree_left;
            if (parent_sibling && (parent_sibling->color == RED)) {
                next->tree_parent->color = BLACK;
                parent_sibling->color = BLACK;
                grandparent->color = RED;
                next = grandparent;
            }
            else {
                if (next == next->tree_parent->tree_left) {
                    next = next->tree_parent;
                    rotate_clockwise(root, next);
                }
                next->tree_parent->color = BLACK;
                next->tree_parent->tree_parent->color = RED;
                rotate_counterclockwise(root, next->tree_parent->tree_parent);
            }
        }
    }
    (*root)->color = BLACK;
}

/* Must be a power of 2 */
#define DEFAULT_HASH_SIZE 16

APR_DECLARE(void) apr_table_overlap(apr_table_t *a, const apr_table_t *b,
				    unsigned flags)
{
    int max_keys;
    int nkeys;
    overlap_key *cat_keys; /* concatenation of the keys of a and b */
    overlap_key **hash_table;
    int nhash;
    int i;
    apr_table_entry_t *elts;
    apr_table_entry_t *dst_elt;

    max_keys = a->a.nelts + b->a.nelts;
    if (!max_keys) {
        /* The following logic won't do anything harmful if we keep
         * going in this situation, but
         *
         *    1) certain memory debuggers don't like an alloc size of 0
         *       so we'd like to avoid that...
         *    2) this isn't all that rare a call anyway, so it is useful
         *       to skip the storage allocation and other checks in the
         *       following logic
         */
        return;
    }
    cat_keys = apr_palloc(b->a.pool, sizeof(overlap_key) * max_keys);
    nhash = DEFAULT_HASH_SIZE;
    while (nhash < max_keys) {
        nhash <<= 1;
    }
    hash_table = (overlap_key **)apr_pcalloc(b->a.pool,
                                             sizeof(overlap_key *) * nhash);

    /* The cat_keys array contains an element for each entry in a,
     * followed by one for each in b.  While populating this array,
     * we also use it as:
     *  1) a hash table, to detect matching keys, and
     *  2) a linked list of multiple values for a given key (in the
     *     APR_OVERLAP_TABLES_MERGE case)
     */

    /* First, the elements of a */
    nkeys = 0;
    elts = (apr_table_entry_t *)a->a.elts;
    for (i = 0; i < a->a.nelts; i++, nkeys++) {
        cat_keys[nkeys].elt = &(elts[i]);
        cat_keys[nkeys].level = 0;
        overlap_hash(&(cat_keys[nkeys]), hash_table, nhash, flags);
    }

    /* Then the elements of b */
    elts = (apr_table_entry_t *)b->a.elts;
    for (i = 0; i < b->a.nelts; i++, nkeys++) {
        cat_keys[nkeys].elt = &(elts[i]);
        cat_keys[nkeys].level = 1;
        overlap_hash(&(cat_keys[nkeys]), hash_table, nhash, flags);
    }

    /* Copy concatenated list of elements into table a to
     * form the new table contents, but:
     *   1) omit the ones marked "skip," and
     *   2) merge values for the same key if needed
     */
    make_array_core(&a->a, b->a.pool, max_keys, sizeof(apr_table_entry_t), 0);
    nkeys = 0;
    dst_elt = (apr_table_entry_t *)a->a.elts;
    for (i = 0; i < max_keys; i++) {
        if (cat_keys[i].skip) {
            continue;
        }
        if (cat_keys[i].merge_next) {
            char *new_val;
            char *val_next;
            overlap_key *next = cat_keys[i].merge_next;
            int len = (cat_keys[i].elt->val ?
                       strlen(cat_keys[i].elt->val) : 0);
            do {
                len += 2;
                if (next->elt->val) {
                    len += strlen(next->elt->val);
                }
                next = next->merge_next;
            } while (next);
            len++;
            new_val = (char *)apr_palloc(b->a.pool, len);
            val_next = new_val;
            if (cat_keys[i].elt->val) {
                strcpy(val_next, cat_keys[i].elt->val);
                val_next += strlen(cat_keys[i].elt->val);
            }
            next = cat_keys[i].merge_next;
            do {
                *val_next++ = ',';
                *val_next++ = ' ';
                if (next->elt->val) {
                    strcpy(val_next, next->elt->val);
                    val_next += strlen(next->elt->val);
                }
                next = next->merge_next;
            } while (next);
            *val_next = 0;
            dst_elt->key = cat_keys[i].elt->key;
            dst_elt->val = new_val;
            dst_elt->key_checksum = cat_keys[i].elt->key_checksum;
            dst_elt++;
        }
        else {
            dst_elt->key = cat_keys[i].elt->key;
            dst_elt->val = cat_keys[i].elt->val;
            dst_elt->key_checksum = cat_keys[i].elt->key_checksum;
            dst_elt++;
        }
    }
    a->a.nelts = dst_elt - (apr_table_entry_t *)a->a.elts;
    table_reindex(a);
}

