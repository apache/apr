/* ====================================================================
 * Copyright (c) 1998-1999 The Apache Group.  All rights reserved.
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
 * The ap_vsnprintf/ap_snprintf functions are based on, and used with the
 * permission of, the  SIO stdiocntxteplacement strx_* functions by Panos
 * Tsirigotis <panos@alumni.cs.colorado.edu> for xinetd.
 *
 * This header file defines the public interfaces for the APR general-purpose
 * library routines.  No others should need to be #included.
 */

#ifndef APR_LIB_H
#define APR_LIB_H

#include "apr_general.h"
#include "apr_file_io.h"
#if HAVE_DIRENT_H
#include <dirent.h>
#endif

#ifndef WIN32
#include "apr_config.h"
#else
#include "../file_io/win32/readdir.h" /* definition of DIR for WIN32 */
#include "apr_win.h"
#endif
#include "hsregex.h"
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define HUGE_STRING_LEN 8192

/*
 * Define the structures used by the APR general-purpose library.
 */

/*
 * Memory allocation stuff, like pools, arrays, and tables.  Pools
 * and tables are opaque structures to applications, but arrays are
 * published.
 */
typedef struct ap_pool_t ap_pool_t;
typedef struct ap_table_t ap_table_t;
typedef struct ap_child_info_t ap_child_info_t;
typedef void ap_mutex_t;
typedef struct ap_array_header_t {
    ap_context_t *cont;
    int elt_size;
    int nelts;
    int nalloc;
    char *elts;
} ap_array_header_t;

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

/*
 * Structure used by the variable-formatter routines.
 */
typedef struct ap_vformatter_buff_t {
    char *curpos;
    char *endpos;
} ap_vformatter_buff_t;

enum kill_conditions {
    kill_never,			/* process is never sent any signals */
    kill_always,		/* process is sent SIGKILL on ap_context_t cleanup */
    kill_after_timeout,		/* SIGTERM, wait 3 seconds, SIGKILL */
    just_wait,			/* wait forever for the process to complete */
    kill_only_once		/* send SIGTERM and then wait */
};

/*
 * Define the prototypes for the various APR GP routines.
 */
API_EXPORT(char *) ap_cpystrn(char *d, const char *s, size_t l);
/*API_EXPORT(ap_mutex_t *) ap_create_mutex(void *m);*/
API_EXPORT(int) ap_slack(int l, int h);
API_EXPORT_NONSTD(int) ap_execle(const char *c, const char *a, ...);
API_EXPORT_NONSTD(int) ap_execve(const char *c, const char *argv[],
				  const char *envp[]);

#define ap_create_mutex(x) (0)
#define ap_release_mutex(x) (0)
#define ap_acquire_mutex(x) (0)

/* These macros allow correct support of 8-bit characters on systems which
 * support 8-bit characters.  Pretty dumb how the cast is required, but
 * that's legacy libc for ya.  These new macros do not support EOF like
 * the standard macros do.  Tough.
 */
#define ap_isalnum(c) (isalnum(((unsigned char)(c))))
#define ap_isalpha(c) (isalpha(((unsigned char)(c))))
#define ap_iscntrl(c) (iscntrl(((unsigned char)(c))))
#define ap_isdigit(c) (isdigit(((unsigned char)(c))))
#define ap_isgraph(c) (isgraph(((unsigned char)(c))))
#define ap_islower(c) (islower(((unsigned char)(c))))
#define ap_isprint(c) (isprint(((unsigned char)(c))))
#define ap_ispunct(c) (ispunct(((unsigned char)(c))))
#define ap_isspace(c) (isspace(((unsigned char)(c))))
#define ap_isupper(c) (isupper(((unsigned char)(c))))
#define ap_isxdigit(c) (isxdigit(((unsigned char)(c))))
#define ap_tolower(c) (tolower(((unsigned char)(c))))
#define ap_toupper(c) (toupper(((unsigned char)(c))))




/*
 * Small utility macros to make things easier to read.  Not usually a
 * goal, to be sure..
 */

#ifdef WIN32
#define ap_killpg(x, y)
#else /* WIN32 */
#ifdef NO_KILLPG
#define ap_killpg(x, y)		(kill (-(x), (y)))
#else /* NO_KILLPG */
#define ap_killpg(x, y)		(killpg ((x), (y)))
#endif /* NO_KILLPG */
#endif /* WIN32 */

