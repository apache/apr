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

#include "apr_portable.h" /* for get_os_proc */
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_lib.h"
#include "apr_lock.h"
#include "misc.h"

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SIGNAL_H
#include <sys/signal.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

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
 * that their arguments are safe for the ap_table_t they're being placed in.
 * It currently only works with the unix multiprocess model, but could
 * be extended to others.
 */
/* #define POOL_DEBUG */

/*
 * Provide diagnostic information about make_table() calls which are
 * possibly too small.  This requires a recent gcc which supports
 * __builtin_return_address().  The error_log output will be a
 * message such as:
 *    table_push: ap_table_t created by 0x804d874 hit limit of 10
 * Use "l *0x804d874" to find the source that corresponds to.  It
 * indicates that a ap_table_t allocated by a call at that address has
 * possibly too small an initial ap_table_t size guess.
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

#define AP_SLACK_LOW    1
#define AP_SLACK_HIGH   2


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

#define APR_ABORT(conditional, retcode, func, str) \
    if (conditional) { \
        if ((func) == NULL) { \
            return NULL; \
        } \
        else { \
            fprintf(stderr, "%s", str); \
            (*(func))(retcode); \
        } \
    }

/*
 * Static cells for managing our internal synchronisation.
 */
static union block_hdr *block_freelist = NULL;

#if APR_HAS_THREADS
static ap_lock_t *alloc_mutex;
static ap_lock_t *spawn_mutex;
#endif

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

static union block_hdr *malloc_block(int size, int (*apr_abort)(int retcode))
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
    APR_ABORT(blok == NULL, APR_ENOMEM, apr_abort,
              "Ouch!  malloc failed in malloc_block()\n");
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

#if APR_HAS_THREADS
    ap_lock(alloc_mutex);
#endif
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

#if APR_HAS_THREADS
    ap_unlock(alloc_mutex);
#endif /* APR_HAS_THREADS */
#endif /* ALLOC_USE_MALLOC */
}

/*
 * Get a new block, from our own free list if possible, from the system
 * if necessary.  Must be called with alarms blocked.
 */

static union block_hdr *new_block(int min_size, int (*apr_abort)(int retcode))
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
			? min_size : BLOCK_MINALLOC, apr_abort);
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

