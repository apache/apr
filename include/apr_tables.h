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

#include "apr.h"
#include "apr_pools.h"

#if APR_HAVE_STDARG_H
#include <stdarg.h>     /* for va_list */
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

/** The opaque string-content table type */
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

/**
 * The (opaque) structure for string-content tables.
 */
typedef struct apr_table_entry_t apr_table_entry_t;

/** The type for each entry in a string-content table */
struct apr_table_entry_t {
    /** The key for the current table entry */
    char *key;          /* maybe NULL in future;
                         * check when iterating thru table_elts
                         */
    /** The value for the current table entry */
    char *val;
};

/**
 * Get the elements from a table
 * @param t The table
 * @return An array containing the contents of the table
 * @deffunc apr_array_header_t *apr_table_elts(apr_table_t *t)
 */
#define apr_table_elts(t) ((apr_array_header_t *)(t))

/**
 * Determine if the table is empty
 * @param t The table to check
 * @return True if empty, Falso otherwise
 * @deffunc int apr_is_empty_table(apr_table_t *t)
 */
#define apr_is_empty_table(t) (((t) == NULL) \
                               || (((apr_array_header_t *)(t))->nelts == 0))

/**
 * Create an array
 * @param p The pool to allocate the memory out of
 * @param nelts the number of elements in the initial array
 * @param elt_size The size of each element in the array.
 * @return The new array
 * @deffunc apr_array_header_t *apr_make_array(struct apr_pool_t *p, int nelts, int elt_size)
 */
APR_DECLARE(apr_array_header_t *) apr_make_array(struct apr_pool_t *p,
                                                 int nelts, int elt_size);

/**
 * Add a new element to an array
 * @param arr The array to add an element to.
 * @return Location for the new element in the array.
 * @tip If there are no free spots in the array, then this function will
 *      allocate new space for the new element.
 * @deffunc void *apr_push_array(apr_array_header_t *arr)
 */
APR_DECLARE(void *) apr_push_array(apr_array_header_t *arr);

/**
 * Concatenate two arrays together
 * @param dst The destination array, and the one to go first in the combined 
 *            array
 * @param src The source array to add to the destination array
 * @deffunc void apr_array_cat(apr_array_header_t *dst, const apr_array_header_t *src)
 */
APR_DECLARE(void) apr_array_cat(apr_array_header_t *dst,
			        const apr_array_header_t *src);

/**
 * Copy the entire array
 * @param p The pool to allocate the copy of the array out of
 * @param arr The array to copy
 * @return An exact copy of the array passed in
 * @deffunc apr_array_header_t *apr_copy_array(apr_pool_t *p, const apr_array_header_t *arr)
 * @tip The alternate apr_copy_array_hdr copies only the header, and arranges 
 * for the elements to be copied if (and only if) the code subsequently does 
 * a push or arraycat.
 */
APR_DECLARE(apr_array_header_t *) 
                apr_copy_array(struct apr_pool_t *p,
                               const apr_array_header_t *arr);
/**
 * Copy the headers of the array, and arrange for the elements to be copied if
 * and only if the code subsequently does a push or arraycat.
 * @param p The pool to allocate the copy of the array out of
 * @param arr The array to copy
 * @return An exact copy of the array passed in
 * @deffunc apr_array_header_t *apr_copy_array_hdr(apr_pool_t *p, const apr_array_header_t *arr)
 * @tip The alternate apr_copy_array copies the *entire* array.
 */
APR_DECLARE(apr_array_header_t *)
                apr_copy_array_hdr(struct apr_pool_t *p,
                                   const apr_array_header_t *arr);

/**
 * Append one array to the end of another, creating a new array in the process.
 * @param p The pool to allocate the new array out of
 * @param first The array to put first in the new array.
 * @param second The array to put second in the new array.
 * @param return A new array containing the data from the two arrays passed in.
 * @deffunc apr_array_header_t *apr_append_arrays(apr_pool_t *p, const apr_array_header_t *first, const apr_array_header_t *second)
*/
APR_DECLARE(apr_array_header_t *)
                apr_append_arrays(struct apr_pool_t *p,
                                  const apr_array_header_t *first,
                                  const apr_array_header_t *second);

