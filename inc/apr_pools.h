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
 * we can delete everything in the per-transaction ap_context_t without fear, and
 * without thinking too hard about it either.
 *
 * rst
 */

/* Arenas for configuration info and transaction info
 * --- actual layout of the ap_context_t structure is private to 
 * alloc.c.  
 */

 /* Need declaration of DIR on Win32 */
#ifdef WIN32
/*#include "../os/win32/readdir.h"*/
#endif
#include "apr_lib.h"

#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <signal.h>

struct process_chain {
    pid_t pid;
    enum kill_conditions kill_how;
    struct process_chain *next;
};

struct ap_pool_t {
    union block_hdr *first;
    union block_hdr *last;
    struct cleanup *cleanups;
    struct process_chain *subprocesses;
    ap_pool_t *sub_pools;
    ap_pool_t *sub_next;
    ap_pool_t *sub_prev;
    ap_pool_t *parent;
    char *free_first_avail;
#ifdef ALLOC_USE_MALLOC
    void *allocation_list;
#endif
#ifdef POOL_DEBUG
    ap_pool_t *joined;
#endif
};

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

/*
 * Tables.  Implemented alist style, for now, though we try to keep
 * it so that imposing a hash table structure on top in the future
 * wouldn't be *too* hard...
 *
 * Note that key comparisons for these are case-insensitive, largely
 * because that's what's appropriate and convenient everywhere they're
 * currently being used...
 */

ap_pool_t *ap_init_alloc(void);		/* Set up everything */

/* used to guarantee to the pool debugging code that the sub pool will not be
 * destroyed before the parent pool
 */
#ifndef POOL_DEBUG
#ifdef ap_pool_join
#undef ap_pool_join
#endif /* ap_pool_join */
#define ap_pool_join(a,b)
#endif /* POOL_DEBUG */

/* Clearing out EVERYTHING in an pool... destroys any sub-pools */

/* Preparing for exec() --- close files, etc., but *don't* flush I/O
 * buffers, *don't* wait for subprocesses, and *don't* free any memory.
 */

/* routines to allocate memory from an pool... */

API_EXPORT_NONSTD(char *) ap_psprintf(struct context_t *, const char *fmt, ...)
    __attribute__((format(printf,2,3)));

/* array and alist management... keeping lists of things.
 * Common enough to want common support code ...
 */

/* ap_array_pstrcat generates a new string from the ap_context_t containing
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

/* A set of flags which indicate places where the server should raise(SIGSTOP).
 * This is useful for debugging, because you can then attach to that process
 * with gdb and continue.  This is important in cases where one_process
 * debugging isn't possible.
 */
#define SIGSTOP_DETACH                  1
#define SIGSTOP_MAKE_CHILD              2                                       #define SIGSTOP_SPAWN_CHILD             4
#define SIGSTOP_PIPED_LOG_SPAWN         8
#define SIGSTOP_CGI_CHILD               16

#ifdef DEBUG_SIGSTOP
extern int raise_sigstop_flags;
#define RAISE_SIGSTOP(x)        do { \
        if (raise_sigstop_flags & SIGSTOP_##x) raise(SIGSTOP);\
    } while (0)
#else
#define RAISE_SIGSTOP(x)
#endif

#ifdef ULTRIX_BRAIN_DEATH
#define ap_fdopen(d,m) fdopen((d), (char *)(m))
#else
#define ap_fdopen(d,m) fdopen((d), (m))
#endif

/* XXX - the socket functions for pools should (and will) use APR sockets.
 * This is temporary. */
#ifndef BEOS /* this really screws up BeOS R4.5 !! */
#define closesocket(s) close(s)
#endif



/* XXX: these know about the definition of struct ap_table_t in alloc.c.  That
 * definition is not here because it is supposed to be private, and by not
 * placing it here we are able to get compile-time diagnostics from modules
 * written which assume that a ap_table_t is the same as an ap_array_header_t. -djg
 */
#define ap_table_elts(t) ((ap_array_header_t *)(t))
#define ap_is_empty_table(t) (((t) == NULL)||(((ap_array_header_t *)(t))->nelts == 0))