API_EXPORT(ap_pool_t *) ap_make_sub_pool(ap_pool_t *p, int (*apr_abort)(int retcode))
{
    union block_hdr *blok;
    ap_pool_t *new_pool;

    ap_block_alarms();

#if APR_HAS_THREADS
    ap_lock(alloc_mutex);
#endif

    blok = new_block(POOL_HDR_BYTES, apr_abort);
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
    else {
        permanent_pool = new_pool;
    }

#if APR_HAS_THREADS
    ap_unlock(alloc_mutex);
#endif
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

API_EXPORT(void) ap_register_cleanup(ap_pool_t *p, void *data,
				      ap_status_t (*plain_cleanup) (void *),
				      ap_status_t (*child_cleanup) (void *))
{
    struct cleanup *c;

    if (p != NULL) {
        c = (struct cleanup *) ap_palloc(p, sizeof(struct cleanup));
        c->data = data;
        c->plain_cleanup = plain_cleanup;
        c->child_cleanup = child_cleanup;
        c->next = p->cleanups;
        p->cleanups = c;
    }
}

API_EXPORT(void) ap_kill_cleanup(ap_pool_t *p, void *data,
				  ap_status_t (*cleanup) (void *))
{
    struct cleanup *c;
    struct cleanup **lastp;

    if (p == NULL)
        return;
    c = p->cleanups;
    lastp = &p->cleanups;
    while (c) {
        if (c->data == data && c->plain_cleanup == cleanup) {
            *lastp = c->next;
            break;
        }

        lastp = &c->next;
        c = c->next;
    }
}

API_EXPORT(ap_status_t) ap_run_cleanup(ap_pool_t *p, void *data,
				 ap_status_t (*cleanup) (void *))
{
    ap_status_t rv;

    ap_block_alarms();		/* Run cleanup only once! */
    rv = (*cleanup) (data);
    ap_kill_cleanup(p, data, cleanup);
    ap_unblock_alarms();
    return rv;
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

API_EXPORT_NONSTD(ap_status_t) ap_null_cleanup(void *data)
{
    /* do nothing cleanup routine */
    return APR_SUCCESS;
}

ap_status_t ap_init_alloc(void)
{
#if APR_HAS_THREADS
    ap_status_t status;
#endif
#ifdef POOL_DEBUG
    char s;

    known_stack_point = &s;
    stack_var_init(&s);
#endif
#if APR_HAS_THREADS
    status = ap_create_lock(&alloc_mutex, APR_MUTEX, APR_INTRAPROCESS,
                   NULL, NULL);
    if (status != APR_SUCCESS) {
        ap_destroy_lock(alloc_mutex); 
        return status;
    }
    status = ap_create_lock(&spawn_mutex, APR_MUTEX, APR_INTRAPROCESS,
                   NULL, NULL);
    if (status != APR_SUCCESS) {
        ap_destroy_lock(spawn_mutex); 
        return status;
    }
#endif

#ifdef ALLOC_STATS
    atexit(dump_stats);
#endif

    return APR_SUCCESS;
}

void ap_term_alloc(void)
{
#if APR_HAS_THREADS
    ap_destroy_lock(alloc_mutex);
    ap_destroy_lock(spawn_mutex);
#endif
}

/* We only want to lock the mutex if we are being called from ap_clear_pool.
 * This is because if we also call this function from ap_destroy_real_pool,
 * which also locks the same mutex, and recursive locks aren't portable.  
 * This way, we are garaunteed that we only lock this mutex once when calling
 * either one of these functions.
 */
API_EXPORT(void) ap_clear_pool(ap_pool_t *a)
{
    ap_block_alarms();

    while (a->sub_pools) {
	ap_destroy_pool(a->sub_pools);
    }
    /*
     * Don't hold the mutex during cleanups.
     */
    run_cleanups(a->cleanups);
    a->cleanups = NULL;
    free_proc_chain(a->subprocesses);
    a->subprocesses = NULL;
    free_blocks(a->first->h.next);
    a->first->h.next = NULL;

    a->last = a->first;
    a->first->h.first_avail = a->free_first_avail;
    debug_fill(a->first->h.first_avail,
	       a->first->h.endp - a->first->h.first_avail);

#ifdef ALLOC_USE_MALLOC
    {
	void *c, *n;

	for (c = a->allocation_list; c; c = n) {
	    n = *(void **)c;
	    free(c);
	}
	a->allocation_list = NULL;
    }
#endif

    ap_unblock_alarms();
}

API_EXPORT(void) ap_destroy_pool(ap_pool_t *a)
{
    ap_block_alarms();
    ap_clear_pool(a);
#if APR_HAS_THREADS
    ap_lock(alloc_mutex);
#endif

    if (a->parent) {
	if (a->parent->sub_pools == a) {
	    a->parent->sub_pools = a->sub_next;
	}
	if (a->sub_prev) {
	    a->sub_prev->sub_next = a->sub_next;
	}
	if (a->sub_next) {
	    a->sub_next->sub_prev = a->sub_prev;
	}
    }
#if APR_HAS_THREADS
    ap_unlock(alloc_mutex);
#endif
    free_blocks(a->first);
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
API_EXPORT(ap_pool_t *) ap_find_pool(const void *ts, int (apr_abort)(int retcode))
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
    APR_ABORT((stack_direction == -1 && 
              is_ptr_in_range(s, &ts, known_stack_point)) || 
              (stack_direction == 1 &&    
              is_ptr_in_range(s, known_stack_point, &ts)), 1, apr_abort,
              "Ouch!  find_pool() called on pointer in a free block\n");
    ap_block_alarms();
    /* search the global_block_list */
    for (pb = &global_block_list; *pb; pb = &b->h.global_next) {
	b = *pb;
	if (is_ptr_in_range(s, b, b->h.endp)) {
	    if (b->h.owning_pool == FREE_POOL) {
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
API_EXPORT(void) ap_pool_join(ap_pool_t *p, ap_pool_t *sub, 
                              int (*apr_abort)(int retcode))
{
    union block_hdr *b;

    /* We could handle more general cases... but this is it for now. */
    APR_ABORT(sub->parent != p, 1, apr_abort,
              "pool_join: p is not a parent of sub\n");
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

void * ap_palloc(ap_pool_t *a, int reqsize)
{
#ifdef ALLOC_USE_MALLOC
    int size = reqsize + CLICK_SZ;
    void *ptr;

    if (a == NULL) {
        return malloc(reqsize);
    }
    ap_block_alarms();
    if (c == NULL) {
        return malloc(reqsize);
    }
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
    int nclicks;
    int size;

    /* First, see if we have space in the block most recently
     * allocated to this pool
     */

    union block_hdr *blok;
    char *first_avail;
    char *new_first_avail;

    if (a == NULL) {
        return malloc(reqsize);
    }

    nclicks = 1 + ((reqsize - 1) / CLICK_SZ);
    size = nclicks * CLICK_SZ;

    /* First, see if we have space in the block most recently
     * allocated to this pool
     */

    blok = a->last;
    first_avail = blok->h.first_avail;

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

#if APR_HAS_THREADS
    ap_lock(alloc_mutex);
#endif

    blok = new_block(size, a->apr_abort);
    a->last->h.next = blok;
    a->last = blok;
#ifdef POOL_DEBUG
    blok->h.owning_pool = a;
#endif

#if APR_HAS_THREADS
    ap_unlock(alloc_mutex);
#endif

    ap_unblock_alarms();

    first_avail = blok->h.first_avail;
    blok->h.first_avail += size;

    return (void *) first_avail;
#endif
}

API_EXPORT(void *) ap_pcalloc(ap_pool_t *a, int size)
{
    void *res = ap_palloc(a, size);
    memset(res, '\0', size);
    return res;
}

API_EXPORT(char *) ap_pstrdup(ap_pool_t *a, const char *s)
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

API_EXPORT(char *) ap_pstrndup(ap_pool_t *a, const char *s, int n)
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

API_EXPORT_NONSTD(char *) ap_pstrcat(ap_pool_t *a, ...)
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
 * allocate in this ap_pool_t while ap_psprintf is running.  alarms are
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
#if APR_HAS_THREADS
    ap_lock(alloc_mutex);
#endif
    nblok = new_block(2 * cur_len, NULL);
#if APR_HAS_THREADS
    ap_unlock(alloc_mutex);
#endif
    memcpy(nblok->h.first_avail, blok->h.first_avail, cur_len);
    ps->vbuff.curpos = nblok->h.first_avail + cur_len;
    /* save a byte for the NUL terminator */
    ps->vbuff.endpos = nblok->h.endp - 1;

    /* did we allocate the current blok? if so free it up */
    if (ps->got_a_new_block) {
	debug_fill(blok->h.first_avail, blok->h.endp - blok->h.first_avail);
#if APR_HAS_THREADS
        ap_lock(alloc_mutex);
#endif
	blok->h.next = block_freelist;
	block_freelist = blok;
#if APR_HAS_THREADS
        ap_unlock(alloc_mutex);
#endif
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

API_EXPORT(char *) ap_pvsprintf(ap_pool_t *p, const char *fmt, va_list ap)
{
#ifdef ALLOC_USE_MALLOC
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

API_EXPORT_NONSTD(char *) ap_psprintf(ap_pool_t *p, const char *fmt, ...)
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
 * More grotty system stuff... subprocesses.  Frump.  These don't use
 * the generic cleanup interface because I don't want multiple
 * subprocesses to result in multiple three-second pauses; the
 * subprocesses have to be "freed" all at once.  If someone comes
 * along with another resource they want to allocate which has the
 * same property, we might want to fold support for that into the
 * generic interface, but for now, it's a special case
 */

API_EXPORT(void) ap_note_subprocess(ap_pool_t *a, ap_proc_t *pid,
				     enum kill_conditions how)
{
    struct process_chain *new =
    (struct process_chain *) ap_palloc(a, sizeof(struct process_chain));

    new->pid = pid;
    new->kill_how = how;
    new->next = a->subprocesses;
    a->subprocesses = new;
}

static void free_proc_chain(struct process_chain *procs)
{
    /* Dispose of the subprocesses we've spawned off in the course of
     * whatever it was we're cleaning up now.  This may involve killing
     * some of them off...
     */
    struct process_chain *p;
    int need_timeout = 0;

    if (procs == NULL) {
	return;			/* No work.  Whew! */
    }

    /* First, check to see if we need to do the SIGTERM, sleep, SIGKILL
     * dance with any of the processes we're cleaning up.  If we've got
     * any kill-on-sight subprocesses, ditch them now as well, so they
     * don't waste any more cycles doing whatever it is that they shouldn't
     * be doing anymore.
     */

#ifndef NEED_WAITPID
    /* Pick up all defunct processes */
    for (p = procs; p; p = p->next) {
        if (ap_wait_proc(p->pid, APR_NOWAIT) == APR_CHILD_DONE) {
            p->kill_how = kill_never;
        }
    }
#endif

    for (p = procs; p; p = p->next) {
        if ((p->kill_how == kill_after_timeout)
            || (p->kill_how == kill_only_once)) {
            /*
             * Subprocess may be dead already.  Only need the timeout if not.
             * Note: ap_kill on Windows is TerminateProcess(), which is 
             * similar to a SIGKILL, so always give the process a timeout
             * under Windows before killing it.
             */
#ifdef WIN32
            need_timeout = 1;
#else
	    if (ap_kill(p->pid, APR_SIGTERM) == APR_SUCCESS) {
		need_timeout = 1;
	    }
#endif
	}
	else if (p->kill_how == kill_always) {
	    ap_kill(p->pid, APR_SIGKILL);
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
	    ap_kill(p->pid, APR_SIGKILL);
	}
    }
#ifdef WIN32
    /* 
     * Do we need an APR function to clean-up a proc_t?
     */
    {
        PROCESS_INFORMATION pi;
        for (p = procs; p; p = p->next) {
            ap_get_os_proc(&pi, p->pid);
            CloseHandle(pi.hProcess);
        }
    }
#endif /* WIN32 */

    /* Now wait for all the signaled processes to die */
    for (p = procs; p; p = p->next) {
	if (p->kill_how != kill_never) {
	    (void) ap_wait_proc(p->pid, APR_WAIT);
	}
    }
}
