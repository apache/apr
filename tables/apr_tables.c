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
#ifdef HAVE_MALLOC_H
#include <malloc.h>
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

APR_DECLARE(apr_array_header_t *) apr_array_make(apr_pool_t *p,
						int nelts, int elt_size)
{
    apr_array_header_t *res;

    res = (apr_array_header_t *) apr_palloc(p, sizeof(apr_array_header_t));
    make_array_core(res, p, nelts, elt_size, 1);
    return res;
}

APR_DECLARE(void *) apr_array_push(apr_array_header_t *arr)
{
    if (arr->nelts == arr->nalloc) {
        int new_size = (arr->nalloc <= 0) ? 1 : arr->nalloc * 2;
        char *new_data;

        new_data = apr_pcalloc(arr->pool, arr->elt_size * new_size);

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

/*
 * XXX: if you tweak this you should look at is_empty_table() and table_elts()
 * in alloc.h
 */
#ifdef MAKE_TABLE_PROFILE
static apr_table_entry_t *table_push(apr_table_t *t)
{
    if (t->a.nelts == t->a.nalloc) {
        return NULL;
    }
    return (apr_table_entry_t *) apr_array_push(&t->a);
}
#else /* MAKE_TABLE_PROFILE */
#define table_push(t)	((apr_table_entry_t *) apr_array_push(&(t)->a))
#endif /* MAKE_TABLE_PROFILE */


APR_DECLARE(apr_table_t *) apr_table_make(apr_pool_t *p, int nelts)
{
    apr_table_t *t = apr_palloc(p, sizeof(apr_table_t));

    make_array_core(&t->a, p, nelts, sizeof(apr_table_entry_t), 0);
#ifdef MAKE_TABLE_PROFILE
    t->creator = __builtin_return_address(0);
#endif
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
    return new;
}

APR_DECLARE(void) apr_table_clear(apr_table_t *t)
{
    t->a.nelts = 0;
}

APR_DECLARE(const char *) apr_table_get(const apr_table_t *t, const char *key)
{
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int i;
    apr_uint32_t checksum;

    if (key == NULL) {
	return NULL;
    }

    COMPUTE_KEY_CHECKSUM(key, checksum);
    for (i = 0; i < t->a.nelts; ++i) {
	if ((checksum == elts[i].key_checksum) && !strcasecmp(elts[i].key, key)) {
	    return elts[i].val;
	}
    }

    return NULL;
}

APR_DECLARE(void) apr_table_set(apr_table_t *t, const char *key,
			       const char *val)
{
    register int i, j, k;
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int done = 0;
    apr_uint32_t checksum;

    COMPUTE_KEY_CHECKSUM(key, checksum);
    for (i = 0; i < t->a.nelts; ) {
	if ((checksum == elts[i].key_checksum) && !strcasecmp(elts[i].key, key)) {
	    if (!done) {
		elts[i].val = apr_pstrdup(t->a.pool, val);
		done = 1;
		++i;
	    }
	    else {		/* delete an extraneous element */
		for (j = i, k = i + 1; k < t->a.nelts; ++j, ++k) {
		    elts[j].key = elts[k].key;
		    elts[j].val = elts[k].val;
                    elts[j].key_checksum = elts[k].key_checksum;
		}
		--t->a.nelts;
	    }
	}
	else {
	    ++i;
	}
    }

    if (!done) {
	elts = (apr_table_entry_t *) table_push(t);
	elts->key = apr_pstrdup(t->a.pool, key);
	elts->val = apr_pstrdup(t->a.pool, val);
        elts->key_checksum = checksum;
    }
}

APR_DECLARE(void) apr_table_setn(apr_table_t *t, const char *key,
				const char *val)
{
    register int i, j, k;
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int done = 0;
    apr_uint32_t checksum;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_find_pool(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_find_pool(val), t->a.pool)) {
	    fprintf(stderr, "table_set: val not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    COMPUTE_KEY_CHECKSUM(key, checksum);
    for (i = 0; i < t->a.nelts; ) {
	if ((checksum == elts[i].key_checksum) && !strcasecmp(elts[i].key, key)) {
	    if (!done) {
		elts[i].val = (char *)val;
		done = 1;
		++i;
	    }
	    else {		/* delete an extraneous element */
		for (j = i, k = i + 1; k < t->a.nelts; ++j, ++k) {
		    elts[j].key = elts[k].key;
		    elts[j].val = elts[k].val;
		    elts[j].key_checksum = elts[k].key_checksum;
		}
		--t->a.nelts;
	    }
	}
	else {
	    ++i;
	}
    }

    if (!done) {
	elts = (apr_table_entry_t *) table_push(t);
	elts->key = (char *)key;
	elts->val = (char *)val;
	elts->key_checksum = checksum;
    }
}

APR_DECLARE(void) apr_table_unset(apr_table_t *t, const char *key)
{
    register int i, j, k;
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    apr_uint32_t checksum;

    COMPUTE_KEY_CHECKSUM(key, checksum);
    for (i = 0; i < t->a.nelts; ) {
	if ((checksum == elts[i].key_checksum) && !strcasecmp(elts[i].key, key)) {

	    /* found an element to skip over
	     * there are any number of ways to remove an element from
	     * a contiguous block of memory.  I've chosen one that
	     * doesn't do a memcpy/bcopy/array_delete, *shrug*...
	     */
	    for (j = i, k = i + 1; k < t->a.nelts; ++j, ++k) {
		elts[j].key = elts[k].key;
		elts[j].val = elts[k].val;
		elts[j].key_checksum = elts[k].key_checksum;
	    }
	    --t->a.nelts;
	}
	else {
	    ++i;
	}
    }
}

APR_DECLARE(void) apr_table_merge(apr_table_t *t, const char *key,
				 const char *val)
{
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int i;
    apr_uint32_t checksum;

    COMPUTE_KEY_CHECKSUM(key, checksum);
    for (i = 0; i < t->a.nelts; ++i) {
	if ((checksum == elts[i].key_checksum) && !strcasecmp(elts[i].key, key)) {
	    elts[i].val = apr_pstrcat(t->a.pool, elts[i].val, ", ", val, NULL);
	    return;
	}
    }

    elts = (apr_table_entry_t *) table_push(t);
    elts->key = apr_pstrdup(t->a.pool, key);
    elts->val = apr_pstrdup(t->a.pool, val);
    elts->key_checksum = checksum;
}

APR_DECLARE(void) apr_table_mergen(apr_table_t *t, const char *key,
				  const char *val)
{
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int i;
    apr_uint32_t checksum;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_find_pool(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_find_pool(val), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    COMPUTE_KEY_CHECKSUM(key, checksum);
    for (i = 0; i < t->a.nelts; ++i) {
	if ((checksum == elts[i].key_checksum) && !strcasecmp(elts[i].key, key)) {
	    elts[i].val = apr_pstrcat(t->a.pool, elts[i].val, ", ", val, NULL);
	    return;
	}
    }

    elts = (apr_table_entry_t *) table_push(t);
    elts->key = (char *)key;
    elts->val = (char *)val;
    elts->key_checksum = checksum;
}

APR_DECLARE(void) apr_table_add(apr_table_t *t, const char *key,
			       const char *val)
{
    apr_table_entry_t *elts;
    apr_uint32_t checksum;

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

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_find_pool(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_find_pool(val), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

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
APR_DECLARE(void) apr_table_do(int (*comp) (void *, const char *, const char *),
			      void *rec, const apr_table_t *t, ...)
{
    va_list vp;
    va_start(vp, t);
    apr_table_vdo(comp, rec, t, vp);
    va_end(vp);  
} 
APR_DECLARE(void) apr_table_vdo(int (*comp) (void *, const char *, const char *),
				void *rec, const apr_table_t *t, va_list vp)
{
    char *argp;
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int rv, i;
    argp = va_arg(vp, char *);
    do {
        apr_uint32_t checksum = 0;
        if (argp) {
            COMPUTE_KEY_CHECKSUM(argp, checksum);
        }
	for (rv = 1, i = 0; rv && (i < t->a.nelts); ++i) {
	    if (elts[i].key && (!argp ||
                                ((checksum == elts[i].key_checksum) &&
                                 !strcasecmp(elts[i].key, argp)))) {
		rv = (*comp) (rec, elts[i].key, elts[i].val);
	    }
	}
    } while (argp && ((argp = va_arg(vp, char *)) != NULL));
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
                 * be linked into the tree, becaue the node at
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

    max_keys = a->a.nelts + b->a.nelts;
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
    for (i = 0; i < max_keys; i++) {
        if (cat_keys[i].skip) {
            continue;
        }
        if (cat_keys[i].merge_next) {
            apr_table_entry_t *elt;
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
            elt = (apr_table_entry_t *)table_push(a);
            elt->key = cat_keys[i].elt->key;
            elt->val = new_val;
            elt->key_checksum = cat_keys[i].elt->key_checksum;
        }
        else {
            apr_table_entry_t *elt = (apr_table_entry_t *)table_push(a);
            elt->key = cat_keys[i].elt->key;
            elt->val = cat_keys[i].elt->val;
            elt->key_checksum = cat_keys[i].elt->key_checksum;
        }
    }
}

