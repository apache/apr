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
#include "misc.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

/*****************************************************************
 * This file contains array and apr_table_t functions only.
 */

/*****************************************************************
 *
 * The 'array' functions...
 */

static void make_array_core(apr_array_header_t *res, apr_pool_t *c,
			    int nelts, int elt_size)
{
    /*
     * Assure sanity if someone asks for
     * array of zero elts.
     */
    if (nelts < 1) {
	nelts = 1;
    }

    res->elts = apr_pcalloc(c, nelts * elt_size);

    res->cont = c;
    res->elt_size = elt_size;
    res->nelts = 0;		/* No active elements yet... */
    res->nalloc = nelts;	/* ...but this many allocated */
}

APR_DECLARE(apr_array_header_t *) apr_make_array(apr_pool_t *p,
						int nelts, int elt_size)
{
    apr_array_header_t *res;

    res = (apr_array_header_t *) apr_palloc(p, sizeof(apr_array_header_t));
    make_array_core(res, p, nelts, elt_size);
    return res;
}

APR_DECLARE(void *) apr_push_array(apr_array_header_t *arr)
{
    if (arr->nelts == arr->nalloc) {
	int new_size = (arr->nalloc <= 0) ? 1 : arr->nalloc * 2;
	char *new_data;

	new_data = apr_pcalloc(arr->cont, arr->elt_size * new_size);

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

	new_data = apr_pcalloc(dst->cont, elt_size * new_size);
	memcpy(new_data, dst->elts, dst->nalloc * elt_size);

	dst->elts = new_data;
	dst->nalloc = new_size;
    }

    memcpy(dst->elts + dst->nelts * elt_size, src->elts,
	   elt_size * src->nelts);
    dst->nelts += src->nelts;
}

