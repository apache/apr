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

/*
 * Debugging support: Define this to enable code which helps detect re-use
 * of freed memory and other such nonsense.
 *
 * The theory is simple.  The FILL_BYTE (0xa5) is written over all malloc'd
 * memory as we receive it, and is written over everything that we free up
 * during a clear_pool.  We check that blocks on the free list always
 * have the FILL_BYTE in them, and we check during palloc() that the bytes
 * still have FILL_BYTE in them.  If you ever see garbage URLs or whatnot
 * containing lots of 0xa5s then you know something used data that's been
 * freed or uninitialized.
 */
/* #define ALLOC_DEBUG */

/*
 * Debugging support: If defined all allocations will be done with
 * malloc and free()d appropriately at the end.  This is intended to be
 * used with something like Electric Fence or Purify to help detect
 * memory problems.  Note that if you're using efence then you should also
 * add in ALLOC_DEBUG.  But don't add in ALLOC_DEBUG if you're using Purify
 * because ALLOC_DEBUG would hide all the uninitialized read errors that
 * Purify can diagnose.
 */
/* #define ALLOC_USE_MALLOC */

/*
 * Pool debugging support:  This is intended to detect cases where the
 * wrong pool is used when assigning data to an object in another pool.
 * In particular, it causes the table_{set,add,merge}n routines to check
 * that their arguments are safe for the table they're being placed in.
 * It currently only works with the unix multiprocess model, but could
 * be extended to others.
 */
/* #define POOL_DEBUG */

/*
 * Provide diagnostic information about make_table() calls which are
 * possibly too small.  This requires a recent gcc which supports
 * __builtin_return_address().  The error_log output will be a
 * message such as:
 *    table_push: table created by 0x804d874 hit limit of 10
 * Use "l *0x804d874" to find the source that corresponds to.  It
 * indicates that a table allocated by a call at that address has
 * possibly too small an initial table size guess.
 */
/* #define MAKE_TABLE_PROFILE */

/*
 * Provide some statistics on the cost of allocations.  It requires a
 * bit of an understanding of how alloc.c works.
 */
/* #define ALLOC_STATS */

#ifdef POOL_DEBUG
#ifdef ALLOC_USE_MALLOC
#error "sorry, no support for ALLOC_USE_MALLOC and POOL_DEBUG at the same time"
#endif /* ALLOC_USE_MALLOC */

#ifdef MULTITHREAD
# error "sorry, no support for MULTITHREAD and POOL_DEBUG at the same time"
#endif /* MULTITHREAD */

#endif /* POOL_DEBUG */

#ifdef ALLOC_USE_MALLOC
#undef BLOCK_MINFREE
#undef BLOCK_MINALLOC
#define BLOCK_MINFREE	0
#define BLOCK_MINALLOC	0
#endif /* ALLOC_USE_MALLOC */

/*****************************************************************
 *
 * Managing free storage blocks...
 */

union align {
    /*
     * Types which are likely to have the longest RELEVANT alignment
     * restrictions...
     */

    char *cp;
    void (*f) (void);
    long l;
    FILE *fp;
    double d;
};

#define CLICK_SZ (sizeof(union align))

union block_hdr {
    union align a;

    /* Actual header... */

    struct {
	char *endp;
	union block_hdr *next;
	char *first_avail;
#ifdef POOL_DEBUG
	union block_hdr *global_next;
	ap_pool_t *owning_pool;
#endif /* POOL_DEBUG */
    } h;
};

/*
 * Static cells for managing our internal synchronisation.
 */
static union block_hdr *block_freelist = NULL;
static ap_mutex_t *alloc_mutex = NULL;
static ap_mutex_t *spawn_mutex = NULL;

#ifdef POOL_DEBUG
static char *known_stack_point;
static int stack_direction;
static union block_hdr *global_block_list;
#define FREE_POOL	((ap_pool_t *)(-1))
#endif /* POOL_DEBUG */

#ifdef ALLOC_STATS
static unsigned long long num_free_blocks_calls;
static unsigned long long num_blocks_freed;
static unsigned max_blocks_in_one_free;
static unsigned num_malloc_calls;
static unsigned num_malloc_bytes;
#endif /* ALLOC_STATS */

#ifdef ALLOC_DEBUG
#define FILL_BYTE	((char)(0xa5))
#define debug_fill(ptr,size)	((void)memset((ptr), FILL_BYTE, (size)))

static APR_INLINE void debug_verify_filled(const char *ptr, const char *endp,
					   const char *error_msg)
{
    for ( ; ptr < endp; ++ptr) {
	if (*ptr != FILL_BYTE) {
	    fputs(error_msg, stderr);
	    abort();
	    exit(1);
	}
    }
}

#else /* ALLOC_DEBUG */
#define debug_fill(a,b)
#define debug_verify_filled(a,b,c)
#endif /* ALLOC_DEBUG */

/*
 * Get a completely new block from the system pool. Note that we rely on
 * malloc() to provide aligned memory.
 */

static union block_hdr *malloc_block(int size)
{
    union block_hdr *blok;

#ifdef ALLOC_DEBUG
    /* make some room at the end which we'll fill and expect to be
     * always filled
     */
    size += CLICK_SZ;
#endif /* ALLOC_DEBUG */

#ifdef ALLOC_STATS
    ++num_malloc_calls;
    num_malloc_bytes += size + sizeof(union block_hdr);
#endif /* ALLOC_STATS */

    blok = (union block_hdr *) malloc(size + sizeof(union block_hdr));
    if (blok == NULL) {
	fprintf(stderr, "Ouch!  malloc failed in malloc_block()\n");
	exit(1);
    }
    debug_fill(blok, size + sizeof(union block_hdr));
    blok->h.next = NULL;
    blok->h.first_avail = (char *) (blok + 1);
    blok->h.endp = size + blok->h.first_avail;

#ifdef ALLOC_DEBUG
    blok->h.endp -= CLICK_SZ;
#endif /* ALLOC_DEBUG */

#ifdef POOL_DEBUG
    blok->h.global_next = global_block_list;
    global_block_list = blok;
    blok->h.owning_pool = NULL;
#endif /* POOL_DEBUG */

    return blok;
}



#if defined(ALLOC_DEBUG) && !defined(ALLOC_USE_MALLOC)
static void chk_on_blk_list(union block_hdr *blok, union block_hdr *free_blk)
{
    debug_verify_filled(blok->h.endp, blok->h.endp + CLICK_SZ,
			"Ouch!  Someone trounced the padding "
			"at the end of a block!\n");
    while (free_blk) {
	if (free_blk == blok) {
	    fprintf(stderr, "Ouch!  Freeing free block\n");
	    abort();
	    exit(1);
	}
	free_blk = free_blk->h.next;
    }
}
#else /* defined(ALLOC_DEBUG) && !defined(ALLOC_USE_MALLOC) */
#define chk_on_blk_list(_x, _y)
#endif /* defined(ALLOC_DEBUG) && !defined(ALLOC_USE_MALLOC) */

/* Free a chain of blocks --- must be called with alarms blocked. */

