/* ====================================================================
 * Copyright (c) 1995-1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

/*
 * Resource allocation code... the code here is responsible for making
 * sure that nothing leaks.
 *
 * rst --- 4/95 --- 6/95
 */

#ifndef WIN32
#include "apr_config.h"
#else
#include "apr_win.h"
#endif

#include "apr_general.h"
#include "apr_pools.h"
#include "apr_lib.h"
#include "misc.h"
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

/*****************************************************************
 * This file contains array and table functions only.
 */

/*****************************************************************
 *
 * The 'array' functions...
 */

static void make_array_core(ap_array_header_t *res, struct context_t *c,
			    int nelts, int elt_size)
{
    /*
     * Assure sanity if someone asks for
     * array of zero elts.
     */
    if (nelts < 1) {
	nelts = 1;
    }

    res->elts = ap_pcalloc(c, nelts * elt_size);

    res->cont = c;
    res->elt_size = elt_size;
    res->nelts = 0;		/* No active elements yet... */
    res->nalloc = nelts;	/* ...but this many allocated */
}

API_EXPORT(ap_array_header_t *) ap_make_array(struct context_t *p,
						int nelts, int elt_size)
{
    ap_array_header_t *res;

    res = (ap_array_header_t *) ap_palloc(p, sizeof(ap_array_header_t));
    make_array_core(res, p, nelts, elt_size);
    return res;
}

API_EXPORT(void *) ap_push_array(ap_array_header_t *arr)
{
    if (arr->nelts == arr->nalloc) {
	int new_size = (arr->nalloc <= 0) ? 1 : arr->nalloc * 2;
	char *new_data;

	new_data = ap_pcalloc(arr->cont, arr->elt_size * new_size);

	memcpy(new_data, arr->elts, arr->nalloc * arr->elt_size);
	arr->elts = new_data;
	arr->nalloc = new_size;
    }

    ++arr->nelts;
    return arr->elts + (arr->elt_size * (arr->nelts - 1));
}

API_EXPORT(void) ap_array_cat(ap_array_header_t *dst,
			       const ap_array_header_t *src)
{
    int elt_size = dst->elt_size;

    if (dst->nelts + src->nelts > dst->nalloc) {
	int new_size = (dst->nalloc <= 0) ? 1 : dst->nalloc * 2;
	char *new_data;

	while (dst->nelts + src->nelts > new_size) {
	    new_size *= 2;
	}

	new_data = ap_pcalloc(dst->cont, elt_size * new_size);
	memcpy(new_data, dst->elts, dst->nalloc * elt_size);

	dst->elts = new_data;
	dst->nalloc = new_size;
    }

    memcpy(dst->elts + dst->nelts * elt_size, src->elts,
	   elt_size * src->nelts);
    dst->nelts += src->nelts;
}

