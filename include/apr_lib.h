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

#include "apr.h"
#include "apr_pools.h"
#include "apr_general.h"
#include "apr_tables.h"
#include "apr_file_io.h"
#include "apr_thread_proc.h"

#if APR_HAVE_CTYPE_H
#include <ctype.h>
#endif
#if APR_HAVE_STDARG_H
#include <stdarg.h>
#endif
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/**
 * @package APR general-purpose library
 */

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
typedef struct apr_vformatter_buff_t {
    char *curpos;
    char *endpos;
} apr_vformatter_buff_t;

/**
 * return the final element of the pathname
 * @param pathname The path to get the final element of
 * @return the final element of the path
 * @tip Examples:  
 * <PRE>
 *                 "/foo/bar/gum"   -> "gum"
 *                 "/foo/bar/gum/"  -> ""
 *                 "gum"            -> "gum"
 *                 "wi\\n32\\stuff" -> "stuff"
 * </PRE>
 * @deffunc const char * apr_filename_of_pathname(const char *pathname)
 */
APR_DECLARE(const char *) apr_filename_of_pathname(const char *pathname);

/* These macros allow correct support of 8-bit characters on systems which
 * support 8-bit characters.  Pretty dumb how the cast is required, but
 * that's legacy libc for ya.  These new macros do not support EOF like
 * the standard macros do.  Tough.
 */
#define apr_isalnum(c) (isalnum(((unsigned char)(c))))
#define apr_isalpha(c) (isalpha(((unsigned char)(c))))
#define apr_iscntrl(c) (iscntrl(((unsigned char)(c))))
#define apr_isdigit(c) (isdigit(((unsigned char)(c))))
#define apr_isgraph(c) (isgraph(((unsigned char)(c))))
#define apr_islower(c) (islower(((unsigned char)(c))))
#define apr_isascii(c) (isascii(((unsigned char)(c))))
#define apr_isprint(c) (isprint(((unsigned char)(c))))
#define apr_ispunct(c) (ispunct(((unsigned char)(c))))
#define apr_isspace(c) (isspace(((unsigned char)(c))))
#define apr_isupper(c) (isupper(((unsigned char)(c))))
#define apr_isxdigit(c) (isxdigit(((unsigned char)(c))))
#define apr_tolower(c) (tolower(((unsigned char)(c))))
#define apr_toupper(c) (toupper(((unsigned char)(c))))

/*
 * Small utility macros to make things easier to read.  Not usually a
 * goal, to be sure..
 */

#ifdef WIN32
#define apr_killpg(x, y)
#else /* WIN32 */
#ifdef NO_KILLPG
#define apr_killpg(x, y)        (kill (-(x), (y)))
#else /* NO_KILLPG */
#define apr_killpg(x, y)        (killpg ((x), (y)))
#endif /* NO_KILLPG */
#endif /* WIN32 */

/**
 * apr_vformatter() is a generic printf-style formatting routine
 * with some extensions.
 * @param flush_func The function to call when the buffer is full
 * @param c The buffer to write to
 * @param fmt The format string
 * @param ap The arguments to use to fill out the format string.
 *
 * @tip <PRE>
 * The extensions are:
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
 * Additionally, apr_vformatter allows for arbitrary output methods
 * using the apr_vformatter_buff and flush_func.
 *
 * The apr_vformatter_buff has two elements curpos and endpos.
 * curpos is where apr_vformatter will write the next byte of output.
 * It proceeds writing output to curpos, and updating curpos, until
 * either the end of output is reached, or curpos == endpos (i.e. the
 * buffer is full).
 *
 * If the end of output is reached, apr_vformatter returns the
 * number of bytes written.
 *
 * When the buffer is full, the flush_func is called.  The flush_func
 * can return -1 to indicate that no further output should be attempted,
 * and apr_vformatter will return immediately with -1.  Otherwise
 * the flush_func should flush the buffer in whatever manner is
 * appropriate, re apr_pool_t nitialize curpos and endpos, and return 0.
 *
 * Note that flush_func is only invoked as a result of attempting to
 * write another byte at curpos when curpos >= endpos.  So for
 * example, it's possible when the output exactly matches the buffer
 * space available that curpos == endpos will be true when
 * apr_vformatter returns.
 *
 * apr_vformatter does not call out to any other code, it is entirely
 * self-contained.  This allows the callers to do things which are
 * otherwise "unsafe".  For example, apr_psprintf uses the "scratch"
 * space at the unallocated end of a block, and doesn't actually
 * complete the allocation until apr_vformatter returns.  apr_psprintf
 * would be completely broken if apr_vformatter were to call anything
 * that used a apr_pool_t.  Similarly http_bprintf() uses the "scratch"
 * space at the end of its output buffer, and doesn't actually note
 * that the space is in use until it either has to flush the buffer
 * or until apr_vformatter returns.
 * </PRE>
 * @deffunc int apr_vformatter(int (*flush_func)(apr_vformatter_buff_t *b), apr_vformatter_buff_t *c, const char *fmt, va_list ap)
 */