static void free_blocks(union block_hdr *blok)
{
#ifdef ALLOC_USE_MALLOC
    union block_hdr *next;

    for ( ; blok; blok = next) {
	next = blok->h.next;
	free(blok);
    }
#else /* ALLOC_USE_MALLOC */

#ifdef ALLOC_STATS
    unsigned num_blocks;
#endif /* ALLOC_STATS */

    /*
     * First, put new blocks at the head of the free list ---
     * we'll eventually bash the 'next' pointer of the last block
     * in the chain to point to the free blocks we already had.
     */

    union block_hdr *old_free_list;

    if (blok == NULL) {
	return;			/* Sanity check --- freeing empty pool? */
    }

    (void) ap_acquire_mutex(alloc_mutex);
    old_free_list = block_freelist;
    block_freelist = blok;

    /*
     * Next, adjust first_avail pointers of each block --- have to do it
     * sooner or later, and it simplifies the search in new_block to do it
     * now.
     */

#ifdef ALLOC_STATS
    num_blocks = 1;
#endif /* ALLOC_STATS */

    while (blok->h.next != NULL) {

#ifdef ALLOC_STATS
	++num_blocks;
#endif /* ALLOC_STATS */

	chk_on_blk_list(blok, old_free_list);
	blok->h.first_avail = (char *) (blok + 1);
	debug_fill(blok->h.first_avail, blok->h.endp - blok->h.first_avail);
#ifdef POOL_DEBUG
	blok->h.owning_pool = FREE_POOL;
#endif /* POOL_DEBUG */
	blok = blok->h.next;
    }

    chk_on_blk_list(blok, old_free_list);
    blok->h.first_avail = (char *) (blok + 1);
    debug_fill(blok->h.first_avail, blok->h.endp - blok->h.first_avail);
#ifdef POOL_DEBUG
    blok->h.owning_pool = FREE_POOL;
#endif /* POOL_DEBUG */

    /* Finally, reset next pointer to get the old free blocks back */

    blok->h.next = old_free_list;

#ifdef ALLOC_STATS
    if (num_blocks > max_blocks_in_one_free) {
	max_blocks_in_one_free = num_blocks;
    }
    ++num_free_blocks_calls;
    num_blocks_freed += num_blocks;
#endif /* ALLOC_STATS */

    (void) ap_release_mutex(alloc_mutex);
#endif /* ALLOC_USE_MALLOC */
}

/*
 * Get a new block, from our own free list if possible, from the system
 * if necessary.  Must be called with alarms blocked.
 */

static union block_hdr *new_block(int min_size)
{
    union block_hdr **lastptr = &block_freelist;
    union block_hdr *blok = block_freelist;

    /* First, see if we have anything of the required size
     * on the free list...
     */

    while (blok != NULL) {
	if (min_size + BLOCK_MINFREE <= blok->h.endp - blok->h.first_avail) {
	    *lastptr = blok->h.next;
	    blok->h.next = NULL;
	    debug_verify_filled(blok->h.first_avail, blok->h.endp,
				"Ouch!  Someone trounced a block "
				"on the free list!\n");
	    return blok;
	}
	else {
	    lastptr = &blok->h.next;
	    blok = blok->h.next;
	}
    }

    /* Nope. */

    min_size += BLOCK_MINFREE;
    blok = malloc_block((min_size > BLOCK_MINALLOC)
			? min_size : BLOCK_MINALLOC);
    return blok;
}


/* Accounting */

static long bytes_in_block_list(union block_hdr *blok)
{
    long size = 0;

    while (blok) {
	size += blok->h.endp - (char *) (blok + 1);
	blok = blok->h.next;
    }

    return size;
}


/*****************************************************************
 *
 * Pool internals and management...
 * NB that subprocesses are not handled by the generic cleanup code,
 * basically because we don't want cleanups for multiple subprocesses
 * to result in multiple three-second pauses.
 */

struct process_chain;
struct cleanup;

static void run_cleanups(struct cleanup *c);
static void free_proc_chain(struct process_chain *p);

static ap_pool_t *permanent_pool;

/* Each pool structure is allocated in the start of its own first block,
 * so we need to know how many bytes that is (once properly aligned...).
 * This also means that when a pool's sub-pool is destroyed, the storage
 * associated with it is *completely* gone, so we have to make sure it
 * gets taken off the parent's sub-pool list...
 */

#define POOL_HDR_CLICKS (1 + ((sizeof(struct ap_pool_t) - 1) / CLICK_SZ))
#define POOL_HDR_BYTES (POOL_HDR_CLICKS * CLICK_SZ)

API_EXPORT(ap_pool_t *) ap_make_sub_pool(ap_pool_t *p)
{
    union block_hdr *blok;
    ap_pool_t *new_pool;

    ap_block_alarms();

    (void) ap_acquire_mutex(alloc_mutex);

    blok = new_block(POOL_HDR_BYTES);
    new_pool = (ap_pool_t *) blok->h.first_avail;
    blok->h.first_avail += POOL_HDR_BYTES;
#ifdef POOL_DEBUG
    blok->h.owning_pool = new_pool;
#endif

    memset((char *) new_pool, '\0', sizeof(struct ap_pool_t));
    new_pool->free_first_avail = blok->h.first_avail;
    new_pool->first = new_pool->last = blok;

    if (p) {
	new_pool->parent = p;
	new_pool->sub_next = p->sub_pools;
	if (new_pool->sub_next) {
	    new_pool->sub_next->sub_prev = new_pool;
	}
	p->sub_pools = new_pool;
    }

    (void) ap_release_mutex(alloc_mutex);
    ap_unblock_alarms();

    return new_pool;
}

#ifdef POOL_DEBUG
static void stack_var_init(char *s)
{
    char t;

    if (s < &t) {
	stack_direction = 1; /* stack grows up */
    }
    else {
	stack_direction = -1; /* stack grows down */
    }
}
#endif

#ifdef ALLOC_STATS
static void dump_stats(void)
{
    fprintf(stderr,
	    "alloc_stats: [%d] #free_blocks %llu #blocks %llu max "
	    "%u #malloc %u #bytes %u\n",
	(int) getpid(),
	num_free_blocks_calls,
	num_blocks_freed,
	max_blocks_in_one_free,
	num_malloc_calls,
	num_malloc_bytes);
}
#endif

ap_pool_t *ap_init_alloc(void)
{
#ifdef POOL_DEBUG
    char s;

    known_stack_point = &s;
    stack_var_init(&s);
#endif
    alloc_mutex = ap_create_mutex(NULL);
    spawn_mutex = ap_create_mutex(NULL);
    permanent_pool = ap_make_sub_pool(NULL);
#ifdef ALLOC_STATS
    atexit(dump_stats);
#endif

    return permanent_pool;
}

API_EXPORT(void) ap_clear_pool(struct context_t *a)
{
    ap_block_alarms();

    (void) ap_acquire_mutex(alloc_mutex);
    while (a->pool->sub_pools) {
	ap_destroy_pool(a);
    }
    (void) ap_release_mutex(alloc_mutex);
    /*
     * Don't hold the mutex during cleanups.
     */
    run_cleanups(a->pool->cleanups);
    a->pool->cleanups = NULL;
    free_proc_chain(a->pool->subprocesses);
    a->pool->subprocesses = NULL;
    free_blocks(a->pool->first->h.next);
    a->pool->first->h.next = NULL;

    a->pool->last = a->pool->first;
    a->pool->first->h.first_avail = a->pool->free_first_avail;
    debug_fill(a->pool->first->h.first_avail,
	       a->pool->first->h.endp - a->pool->first->h.first_avail);

#ifdef ALLOC_USE_MALLOC
    {
	void *c, *n;

	for (c = a->pool->allocation_list; c; c = n) {
	    n = *(void **)c;
	    free(c);
	}
	a->pool->allocation_list = NULL;
    }
#endif

    ap_unblock_alarms();
}

