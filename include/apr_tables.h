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

/*
 * Memory allocation stuff, like pools, arrays, and tables.  Pools
 * and tables are opaque structures to applications, but arrays are
 * published.
 */
typedef struct ap_table_t ap_table_t;
typedef struct ap_array_header_t {
    ap_pool_t *cont;
    int elt_size;
    int nelts;
    int nalloc;
    char *elts;
} ap_array_header_t;

struct ap_table_t {
    /* This has to be first to promote backwards compatibility with
     * older modules which cast a ap_table_t * to an ap_array_header_t *...
     * they should use the table_elts() function for most of the
     * cases they do this for.
     */
    ap_array_header_t a;
#ifdef MAKE_TABLE_PROFILE
    void *creator;
#endif
};

typedef struct ap_table_entry_t {
    char *key;          /* maybe NULL in future;
                         * check when iterating thru table_elts
                         */
    char *val;
} ap_table_entry_t;

/* XXX: these know about the definition of struct ap_table_t in alloc.c.  That
 * definition is not here because it is supposed to be private, and by not
 * placing it here we are able to get compile-time diagnostics from modules
 * written which assume that a ap_table_t is the same as an ap_array_header_t. -djg
 */
#define ap_table_elts(t) ((ap_array_header_t *)(t))
#define ap_is_empty_table(t) (((t) == NULL)||(((ap_array_header_t *)(t))->nelts == 0))

APR_EXPORT(ap_array_header_t *) ap_make_array(struct ap_pool_t *p, int nelts,
						int elt_size);
APR_EXPORT(void *) ap_push_array(ap_array_header_t *arr);
APR_EXPORT(void) ap_array_cat(ap_array_header_t *dst,
			       const ap_array_header_t *src);
APR_EXPORT(ap_array_header_t *) ap_copy_array(struct ap_pool_t *p,
						const ap_array_header_t *arr);
APR_EXPORT(ap_array_header_t *)
	ap_copy_array_hdr(struct ap_pool_t *p,
			   const ap_array_header_t *arr);
APR_EXPORT(ap_array_header_t *)
	ap_append_arrays(struct ap_pool_t *p,
			  const ap_array_header_t *first,
			  const ap_array_header_t *second);
APR_EXPORT(char *) ap_array_pstrcat(struct ap_pool_t *p,
				     const ap_array_header_t *arr,
				     const char sep);
APR_EXPORT(ap_table_t *) ap_make_table(struct ap_pool_t *p, int nelts);
APR_EXPORT(ap_table_t *) ap_copy_table(struct ap_pool_t *p, const ap_table_t *t);
APR_EXPORT(void) ap_clear_table(ap_table_t *t);
APR_EXPORT(const char *) ap_table_get(const ap_table_t *t, const char *key);
APR_EXPORT(void) ap_table_set(ap_table_t *t, const char *key,
			       const char *val);
APR_EXPORT(void) ap_table_setn(ap_table_t *t, const char *key,
				const char *val);
APR_EXPORT(void) ap_table_unset(ap_table_t *t, const char *key);
APR_EXPORT(void) ap_table_merge(ap_table_t *t, const char *key,
				 const char *val);
APR_EXPORT(void) ap_table_mergen(ap_table_t *t, const char *key,
				  const char *val);
APR_EXPORT(void) ap_table_add(ap_table_t *t, const char *key,
			       const char *val);
APR_EXPORT(void) ap_table_addn(ap_table_t *t, const char *key,
				const char *val);
APR_EXPORT(ap_table_t *) ap_overlay_tables(struct ap_pool_t *p,
					     const ap_table_t *overlay,
					     const ap_table_t *base);
APR_EXPORT(void)
	ap_table_do(int (*comp) (void *, const char *, const char *),
		     void *rec, const ap_table_t *t, ...);
APR_EXPORT(void)
        ap_table_vdo(int (*comp) (void *, const char *, const char *),
                     void *rec, const ap_table_t *t, va_list);                  
#define AP_OVERLAP_TABLES_SET   (0)
#define AP_OVERLAP_TABLES_MERGE (1)
APR_EXPORT(void) ap_overlap_tables(ap_table_t *a, const ap_table_t *b,
				    unsigned flags);

#ifdef __cplusplus
}
#endif

#endif	/* ! APR_TABLES_H */