/*
 * ap_vformatter() is a generic printf-style formatting routine
 * with some extensions.  The extensions are:
 *
 * %pA	takes a struct in_addr *, and prints it as a.b.c.d
 * %pI	takes a struct sockaddr_in * and prints it as a.b.c.d:port
 * %pp  takes a void * and outputs it in hex
 *
 * The %p hacks are to force gcc's printf warning code to skip
 * over a pointer argument without complaining.  This does
 * mean that the ANSI-style %p (output a void * in hex format) won't
 * work as expected at all, but that seems to be a fair trade-off
 * for the increased robustness of having printf-warnings work.
 *
 * Additionally, ap_vformatter allows for arbitrary output methods
 * using the ap_vformatter_buff and flush_func.
 *
 * The ap_vformatter_buff has two elements curpos and endpos.
 * curpos is where ap_vformatter will write the next byte of output.
 * It proceeds writing output to curpos, and updating curpos, until
 * either the end of output is reached, or curpos == endpos (i.e. the
 * buffer is full).
 *
 * If the end of output is reached, ap_vformatter returns the
 * number of bytes written.
 *
 * When the buffer is full, the flush_func is called.  The flush_func
 * can return -1 to indicate that no further output should be attempted,
 * and ap_vformatter will return immediately with -1.  Otherwise
 * the flush_func should flush the buffer in whatever manner is
 * appropriate, re ap_context_t nitialize curpos and endpos, and return 0.
 *
 * Note that flush_func is only invoked as a result of attempting to
 * write another byte at curpos when curpos >= endpos.  So for
 * example, it's possible when the output exactly matches the buffer
 * space available that curpos == endpos will be true when
 * ap_vformatter returns.
 *
 * ap_vformatter does not call out to any other code, it is entirely
 * self-contained.  This allows the callers to do things which are
 * otherwise "unsafe".  For example, ap_psprintf uses the "scratch"
 * space at the unallocated end of a block, and doesn't actually
 * complete the allocation until ap_vformatter returns.  ap_psprintf
 * would be completely broken if ap_vformatter were to call anything
 * that used a ap_pool_t.  Similarly http_bprintf() uses the "scratch"
 * space at the end of its output buffer, and doesn't actually note
 * that the space is in use until it either has to flush the buffer
 * or until ap_vformatter returns.
 */

API_EXPORT(int) ap_vformatter(int (*flush_func)(ap_vformatter_buff_t *b),
			       ap_vformatter_buff_t *c, const char *fmt,
			       va_list ap);

/*
 * These are snprintf implementations based on ap_vformatter().
 *
 * Note that various standards and implementations disagree on the return
 * value of snprintf, and side-effects due to %n in the formatting string.
 * ap_snprintf behaves as follows:
 *
 * Process the format string until the entire string is exhausted, or
 * the buffer fills.  If the buffer fills then stop processing immediately
 * (so no further %n arguments are processed), and return the buffer
 * length.  In all cases the buffer is NUL terminated.
 *
 * In no event does ap_snprintf return a negative number.  It's not possible
 * to distinguish between an output which was truncated, and an output which
 * exactly filled the buffer.
 */
API_EXPORT(int) ap_snprintf(char *buf, size_t len, const char *format, ...)
	__attribute__((format(printf,3,4)));
API_EXPORT(int) ap_vsnprintf(char *buf, size_t len, const char *format,
			      va_list ap);

/*
 * APR memory structure manipulators (pools, tables, and arrays).
 */
API_EXPORT(ap_pool_t *) ap_make_sub_pool(ap_pool_t *p);
API_EXPORT(void) ap_clear_pool(struct context_t *p);
API_EXPORT(void) ap_destroy_pool(struct context_t *p);
API_EXPORT(long) ap_bytes_in_pool(ap_pool_t *p);
API_EXPORT(long) ap_bytes_in_free_blocks(void);
API_EXPORT(ap_pool_t *) ap_find_pool(const void *ts);
API_EXPORT(int) ap_pool_is_ancestor(ap_pool_t *a, ap_pool_t *b);
API_EXPORT(void) ap_pool_join(ap_pool_t *p, ap_pool_t *sub);

/* used to guarantee to the ap_context_t debugging code that the sub ap_context_t will not be
 * destroyed before the parent pool
 */
#ifndef POOL_DEBUG
#ifdef ap_pool_join
#undef ap_pool_join
#endif /* ap_pool_join */
#define ap_pool_join(a,b)
#endif /* POOL_DEBUG */


API_EXPORT(void *) ap_palloc(struct context_t *c, int reqsize);
API_EXPORT(void *) ap_pcalloc(struct context_t *p, int size);
API_EXPORT(char *) ap_pstrdup(struct context_t *p, const char *s);
API_EXPORT(char *) ap_pstrndup(struct context_t *p, const char *s, int n);
API_EXPORT_NONSTD(char *) ap_pstrcat(struct context_t *p, ...);
API_EXPORT(char *) ap_pvsprintf(struct context_t *p, const char *fmt, va_list ap);
API_EXPORT_NONSTD(char *) ap_psprintf(struct context_t *p, const char *fmt, ...);
API_EXPORT(ap_array_header_t *) ap_make_array(struct context_t *p, int nelts,
						int elt_size);