API_EXPORT(void) ap_destroy_pool(struct context_t *a)
{
    ap_block_alarms();
    ap_clear_pool(a);

    (void) ap_acquire_mutex(alloc_mutex);
    if (a->pool->parent) {
	if (a->pool->parent->sub_pools == a->pool) {
	    a->pool->parent->sub_pools = a->pool->sub_next;
	}
	if (a->pool->sub_prev) {
	    a->pool->sub_prev->sub_next = a->pool->sub_next;
	}
	if (a->pool->sub_next) {
	    a->pool->sub_next->sub_prev = a->pool->sub_prev;
	}
    }
    (void) ap_release_mutex(alloc_mutex);

    free_blocks(a->pool->first);
    ap_unblock_alarms();
}

API_EXPORT(long) ap_bytes_in_pool(ap_pool_t *p)
{
    return bytes_in_block_list(p->first);
}
API_EXPORT(long) ap_bytes_in_free_blocks(void)
{
    return bytes_in_block_list(block_freelist);
}

/*****************************************************************
 * POOL_DEBUG support
 */
#ifdef POOL_DEBUG

/* the unix linker defines this symbol as the last byte + 1 of
 * the executable... so it includes TEXT, BSS, and DATA
 */
extern char _end;

/* is ptr in the range [lo,hi) */
#define is_ptr_in_range(ptr, lo, hi) \
    (((unsigned long)(ptr) - (unsigned long)(lo)) \
     < (unsigned long)(hi) - (unsigned long)(lo))

/* Find the pool that ts belongs to, return NULL if it doesn't
 * belong to any pool.
 */
API_EXPORT(ap_pool_t *) ap_find_pool(const void *ts)
{
    const char *s = ts;
    union block_hdr **pb;
    union block_hdr *b;

    /* short-circuit stuff which is in TEXT, BSS, or DATA */
    if (is_ptr_in_range(s, 0, &_end)) {
	return NULL;
    }
    /* consider stuff on the stack to also be in the NULL pool...
     * XXX: there's cases where we don't want to assume this
     */
    if ((stack_direction == -1 && is_ptr_in_range(s, &ts, known_stack_point))
	|| (stack_direction == 1
	    && is_ptr_in_range(s, known_stack_point, &ts))) {
	abort();
	return NULL;
    }
    ap_block_alarms();
    /* search the global_block_list */
    for (pb = &global_block_list; *pb; pb = &b->h.global_next) {
	b = *pb;
	if (is_ptr_in_range(s, b, b->h.endp)) {
	    if (b->h.owning_pool == FREE_POOL) {
		fprintf(stderr,
			"Ouch!  find_pool() called on pointer "
			"in a free block\n");
		abort();
		exit(1);
	    }
	    if (b != global_block_list) {
		/*
		 * promote b to front of list, this is a hack to speed
		 * up the lookup
		 */
		*pb = b->h.global_next;
		b->h.global_next = global_block_list;
		global_block_list = b;
	    }
	    ap_unblock_alarms();
	    return b->h.owning_pool;
	}
    }
    ap_unblock_alarms();
    return NULL;
}

/* return TRUE iff a is an ancestor of b
 * NULL is considered an ancestor of all pools
 */
API_EXPORT(int) ap_pool_is_ancestor(ap_pool_t *a, ap_pool_t *b)
{
    if (a == NULL) {
	return 1;
    }
    while (a->joined) {
	a = a->joined;
    }
    while (b) {
	if (a == b) {
	    return 1;
	}
	b = b->parent;
    }
    return 0;
}

/*
 * All blocks belonging to sub will be changed to point to p
 * instead.  This is a guarantee by the caller that sub will not
 * be destroyed before p is.
 */
API_EXPORT(void) ap_pool_join(ap_pool_t *p, ap_pool_t *sub)
{
    union block_hdr *b;

    /* We could handle more general cases... but this is it for now. */
    if (sub->parent != p) {
	fprintf(stderr, "pool_join: p is not parent of sub\n");
	abort();
    }
    ap_block_alarms();
    while (p->joined) {
	p = p->joined;
    }
    sub->joined = p;
    for (b = global_block_list; b; b = b->h.global_next) {
	if (b->h.owning_pool == sub) {
	    b->h.owning_pool = p;
	}
    }
    ap_unblock_alarms();
}
#endif

/*****************************************************************
 *
 * Allocating stuff...
 */


API_EXPORT(void *) ap_palloc(struct context_t *c, int reqsize)
{
#ifdef ALLOC_USE_MALLOC
    ap_pool_t *a = c->pool;
    int size = reqsize + CLICK_SZ;
    void *ptr;

    ap_block_alarms();
    ptr = malloc(size);
    if (ptr == NULL) {
	fputs("Ouch!  Out of memory!\n", stderr);
	exit(1);
    }
    debug_fill(ptr, size); /* might as well get uninitialized protection */
    *(void **)ptr = a->allocation_list;
    a->allocation_list = ptr;
    ap_unblock_alarms();
    return (char *)ptr + CLICK_SZ;
#else

    /*
     * Round up requested size to an even number of alignment units
     * (core clicks)
     */
    ap_pool_t *a = c->pool;
    int nclicks = 1 + ((reqsize - 1) / CLICK_SZ);
    int size = nclicks * CLICK_SZ;

    /* First, see if we have space in the block most recently
     * allocated to this pool
     */

    union block_hdr *blok = a->last;
    char *first_avail = blok->h.first_avail;
    char *new_first_avail;

    if (reqsize <= 0) {
	return NULL;
    }

    new_first_avail = first_avail + size;

    if (new_first_avail <= blok->h.endp) {
	debug_verify_filled(first_avail, blok->h.endp,
			    "Ouch!  Someone trounced past the end "
			    "of their allocation!\n");
	blok->h.first_avail = new_first_avail;
	return (void *) first_avail;
    }

    /* Nope --- get a new one that's guaranteed to be big enough */

    ap_block_alarms();

    (void) ap_acquire_mutex(alloc_mutex);

    blok = new_block(size);
    a->last->h.next = blok;
    a->last = blok;
#ifdef POOL_DEBUG
    blok->h.owning_pool = a;
#endif

    (void) ap_release_mutex(alloc_mutex);

    ap_unblock_alarms();

    first_avail = blok->h.first_avail;
    blok->h.first_avail += size;

    return (void *) first_avail;
#endif
}

API_EXPORT(void *) ap_pcalloc(struct context_t *a, int size)
{
    void *res = ap_palloc(a, size);
    memset(res, '\0', size);
    return res;
}

API_EXPORT(char *) ap_pstrdup(struct context_t *a, const char *s)
{
    char *res;
    size_t len;

    if (s == NULL) {
	return NULL;
    }
    len = strlen(s) + 1;
    res = ap_palloc(a, len);
    memcpy(res, s, len);
    return res;
}

API_EXPORT(char *) ap_pstrndup(struct context_t *a, const char *s, int n)
{
    char *res;

    if (s == NULL) {
	return NULL;
    }
    res = ap_palloc(a, n + 1);
    memcpy(res, s, n);
    res[n] = '\0';
    return res;
}

