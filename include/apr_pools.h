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

#include "apr.h"
#include "apr_thread_proc.h"

#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if APR_HAVE_STDARG_H
#include <stdarg.h>
#endif

enum kill_conditions {
    kill_never,                 /* process is never sent any signals */
    kill_always,                /* process is sent SIGKILL on ap_pool_t cleanup */
    kill_after_timeout,         /* SIGTERM, wait 3 seconds, SIGKILL */
    just_wait,                  /* wait forever for the process to complete */
    kill_only_once              /* send SIGTERM and then wait */
};

struct process_chain {
    ap_proc_t *pid;
    enum kill_conditions kill_how;
    struct process_chain *next;
};

/* used to guarantee to the ap_pool_t debugging code that the sub ap_pool_t 
 * will not be destroyed before the parent pool
 */
#ifndef POOL_DEBUG
APR_EXPORT(ap_pool_t *) ap_find_pool(const void *ts);
#else
APR_EXPORT(int) ap_pool_join(ap_pool_t *p, ap_pool_t *sub, int (*apr_abort)(int retcode));
APR_EXPORT(ap_pool_t *) ap_find_pool(const void *ts, int (*apr_abort)(int retcode));
#endif /* POOL_DEBUG */

#ifdef ULTRIX_BRAIN_DEATH
#define ap_fdopen(d,m) fdopen((d), (char *)(m))
#else
#define ap_fdopen(d,m) fdopen((d), (m))
#endif

/*
 * APR memory structure manipulators (pools, tables, and arrays).
 */

/*

=head1 ap_status_t ap_init_alloc(void)

B<Setup all of the internal structures required to use pools>

B<NOTE>:  Programs do B<NOT> need to call this directly.  APR will call this
          automatically from ap_initialize. 
=cut
 */
ap_status_t ap_init_alloc(void);	/* Set up everything */

/*

=head1 void ap_term_alloc(void)

B<Tear down all of the internal structures required to use pools>

B<NOTE>:  Programs do B<NOT> need to call this directly.  APR will call this
          automatically from ap_terminate. 

=cut
 */
void        ap_term_alloc(void);        /* Tear down everything */

/*

=head1 ap_pool_t *ap_make_sub_pool(ap_pool_t *p, int (*apr_abort)(int retcode))

B<make a sub pool from the current pool>

    arg 1) The pool to use as a parent pool
    arg 2) A function to use if the pool cannot allocate more memory.
    return) The new sub-pool

B<NOTE>:  The apr_abort function provides a way to quit the program if the
          machine is out of memory.  By default, APR will return with an
          error.

=cut
 */
APR_EXPORT(ap_pool_t *) ap_make_sub_pool(ap_pool_t *p, int (*apr_abort)(int retcode));

/*

=head1 void ap_clear_pool(ap_pool_t *p)

B<clear all memory in the pool>

    arg 1) The pool to clear

B<NOTE>:  This does not actually free the memory, it just allows the pool
          to re-use this memory for the next allocation.

=cut
 */
APR_EXPORT(void) ap_clear_pool(ap_pool_t *p);

/*

=head1 void ap_destroy_pool(ap_pool_t *p)

B<destroy the pool>

    arg 1) The pool to destroy

B<NOTE>:  This will actually free the memory

=cut
 */
APR_EXPORT(void) ap_destroy_pool(ap_pool_t *p);

/*

=head1 ap_size_t ap_bytes_in_pool(ap_pool_t *p)

B<report the number of bytes currently in the pool>

    arg 1) The pool to inspect
    return) The number of bytes

=cut
 */
APR_EXPORT(ap_size_t) ap_bytes_in_pool(ap_pool_t *p);

/*

=head1 ap_size_t ap_bytes_in_free_blocks(ap_pool_t *p)

B<report the number of bytes currently in the list of free blocks>

    return) The number of bytes

=cut
 */
APR_EXPORT(ap_size_t) ap_bytes_in_free_blocks(void);

/*

=head1 ap_pool_t *ap_pool_is_ancestor(ap_pool_t *a, ap_pool_t *b)

B<Determine if pool a is an ancestor of pool b>

    arg 1) The pool to search 
    arg 2) The pool to search for
    return) True if a is an ancestor of b, NULL is considered an ancestor
            of all pools.

=cut
 */
APR_EXPORT(int) ap_pool_is_ancestor(ap_pool_t *a, ap_pool_t *b);

/*

=head1 void *ap_palloc(ap_pool_t *c, ap_size_t reqsize)

B<Allocate a block of memory from a pool>

    arg 1) The pool to allocate out of 
    arg 2) The amount of memory to allocate 
    return) The allocated memory

=cut
 */
APR_EXPORT(void *) ap_palloc(ap_pool_t *c, ap_size_t reqsize);

/*

=head1 void *ap_pcalloc(ap_pool_t *c, ap_size_t reqsize)

B<Allocate a block of memory from a pool and set all of the memory to 0>

    arg 1) The pool to allocate out of 
    arg 2) The amount of memory to allocate 
    return) The allocated memory

=cut
 */