API_EXPORT(void *) ap_push_array(ap_array_header_t *arr);
API_EXPORT(void) ap_array_cat(ap_array_header_t *dst,
			       const ap_array_header_t *src);
API_EXPORT(ap_array_header_t *) ap_copy_array(struct context_t *p,
						const ap_array_header_t *arr);
API_EXPORT(ap_array_header_t *)
	ap_copy_array_hdr(struct context_t *p,
			   const ap_array_header_t *arr);
API_EXPORT(ap_array_header_t *)
	ap_append_arrays(struct context_t *p,
			  const ap_array_header_t *first,
			  const ap_array_header_t *second);
API_EXPORT(char *) ap_array_pstrcat(struct context_t *p,
				     const ap_array_header_t *arr,
				     const char sep);
API_EXPORT(ap_table_t *) ap_make_table(struct context_t *p, int nelts);
API_EXPORT(ap_table_t *) ap_copy_table(struct context_t *p, const ap_table_t *t);
API_EXPORT(void) ap_clear_table(ap_table_t *t);
API_EXPORT(const char *) ap_table_get(const ap_table_t *t, const char *key);
API_EXPORT(void) ap_table_set(ap_table_t *t, const char *key,
			       const char *val);
API_EXPORT(void) ap_table_setn(ap_table_t *t, const char *key,
				const char *val);
API_EXPORT(void) ap_table_unset(ap_table_t *t, const char *key);
API_EXPORT(void) ap_table_merge(ap_table_t *t, const char *key,
				 const char *val);
API_EXPORT(void) ap_table_mergen(ap_table_t *t, const char *key,
				  const char *val);
API_EXPORT(void) ap_table_add(ap_table_t *t, const char *key,
			       const char *val);
API_EXPORT(void) ap_table_addn(ap_table_t *t, const char *key,
				const char *val);
API_EXPORT(ap_table_t *) ap_overlay_tables(struct context_t *p,
					     const ap_table_t *overlay,
					     const ap_table_t *base);
API_EXPORT(void)
	ap_table_do(int (*comp) (void *, const char *, const char *),
		     void *rec, const ap_table_t *t, ...);
#define AP_OVERLAP_TABLES_SET   (0)
#define AP_OVERLAP_TABLES_MERGE (1)
API_EXPORT(void) ap_overlap_tables(ap_table_t *a, const ap_table_t *b,
				    unsigned flags);
API_EXPORT(void) ap_register_cleanup(struct context_t *p, void *data,
				      ap_status_t (*plain_cleanup) (void *),
				      ap_status_t (*child_cleanup) (void *));
API_EXPORT(void) ap_kill_cleanup(struct context_t *p, void *data,
				  ap_status_t (*cleanup) (void *));
API_EXPORT(void) ap_run_cleanup(struct context_t *p, void *data,
				 ap_status_t (*cleanup) (void *));
API_EXPORT(void) ap_cleanup_for_exec(void);
API_EXPORT(ap_status_t) ap_getpass(const char *prompt, char *pwbuf, size_t *bufsize);
API_EXPORT_NONSTD(ap_status_t) ap_null_cleanup(void *data);

API_EXPORT(regex_t *) ap_pregcomp(ap_context_t *p, const char *pattern,
				   int cflags);
API_EXPORT(void) ap_pregfree(ap_context_t *p, regex_t *reg);
/*API_EXPORT(void) ap_note_subprocess(ap_pool_t *a, pid_t pid,
				     enum kill_conditions how);
*/
API_EXPORT(int)
	ap_spawn_child(ap_context_t *p,
			int (*func) (void *a, ap_child_info_t *c),
			void *data, enum kill_conditions kill_how,
			FILE **pipe_in, FILE **pipe_out,
			FILE **pipe_err);
#if 0
API_EXPORT(int)
	ap_bspawn_child(ap_context_t *p,
			 int (*func) (void *v, ap_child_info_t *c),
			 void *data, enum kill_conditions kill_how,
			 BUFF **pipe_in, BUFF **pipe_out, BUFF **pipe_err);
#endif /* 0 */

API_EXPORT(char *) ap_cpystrn(char *dst, const char *src, size_t dst_size);

/*
 * Routine definitions that only work on Windows.
 */

/*#ifdef TPF*/
#define ap_block_alarms()
#define ap_unblock_alarms()
/*#else 
API_EXPORT(void) ap_block_alarms(void);
API_EXPORT(void) ap_unblock_alarms(void);
#endif */

#ifdef __cplusplus
}
#endif

#endif	/* ! APR_LIB_H */