API_EXPORT_NONSTD(char *) ap_pstrcat(struct context_t *a, ...)
{
    char *cp, *argp, *res;

    /* Pass one --- find length of required string */

    int len = 0;
    va_list adummy;

    va_start(adummy, a);

    while ((cp = va_arg(adummy, char *)) != NULL) {
	len += strlen(cp);
    }

    va_end(adummy);

    /* Allocate the required string */

    res = (char *) ap_palloc(a, len + 1);
    cp = res;
    *cp = '\0';

    /* Pass two --- copy the argument strings into the result space */

    va_start(adummy, a);

    while ((argp = va_arg(adummy, char *)) != NULL) {
	strcpy(cp, argp);
	cp += strlen(argp);
    }

    va_end(adummy);

    /* Return the result string */

    return res;
}

/*
 * ap_psprintf is implemented by writing directly into the current
 * block of the pool, starting right at first_avail.  If there's
 * insufficient room, then a new block is allocated and the earlier
 * output is copied over.  The new block isn't linked into the pool
 * until all the output is done.
 *
 * Note that this is completely safe because nothing else can
 * allocate in this pool while ap_psprintf is running.  alarms are
 * blocked, and the only thing outside of alloc.c that's invoked
 * is ap_vformatter -- which was purposefully written to be
 * self-contained with no callouts.
 */

struct psprintf_data {
    ap_vformatter_buff_t vbuff;
#ifdef ALLOC_USE_MALLOC
    char *base;
#else
    union block_hdr *blok;
    int got_a_new_block;
#endif
};

static int psprintf_flush(ap_vformatter_buff_t *vbuff)
{
    struct psprintf_data *ps = (struct psprintf_data *)vbuff;
#ifdef ALLOC_USE_MALLOC
    int size;
    char *ptr;

    size = (char *)ps->vbuff.curpos - ps->base;
    ptr = realloc(ps->base, 2*size);
    if (ptr == NULL) {
	fputs("Ouch!  Out of memory!\n", stderr);
	exit(1);
    }
    ps->base = ptr;
    ps->vbuff.curpos = ptr + size;
    ps->vbuff.endpos = ptr + 2*size - 1;
    return 0;
#else
    union block_hdr *blok;
    union block_hdr *nblok;
    size_t cur_len;
    char *strp;

    blok = ps->blok;
    strp = ps->vbuff.curpos;
    cur_len = strp - blok->h.first_avail;

    /* must try another blok */
    (void) ap_acquire_mutex(alloc_mutex);
    nblok = new_block(2 * cur_len);
    (void) ap_release_mutex(alloc_mutex);
    memcpy(nblok->h.first_avail, blok->h.first_avail, cur_len);
    ps->vbuff.curpos = nblok->h.first_avail + cur_len;
    /* save a byte for the NUL terminator */
    ps->vbuff.endpos = nblok->h.endp - 1;

    /* did we allocate the current blok? if so free it up */
    if (ps->got_a_new_block) {
	debug_fill(blok->h.first_avail, blok->h.endp - blok->h.first_avail);
	(void) ap_acquire_mutex(alloc_mutex);
	blok->h.next = block_freelist;
	block_freelist = blok;
	(void) ap_release_mutex(alloc_mutex);
    }
    ps->blok = nblok;
    ps->got_a_new_block = 1;
    /* note that we've deliberately not linked the new block onto
     * the pool yet... because we may need to flush again later, and
     * we'd have to spend more effort trying to unlink the block.
     */
    return 0;
#endif
}

API_EXPORT(char *) ap_pvsprintf(struct context_t *c, const char *fmt, va_list ap)
{
#ifdef ALLOC_USE_MALLOC
    ap_pool_t *p = c->pool;
    struct psprintf_data ps;
    void *ptr;

    ap_block_alarms();
    ps.base = malloc(512);
    if (ps.base == NULL) {
	fputs("Ouch!  Out of memory!\n", stderr);
	exit(1);
    }
    /* need room at beginning for allocation_list */
    ps.vbuff.curpos = ps.base + CLICK_SZ;
    ps.vbuff.endpos = ps.base + 511;
    ap_vformatter(psprintf_flush, &ps.vbuff, fmt, ap);
    *ps.vbuff.curpos++ = '\0';
    ptr = ps.base;
    /* shrink */
    ptr = realloc(ptr, (char *)ps.vbuff.curpos - (char *)ptr);
    if (ptr == NULL) {
	fputs("Ouch!  Out of memory!\n", stderr);
	exit(1);
    }
    *(void **)ptr = p->allocation_list;
    p->allocation_list = ptr;
    ap_unblock_alarms();
    return (char *)ptr + CLICK_SZ;
#else
    struct psprintf_data ps;
    char *strp;
    int size;
    ap_pool_t *p = c->pool;

    ap_block_alarms();
    ps.blok = p->last;
    ps.vbuff.curpos = ps.blok->h.first_avail;
    ps.vbuff.endpos = ps.blok->h.endp - 1;	/* save one for NUL */
    ps.got_a_new_block = 0;

    ap_vformatter(psprintf_flush, &ps.vbuff, fmt, ap);

    strp = ps.vbuff.curpos;
    *strp++ = '\0';

    size = strp - ps.blok->h.first_avail;
    size = (1 + ((size - 1) / CLICK_SZ)) * CLICK_SZ;
    strp = ps.blok->h.first_avail;	/* save away result pointer */
    ps.blok->h.first_avail += size;

    /* have to link the block in if it's a new one */
    if (ps.got_a_new_block) {
	p->last->h.next = ps.blok;
	p->last = ps.blok;
#ifdef POOL_DEBUG
	ps.blok->h.owning_pool = p;
#endif
    }
    ap_unblock_alarms();

    return strp;
#endif
}

API_EXPORT_NONSTD(char *) ap_psprintf(struct context_t *p, const char *fmt, ...)
{
    va_list ap;
    char *res;

    va_start(ap, fmt);
    res = ap_pvsprintf(p, fmt, ap);
    va_end(ap);
    return res;
}

/*****************************************************************
 *
 * Managing generic cleanups.  
 */

struct cleanup {
    void *data;
    ap_status_t (*plain_cleanup) (void *);
    ap_status_t (*child_cleanup) (void *);
    struct cleanup *next;
};

API_EXPORT(void) ap_register_cleanup(struct context_t *p, void *data,
				      ap_status_t (*plain_cleanup) (void *),
				      ap_status_t (*child_cleanup) (void *))
{
    struct cleanup *c;
    c = (struct cleanup *) ap_palloc(p, sizeof(struct cleanup));
    c->data = data;
    c->plain_cleanup = plain_cleanup;
    c->child_cleanup = child_cleanup;
    c->next = p->pool->cleanups;
    p->pool->cleanups = c;
}

API_EXPORT(void) ap_kill_cleanup(struct context_t *p, void *data,
				  ap_status_t (*cleanup) (void *))
{
    struct cleanup *c = p->pool->cleanups;
    struct cleanup **lastp = &p->pool->cleanups;

    while (c) {
	if (c->data == data && c->plain_cleanup == cleanup) {
	    *lastp = c->next;
	    break;
	}

	lastp = &c->next;
	c = c->next;
    }
}

API_EXPORT(void) ap_run_cleanup(struct context_t *p, void *data,
				 ap_status_t (*cleanup) (void *))
{
    ap_block_alarms();		/* Run cleanup only once! */
    (*cleanup) (data);
    ap_kill_cleanup(p, data, cleanup);
    ap_unblock_alarms();
}

static void run_cleanups(struct cleanup *c)
{
    while (c) {
	(*c->plain_cleanup) (c->data);
	c = c->next;
    }
}