APR_DECLARE(int) apr_vformatter(int (*flush_func)(apr_vformatter_buff_t *b),
			       apr_vformatter_buff_t *c, const char *fmt,
			       va_list ap);

/**
 * Validate any password encypted with any algorithm that APR understands
 * @param passwd The password to validate
 * @param hash The password to validate against
 * @deffunc apr_status_t apr_validate_password(const char *passwd, const char *hash)
 */
APR_DECLARE(apr_status_t) apr_validate_password(const char *passwd, const char *hash);


/*
 * These are snprintf implementations based on apr_vformatter().
 *
 * Note that various standards and implementations disagree on the return
 * value of snprintf, and side-effects due to %n in the formatting string.
 * apr_snprintf behaves as follows:
 *
 * Process the format string until the entire string is exhausted, or
 * the buffer fills.  If the buffer fills then stop processing immediately
 * (so no further %n arguments are processed), and return the buffer
 * length.  In all cases the buffer is NUL terminated.
 *
 * In no event does apr_snprintf return a negative number.  It's not possible
 * to distinguish between an output which was truncated, and an output which
 * exactly filled the buffer.
 */

/**
 *snprintf routine based on apr_vformatter.  This means it understands the 
 *same extensions.>
 * @param buf The buffer to write to
 * @param len The size of the buffer
 * @param format The format string
 * @param ... The arguments to use to fill out the format string.
 * @deffunc int apr_snprintf(char *buf, size_t len, const char *format, ...)
 */
APR_DECLARE_NONSTD(int) apr_snprintf(char *buf, size_t len, 
                                   const char *format, ...)
	__attribute__((format(printf,3,4)));

/**
 * vsnprintf routine based on apr_vformatter.  This means it understands the 
 * same extensions.
 * @param buf The buffer to write to
 * @param len The size of the buffer
 * @param format The format string
 * @param ap The arguments to use to fill out the format string.
 * @deffunc int apr_vsnprintf(char *buf, size_t len, const char *format, va_list ap)
 */
APR_DECLARE(int) apr_vsnprintf(char *buf, size_t len, const char *format,
			      va_list ap);

/**
 * Display a prompt and read in the password from stdin.
 * @param prompt The prompt to display
 * @param pwbuf Where to store the password
 * @param bufsize The length of the password string.
 * @deffunc apr_status_t apr_getpass(const char *prompt, char *pwbuf, size_t *bufsize)
 */
APR_DECLARE(apr_status_t) apr_getpass(const char *prompt, char *pwbuf, size_t *bufsize);

/**
 * Register a process to be killed when a pool dies.
 * @param a The pool to use to define the processes lifetime 
 * @param pid The process to register
 * @param how How to kill the process, one of:
 * <PRE>
 *         kill_never   	   -- process is never sent any signals
 *         kill_always 	   -- process is sent SIGKILL on apr_pool_t cleanup	
 *         kill_after_timeout -- SIGTERM, wait 3 seconds, SIGKILL
 *         just_wait          -- wait forever for the process to complete
 *         kill_only_once     -- send SIGTERM and then wait
 * </PRE>
 * @deffunc void apr_note_subprocess(struct apr_pool_t *a, apr_proc_t *pid, enum kill_conditions how)
 */
APR_DECLARE(void) apr_note_subprocess(struct apr_pool_t *a, apr_proc_t *pid,
				     enum kill_conditions how);

#ifdef __cplusplus
}
#endif

#endif	/* ! APR_LIB_H */