API_EXPORT(ap_array_header_t *) ap_copy_array(struct context_t *p,
						const ap_array_header_t *arr)
{
    ap_array_header_t *res = ap_make_array(p, arr->nalloc, arr->elt_size);

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

static APR_INLINE void copy_array_hdr_core(ap_array_header_t *res,
					   const ap_array_header_t *arr)
{
    res->elts = arr->elts;
    res->elt_size = arr->elt_size;
    res->nelts = arr->nelts;
    res->nalloc = arr->nelts;	/* Force overflow on push */
}

API_EXPORT(ap_array_header_t *)
    ap_copy_array_hdr(struct context_t *p,
		       const ap_array_header_t *arr)
{
    ap_array_header_t *res;

    res = (ap_array_header_t *) ap_palloc(p, sizeof(ap_array_header_t));
    res->cont = p;
    copy_array_hdr_core(res, arr);
    return res;
}

/* The above is used here to avoid consing multiple new array bodies... */

API_EXPORT(ap_array_header_t *)
    ap_append_arrays(struct context_t *p,
		      const ap_array_header_t *first,
		      const ap_array_header_t *second)
{
    ap_array_header_t *res = ap_copy_array_hdr(p, first);

    ap_array_cat(res, second);
    return res;
}

/* ap_array_pstrcat generates a new string from the pool containing
 * the concatenated sequence of substrings referenced as elements within
 * the array.  The string will be empty if all substrings are empty or null,
 * or if there are no elements in the array.
 * If sep is non-NUL, it will be inserted between elements as a separator.
 */
API_EXPORT(char *) ap_array_pstrcat(struct context_t *p,
				     const ap_array_header_t *arr,
				     const char sep)
{
    char *cp, *res, **strpp;
    int i, len;

    if (arr->nelts <= 0 || arr->elts == NULL) {    /* Empty table? */
        return (char *) ap_pcalloc(p, 1);
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

    res = (char *) ap_palloc(p, len + 1);
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
static ap_table_entry_t *table_push(ap_table_t *t)
{
    if (t->a.nelts == t->a.nalloc) {
	fprintf(stderr,
		"table_push: table created by %p hit limit of %u\n",
		t->creator, t->a.nalloc);
    }
    return (ap_table_entry_t *) ap_push_array(&t->a);
}
#else /* MAKE_TABLE_PROFILE */
#define table_push(t)	((ap_table_entry_t *) ap_push_array(&(t)->a))
#endif /* MAKE_TABLE_PROFILE */


API_EXPORT(ap_table_t *) ap_make_table(struct context_t *p, int nelts)
{
    ap_table_t *t = ap_palloc(p, sizeof(ap_table_t));

    make_array_core(&t->a, p, nelts, sizeof(ap_table_entry_t));
#ifdef MAKE_TABLE_PROFILE
    t->creator = __builtin_return_address(0);
#endif
    return t;
}

API_EXPORT(ap_table_t *) ap_copy_table(struct context_t *p, const ap_table_t *t)
{
    ap_table_t *new = ap_palloc(p, sizeof(ap_table_t));

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that t->a.pool
     * have a life span at least as long as p
     */
    if (!ap_pool_is_ancestor(t->a.pool, p)) {
	fprintf(stderr, "copy_table: t's pool is not an ancestor of p\n");
	abort();
    }
#endif
    make_array_core(&new->a, p, t->a.nalloc, sizeof(ap_table_entry_t));
    memcpy(new->a.elts, t->a.elts, t->a.nelts * sizeof(ap_table_entry_t));
    new->a.nelts = t->a.nelts;
    return new;
}

API_EXPORT(void) ap_clear_table(ap_table_t *t)
{
    t->a.nelts = 0;
}

API_EXPORT(const char *) ap_table_get(const ap_table_t *t, const char *key)
{
    ap_table_entry_t *elts = (ap_table_entry_t *) t->a.elts;
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

API_EXPORT(void) ap_table_set(ap_table_t *t, const char *key,
			       const char *val)
{
    register int i, j, k;
    ap_table_entry_t *elts = (ap_table_entry_t *) t->a.elts;
    int done = 0;

    for (i = 0; i < t->a.nelts; ) {
	if (!strcasecmp(elts[i].key, key)) {
	    if (!done) {
		elts[i].val = ap_pstrdup(t->a.cont, val);
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
	elts = (ap_table_entry_t *) table_push(t);
	elts->key = ap_pstrdup(t->a.cont, key);
	elts->val = ap_pstrdup(t->a.cont, val);
    }
}

API_EXPORT(void) ap_table_setn(ap_table_t *t, const char *key,
				const char *val)
{
    register int i, j, k;
    ap_table_entry_t *elts = (ap_table_entry_t *) t->a.elts;
    int done = 0;

#ifdef POOL_DEBUG
    {
	if (!ap_pool_is_ancestor(ap_find_pool(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!ap_pool_is_ancestor(ap_find_pool(val), t->a.pool)) {
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
	elts = (ap_table_entry_t *) table_push(t);
	elts->key = (char *)key;
	elts->val = (char *)val;
    }
}

API_EXPORT(void) ap_table_unset(ap_table_t *t, const char *key)
{
    register int i, j, k;
    ap_table_entry_t *elts = (ap_table_entry_t *) t->a.elts;

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

API_EXPORT(void) ap_table_merge(ap_table_t *t, const char *key,
				 const char *val)
{
    ap_table_entry_t *elts = (ap_table_entry_t *) t->a.elts;
    int i;

    for (i = 0; i < t->a.nelts; ++i) {
	if (!strcasecmp(elts[i].key, key)) {
	    elts[i].val = ap_pstrcat(t->a.cont, elts[i].val, ", ", val, NULL);
	    return;
	}
    }

    elts = (ap_table_entry_t *) table_push(t);
    elts->key = ap_pstrdup(t->a.cont, key);
    elts->val = ap_pstrdup(t->a.cont, val);
}

API_EXPORT(void) ap_table_mergen(ap_table_t *t, const char *key,
				  const char *val)
{
    ap_table_entry_t *elts = (ap_table_entry_t *) t->a.elts;
    int i;

#ifdef POOL_DEBUG
    {
	if (!ap_pool_is_ancestor(ap_find_pool(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!ap_pool_is_ancestor(ap_find_pool(val), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    for (i = 0; i < t->a.nelts; ++i) {
	if (!strcasecmp(elts[i].key, key)) {
	    elts[i].val = ap_pstrcat(t->a.cont, elts[i].val, ", ", val, NULL);
	    return;
	}
    }

    elts = (ap_table_entry_t *) table_push(t);
    elts->key = (char *)key;
    elts->val = (char *)val;
}

API_EXPORT(void) ap_table_add(ap_table_t *t, const char *key,
			       const char *val)
{
    ap_table_entry_t *elts = (ap_table_entry_t *) t->a.elts;

    elts = (ap_table_entry_t *) table_push(t);
    elts->key = ap_pstrdup(t->a.cont, key);
    elts->val = ap_pstrdup(t->a.cont, val);
}

API_EXPORT(void) ap_table_addn(ap_table_t *t, const char *key,
				const char *val)
{
    ap_table_entry_t *elts = (ap_table_entry_t *) t->a.elts;

#ifdef POOL_DEBUG
    {
	if (!ap_pool_is_ancestor(ap_find_pool(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!ap_pool_is_ancestor(ap_find_pool(val), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    elts = (ap_table_entry_t *) table_push(t);
    elts->key = (char *)key;
    elts->val = (char *)val;
}

API_EXPORT(ap_table_t *) ap_overlay_tables(struct context_t *p,
					     const ap_table_t *overlay,
					     const ap_table_t *base)
{
    ap_table_t *res;

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that
     * overlay->a.pool and base->a.pool have a life span at least
     * as long as p
     */
    if (!ap_pool_is_ancestor(overlay->a.pool, p->pool)) {
	fprintf(stderr,
		"overlay_tables: overlay's pool is not an ancestor of p\n");
	abort();
    }
    if (!ap_pool_is_ancestor(base->a.pool, p->pool)) {
	fprintf(stderr,
		"overlay_tables: base's pool is not an ancestor of p\n");
	abort();
    }
#endif

    res = ap_palloc(p, sizeof(ap_table_t));
    /* behave like append_arrays */
    res->a.cont = p;
    copy_array_hdr_core(&res->a, &overlay->a);
    ap_array_cat(&res->a, &base->a);

    return res;
}

/* And now for something completely abstract ...

 * For each key value given as a vararg:
 *   run the function pointed to as
 *     int comp(void *r, char *key, char *value);
 *   on each valid key-value pair in the table t that matches the vararg key,
 *   or once for every valid key-value pair if the vararg list is empty,
 *   until the function returns false (0) or we finish the table.
 *
 * Note that we restart the traversal for each vararg, which means that
 * duplicate varargs will result in multiple executions of the function
 * for each matching key.  Note also that if the vararg list is empty,
 * only one traversal will be made and will cut short if comp returns 0.
 *
 * Note that the table_get and table_merge functions assume that each key in
 * the table is unique (i.e., no multiple entries with the same key).  This
 * function does not make that assumption, since it (unfortunately) isn't
 * true for some of Apache's tables.
 *
 * Note that rec is simply passed-on to the comp function, so that the
 * caller can pass additional info for the task.
 */
API_EXPORT(void) ap_table_do(int (*comp) (void *, const char *, const char *),
			      void *rec, const ap_table_t *t, ...)
{
    va_list vp;
    char *argp;
    ap_table_entry_t *elts = (ap_table_entry_t *) t->a.elts;
    int rv, i;

    va_start(vp, t);

    argp = va_arg(vp, char *);

    do {
	for (rv = 1, i = 0; rv && (i < t->a.nelts); ++i) {
	    if (elts[i].key && (!argp || !strcasecmp(elts[i].key, argp))) {
		rv = (*comp) (rec, elts[i].key, elts[i].val);
	    }
	}
    } while (argp && ((argp = va_arg(vp, char *)) != NULL));

    va_end(vp);
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
#ifndef ap_OVERLAP_TABLES_ON_STACK
#define ap_OVERLAP_TABLES_ON_STACK	(512)
#endif

API_EXPORT(void) ap_overlap_tables(ap_table_t *a, const ap_table_t *b,
				    unsigned flags)
{
    overlap_key cat_keys_buf[ap_OVERLAP_TABLES_ON_STACK];
    overlap_key *cat_keys;
    int nkeys;
    ap_table_entry_t *e;
    ap_table_entry_t *last_e;
    overlap_key *left;
    overlap_key *right;
    overlap_key *last;

    nkeys = a->a.nelts + b->a.nelts;
    if (nkeys < ap_OVERLAP_TABLES_ON_STACK) {
	cat_keys = cat_keys_buf;
    }
    else {
	/* XXX: could use scratch free space in a or b's pool instead...
	 * which could save an allocation in b's pool.
	 */
	cat_keys = ap_palloc(b->a.cont, sizeof(overlap_key) * nkeys);
    }

    nkeys = 0;

    /* Create a list of the entries from a concatenated with the entries
     * from b.
     */
    e = (ap_table_entry_t *)a->a.elts;
    last_e = e + a->a.nelts;
    while (e < last_e) {
	cat_keys[nkeys].key = e->key;
	cat_keys[nkeys].val = e->val;
	cat_keys[nkeys].order = nkeys;
	++nkeys;
	++e;
    }

    e = (ap_table_entry_t *)b->a.elts;
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
	a->a.elts = ap_palloc(a->a.cont, a->a.elt_size * nkeys * 2);
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

    if (flags & ap_OVERLAP_TABLES_MERGE) {
	left = cat_keys;
	last = left + nkeys;
	while (left < last) {
	    right = left + 1;
	    if (right == last
		|| strcasecmp(left->key, right->key)) {
		ap_table_addn(a, left->key, left->val);
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
		value = ap_palloc(a->a.cont, len + 1);
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
		ap_table_addn(a, (left-1)->key, value);
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
	    ap_table_addn(a, (right-1)->key, (right-1)->val);
	    left = right;
	}
    }
}