/**
 * Generates a new string from the apr_pool_t containing the concatenated 
 * sequence of substrings referenced as elements within the array.  The string 
 * will be empty if all substrings are empty or null, or if there are no 
 * elements in the array.  If sep is non-NUL, it will be inserted between 
 * elements as a separator.
 * @param p The pool to allocate the string out of
 * @param arr The array to generate the string from
 * @param sep The separator to use
 * @return A string containing all of the data in the array.
 * @deffunc char *apr_array_pstrcat(apr_pool_t *p, const apr_array_header_t *arr, const char sep)
 */
APR_DECLARE(char *) apr_array_pstrcat(struct apr_pool_t *p,
				      const apr_array_header_t *arr,
				      const char sep);

/**
 * Make a new table
 * @param p The pool to allocate the pool out of
 * @param nelts The number of elements in the initial table.
 * @return The new table.
 * @warning This table can only store text data
 * @deffunc apr_table_t *apr_make_table(apr_pool_t *p, int nelts)
 */
APR_DECLARE(apr_table_t *) apr_make_table(struct apr_pool_t *p, int nelts);

/**
 * Create a new table and copy another table into it
 * @param p The pool to allocate the new table out of
 * @param t The table to copy
 * @return A copy of the table passed in
 * @deffunc apr_table_t *apr_copy_table(apr_pool_t *p, const apr_table_t *t)
 */
APR_DECLARE(apr_table_t *) apr_copy_table(struct apr_pool_t *p,
                                          const apr_table_t *t);

/**
 * Delete all of the elements from a table
 * @param t The table to clear
 * @deffunc void apr_clear_table(apr_table_t *t)
 */
APR_DECLARE(void) apr_clear_table(apr_table_t *t);

/**
 * Get the value associated with a given key from the table.  After this call,
 * The data is still in the table
 * @param t The table to search for the key
 * @param key The key to search for
 * @return The value associated with the key
 * @deffunc const char *apr_table_get(const apr_table_t *t, const char *key)
 */
APR_DECLARE(const char *) apr_table_get(const apr_table_t *t, const char *key);

/**
 * Add a key/value pair to a table, if another element already exists with the
 * same key, this will over-write the old data.
 * @param t The table to add the data to.
 * @param key The key fo use
 * @param val The value to add
 * @tip When adding data, this function makes a copy of both the key and the
 *      value.
 * @deffunc void apr_table_set(apr_table_t *t, const char *key, const char *val)
 */
APR_DECLARE(void) apr_table_set(apr_table_t *t, const char *key,
                                const char *val);

/**
 * Add a key/value pair to a table, if another element already exists with the
 * same key, this will over-write the old data.
 * @param t The table to add the data to.
 * @param key The key fo use
 * @param val The value to add
 * @tip When adding data, this function does not make a copy of the key or the
 *      value, so care should be taken to ensure that the values will not 
 *      change after they have been added..
 * @deffunc void apr_table_setn(apr_table_t *t, const char *key, const char *val)
 */
APR_DECLARE(void) apr_table_setn(apr_table_t *t, const char *key,
                                 const char *val);

/**
 * Remove data from the table
 * @param t The table to remove data from
 * @param key The key of the data being removed
 * @deffunc void apr_table_unset(apr_table_t *t, const char *key)
 */
APR_DECLARE(void) apr_table_unset(apr_table_t *t, const char *key);

/**
 * Add data to a table by merging the value with data that has already been 
 * stored
 * @param t The table to search for the data
 * @param key The key to merge data for
 * @param val The data to add
 * @tip If the key is not found, then this function acts like apr_table_add
 * @deffunc void apr_table_merge(apr_table_t *t, const char *key, const char *val)
 */
APR_DECLARE(void) apr_table_merge(apr_table_t *t, const char *key,
                                  const char *val);

/**
 * Add data to a table by merging the value with data that has already been 
 * stored
 * @param t The table to search for the data
 * @param key The key to merge data for
 * @param val The data to add
 * @tip If the key is not found, then this function acts like apr_table_addn
 * @deffunc void apr_table_mergen(apr_table_t *t, const char *key, const char *val)
 */
APR_DECLARE(void) apr_table_mergen(apr_table_t *t, const char *key,
                                   const char *val);

/**
 * Add data to a table, regardless of whether there is another element with the
 * same key.
 * @param t The table to add to
 * @param key The key to use
 * @param val The value to add.
 * @tip When adding data, this function makes a copy of both the key and the
 *      value.
 * @deffunc void apr_table_add(apr_table_t *t, const char *key, const char *val)
 */
APR_DECLARE(void) apr_table_add(apr_table_t *t, const char *key,
                                const char *val);

