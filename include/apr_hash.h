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

#ifndef ap_HASH_H
#define ap_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apr_pools.h"


/*
 * Abstract type for hash tables.
 */
typedef struct ap_hash_t ap_hash_t;

/*
 * Abstract type for scanning hash tables.
 */
typedef struct ap_hash_index_t ap_hash_index_t;

/*

=head1 ap_hash_t *ap_make_hash(ap_pool_t *pool)

B<Create a hash table within a pool.>

=cut
*/
ap_hash_t *ap_make_hash(ap_pool_t *pool);

/*

=head1 void ap_hash_set(ap_hash_t *ht, const void *key, size_t klen,
                        const void *val)

B<Associate a value with a key in a hash table.>

    arg 1) The hash table
    arg 2) Pointer to the key
    arg 3) Length of the key
           If the length is 0 it is assumed to be strlen(key)+1
    arg 4) Value to associate with the key

If the value is NULL the hash entry is deleted.

=cut
*/
void ap_hash_set(ap_hash_t *ht, const void *key, size_t klen, const void *val);

/*

=head1 void *ap_hash_get(ap_hash_t *ht, const void *key, size_t klen)

B<Look up the value associated with a key in a hash table.>

    arg 1) The hash table
    arg 2) Pointer to the key
    arg 3) Length of the key
           If the length is 0 it is assumed to be strlen(key)+1

Returns NULL if the key is not present.

=cut
*/
void *ap_hash_get(ap_hash_t *ht, const void *key, size_t klen);

/*

=head1 ap_hash_index_t *ap_hash_first(ap_hash_t *ht)

B<Start iterating over the entries in a hash table.>

    arg 1) The hash table

Returns a pointer to the iteration state, or NULL if there are no
entries.

=cut
*/
ap_hash_index_t *ap_hash_first(ap_hash_t *ht);

/*

=head1 ap_hash_index_t *ap_hash_next(ap_hash_index_t *hi)

B<Continue iterating over the entries in a hash table.>

    arg 1) The iteration state

Returns a pointer to the updated iteration state, or NULL if there are
no more entries.

*/
ap_hash_index_t *ap_hash_next(ap_hash_index_t *hi);

/*

=head1 void ap_hash_this(ap_hash_index_t *hi, const void **key, size_t *klen,
                         void **val)

B<Get the current entry's details from the iteration state.>

    arg 1) The iteration state
    arg 2) Return pointer for the pointer to the key.
    arg 3) Return pointer for the key length.
    arg 4) Return pointer for the associated value.

The return pointers should point to a variable that will be set to the
corresponding data, or they may be NULL if the data isn't interesting.

=cut
*/
void ap_hash_this(ap_hash_index_t *hi, const void **key, size_t *klen,
                  void **val);

/*

=head2 Using the iteration functions

Example:

    int sum_values(ap_hash_t *ht)
    {
        ap_hash_index_t *hi;
	void *val;
	int sum = 0;
	for (hi = ap_hash_first(ht); hi; hi = ap_hash_next(hi)) {
	    ap_hash_this(hi, NULL, NULL, &val);
	    sum += *(int *)val;
	}
	return sum;
    }

There is no restriction on adding or deleting hash entries during an
iteration (although the results may be unpredictable unless all you do
is delete the current entry) and multiple iterations can be in
progress at the same time.

=cut
*/

#ifdef __cplusplus
}
#endif

#endif	/* !ap_HASH_H */
