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

/*
 * Note: 
 * This is the windows specific autoconf-like config file
 * which unix would create at build time.
 */

#ifdef WIN32

#ifndef APR_PRIVATE_H
#define APR_PRIVATE_H

/* Include the public APR symbols, include our idea of the 'right'
 * subset of the Windows.h header.  This saves us repetition.
 */
#include "apr.h"

/* 
 * Add a _very_few_ declarations missing from the restricted set of headers
 * (If this list becomes extensive, re-enable the required headers above!)
 * winsock headers were excluded by WIN32_LEAN_AND_MEAN, so include them now
 */
#ifndef SW_HIDE
#define SW_HIDE             0
#endif

/* For the misc.h late-loaded dynamic symbols, we need some obscure types 
 * Avoid dragging in wtypes.h unless it's absolutely necessary [generally
 * not with APR itself, until some GUI-related security is introduced.]
 */
#ifndef _WIN32_WCE
#define HAVE_ACLAPI 1
#ifdef __wtypes_h__
#include <accctrl.h>
#else
#define __wtypes_h__
#include <accctrl.h>
#undef __wtypes_h__
#endif
#else
#define HAVE_ACLAPI 0
#endif

#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if APR_HAVE_STDDEF_H
#include <stddef.h>
#endif
#include <stdio.h>
#if APR_HAVE_TIME_H
#include <time.h>
#endif

/* Use this section to define all of the HAVE_FOO_H
 * that are required to build properly.
 */
#define HAVE_LIMITS_H 1
#define HAVE_MALLOC_H 1
#define HAVE_SIGNAL_H 1
/* #define HAVE_STDDEF_H 1 why not? */
#define HAVE_STDLIB_H 1

#define HAVE_STRICMP  1
#define HAVE_STRNICMP 1
#define HAVE_STRDUP   1
#define HAVE_STRSTR   1
#define HAVE_MEMCHR   1

#define SIGHUP     1
/* 2 is used for SIGINT on windows */
#define SIGQUIT    3
/* 4 is used for SIGILL on windows */
#define SIGTRAP    5
#define SIGIOT     6
#define SIGBUS     7
/* 8 is used for SIGFPE on windows */
#define SIGKILL    9
#define SIGUSR1    10
/* 11 is used for SIGSEGV on windows */
#define SIGUSR2    12
#define SIGPIPE    13
#define SIGALRM    14
/* 15 is used for SIGTERM on windows */
#define SIGSTKFLT  16
#define SIGCHLD    17 
#define SIGCONT    18
#define SIGSTOP    19
#define SIGTSTP    20
/* 21 is used for SIGBREAK on windows */
/* 22 is used for SIGABRT on windows */
#define SIGTTIN    23
#define SIGTTOU    24
#define SIGURG     25
#define SIGXCPU    26
#define SIGXFSZ    27
#define SIGVTALRM  28
#define SIGPROF    29
#define SIGWINCH   30
#define SIGIO      31

#define __attribute__(__x) 

/* APR COMPATABILITY FUNCTIONS
 * This section should be used to define functions and
 * macros which are need to make Windows features look
 * like POSIX features.
 */
typedef void (Sigfunc)(int);

#define sleep(t)                 Sleep((t) * 1000)

#define SIZEOF_SHORT           2
#define SIZEOF_INT             4
#define SIZEOF_LONGLONG        8
#define SIZEOF_CHAR            1
#define SIZEOF_SSIZE_T         SIZEOF_INT

unsigned __stdcall SignalHandling(void *);
int thread_ready(void);

#if !APR_HAVE_ERRNO_H
APR_DECLARE_DATA int errno;
#define ENOSPC 1
#endif

#if APR_HAVE_IPV6
#define HAVE_GETADDRINFO 1
#define HAVE_GETNAMEINFO 1
#endif

/*
 * Include common private declarations.
 */
#include "../apr_private_common.h"

#endif  /*APR_PRIVATE_H*/
#endif  /*WIN32*/
