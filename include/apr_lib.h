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

#ifndef APR_LIB_H
#define APR_LIB_H

#include "apr_general.h"
#include "apr_tables.h"
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

#define HUGE_STRING_LEN 8192

/*
 * Define the structures used by the APR general-purpose library.
 */

/*
 * Structure used by the variable-formatter routines.
 */
typedef struct ap_vformatter_buff_t {
    char *curpos;
    char *endpos;
} ap_vformatter_buff_t;

enum kill_conditions {
    kill_never,			/* process is never sent any signals */
    kill_always,		/* process is sent SIGKILL on ap_pool_t cleanup */
    kill_after_timeout,		/* SIGTERM, wait 3 seconds, SIGKILL */
    just_wait,			/* wait forever for the process to complete */
    kill_only_once		/* send SIGTERM and then wait */
};

/*
 * Define the prototypes for the various APR GP routines.
 */
APR_EXPORT(char *) ap_cpystrn(char *d, const char *s, size_t l);
APR_EXPORT(ap_status_t) ap_tokenize_to_argv(const char *arg_str, 
                                            char ***argv_out,
                                            ap_pool_t *token_context);
APR_EXPORT(const char *) ap_filename_of_pathname(const char *pathname);
APR_EXPORT(char *) ap_collapse_spaces(char *dest, const char *src);

/*APR_EXPORT(ap_mutex_t *) ap_create_mutex(void *m);*/
APR_EXPORT(int) ap_slack(int l, int h);
APR_EXPORT_NONSTD(ap_status_t) ap_execle(const char *c, const char *a, ...);
APR_EXPORT_NONSTD(ap_status_t) ap_execve(const char *c, const char *argv[],
				  const char *envp[]);

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
 * appropriate, re ap_pool_t nitialize curpos and endpos, and return 0.
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

APR_EXPORT(int) ap_vformatter(int (*flush_func)(ap_vformatter_buff_t *b),
			       ap_vformatter_buff_t *c, const char *fmt,
			       va_list ap);


/* A small routine to validate a plain text password with a password
 * that has been encrypted using any algorithm APR knows about.
 */
APR_EXPORT(ap_status_t) ap_validate_password(const char *passwd, const char *hash);


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
APR_EXPORT(int) ap_snprintf(char *buf, size_t len, const char *format, ...)
	__attribute__((format(printf,3,4)));
APR_EXPORT(int) ap_vsnprintf(char *buf, size_t len, const char *format,
			      va_list ap);

/*
 * APR memory structure manipulators (pools, tables, and arrays).
 */
APR_EXPORT(ap_pool_t *) ap_make_sub_pool(ap_pool_t *p, int (*apr_abort)(int retcode));
APR_EXPORT(void) ap_clear_pool(struct ap_pool_t *p);
APR_EXPORT(void) ap_destroy_pool(struct ap_pool_t *p);
APR_EXPORT(long) ap_bytes_in_pool(ap_pool_t *p);
APR_EXPORT(long) ap_bytes_in_free_blocks(void);
APR_EXPORT(ap_pool_t *) ap_find_pool(const void *ts);
APR_EXPORT(int) ap_pool_is_ancestor(ap_pool_t *a, ap_pool_t *b);
APR_EXPORT(void) ap_pool_join(ap_pool_t *p, ap_pool_t *sub);

/* used to guarantee to the ap_pool_t debugging code that the sub ap_pool_t will not be
 * destroyed before the parent pool
 */
#ifndef POOL_DEBUG
#ifdef ap_pool_join
#undef ap_pool_join
#endif /* ap_pool_join */
#define ap_pool_join(a,b)
#endif /* POOL_DEBUG */


APR_EXPORT(void *) ap_palloc(struct ap_pool_t *c, int reqsize);
APR_EXPORT(void *) ap_pcalloc(struct ap_pool_t *p, int size);
APR_EXPORT(char *) ap_pstrdup(struct ap_pool_t *p, const char *s);
APR_EXPORT(char *) ap_pstrndup(struct ap_pool_t *p, const char *s, int n);
APR_EXPORT_NONSTD(char *) ap_pstrcat(struct ap_pool_t *p, ...);
APR_EXPORT(char *) ap_pvsprintf(struct ap_pool_t *p, const char *fmt, va_list ap);
APR_EXPORT_NONSTD(char *) ap_psprintf(struct ap_pool_t *p, const char *fmt, ...);
APR_EXPORT(void) ap_register_cleanup(struct ap_pool_t *p, void *data,
				      ap_status_t (*plain_cleanup) (void *),
				      ap_status_t (*child_cleanup) (void *));
APR_EXPORT(void) ap_kill_cleanup(struct ap_pool_t *p, void *data,
				  ap_status_t (*cleanup) (void *));
APR_EXPORT(ap_status_t) ap_run_cleanup(struct ap_pool_t *p, void *data,
				 ap_status_t (*cleanup) (void *));
APR_EXPORT(void) ap_cleanup_for_exec(void);
APR_EXPORT(ap_status_t) ap_getpass(const char *prompt, char *pwbuf, size_t *bufsize);
APR_EXPORT_NONSTD(ap_status_t) ap_null_cleanup(void *data);

APR_EXPORT(void) ap_note_subprocess(struct ap_pool_t *a, ap_proc_t *pid,
				     enum kill_conditions how);

APR_EXPORT(char *) ap_cpystrn(char *dst, const char *src, size_t dst_size);

#ifdef __cplusplus
}
#endif

#endif	/* ! APR_LIB_H */
