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

#ifndef APR_TABLES_H
#define APR_TABLES_H

#include "apr_general.h"
#include "apr_file_io.h"
#include "apr_thread_proc.h"

#if APR_HAVE_STDARG_H
#include <stdarg.h>
#endif
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Define the structures used by the APR general-purpose library.
 */

/**
 * @package APR Table library
 */

/*
 * Memory allocation stuff, like pools, arrays, and tables.  Pools
 * and tables are opaque structures to applications, but arrays are
 * published.
 */
typedef struct apr_table_t apr_table_t;
typedef struct apr_array_header_t apr_array_header_t;

/** An opaque array type */
struct apr_array_header_t {
    /** The pool the array is allocated out of */
    apr_pool_t *cont;
    /** The amount of memory allocated for each element of the array */
    int elt_size;
    /** The number of active elements in the array */
    int nelts;
    /** The number of elements allocated in the array */
    int nalloc;
    /** The elements in the array */
    char *elts;
};

/** The opaque table type */
struct apr_table_t {
    /* This has to be first to promote backwards compatibility with
     * older modules which cast a apr_table_t * to an apr_array_header_t *...
     * they should use the table_elts() function for most of the
     * cases they do this for.
     */
    /** The underlying array for the table */
    apr_array_header_t a;
#ifdef MAKE_TABLE_PROFILE
    /** Who created the array. */
    void *creator;
#endif
};

typedef struct apr_table_entry_t apr_table_entry_t;

/** The type for each entry in a table */
struct apr_table_entry_t {
    /** The key for the current table entry */
    char *key;          /* maybe NULL in future;
                         * check when iterating thru table_elts
                         */
    /** The value for the current table entry */
    char *val;
};

/* XXX: these know about the definition of struct apr_table_t in alloc.c.  That
 * definition is not here because it is supposed to be private, and by not
 * placing it here we are able to get compile-time diagnostics from modules
 * written which assume that a apr_table_t is the same as an apr_array_header_t. -djg
 */
#define apr_table_elts(t) ((apr_array_header_t *)(t))
#define apr_is_empty_table(t) (((t) == NULL)||(((apr_array_header_t *)(t))->nelts == 0))

APR_EXPORT(apr_array_header_t *) apr_make_array(struct apr_pool_t *p, int nelts,
						int elt_size);
APR_EXPORT(void *) apr_push_array(apr_array_header_t *arr);
APR_EXPORT(void) apr_array_cat(apr_array_header_t *dst,
			       const apr_array_header_t *src);

/* copy_array copies the *entire* array.  copy_array_hdr just copies
 * the header, and arranges for the elements to be copied if (and only
 * if) the code subsequently does a push or arraycat.
 */
APR_EXPORT(apr_array_header_t *) apr_copy_array(struct apr_pool_t *p,
						const apr_array_header_t *arr);
APR_EXPORT(apr_array_header_t *)
	apr_copy_array_hdr(struct apr_pool_t *p,
			   const apr_array_header_t *arr);
APR_EXPORT(apr_array_header_t *)
	apr_append_arrays(struct apr_pool_t *p,
			  const apr_array_header_t *first,
			  const apr_array_header_t *second);

/* apr_array_pstrcat generates a new string from the apr_pool_t containing
 * the concatenated sequence of substrings referenced as elements within
 * the array.  The string will be empty if all substrings are empty or null,
 * or if there are no elements in the array.
 * If sep is non-NUL, it will be inserted between elements as a separator.
 */
APR_EXPORT(char *) apr_array_pstrcat(struct apr_pool_t *p,
				     const apr_array_header_t *arr,
				     const char sep);
APR_EXPORT(apr_table_t *) apr_make_table(struct apr_pool_t *p, int nelts);
APR_EXPORT(apr_table_t *) apr_copy_table(struct apr_pool_t *p, const apr_table_t *t);
APR_EXPORT(void) apr_clear_table(apr_table_t *t);
APR_EXPORT(const char *) apr_table_get(const apr_table_t *t, const char *key);
APR_EXPORT(void) apr_table_set(apr_table_t *t, const char *key,
			       const char *val);
APR_EXPORT(void) apr_table_setn(apr_table_t *t, const char *key,
				const char *val);
APR_EXPORT(void) apr_table_unset(apr_table_t *t, const char *key);
APR_EXPORT(void) apr_table_merge(apr_table_t *t, const char *key,
				 const char *val);
APR_EXPORT(void) apr_table_mergen(apr_table_t *t, const char *key,
				  const char *val);
APR_EXPORT(void) apr_table_add(apr_table_t *t, const char *key,
			       const char *val);
APR_EXPORT(void) apr_table_addn(apr_table_t *t, const char *key,
				const char *val);
APR_EXPORT(apr_table_t *) apr_overlay_tables(struct apr_pool_t *p,
					     const apr_table_t *overlay,
					     const apr_table_t *base);
APR_EXPORT(void)
	apr_table_do(int (*comp) (void *, const char *, const char *),
		     void *rec, const apr_table_t *t, ...);
APR_EXPORT(void)
        apr_table_vdo(int (*comp) (void *, const char *, const char *),
                     void *rec, const apr_table_t *t, va_list);                  

/* Conceptually, apr_overlap_tables does this:

    apr_array_header_t *barr = apr_table_elts(b);
    apr_table_entry_t *belt = (apr_table_entry_t *)barr->elts;
    int i;

    for (i = 0; i < barr->nelts; ++i) {
        if (flags & apr_OVERLAP_TABLES_MERGE) {
            apr_table_mergen(a, belt[i].key, belt[i].val);
        }
        else {
            apr_table_setn(a, belt[i].key, belt[i].val);
        }
    }

    Except that it is more efficient (less space and cpu-time) especially
    when b has many elements.

    Notice the assumptions on the keys and values in b -- they must be
    in an ancestor of a's pool.  In practice b and a are usually from
    the same pool.
*/
#define APR_OVERLAP_TABLES_SET   (0)
#define APR_OVERLAP_TABLES_MERGE (1)
APR_EXPORT(void) apr_overlap_tables(apr_table_t *a, const apr_table_t *b,
				    unsigned flags);

#ifdef __cplusplus
}
#endif

#endif	/* ! APR_TABLES_H */
