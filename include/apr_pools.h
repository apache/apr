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

#ifndef ap_POOLS_H
#define ap_POOLS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Resource allocation routines...
 *
 * designed so that we don't have to keep track of EVERYTHING so that
 * it can be explicitly freed later (a fundamentally unsound strategy ---
 * particularly in the presence of die()).
 *
 * Instead, we maintain pools, and allocate items (both memory and I/O
 * handlers) from the pools --- currently there are two, one for per
 * transaction info, and one for config info.  When a transaction is over,
 * we can delete everything in the per-transaction ap_pool_t without fear, and
 * without thinking too hard about it either.
 *
 * rst
 */

/* Arenas for configuration info and transaction info
 * --- actual layout of the ap_pool_t structure is private to 
 * alloc.c.  
 */

#include "apr_lib.h"

#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

struct process_chain {
    ap_proc_t *pid;
    enum kill_conditions kill_how;
    struct process_chain *next;
};

ap_status_t ap_init_alloc(void);	/* Set up everything */
void        ap_term_alloc(void);        /* Tear down everything */

/* used to guarantee to the ap_pool_t debugging code that the sub ap_pool_t will not be
 * destroyed before the parent pool
 */
#ifndef POOL_DEBUG
APR_EXPORT(ap_pool_t *) ap_find_pool(const void *ts);
#define ap_pool_join (a,b)
#else
APR_EXPORT(int) ap_pool_join(ap_pool_t *p, ap_pool_t *sub, int (apr_abort)(int retcode));
APR_EXPORT(ap_pool_t *) ap_find_pool(const void *ts, int (apr_abort)(int retcode));
#endif /* POOL_DEBUG */


/* Clearing out EVERYTHING in an pool... destroys any sub-pools */

/* Preparing for exec() --- close files, etc., but *don't* flush I/O
 * buffers, *don't* wait for subprocesses, and *don't* free any memory.
 */

/* routines to allocate memory from an pool... */

APR_EXPORT_NONSTD(char *) ap_psprintf(struct ap_pool_t *, const char *fmt, ...)
    __attribute__((format(printf,2,3)));

/* array and alist management... keeping lists of things.
 * Common enough to want common support code ...
 */

/* ap_array_pstrcat generates a new string from the ap_pool_t containing
 * the concatenated sequence of substrings referenced as elements within
 * the array.  The string will be empty if all substrings are empty or null,
 * or if there are no elements in the array.
 * If sep is non-NUL, it will be inserted between elements as a separator.
 */

/* copy_array copies the *entire* array.  copy_array_hdr just copies
 * the header, and arranges for the elements to be copied if (and only
 * if) the code subsequently does a push or arraycat.
 */



/* Conceptually, ap_overlap_tables does this:

    ap_array_header_t *barr = ap_table_elts(b);
    ap_table_entry_t *belt = (ap_table_entry_t *)barr->elts;
    int i;

    for (i = 0; i < barr->nelts; ++i) {
	if (flags & ap_OVERLAP_TABLES_MERGE) {
	    ap_table_mergen(a, belt[i].key, belt[i].val);
	}
	else {
	    ap_table_setn(a, belt[i].key, belt[i].val);
	}
    }

    Except that it is more efficient (less space and cpu-time) especially
    when b has many elements.

    Notice the assumptions on the keys and values in b -- they must be
    in an ancestor of a's pool.  In practice b and a are usually from
    the same pool.
*/
#define ap_OVERLAP_TABLES_SET	(0)
#define ap_OVERLAP_TABLES_MERGE	(1)

#ifdef ULTRIX_BRAIN_DEATH
#define ap_fdopen(d,m) fdopen((d), (char *)(m))
#else
#define ap_fdopen(d,m) fdopen((d), (m))
#endif

/* magic numbers --- min free bytes to consider a free ap_pool_t block useable,
 * and the min amount to allocate if we have to go to malloc() */

#ifndef BLOCK_MINFREE
#define BLOCK_MINFREE 4096
#endif
#ifndef BLOCK_MINALLOC
#define BLOCK_MINALLOC 8192
#endif

/* Finally, some accounting */

APR_EXPORT(long) ap_bytes_in_pool(ap_pool_t *p);
APR_EXPORT(long) ap_bytes_in_free_blocks(void);

#ifdef __cplusplus
}
#endif

#endif	/* !ap_POOLS_H */