static void run_child_cleanups(struct cleanup *c)
{
    while (c) {
	(*c->child_cleanup) (c->data);
	c = c->next;
    }
}

static void cleanup_pool_for_exec(ap_pool_t *p)
{
    run_child_cleanups(p->cleanups);
    p->cleanups = NULL;

    for (p = p->sub_pools; p; p = p->sub_next) {
	cleanup_pool_for_exec(p);
    }
}

API_EXPORT(void) ap_cleanup_for_exec(void)
{
#if !defined(WIN32) && !defined(OS2)
    /*
     * Don't need to do anything on NT or OS/2, because I
     * am actually going to spawn the new process - not
     * exec it. All handles that are not inheritable, will
     * be automajically closed. The only problem is with
     * file handles that are open, but there isn't much
     * I can do about that (except if the child decides
     * to go out and close them
     */
    ap_block_alarms();
    cleanup_pool_for_exec(permanent_pool);
    ap_unblock_alarms();
#endif /* ndef WIN32 */
}

API_EXPORT_NONSTD(void) ap_null_cleanup(void *data)
{
    /* do nothing cleanup routine */
}

/*****************************************************************
 *
 * Files and file descriptors; these are just an application of the
 * generic cleanup interface.
 */
#if 0 /*not really needed any more, apr takes care of this stuff */
static void fd_cleanup(void *fdv)
{
    close((int) (long) fdv);
}

API_EXPORT(void) ap_note_cleanups_for_fd(ap_pool_t *p, int fd)
{
    ap_register_cleanup(p, (void *) (long) fd, fd_cleanup, fd_cleanup);
}

API_EXPORT(void) ap_kill_cleanups_for_fd(ap_pool_t *p, int fd)
{
    ap_kill_cleanup(p, (void *) (long) fd, fd_cleanup);
}

API_EXPORT(int) ap_popenf(ap_pool_t *a, const char *name, int flg, int mode)
{
    int fd;
    int save_errno;

    ap_block_alarms();
    fd = open(name, flg, mode);
    save_errno = errno;
    if (fd >= 0) {
	fd = ap_slack(fd, ap_SLACK_HIGH);
	ap_note_cleanups_for_fd(a, fd);
    }
    ap_unblock_alarms();
    errno = save_errno;
    return fd;
}

API_EXPORT(int) ap_pclosef(ap_pool_t *a, int fd)
{
    int res;
    int save_errno;

    ap_block_alarms();
    res = close(fd);
    save_errno = errno;
    ap_kill_cleanup(a, (void *) (long) fd, fd_cleanup);
    ap_unblock_alarms();
    errno = save_errno;
    return res;
}

#ifdef WIN32
static void h_cleanup(void *fdv)
{
    CloseHandle((HANDLE) fdv);
}

API_EXPORT(void) ap_note_cleanups_for_h(ap_pool_t *p, HANDLE hDevice)
{
    ap_register_cleanup(p, (void *) hDevice, h_cleanup, h_cleanup);
}

API_EXPORT(int) ap_pcloseh(ap_pool_t *a, HANDLE hDevice)
{
    int res=0;
    int save_errno;

    ap_block_alarms();
    
    if (!CloseHandle(hDevice)) {
        res = GetLastError();
    }
    
    save_errno = errno;
    ap_kill_cleanup(a, (void *) hDevice, h_cleanup);
    ap_unblock_alarms();
    errno = save_errno;
    return res;
}
#endif
*/
/* Note that we have separate plain_ and child_ cleanups for FILE *s,
 * since fclose() would flush I/O buffers, which is extremely undesirable;
 * we just close the descriptor.
 */
static void file_cleanup(void *fpv)
{
    fclose((FILE *) fpv);
}
static void file_child_cleanup(void *fpv)
{
    close(fileno((FILE *) fpv));
}

API_EXPORT(void) ap_note_cleanups_for_file(ap_pool_t *p, FILE *fp)
{
    ap_register_cleanup(p, (void *) fp, file_cleanup, file_child_cleanup);
}

API_EXPORT(FILE *) ap_pfopen(ap_pool_t *a, const char *name,
			      const char *mode)
{
    FILE *fd = NULL;
    int baseFlag, desc;
    int modeFlags = 0;
    int saved_errno;

#ifdef WIN32
    modeFlags = _S_IREAD | _S_IWRITE;
#else 
    modeFlags = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
#endif

    ap_block_alarms();

    if (*mode == 'a') {
	/* Work around faulty implementations of fopen */ 
	baseFlag = (*(mode + 1) == '+') ? O_RDWR : O_WRONLY;
	desc = open(name, baseFlag | O_APPEND | O_CREAT,
		    modeFlags);
	if (desc >= 0) {
	    desc = ap_slack(desc, ap_SLACK_LOW);
	    fd = ap_fdopen(desc, mode);
	}
    }
    else {
	fd = fopen(name, mode);
    }
    saved_errno = errno;
    if (fd != NULL) {
	ap_note_cleanups_for_file(a, fd);
    }
    ap_unblock_alarms();
    errno = saved_errno;
    return fd;
}

API_EXPORT(FILE *) ap_pfdopen(ap_pool_t *a, int fd, const char *mode)
{
    FILE *f;
    int saved_errno;

    ap_block_alarms();
    f = ap_fdopen(fd, mode);
    saved_errno = errno;
    if (f != NULL) {
	ap_note_cleanups_for_file(a, f);
    }
    ap_unblock_alarms();
    errno = saved_errno;
    return f;
}


API_EXPORT(int) ap_pfclose(ap_pool_t *a, FILE *fd)
{
    int res;

    ap_block_alarms();
    res = fclose(fd);
    ap_kill_cleanup(a, (void *) fd, file_cleanup);
    ap_unblock_alarms();
    return res;
}

/*
 * DIR * with cleanup
 */

static void dir_cleanup(void *dv)
{
    closedir((DIR *) dv);
}

API_EXPORT(DIR *) ap_popendir(ap_pool_t *p, const char *name)
{
    DIR *d;
    int save_errno;

    ap_block_alarms();
    d = opendir(name);
    if (d == NULL) {
	save_errno = errno;
	ap_unblock_alarms();
	errno = save_errno;
	return NULL;
    }
    ap_register_cleanup(p, (void *) d, dir_cleanup, dir_cleanup);
    ap_unblock_alarms();
    return d;
}

API_EXPORT(void) ap_pclosedir(ap_pool_t *p, DIR * d)
{
    ap_block_alarms();
    ap_kill_cleanup(p, (void *) d, dir_cleanup);
    closedir(d);
    ap_unblock_alarms();
}

/*****************************************************************
 *
 * Files and file descriptors; these are just an application of the
 * generic cleanup interface.
 */

static void socket_cleanup(void *fdv)
{
    closesocket((int) (long) fdv);
}

API_EXPORT(void) ap_note_cleanups_for_socket(ap_pool_t *p, int fd)
{
    ap_register_cleanup(p, (void *) (long) fd, socket_cleanup,
			 socket_cleanup);
}

API_EXPORT(void) ap_kill_cleanups_for_socket(ap_pool_t *p, int sock)
{
    ap_kill_cleanup(p, (void *) (long) sock, socket_cleanup);
}

API_EXPORT(int) ap_psocket(ap_pool_t *p, int domain, int type, int protocol)
{
    int fd;

    ap_block_alarms();
    fd = socket(domain, type, protocol);
    if (fd == -1) {
	int save_errno = errno;
	ap_unblock_alarms();
	errno = save_errno;
	return -1;
    }
    ap_note_cleanups_for_socket(p, fd);
    ap_unblock_alarms();
    return fd;
}