APR_EXPORT(void *) ap_pcalloc(ap_pool_t *p, ap_size_t size);

/*

=head1 char *ap_pstrdup(ap_pool_t *c, const char *s)

B<duplicate a string into memory allocated out of a pool>

    arg 1) The pool to allocate out of 
    arg 2) The string to allocate
    return) The new string

=cut
 */
APR_EXPORT(char *) ap_pstrdup(ap_pool_t *p, const char *s);

/*

=head1 char *ap_pstrndup(ap_pool_t *c, const char *s, ap_size_t n)

B<duplicate the first n characters ofa string into memory allocated out of a pool>

    arg 1) The pool to allocate out of 
    arg 2) The string to allocate
    arg 3) The number of characters to duplicate
    return) The new string

=cut
 */
APR_EXPORT(char *) ap_pstrndup(ap_pool_t *p, const char *s, ap_size_t n);

/*

=head1 char *ap_pstrcat(ap_pool_t *c, ...)

B<Concatenate multiple strings, allocating memory out a pool>

    arg 1) The pool to allocate out of 
    ...) The strings to concatenate.  The final string must be NULL
    return) The new string

=cut
 */
APR_EXPORT_NONSTD(char *) ap_pstrcat(ap_pool_t *p, ...);

/*

=head1 char *ap_pvsprintf(ap_pool_t *c, const char *fmt, va_list ap)

B<printf-style style printing routine.  The data is output to a string allocated from a pool>

    arg 1) The pool to allocate out of 
    arg 2) The format of the string
    arg 3) The arguments to use while printing the data
    return) The new string

=cut
 */
APR_EXPORT(char *) ap_pvsprintf(ap_pool_t *p, const char *fmt, va_list ap);

/*

=head1 char *ap_psprintf(ap_pool_t *c, const char *fmt, ...)

B<printf-style style printing routine.  The data is output to a string allocated from a pool>

    arg 1) The pool to allocate out of 
    arg 2) The format of the string
    ...) The arguments to use while printing the data
    return) The new string

=cut
 */
APR_EXPORT_NONSTD(char *) ap_psprintf(ap_pool_t *p, const char *fmt, ...);

/*

=head1 void ap_register_cleanup(ap_pool_t *p, const void *data,
                                ap_status_t (*plain_cleanup)(void *),
                                ap_status_t (*child_cleanup)(void *))

B<Register a function to be called when a pool is cleared or destroyed>

    arg 1) The pool register the cleanup with 
    arg 2) The data to pass to the cleanup function.
    arg 3) The function to call when the pool is cleared or destroyed
    arg 4) The function to call when a child process is created 

=cut
 */
APR_EXPORT(void) ap_register_cleanup(ap_pool_t *p, const void *data,
                                     ap_status_t (*plain_cleanup) (void *),
                                     ap_status_t (*child_cleanup) (void *));

/*

=head1 void ap_kill_cleanup(ap_pool_t *p, const void *data,
                            ap_status_t (*cleanup(void *))

B<remove a previously registered cleanup function>

    arg 1) The pool remove the cleanup from 
    arg 2) The data to remove from cleanup
    arg 3) The function to remove from cleanup

=cut
 */
APR_EXPORT(void) ap_kill_cleanup(ap_pool_t *p, const void *data,
                                 ap_status_t (*cleanup) (void *));

/*

=head1 ap_status_t ap_run_cleanup(ap_pool_t *p, void *data, ap_status_t (*cleanup(void *))

B<Run the specified cleanup function immediately and unregister it>

    arg 1) The pool remove the cleanup from 
    arg 2) The data to remove from cleanup
    arg 3) The function to remove from cleanup

=cut
 */
APR_EXPORT(ap_status_t) ap_run_cleanup(ap_pool_t *p, void *data,
                                       ap_status_t (*cleanup) (void *));

/*

=head1 void ap_cleanup_for_exec(void)

B<Run all of the child_cleanups, so that any unnecessary files are closed because we are about to exec a new program>

=cut
 */
/* Preparing for exec() --- close files, etc., but *don't* flush I/O
 * buffers, *don't* wait for subprocesses, and *don't* free any memory.
 */
APR_EXPORT(void) ap_cleanup_for_exec(void);
APR_EXPORT(ap_status_t) ap_getpass(const char *prompt, char *pwbuf, size_t *bufsize);
APR_EXPORT_NONSTD(ap_status_t) ap_null_cleanup(void *data);


/* used to guarantee to the ap_pool_t debugging code that the sub ap_pool_t will not be
 * destroyed before the parent pool
 */
#ifndef POOL_DEBUG
#ifdef ap_pool_join
#undef ap_pool_join
#endif /* ap_pool_join */
#define ap_pool_join(a,b)
#endif /* POOL_DEBUG */

#ifdef __cplusplus
}
#endif

#endif	/* !ap_POOLS_H */
