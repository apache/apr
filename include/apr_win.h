/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#ifdef WIN32

#ifndef APR_WIN_H
#define APR_WIN_H
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WIN32_WINNT
/* 
 * Compile the server including all the Windows NT 4.0 header files by 
 * default.
 */
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>
#include <winsock2.h>
#include <mswsock.h>
#include <sys\types.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

typedef enum {APR_WIN_NT, APR_WIN_95, APR_WIN_98} ap_oslevel_e;

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

typedef _off_t      off_t;
typedef int         pid_t;
typedef void (Sigfunc)(int);

#define __attribute__(__x) 

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONGLONG 8

#define API_EXPORT(x)            x
#define API_EXPORT_NONSTD(x)     x
#define API_THREAD_FUNC __stdcall
#define API_VAR_IMPORT _declspec(dllimport)

#define strcasecmp(s1, s2)       stricmp(s1, s2)
#define sleep(t)                 Sleep(t * 1000)

time_t WinTimeToUnixTime(FILETIME *);
unsigned __stdcall SignalHandling(void *);

int thread_ready(void);

#endif  /*APR_WIN_H*/
#endif  /*WIN32*/