API_EXPORT(int) ap_pclosesocket(ap_pool_t *a, int sock)
{
    int res;
    int save_errno;

    ap_block_alarms();
    res = closesocket(sock);
#ifdef WIN32
    errno = WSAGetLastError();
#endif 
    save_errno = errno;
    ap_kill_cleanup(a, (void *) (long) sock, socket_cleanup);
    ap_unblock_alarms();
    errno = save_errno;
    return res;
}


/*
 * Here's a pool-based interface to POSIX regex's regcomp().
 * Note that we return regex_t instead of being passed one.
 * The reason is that if you use an already-used regex_t structure,
 * the memory that you've already allocated gets forgotten, and
 * regfree() doesn't clear it. So we don't allow it.
 */

static void regex_cleanup(void *preg)
{
    regfree((regex_t *) preg);
}

API_EXPORT(regex_t *) ap_pregcomp(ap_pool_t *p, const char *pattern,
				   int cflags)
{
    regex_t *preg = ap_palloc(p, sizeof(regex_t));

    if (regcomp(preg, pattern, cflags)) {
	return NULL;
    }

    ap_register_cleanup(p, (void *) preg, regex_cleanup, regex_cleanup);

    return preg;
}


API_EXPORT(void) ap_pregfree(ap_pool_t *p, regex_t * reg)
{
    ap_block_alarms();
    regfree(reg);
    ap_kill_cleanup(p, (void *) reg, regex_cleanup);
    ap_unblock_alarms();
}
#endif /* if 0 not really needed anymore.  APR takes care of this. */
/*****************************************************************
 *
 * More grotty system stuff... subprocesses.  Frump.  These don't use
 * the generic cleanup interface because I don't want multiple
 * subprocesses to result in multiple three-second pauses; the
 * subprocesses have to be "freed" all at once.  If someone comes
 * along with another resource they want to allocate which has the
 * same property, we might want to fold support for that into the
 * generic interface, but for now, it's a special case
 */

API_EXPORT(void) ap_note_subprocess(struct context_t *a, pid_t pid,
				     enum kill_conditions how)
{
    struct process_chain *new =
    (struct process_chain *) ap_palloc(a, sizeof(struct process_chain));

    new->pid = pid;
    new->kill_how = how;
    new->next = a->pool->subprocesses;
    a->pool->subprocesses = new;
}

#ifdef WIN32
#define os_pipe(fds) _pipe(fds, 512, O_BINARY | O_NOINHERIT)
#else
#define os_pipe(fds) pipe(fds)
#endif /* WIN32 */

/* for ap_fdopen, to get binary mode */
#if defined (OS2) || defined (WIN32)
#define BINMODE	"b"
#else
#define BINMODE
#endif

#if 0
static pid_t spawn_child_core(ap_pool_t *p,
			      int (*func) (void *, ap_child_info_t *),
			      void *data,enum kill_conditions kill_how,
			      int *pipe_in, int *pipe_out, int *pipe_err)
{
    pid_t pid;
    int in_fds[2];
    int out_fds[2];
    int err_fds[2];
    int save_errno;

    if (pipe_in && os_pipe(in_fds) < 0) {
	return 0;
    }

    if (pipe_out && os_pipe(out_fds) < 0) {
	save_errno = errno;
	if (pipe_in) {
	    close(in_fds[0]);
	    close(in_fds[1]);
	}
	errno = save_errno;
	return 0;
    }

    if (pipe_err && os_pipe(err_fds) < 0) {
	save_errno = errno;
	if (pipe_in) {
	    close(in_fds[0]);
	    close(in_fds[1]);
	}
	if (pipe_out) {
	    close(out_fds[0]);
	    close(out_fds[1]);
	}
	errno = save_errno;
	return 0;
    }

#ifdef WIN32

    {
	HANDLE thread_handle;
	int hStdIn, hStdOut, hStdErr;
	int old_priority;
	ap_child_info_t info;

	(void) ap_acquire_mutex(spawn_mutex);
	thread_handle = GetCurrentThread();	/* doesn't need to be closed */
	old_priority = GetThreadPriority(thread_handle);
	SetThreadPriority(thread_handle, THREAD_PRIORITY_HIGHEST);
	/* Now do the right thing with your pipes */
	if (pipe_in) {
	    hStdIn = dup(fileno(stdin));
	    if (dup2(in_fds[0], fileno(stdin))) {
		ap_log_error(APLOG_MARK, APLOG_ERR, NULL,
			      "dup2(stdin) failed");
	    }
	    close(in_fds[0]);
	}
	if (pipe_out) {
	    hStdOut = dup(fileno(stdout));
	    close(fileno(stdout));
	    if (dup2(out_fds[1], fileno(stdout))) {
		ap_log_error(APLOG_MARK, APLOG_ERR, NULL,
			      "dup2(stdout) failed");
	    }
	    close(out_fds[1]);
	}
	if (pipe_err) {
	    hStdErr = dup(fileno(stderr));
	    if (dup2(err_fds[1], fileno(stderr))) {
		ap_log_error(APLOG_MARK, APLOG_ERR, NULL,
			      "dup2(stdin) failed");
	    }
	    close(err_fds[1]);
	}

	info.hPipeInputRead   = GetStdHandle(STD_INPUT_HANDLE);
	info.hPipeOutputWrite = GetStdHandle(STD_OUTPUT_HANDLE);
	info.hPipeErrorWrite  = GetStdHandle(STD_ERROR_HANDLE);

	pid = (*func) (data, &info);
        if (pid == -1) {
	    pid = 0;   /* map Win32 error code onto Unix default */
	}

        if (!pid) {
	    save_errno = errno;
	    close(in_fds[1]);
	    close(out_fds[0]);
	    close(err_fds[0]);
	}

	/* restore the original stdin, stdout and stderr */
	if (pipe_in) {
	    dup2(hStdIn, fileno(stdin));
	    close(hStdIn);
        }
	if (pipe_out) {
	    dup2(hStdOut, fileno(stdout));
	    close(hStdOut);
	}
	if (pipe_err) {
	    dup2(hStdErr, fileno(stderr));
	    close(hStdErr);
	}

        if (pid) {
	    ap_note_subprocess(p, pid, kill_how);
	    if (pipe_in) {
		*pipe_in = in_fds[1];
	    }
	    if (pipe_out) {
		*pipe_out = out_fds[0];
	    }
	    if (pipe_err) {
		*pipe_err = err_fds[0];
	    }
	}
	SetThreadPriority(thread_handle, old_priority);
	(void) ap_release_mutex(spawn_mutex);
	/*
	 * go on to the end of the function, where you can
	 * unblock alarms and return the pid
	 */

    }
#elif defined(OS2)
    {
        int save_in=-1, save_out=-1, save_err=-1;
        
        if (pipe_out) {
            save_out = dup(STDOUT_FILENO);
            dup2(out_fds[1], STDOUT_FILENO);
            close(out_fds[1]);
        }

        if (pipe_in) {
            save_in = dup(STDIN_FILENO);
            dup2(in_fds[0], STDIN_FILENO);
            close(in_fds[0]);
        }

        if (pipe_err) {
            save_err = dup(STDERR_FILENO);
            dup2(err_fds[1], STDERR_FILENO);
            close(err_fds[1]);
        }
    
        pid = func(data, NULL);
    
        if (pid) {
            ap_note_subprocess(p, pid, kill_how);
	}

        if (pipe_out) {
            close(STDOUT_FILENO);
            dup2(save_out, STDOUT_FILENO);
            close(save_out);
            *pipe_out = out_fds[0];
        }

        if (pipe_in) {
            close(STDIN_FILENO);
            dup2(save_in, STDIN_FILENO);
            close(save_in);
            *pipe_in = in_fds[1];
        }

        if (pipe_err) {
            close(STDERR_FILENO);
            dup2(save_err, STDERR_FILENO);
            close(save_err);
            *pipe_err = err_fds[0];
        }
    }
#elif defined(TPF)
   return (pid = ap_tpf_spawn_child(p, func, data, kill_how,	
				     pipe_in, pipe_out, pipe_err, out_fds,
				     in_fds, err_fds));
#else

    if ((pid = fork()) < 0) {
	save_errno = errno;
	if (pipe_in) {
	    close(in_fds[0]);
	    close(in_fds[1]);
	}
	if (pipe_out) {
	    close(out_fds[0]);
	    close(out_fds[1]);
	}
	if (pipe_err) {
	    close(err_fds[0]);
	    close(err_fds[1]);
	}
	errno = save_errno;
	return 0;
    }

    if (!pid) {
	/* Child process */
	RAISE_SIGSTOP(SPAWN_CHILD);

	if (pipe_out) {
	    close(out_fds[0]);
	    dup2(out_fds[1], STDOUT_FILENO);
	    close(out_fds[1]);
	}

	if (pipe_in) {
	    close(in_fds[1]);
	    dup2(in_fds[0], STDIN_FILENO);
	    close(in_fds[0]);
	}

	if (pipe_err) {
	    close(err_fds[0]);
	    dup2(err_fds[1], STDERR_FILENO);
	    close(err_fds[1]);
	}

	/*
	 * HP-UX SIGCHLD fix goes here, if someone will remind me
	 * what it is...
	 */
	signal(SIGCHLD, SIG_DFL);	/* Was that it? */

	func(data, NULL);
	exit(1);	/* Should only get here if the exec in func() failed */
    }

    /* Parent process */

    ap_note_subprocess(p, pid, kill_how);

    if (pipe_out) {
	close(out_fds[1]);
	*pipe_out = out_fds[0];
    }

    if (pipe_in) {
	close(in_fds[0]);
	*pipe_in = in_fds[1];
    }

    if (pipe_err) {
	close(err_fds[1]);
	*pipe_err = err_fds[0];
    }
#endif /* WIN32 */

    return pid;
}