/* routines to remember allocation of other sorts of things...
 * generic interface first.  Note that we want to have two separate
 * cleanup functions in the general case, one for exec() preparation,
 * to keep CGI scripts and the like from inheriting access to things
 * they shouldn't be able to touch, and one for actually cleaning up,
 * when the actual server process wants to get rid of the thing,
 * whatever it is.  
 *
 * kill_cleanup disarms a cleanup, presumably because the resource in
 * question has been closed, freed, or whatever, and it's scarce
 * enough to want to reclaim (e.g., descriptors).  It arranges for the
 * resource not to be cleaned up a second time (it might have been
 * reallocated).  run_cleanup does the same, but runs it first.
 *
 * Cleanups are identified for purposes of finding & running them off by the
 * plain_cleanup and data, which should presumably be unique.
 *
 * NB any code which invokes register_cleanup or kill_cleanup directly
 * is a critical section which should be guarded by block_alarms() and
 * unblock_alarms() below...
 */

/* A "do-nothing" cleanup, for register_cleanup; it's faster to do
 * things this way than to test for NULL. */

/* The time between when a resource is actually allocated, and when
 * its cleanup is registered is a critical section, during which the
 * resource could leak if we got interrupted or timed out.  So, anything
 * which registers cleanups should bracket resource allocation and the
 * cleanup registry with these.  (This is done internally by run_cleanup).
 *
 * NB they are actually implemented in http_main.c, since they are bound
 * up with timeout handling in general...
 */

/* Common cases which want utility support..
 * the note_cleanups_for_foo routines are for 
 */

API_EXPORT(FILE *) ap_pfopen(ap_context_t *, const char *name, const char *fmode);
API_EXPORT(FILE *) ap_pfdopen(ap_context_t *, int fd, const char *fmode);
API_EXPORT(int) ap_popenf(ap_context_t *, const char *name, int flg, int mode);

API_EXPORT(void) ap_note_cleanups_for_file(ap_context_t *, FILE *);
API_EXPORT(void) ap_note_cleanups_for_fd(ap_context_t *, int);
API_EXPORT(void) ap_kill_cleanups_for_fd(ap_context_t *p, int fd);
API_EXPORT(void) ap_note_cleanups_for_socket(ap_context_t *, int);
API_EXPORT(void) ap_kill_cleanups_for_socket(ap_context_t *p, int sock);
API_EXPORT(int) ap_psocket(ap_context_t *p, int, int, int);
API_EXPORT(int) ap_pclosesocket(ap_context_t *a, int sock);

/* routines to note closes... file descriptors are constrained enough
 * on some systems that we want to support this.
 */

API_EXPORT(int) ap_pfclose(ap_context_t *, FILE *);
API_EXPORT(int) ap_pclosef(ap_context_t *, int fd);

/* routines to deal with directories */
/*API_EXPORT(DIR *) ap_popendir(ap_context_t *p, const char *name);
API_EXPORT(void) ap_pclosedir(ap_context_t *p, DIR * d);
*/
/* ... even child processes (which we may want to wait for,
 * or to kill outright, on unexpected termination).
 *
 * ap_spawn_child is a utility routine which handles an awful lot of
 * the rigamarole associated with spawning a child --- it arranges
 * for pipes to the child's stdin and stdout, if desired (if not,
 * set the associated args to NULL).  It takes as args a function
 * to call in the child, and an argument to be passed to the function.
 */

API_EXPORT(void) ap_note_subprocess(struct context_t *a, pid_t pid,
				    enum kill_conditions how);

/* magic numbers --- min free bytes to consider a free ap_context_t block useable,
 * and the min amount to allocate if we have to go to malloc() */

#ifndef BLOCK_MINFREE
#define BLOCK_MINFREE 4096
#endif
#ifndef BLOCK_MINALLOC
#define BLOCK_MINALLOC 8192
#endif

/* Finally, some accounting */

API_EXPORT(long) ap_bytes_in_pool(ap_pool_t *p);
API_EXPORT(long) ap_bytes_in_free_blocks(void);

#ifdef __cplusplus
}
#endif

#endif	/* !ap_POOLS_H */
