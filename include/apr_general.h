/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#ifndef APR_GENERAL_H
#define APR_GENERAL_H

/**
 * @file apr_general.h
 * This is collection of oddballs that didn't fit anywhere else,
 * and might move to more appropriate headers with the release
 * of APR 1.0.
 * @brief APR Miscellaneous library routines
 */

#include "apr.h"
#include "apr_pools.h"
#include "apr_errno.h"

#if APR_HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup apr_general Miscellaneous library routines
 * @ingroup APR 
 * This is collection of oddballs that didn't fit anywhere else,
 * and might move to more appropriate headers with the release
 * of APR 1.0.
 * @{
 */

/** FALSE */
#ifndef FALSE
#define FALSE 0
#endif
/** TRUE */
#ifndef TRUE
#define TRUE (!FALSE)
#endif

/** a space */
#define APR_ASCII_BLANK  '\040'
/** a carrige return */
#define APR_ASCII_CR     '\015'
/** a line feed */
#define APR_ASCII_LF     '\012'
/** a tab */
#define APR_ASCII_TAB    '\011'

/** signal numbers typedef */
typedef int               apr_signum_t;

/**
 * Finding offsets of elements within structures.
 * Taken from the X code... they've sweated portability of this stuff
 * so we don't have to.  Sigh...
 * @param p_type pointer type name
 * @param field  data field within the structure pointed to
 * @return offset
 */

#if defined(CRAY) || (defined(__arm) && !defined(LINUX))
#ifdef __STDC__
#define APR_OFFSET(p_type,field) _Offsetof(p_type,field)
#else
#ifdef CRAY2
#define APR_OFFSET(p_type,field) \
        (sizeof(int)*((unsigned int)&(((p_type)NULL)->field)))

#else /* !CRAY2 */

#define APR_OFFSET(p_type,field) ((unsigned int)&(((p_type)NULL)->field))

#endif /* !CRAY2 */
#endif /* __STDC__ */
#else /* ! (CRAY || __arm) */

#define APR_OFFSET(p_type,field) \
        ((long) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))

#endif /* !CRAY */

/**
 * Finding offsets of elements within structures.
 * @param s_type structure type name
 * @param field  data field within the structure
 * @return offset
 */
#if defined(offsetof) && !defined(__cplusplus)
#define APR_OFFSETOF(s_type,field) offsetof(s_type,field)
#else
#define APR_OFFSETOF(s_type,field) APR_OFFSET(s_type*,field)
#endif

/** @deprecated @see APR_OFFSET */
#define APR_XtOffset APR_OFFSET

/** @deprecated @see APR_OFFSETOF */
#define APR_XtOffsetOf APR_OFFSETOF

#ifndef DOXYGEN

/* A couple of prototypes for functions in case some platform doesn't 
 * have it
 */
#if (!APR_HAVE_STRCASECMP) && (APR_HAVE_STRICMP) 
#define strcasecmp(s1, s2) stricmp(s1, s2)
#elif (!APR_HAVE_STRCASECMP)
int strcasecmp(const char *a, const char *b);
#endif

#if (!APR_HAVE_STRNCASECMP) && (APR_HAVE_STRNICMP)
#define strncasecmp(s1, s2, n) strnicmp(s1, s2, n)
#elif (!APR_HAVE_STRNCASECMP)
int strncasecmp(const char *a, const char *b, size_t n);
#endif

#endif

/**
 * Alignment macros
 */

/* APR_ALIGN() is only to be used to align on a power of 2 boundary */
#define APR_ALIGN(size, boundary) \
    (((size) + ((boundary) - 1)) & ~((boundary) - 1))

/** Default alignment */
#define APR_ALIGN_DEFAULT(size) APR_ALIGN(size, 8)


/**
 * String and memory functions
 */

/** Properly quote a value as a string in the C preprocessor */
#define APR_STRINGIFY(n) APR_STRINGIFY_HELPER(n)
/** Helper macro for APR_STRINGIFY */
#define APR_STRINGIFY_HELPER(n) #n

#if (!APR_HAVE_MEMMOVE)
#define memmove(a,b,c) bcopy(b,a,c)
#endif

#if (!APR_HAVE_MEMCHR)
void *memchr(const void *s, int c, size_t n);
#endif

/** @} */

/**
 * @defgroup apr_library Library initialization and termination
 * @{
 */

/**
 * Setup any APR internal data structures.  This MUST be the first function 
 * called for any APR library.
 * @remark See apr_app_initialize if this is an application, rather than
 * a library consumer of apr.
 */
APR_DECLARE(apr_status_t) apr_initialize(void);

/**
 * Set up an application with normalized argc, argv (and optionally env) in
 * order to deal with platform-specific oddities, such as Win32 services,
 * code pages and signals.  This must be the first function called for any
 * APR program.
 * @param argc Pointer to the argc that may be corrected
 * @param argv Pointer to the argv that may be corrected
 * @param env Pointer to the env that may be corrected, may be NULL
 * @remark See apr_initialize if this is a library consumer of apr.
 * Otherwise, this call is identical to apr_initialize, and must be closed
 * with a call to apr_terminate at the end of program execution.
 */
APR_DECLARE(apr_status_t) apr_app_initialize(int *argc, 
                                             char const * const * *argv, 
                                             char const * const * *env);

/**
 * Tear down any APR internal data structures which aren't torn down 
 * automatically.
 * @remark An APR program must call this function at termination once it 
 *         has stopped using APR services.  The APR developers suggest using
 *         atexit to ensure this is called.  When using APR from a language
 *         other than C that has problems with the calling convention, use
 *         apr_terminate2() instead.
 */
APR_DECLARE_NONSTD(void) apr_terminate(void);

/**
 * Tear down any APR internal data structures which aren't torn down 
 * automatically, same as apr_terminate
 * @remark An APR program must call either the apr_terminate or apr_terminate2 
 *         function once it it has finished using APR services.  The APR 
 *         developers suggest using atexit(apr_terminate) to ensure this is done.
 *         apr_terminate2 exists to allow non-c language apps to tear down apr, 
 *         while apr_terminate is recommended from c language applications.
 */
APR_DECLARE(void) apr_terminate2(void);

/** @} */

/**
 * @defgroup apr_random Random Functions
 * @{
 */

#if APR_HAS_RANDOM || defined(DOXYGEN)

/* TODO: I'm not sure this is the best place to put this prototype...*/
/**
 * Generate random bytes.
 * @param buf Buffer to fill with random bytes
 * @param length Length of buffer in bytes (becomes apr_size_t in APR 1.0)
 */
#ifdef APR_ENABLE_FOR_1_0
APR_DECLARE(apr_status_t) apr_generate_random_bytes(unsigned char * buf, 
                                                    apr_size_t length);
#else
APR_DECLARE(apr_status_t) apr_generate_random_bytes(unsigned char * buf, 
                                                    int length);
#endif

#endif
/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_GENERAL_H */
