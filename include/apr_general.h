/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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
 * @brief APR Misc routines
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
 * @defgroup APR_Misc Misc routines
 * @ingroup APR
 * @{
 */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifndef MAXIMUM_WAIT_OBJECTS
#define MAXIMUM_WAIT_OBJECTS 64
#endif

#define APR_ASCII_BLANK  '\040'
#define APR_ASCII_CR     '\015'
#define APR_ASCII_LF     '\012'
#define APR_ASCII_TAB    '\011'

typedef int               apr_signum_t;

/**
 * Finding offsets of elements within structures.
 * Taken from the X code... they've sweated portability of this stuff
 * so we don't have to.  Sigh...
 */

#if defined(CRAY) || (defined(__arm) && !defined(LINUX))
#ifdef __STDC__
#define APR_XtOffset(p_type,field) _Offsetof(p_type,field)
#else
#ifdef CRAY2
#define APR_XtOffset(p_type,field) \
        (sizeof(int)*((unsigned int)&(((p_type)NULL)->field)))

#else /* !CRAY2 */

#define APR_XtOffset(p_type,field) ((unsigned int)&(((p_type)NULL)->field))

#endif /* !CRAY2 */
#endif /* __STDC__ */
#else /* ! (CRAY || __arm) */

#define APR_XtOffset(p_type,field) \
        ((long) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))

#endif /* !CRAY */

#ifdef offsetof
#define APR_XtOffsetOf(s_type,field) offsetof(s_type,field)
#else
#define APR_XtOffsetOf(s_type,field) APR_XtOffset(s_type*,field)
#endif


/**
 * A couple of prototypes for functions in case some platform doesn't 
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

/**
 * String and memory functions
 */

#if (!APR_HAVE_MEMMOVE)
#define memmove(a,b,c) bcopy(b,a,c)
#endif

#if (!APR_HAVE_MEMCHR)
void *memchr(const void *s, int c, size_t n);
#endif

/**
 * Setup any APR internal data structures.  This MUST be the first function 
 * called for any APR program.
 * @deffunc apr_status_t apr_initialize(void)
 */
APR_DECLARE(apr_status_t) apr_initialize(void);

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
 * @defgroup APR_Random Random Functions
 * @ingroup APR
 * @{
 */

#if APR_HAS_RANDOM

/* TODO: I'm not sure this is the best place to put this prototype...*/
/**
 * Generate a string of random bytes.
 * @param buf Random bytes go here
 * @param length size of the buffer
 */
APR_DECLARE(apr_status_t) apr_generate_random_bytes(unsigned char * buf, 
                                                    int length);

#endif
/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_GENERAL_H */