/**
 * Add data to a table, regardless of whether there is another element with the
 * same key.
 * @param t The table to add to
 * @param key The key to use
 * @param val The value to add.
 * @tip When adding data, this function does not make a copy of the key or the
 *      value, so care should be taken to ensure that the values will not 
 *      change after they have been added..
 * @deffunc void apr_table_addn(apr_table_t *t, const char *key, const char *val)
 */
APR_DECLARE(void) apr_table_addn(apr_table_t *t, const char *key,
                                 const char *val);

/**
 * Merge two tables into one new table
 * @param p The pool to use for the new table
 * @param overlay The first table to put in the new table
 * @param base The table to add at the end of the new table
 * @return A new table containing all of the data from the two passed in
 * @deffunc apr_table_t *apr_overlay_tables(apr_pool_t *p, const apr_table_t *overlay, const apr_table_t *base);
 */
APR_DECLARE(apr_table_t *) apr_overlay_tables(struct apr_pool_t *p,
                                              const apr_table_t *overlay,
                                              const apr_table_t *base);

/** 
 * Iterate over a table running the provided function once for every
 * element in the table.  If there is data passed in as a vararg, then the 
 * function is only run on those element's whose key matches something in 
 * the vararg.  If the vararg is NULL, then every element is run through the
 * function.
 * @param comp The function to run
 * @param rec The data to pass as the first argument to the function
 * @param t The table to iterate over
 * @param ... The vararg.  If this is NULL, then all elements in the table are
 *            run through the function, otherwise only those whose key matches
 *            are run.
 * @deffunc void apr_table_do(int (*comp) (void *, const char *, const char *), void *rec, const apr_table_t *t, ...)
 */
APR_DECLARE_NONSTD(void)
                apr_table_do(int (*comp) (void *, const char *, const char *),
                             void *rec, const apr_table_t *t, ...);

/** 
 * Iterate over a table running the provided function once for every
 * element in the table.  If there is data passed in as a vararg, then the 
 * function is only run on those element's whose key matches something in 
 * the vararg.  If the vararg is NULL, then every element is run through the
 * function.
 * @param comp The function to run
 * @param rec The data to pass as the first argument to the function
 * @param t The table to iterate over
 * @param vp The vararg table.  If this is NULL, then all elements in the 
 *                table are run through the function, otherwise only those 
 *                whose key matches are run.
 * @deffunc void apr_table_vdo(int (*comp) (void *, const char *, const char *), void *rec, const apr_table_t *t, va_list vp)
 */
APR_DECLARE(void)
                apr_table_vdo(int (*comp) (void *, const char *, const char *),
                              void *rec, const apr_table_t *t, va_list);                  

/* Conceptually, apr_overlap_tables does this:
 *
 *  apr_array_header_t *barr = apr_table_elts(b);
 *  apr_table_entry_t *belt = (apr_table_entry_t *)barr->elts;
 *  int i;
 *
 *  for (i = 0; i < barr->nelts; ++i) {
 *      if (flags & apr_OVERLAP_TABLES_MERGE) {
 *          apr_table_mergen(a, belt[i].key, belt[i].val);
 *      }
 *      else {
 *          apr_table_setn(a, belt[i].key, belt[i].val);
 *      }
 *  }
 *
 *  Except that it is more efficient (less space and cpu-time) especially
 *  when b has many elements.
 *
 *  Notice the assumptions on the keys and values in b -- they must be
 *  in an ancestor of a's pool.  In practice b and a are usually from
 *  the same pool.
 */
#define APR_OVERLAP_TABLES_SET   (0)
#define APR_OVERLAP_TABLES_MERGE (1)
/**
 * For each element in table b, either use setn or mergen to add the data
 * to table a.  Wich method is used is determined by the flags passed in.
 * @param a The table to add the data to.
 * @param b The table to iterate over, adding it's data to table a
 * @param flags How to add the table to table a.  One of:
 *          APR_OVERLAP_TABLES_SET        Use apr_table_setn
 *          APR_OVERLAP_TABLES_MERGE      Use apr_table_mergen
 * @tip This function is highly optimized, and uses less memory and CPU cycles
 *      than a function that just loops through table b calling other functions.
 * @deffunc void apr_overlap_tables(apr_table_t *a, const apr_table_t *b, unsigned flags)
 */
APR_DECLARE(void) apr_overlap_tables(apr_table_t *a, const apr_table_t *b,
                                     unsigned flags);

#ifdef __cplusplus
}
#endif

#endif	/* ! APR_TABLES_H */