API_EXPORT(int) ap_spawn_child(ap_pool_t *p,
				int (*func) (void *v, ap_child_info_t *c),
				void *data, enum kill_conditions kill_how,
				FILE **pipe_in, FILE **pipe_out,
				FILE **pipe_err)
{
    int fd_in, fd_out, fd_err;
    pid_t pid;
    int save_errno;

    ap_block_alarms();

    pid = spawn_child_core(p, func, data, kill_how,
			   pipe_in ? &fd_in : NULL,
			   pipe_out ? &fd_out : NULL,
			   pipe_err ? &fd_err : NULL);

    if (pid == 0) {
	save_errno = errno;
	ap_unblock_alarms();
	errno = save_errno;
	return 0;
    }

    if (pipe_out) {
	*pipe_out = ap_fdopen(fd_out, "r" BINMODE);
	if (*pipe_out) {
	    ap_note_cleanups_for_file(p, *pipe_out);
	}
	else {
	    close(fd_out);
	}
    }

    if (pipe_in) {
	*pipe_in = ap_fdopen(fd_in, "w" BINMODE);
	if (*pipe_in) {
	    ap_note_cleanups_for_file(p, *pipe_in);
	}
	else {
	    close(fd_in);
	}
    }

    if (pipe_err) {
	*pipe_err = ap_fdopen(fd_err, "r" BINMODE);
	if (*pipe_err) {
	    ap_note_cleanups_for_file(p, *pipe_err);
	}
	else {
	    close(fd_err);
	}
    }

    ap_unblock_alarms();
    return pid;
}