APR_DECLARE(apr_array_header_t *) apr_copy_array(apr_pool_t *p,
						const apr_array_header_t *arr)
{
    apr_array_header_t *res = apr_make_array(p, arr->nalloc, arr->elt_size);

    memcpy(res->elts, arr->elts, arr->elt_size * arr->nelts);
    res->nelts = arr->nelts;
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
    apr_copy_array_hdr(apr_pool_t *p,
		       const apr_array_header_t *arr)
{
    apr_array_header_t *res;

    res = (apr_array_header_t *) apr_palloc(p, sizeof(apr_array_header_t));
    res->cont = p;
    copy_array_hdr_core(res, arr);
    return res;
}

/* The above is used here to avoid consing multiple new array bodies... */

APR_DECLARE(apr_array_header_t *)
    apr_append_arrays(apr_pool_t *p,
		      const apr_array_header_t *first,
		      const apr_array_header_t *second)
{
    apr_array_header_t *res = apr_copy_array_hdr(p, first);

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
    int i, len;

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
    return (apr_table_entry_t *) apr_push_array(&t->a);
}
#else /* MAKE_TABLE_PROFILE */
#define table_push(t)	((apr_table_entry_t *) apr_push_array(&(t)->a))
#endif /* MAKE_TABLE_PROFILE */


APR_DECLARE(apr_table_t *) apr_make_table(apr_pool_t *p, int nelts)
{
    apr_table_t *t = apr_palloc(p, sizeof(apr_table_t));

    make_array_core(&t->a, p, nelts, sizeof(apr_table_entry_t));
#ifdef MAKE_TABLE_PROFILE
    t->creator = __builtin_return_address(0);
#endif
    return t;
}

APR_DECLARE(apr_btable_t *) apr_make_btable(apr_pool_t *p, int nelts)
{
    apr_btable_t *t = apr_palloc(p, sizeof(apr_btable_t));

    make_array_core(&t->a, p, nelts, sizeof(apr_btable_entry_t));
#ifdef MAKE_TABLE_PROFILE
    t->creator = __builtin_return_address(0);
#endif
    return t;
}

APR_DECLARE(apr_table_t *) apr_copy_table(apr_pool_t *p, const apr_table_t *t)
{
    apr_table_t *new = apr_palloc(p, sizeof(apr_table_t));

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that t->a.pool
     * have a life span at least as long as p
     */
    if (!apr_pool_is_ancestor(t->a.cont, p)) {
	fprintf(stderr, "copy_table: t's pool is not an ancestor of p\n");
	abort();
    }
#endif
    make_array_core(&new->a, p, t->a.nalloc, sizeof(apr_table_entry_t));
    memcpy(new->a.elts, t->a.elts, t->a.nelts * sizeof(apr_table_entry_t));
    new->a.nelts = t->a.nelts;
    return new;
}

APR_DECLARE(apr_btable_t *) apr_copy_btable(apr_pool_t *p,
					   const apr_btable_t *t)
{
    apr_btable_t *new = apr_palloc(p, sizeof(apr_btable_entry_t));

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that t->a.pool
     * have a life span at least as long as p
     */
    if (!apr_pool_is_ancestor(t->a.cont, p)) {
	fprintf(stderr, "copy_btable: t's pool is not an ancestor of p\n");
	abort();
    }
#endif
    make_array_core(&new->a, p, t->a.nalloc, sizeof(apr_btable_entry_t));
    memcpy(new->a.elts, t->a.elts, t->a.nelts * sizeof(apr_btable_entry_t));
    new->a.nelts = t->a.nelts;
    return new;
}

APR_DECLARE(void) apr_clear_table(apr_table_t *t)
{
    t->a.nelts = 0;
}

APR_DECLARE(void) apr_clear_btable(apr_btable_t *t)
{
    t->a.nelts = 0;
}

APR_DECLARE(const char *) apr_table_get(const apr_table_t *t, const char *key)
{
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int i;

    if (key == NULL) {
	return NULL;
    }

    for (i = 0; i < t->a.nelts; ++i) {
	if (!strcasecmp(elts[i].key, key)) {
	    return elts[i].val;
	}
    }

    return NULL;
}

APR_DECLARE(const apr_item_t *) apr_btable_get(const apr_btable_t *t,
					      const char *key)
{
    apr_btable_entry_t *elts = (apr_btable_entry_t *) t->a.elts;
    int i;

    if (key == NULL) {
	return NULL;
    }

    for (i = 0; i < t->a.nelts; ++i) {
	if (!strcasecmp(elts[i].key, key)) {
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

    for (i = 0; i < t->a.nelts; ) {
	if (!strcasecmp(elts[i].key, key)) {
	    if (!done) {
		elts[i].val = apr_pstrdup(t->a.cont, val);
		done = 1;
		++i;
	    }
	    else {		/* delete an extraneous element */
		for (j = i, k = i + 1; k < t->a.nelts; ++j, ++k) {
		    elts[j].key = elts[k].key;
		    elts[j].val = elts[k].val;
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
	elts->key = apr_pstrdup(t->a.cont, key);
	elts->val = apr_pstrdup(t->a.cont, val);
    }
}

APR_DECLARE(void) apr_btable_set(apr_btable_t *t, const char *key,
				size_t size, const void *val)
{
    register int i, j, k;
    apr_btable_entry_t *elts = (apr_btable_entry_t *) t->a.elts;
    apr_item_t *item;
    int done = 0;

    item = apr_pcalloc(t->a.cont, sizeof(apr_item_t));
    item->size = size;
    item->data = apr_pcalloc(t->a.cont, size);
    memcpy(item->data, val, size);

    for (i = 0; i < t->a.nelts; ) {
	if (!strcasecmp(elts[i].key, key)) {
	    if (!done) {
		elts[i].val = item;
		done = 1;
		++i;
	    }
	    else {		/* delete an extraneous element */
		for (j = i, k = i + 1; k < t->a.nelts; ++j, ++k) {
		    elts[j].key = elts[k].key;
		    elts[j].val = elts[k].val;
		}
		--t->a.nelts;
	    }
	}
	else {
	    ++i;
	}
    }

    if (!done) {
	elts = (apr_btable_entry_t *) table_push((apr_btable_t *) t);
	elts->key = apr_pstrdup(t->a.cont, key);
	elts->val = item;
    }
}

APR_DECLARE(void) apr_table_setn(apr_table_t *t, const char *key,
				const char *val)
{
    register int i, j, k;
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int done = 0;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_find_pool(key), t->a.cont)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_find_pool(val), t->a.cont)) {
	    fprintf(stderr, "table_set: val not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    for (i = 0; i < t->a.nelts; ) {
	if (!strcasecmp(elts[i].key, key)) {
	    if (!done) {
		elts[i].val = (char *)val;
		done = 1;
		++i;
	    }
	    else {		/* delete an extraneous element */
		for (j = i, k = i + 1; k < t->a.nelts; ++j, ++k) {
		    elts[j].key = elts[k].key;
		    elts[j].val = elts[k].val;
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
    }
}

APR_DECLARE(void) apr_btable_setn(apr_btable_t *t, const char *key,
				 size_t size, const void *val)
{
    register int i, j, k;
    apr_btable_entry_t *elts = (apr_btable_entry_t *) t->a.elts;
    int done = 0;
    apr_item_t *item;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_find_pool(key), t->a.cont)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_find_pool(val), t->a.cont)) {
	    fprintf(stderr, "table_set: val not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    item = (apr_item_t *) apr_pcalloc(t->a.cont, size);
    item->size = size;
    item->data = (void *)val;

    for (i = 0; i < t->a.nelts; ) {
	if (!strcasecmp(elts[i].key, key)) {
	    if (!done) {
		elts[i].val = item;
		done = 1;
		++i;
	    }
	    else {		/* delete an extraneous element */
		for (j = i, k = i + 1; k < t->a.nelts; ++j, ++k) {
		    elts[j].key = elts[k].key;
		    elts[j].val = elts[k].val;
		}
		--t->a.nelts;
	    }
	}
	else {
	    ++i;
	}
    }

    if (!done) {
	elts = (apr_btable_entry_t *) table_push((apr_table_t *)t);
	elts->key = (char *)key;
	elts->val = item;
    }
}

APR_DECLARE(void) apr_table_unset(apr_table_t *t, const char *key)
{
    register int i, j, k;
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;

    for (i = 0; i < t->a.nelts; ) {
	if (!strcasecmp(elts[i].key, key)) {

	    /* found an element to skip over
	     * there are any number of ways to remove an element from
	     * a contiguous block of memory.  I've chosen one that
	     * doesn't do a memcpy/bcopy/array_delete, *shrug*...
	     */
	    for (j = i, k = i + 1; k < t->a.nelts; ++j, ++k) {
		elts[j].key = elts[k].key;
		elts[j].val = elts[k].val;
	    }
	    --t->a.nelts;
	}
	else {
	    ++i;
	}
    }
}

APR_DECLARE(void) apr_btable_unset(apr_btable_t *t, const char *key)
{
    register int i, j, k;
    apr_btable_entry_t *elts = (apr_btable_entry_t *) t->a.elts;

    for (i = 0; i < t->a.nelts; ) {
	if (!strcasecmp(elts[i].key, key)) {

	    /* found an element to skip over
	     * there are any number of ways to remove an element from
	     * a contiguous block of memory.  I've chosen one that
	     * doesn't do a memcpy/bcopy/array_delete, *shrug*...
	     */
	    for (j = i, k = i + 1; k < t->a.nelts; ++j, ++k) {
		elts[j].key = elts[k].key;
		elts[j].val = elts[k].val;
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

    for (i = 0; i < t->a.nelts; ++i) {
	if (!strcasecmp(elts[i].key, key)) {
	    elts[i].val = apr_pstrcat(t->a.cont, elts[i].val, ", ", val, NULL);
	    return;
	}
    }

    elts = (apr_table_entry_t *) table_push(t);
    elts->key = apr_pstrdup(t->a.cont, key);
    elts->val = apr_pstrdup(t->a.cont, val);
}

APR_DECLARE(void) apr_table_mergen(apr_table_t *t, const char *key,
				  const char *val)
{
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;
    int i;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_find_pool(key), t->a.cont)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_find_pool(val), t->a.cont)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    for (i = 0; i < t->a.nelts; ++i) {
	if (!strcasecmp(elts[i].key, key)) {
	    elts[i].val = apr_pstrcat(t->a.cont, elts[i].val, ", ", val, NULL);
	    return;
	}
    }

    elts = (apr_table_entry_t *) table_push(t);
    elts->key = (char *)key;
    elts->val = (char *)val;
}

APR_DECLARE(void) apr_table_add(apr_table_t *t, const char *key,
			       const char *val)
{
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;

    elts = (apr_table_entry_t *) table_push(t);
    elts->key = apr_pstrdup(t->a.cont, key);
    elts->val = apr_pstrdup(t->a.cont, val);
}

APR_DECLARE(void) apr_btable_add(apr_btable_t *t, const char *key,
				size_t size, const void *val)
{
    apr_btable_entry_t *elts = (apr_btable_entry_t *) t->a.elts;
    apr_item_t *item;

    item = (apr_item_t *) apr_pcalloc(t->a.cont, sizeof(apr_item_t));
    item->size = size;
    item->data = apr_pcalloc(t->a.cont, size);
    memcpy(item->data, val, size);
    elts = (apr_btable_entry_t *) table_push((apr_btable_t *)t);
    elts->key = apr_pstrdup(t->a.cont, key);
    elts->val = item;
}

APR_DECLARE(void) apr_table_addn(apr_table_t *t, const char *key,
				const char *val)
{
    apr_table_entry_t *elts = (apr_table_entry_t *) t->a.elts;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_find_pool(key), t->a.cont)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_find_pool(val), t->a.cont)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    elts = (apr_table_entry_t *) table_push(t);
    elts->key = (char *)key;
    elts->val = (char *)val;
}

APR_DECLARE(void) apr_btable_addn(apr_btable_t *t, const char *key,
				 size_t size, const void *val)
{
    apr_btable_entry_t *elts = (apr_btable_entry_t *) t->a.elts;
    apr_item_t *item;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_find_pool(key), t->a.cont)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_find_pool(val), t->a.cont)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    item = (apr_item_t *) apr_pcalloc(t->a.cont, sizeof(apr_item_t));
    item->size = size;
    item->data = apr_pcalloc(t->a.cont, size);
    memcpy(item->data, val, size);
    elts = (apr_btable_entry_t *) table_push((apr_btable_t *)t);
    elts->key = (char *)key;
    elts->val = item;
}

APR_DECLARE(apr_table_t *) apr_overlay_tables(apr_pool_t *p,
					     const apr_table_t *overlay,
					     const apr_table_t *base)
{
    apr_table_t *res;

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that
     * overlay->a.pool and base->a.pool have a life span at least
     * as long as p
     */
    if (!apr_pool_is_ancestor(overlay->a.cont, p)) {
	fprintf(stderr,
		"overlay_tables: overlay's pool is not an ancestor of p\n");
	abort();
    }
    if (!apr_pool_is_ancestor(base->a.cont, p)) {
	fprintf(stderr,
		"overlay_tables: base's pool is not an ancestor of p\n");
	abort();
    }
#endif

    res = apr_palloc(p, sizeof(apr_table_t));
    /* behave like append_arrays */
    res->a.cont = p;
    copy_array_hdr_core(&res->a, &overlay->a);
    apr_array_cat(&res->a, &base->a);

    return res;
}

APR_DECLARE(apr_btable_t *) apr_overlay_btables(apr_pool_t *p,
					       const apr_btable_t *overlay,
					       const apr_btable_t *base)
{
    apr_btable_t *res;

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that
     * overlay->a.pool and base->a.pool have a life span at least
     * as long as p
     */
    if (!apr_pool_is_ancestor(overlay->a.cont, p)) {
	fprintf(stderr,
		"overlay_tables: overlay's pool is not an ancestor of p\n");
	abort();
    }
    if (!apr_pool_is_ancestor(base->a.cont, p)) {
	fprintf(stderr,
		"overlay_tables: base's pool is not an ancestor of p\n");
	abort();
    }
#endif

    res = apr_palloc(p, sizeof(apr_btable_t));
    /* behave like append_arrays */
    res->a.cont = p;
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
	for (rv = 1, i = 0; rv && (i < t->a.nelts); ++i) {
	    if (elts[i].key && (!argp || !strcasecmp(elts[i].key, argp))) {
		rv = (*comp) (rec, elts[i].key, elts[i].val);
	    }
	}
    } while (argp && ((argp = va_arg(vp, char *)) != NULL));
}

/* Curse libc and the fact that it doesn't guarantee a stable sort.  We
 * have to enforce stability ourselves by using the order field.  If it
 * provided a stable sort then we wouldn't even need temporary storage to
 * do the work below. -djg
 *
 * ("stable sort" means that equal keys retain their original relative
 * ordering in the output.)
 */
typedef struct {
    char *key;
    char *val;
    int order;
} overlap_key;

static int sort_overlap(const void *va, const void *vb)
{
    const overlap_key *a = va;
    const overlap_key *b = vb;
    int r;

    r = strcasecmp(a->key, b->key);
    if (r) {
	return r;
    }
    return a->order - b->order;
}

/* prefer to use the stack for temp storage for overlaps smaller than this */
#ifndef APR_OVERLAP_TABLES_ON_STACK
#define APR_OVERLAP_TABLES_ON_STACK	(512)
#endif

APR_DECLARE(void) apr_overlap_tables(apr_table_t *a, const apr_table_t *b,
				    unsigned flags)
{
    overlap_key cat_keys_buf[APR_OVERLAP_TABLES_ON_STACK];
    overlap_key *cat_keys;
    int nkeys;
    apr_table_entry_t *e;
    apr_table_entry_t *last_e;
    overlap_key *left;
    overlap_key *right;
    overlap_key *last;

    nkeys = a->a.nelts + b->a.nelts;
    if (nkeys < APR_OVERLAP_TABLES_ON_STACK) {
	cat_keys = cat_keys_buf;
    }
    else {
	/* XXX: could use scratch free space in a or b's pool instead...
	 * which could save an allocation in b's pool.
	 */
	cat_keys = apr_palloc(b->a.cont, sizeof(overlap_key) * nkeys);
    }

    nkeys = 0;

    /* Create a list of the entries from a concatenated with the entries
     * from b.
     */
    e = (apr_table_entry_t *)a->a.elts;
    last_e = e + a->a.nelts;
    while (e < last_e) {
	cat_keys[nkeys].key = e->key;
	cat_keys[nkeys].val = e->val;
	cat_keys[nkeys].order = nkeys;
	++nkeys;
	++e;
    }

    e = (apr_table_entry_t *)b->a.elts;
    last_e = e + b->a.nelts;
    while (e < last_e) {
	cat_keys[nkeys].key = e->key;
	cat_keys[nkeys].val = e->val;
	cat_keys[nkeys].order = nkeys;
	++nkeys;
	++e;
    }

    qsort(cat_keys, nkeys, sizeof(overlap_key), sort_overlap);

    /* Now iterate over the sorted list and rebuild a.
     * Start by making sure it has enough space.
     */
    a->a.nelts = 0;
    if (a->a.nalloc < nkeys) {
	a->a.elts = apr_palloc(a->a.cont, a->a.elt_size * nkeys * 2);
	a->a.nalloc = nkeys * 2;
    }

    /*
     * In both the merge and set cases we retain the invariant:
     *
     * left->key, (left+1)->key, (left+2)->key, ..., (right-1)->key
     * are all equal keys.  (i.e. strcasecmp returns 0)
     *
     * We essentially need to find the maximal
     * right for each key, then we can do a quick merge or set as
     * appropriate.
     */

    if (flags & APR_OVERLAP_TABLES_MERGE) {
	left = cat_keys;
	last = left + nkeys;
	while (left < last) {
	    right = left + 1;
	    if (right == last
		|| strcasecmp(left->key, right->key)) {
		apr_table_addn(a, left->key, left->val);
		left = right;
	    }
	    else {
		char *strp;
		char *value;
		size_t len;

		/* Have to merge some headers.  Let's re-use the order field,
		 * since it's handy... we'll store the length of val there.
		 */
		left->order = strlen(left->val);
		len = left->order;
		do {
		    right->order = strlen(right->val);
		    len += 2 + right->order;
		    ++right;
		} while (right < last
			 && !strcasecmp(left->key, right->key));
		/* right points one past the last header to merge */
		value = apr_palloc(a->a.cont, len + 1);
		strp = value;
		for (;;) {
		    memcpy(strp, left->val, left->order);
		    strp += left->order;
		    ++left;
		    if (left == right) {
			break;
		    }
		    *strp++ = ',';
		    *strp++ = ' ';
		}
		*strp = 0;
		apr_table_addn(a, (left-1)->key, value);
	    }
	}
    }
    else {
	left = cat_keys;
	last = left + nkeys;
	while (left < last) {
	    right = left + 1;
	    while (right < last && !strcasecmp(left->key, right->key)) {
		++right;
	    }
	    apr_table_addn(a, (right-1)->key, (right-1)->val);
	    left = right;
	}
    }
}