API_EXPORT(int) ap_bspawn_child(ap_pool_t *p,
				 int (*func) (void *v, ap_child_info_t *c),
				 void *data, enum kill_conditions kill_how,
				 BUFF **pipe_in, BUFF **pipe_out,
				 BUFF **pipe_err)
{
#ifdef WIN32
    SECURITY_ATTRIBUTES sa = {0};  
    HANDLE hPipeOutputRead  = NULL;
    HANDLE hPipeOutputWrite = NULL;
    HANDLE hPipeInputRead   = NULL;
    HANDLE hPipeInputWrite  = NULL;
    HANDLE hPipeErrorRead   = NULL;
    HANDLE hPipeErrorWrite  = NULL;
    HANDLE hPipeInputWriteDup = NULL;
    HANDLE hPipeOutputReadDup = NULL;
    HANDLE hPipeErrorReadDup  = NULL;
    HANDLE hCurrentProcess;
    pid_t pid = 0;
    ap_child_info_t info;

    ap_block_alarms();

    /*
     * First thing to do is to create the pipes that we will use
     * for stdin, stdout, and stderr in the child process.
     */      
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;


    /* Create pipes for standard input/output/error redirection. */
    if (pipe_in && !CreatePipe(&hPipeInputRead, &hPipeInputWrite, &sa, 0)) {
	return 0;
    }

    if (pipe_out && !CreatePipe(&hPipeOutputRead, &hPipeOutputWrite, &sa, 0)) {
	if (pipe_in) {
	    CloseHandle(hPipeInputRead);
	    CloseHandle(hPipeInputWrite);
	}
	return 0;
    }

    if (pipe_err && !CreatePipe(&hPipeErrorRead, &hPipeErrorWrite, &sa, 0)) {
	if (pipe_in) {
	    CloseHandle(hPipeInputRead);
	    CloseHandle(hPipeInputWrite);
	}
	if (pipe_out) {
	    CloseHandle(hPipeOutputRead);
	    CloseHandle(hPipeOutputWrite);
	}
	return 0;
    }
    /*
     * When the pipe handles are created, the security descriptor
     * indicates that the handle can be inherited.  However, we do not
     * want the server side handles to the pipe to be inherited by the
     * child CGI process. If the child CGI does inherit the server
     * side handles, then the child may be left around if the server
     * closes its handles (e.g. if the http connection is aborted),
     * because the child will have a valid copy of handles to both
     * sides of the pipes, and no I/O error will occur.  Microsoft
     * recommends using DuplicateHandle to turn off the inherit bit
     * under NT and Win95.
     */
    hCurrentProcess = GetCurrentProcess();
    if ((pipe_in && !DuplicateHandle(hCurrentProcess, hPipeInputWrite,
				     hCurrentProcess,
				     &hPipeInputWriteDup, 0, FALSE,
				     DUPLICATE_SAME_ACCESS))
	|| (pipe_out && !DuplicateHandle(hCurrentProcess, hPipeOutputRead,
					 hCurrentProcess, &hPipeOutputReadDup,
					 0, FALSE, DUPLICATE_SAME_ACCESS))
	|| (pipe_err && !DuplicateHandle(hCurrentProcess, hPipeErrorRead,
					 hCurrentProcess, &hPipeErrorReadDup,
					 0, FALSE, DUPLICATE_SAME_ACCESS))) {
	if (pipe_in) {
	    CloseHandle(hPipeInputRead);
	    CloseHandle(hPipeInputWrite);
	}
	if (pipe_out) {
	    CloseHandle(hPipeOutputRead);
	    CloseHandle(hPipeOutputWrite);
	}
	if (pipe_err) {
	    CloseHandle(hPipeErrorRead);
	    CloseHandle(hPipeErrorWrite);
	}
	return 0;
    }
    else {
	if (pipe_in) {
	    CloseHandle(hPipeInputWrite);
	    hPipeInputWrite = hPipeInputWriteDup;
	}
	if (pipe_out) {
	    CloseHandle(hPipeOutputRead);
	    hPipeOutputRead = hPipeOutputReadDup;
	}
	if (pipe_err) {
	    CloseHandle(hPipeErrorRead);
	    hPipeErrorRead = hPipeErrorReadDup;
	}
    }

    /* The script writes stdout to this pipe handle */
    info.hPipeOutputWrite = hPipeOutputWrite;  

    /* The script reads stdin from this pipe handle */
    info.hPipeInputRead = hPipeInputRead;

    /* The script writes stderr to this pipe handle */
    info.hPipeErrorWrite = hPipeErrorWrite;    
     
    /*
     *  Try to launch the CGI.  Under the covers, this call 
     *  will try to pick up the appropriate interpreter if 
     *  one is needed.
     */
    pid = func(data, &info);
    if (pid == -1) {
        /* Things didn't work, so cleanup */
        pid = 0;   /* map Win32 error code onto Unix default */
        CloseHandle(hPipeOutputRead);
        CloseHandle(hPipeInputWrite);
        CloseHandle(hPipeErrorRead);
    }
    else {
        if (pipe_out) {
            /*
             *  This pipe represents stdout for the script, 
             *  so we read from this pipe.
             */
	    /* Create a read buffer */
            *pipe_out = ap_bcreate(p, B_RD);

	    /* Setup the cleanup routine for the handle */
            ap_note_cleanups_for_h(p, hPipeOutputRead);   

	    /* Associate the handle with the new buffer */
            ap_bpushh(*pipe_out, hPipeOutputRead);
        }
        
        if (pipe_in) {
            /*
             *  This pipe represents stdin for the script, so we 
             *  write to this pipe.
             */
	    /* Create a write buffer */
            *pipe_in = ap_bcreate(p, B_WR);             

	    /* Setup the cleanup routine for the handle */
            ap_note_cleanups_for_h(p, hPipeInputWrite);

	    /* Associate the handle with the new buffer */
            ap_bpushh(*pipe_in, hPipeInputWrite);

        }
      
        if (pipe_err) {
            /*
             *  This pipe represents stderr for the script, so 
             *  we read from this pipe.
             */
	    /* Create a read buffer */
            *pipe_err = ap_bcreate(p, B_RD);

	    /* Setup the cleanup routine for the handle */
            ap_note_cleanups_for_h(p, hPipeErrorRead);

	    /* Associate the handle with the new buffer */
            ap_bpushh(*pipe_err, hPipeErrorRead);
        }
    }  


    /*
     * Now that handles have been inherited, close them to be safe.
     * You don't want to read or write to them accidentally, and we
     * sure don't want to have a handle leak.
     */
    CloseHandle(hPipeOutputWrite);
    CloseHandle(hPipeInputRead);
    CloseHandle(hPipeErrorWrite);

#else
    int fd_in, fd_out, fd_err;
    pid_t pid;
    int save_errno;

    ap_block_alarms();

    pid = spawn_child_core(p, func, data, kill_how,
			   pipe_in ? &fd_in : NULL,
			   pipe_out ? &fd_out : NULL,
			   pipe_err ? &fd_err : NULL);

    if (pid == 0) {
	save_errno = errno;
	ap_unblock_alarms();
	errno = save_errno;
	return 0;
    }

    if (pipe_out) {
	*pipe_out = ap_bcreate(p, B_RD);
	ap_note_cleanups_for_fd(p, fd_out);
	ap_bpushfd(*pipe_out, fd_out, fd_out);
    }

    if (pipe_in) {
	*pipe_in = ap_bcreate(p, B_WR);
	ap_note_cleanups_for_fd(p, fd_in);
	ap_bpushfd(*pipe_in, fd_in, fd_in);
    }

    if (pipe_err) {
	*pipe_err = ap_bcreate(p, B_RD);
	ap_note_cleanups_for_fd(p, fd_err);
	ap_bpushfd(*pipe_err, fd_err, fd_err);
    }
#endif

    ap_unblock_alarms();
    return pid;
}
#endif

static void free_proc_chain(struct process_chain *procs)
{
    /* Dispose of the subprocesses we've spawned off in the course of
     * whatever it was we're cleaning up now.  This may involve killing
     * some of them off...
     */

    struct process_chain *p;
    int need_timeout = 0;
    int status;

    if (procs == NULL) {
	return;			/* No work.  Whew! */
    }

    /* First, check to see if we need to do the SIGTERM, sleep, SIGKILL
     * dance with any of the processes we're cleaning up.  If we've got
     * any kill-on-sight subprocesses, ditch them now as well, so they
     * don't waste any more cycles doing whatever it is that they shouldn't
     * be doing anymore.
     */
#ifdef WIN32
    /* Pick up all defunct processes */
    for (p = procs; p; p = p->next) {
	if (GetExitCodeProcess((HANDLE) p->pid, &status)) {
	    p->kill_how = kill_never;
	}
    }


    for (p = procs; p; p = p->next) {
	if (p->kill_how == kill_after_timeout) {
	    need_timeout = 1;
	}
	else if (p->kill_how == kill_always) {
	    TerminateProcess((HANDLE) p->pid, 1);
	}
    }
    /* Sleep only if we have to... */

    if (need_timeout) {
	sleep(3);
    }

    /* OK, the scripts we just timed out for have had a chance to clean up
     * --- now, just get rid of them, and also clean up the system accounting
     * goop...
     */

    for (p = procs; p; p = p->next) {
	if (p->kill_how == kill_after_timeout) {
	    TerminateProcess((HANDLE) p->pid, 1);
	}
    }

    for (p = procs; p; p = p->next) {
	CloseHandle((HANDLE) p->pid);
    }
#else
#ifndef NEED_WAITPID
    /* Pick up all defunct processes */
    for (p = procs; p; p = p->next) {
	if (waitpid(p->pid, (int *) 0, WNOHANG) > 0) {
	    p->kill_how = kill_never;
	}
    }
#endif

    for (p = procs; p; p = p->next) {
	if ((p->kill_how == kill_after_timeout)
	    || (p->kill_how == kill_only_once)) {
	    /*
	     * Subprocess may be dead already.  Only need the timeout if not.
	     */
	    if (kill(p->pid, SIGTERM) != -1) {
		need_timeout = 1;
	    }
	}
	else if (p->kill_how == kill_always) {
	    kill(p->pid, SIGKILL);
	}
    }

    /* Sleep only if we have to... */

    if (need_timeout) {
	sleep(3);
    }

    /* OK, the scripts we just timed out for have had a chance to clean up
     * --- now, just get rid of them, and also clean up the system accounting
     * goop...
     */

    for (p = procs; p; p = p->next) {

	if (p->kill_how == kill_after_timeout) {
	    kill(p->pid, SIGKILL);
	}

	if (p->kill_how != kill_never) {
	    waitpid(p->pid, &status, 0);
	}
    }
#endif /* WIN32 */
}
