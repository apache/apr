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
#ifndef APRWIN_H
#define APRWIN_H

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
#include <sys/types.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <process.h>

#define ap_inline
#define __attribute__(__x)
#define ENUM_BITFIELD(e,n,w)  signed int n : w

#define APR_HAVE_ERRNO_H        1
#define APR_HAVE_DIRENT_H       0
#define APR_HAVE_FCNTL_H        0
#define APR_HAVE_NETINET_IN_H   0
#define APR_HAVE_PTHREAD_H      0
#define APR_HAVE_STDARG_H       1
#define APR_HAVE_STDIO_H        1
#define APR_HAVE_SYS_TYPES_H    1
#define APR_HAVE_SYS_UIO_H      0

#define APR_USE_FLOCK_SERIALIZE           0 
#define APR_USE_SYSVSEM_SERIALIZE         0
#define APR_USE_FCNTL_SERIALIZE           0
#define APR_USE_PROC_PTHREAD_SERIALIZE    0 
#define APR_USE_PTHREAD_SERIALIZE         0 

#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/*  APR Feature Macros */
#define APR_HAS_THREADS  1
#define APR_HAS_SENDFILE 0
#define APR_HAS_MMAP     0

/* Typedefs that APR needs. */

typedef  short           ap_int16_t;
typedef  unsigned short  ap_uint16_t;
                                               
typedef  int           ap_int32_t;
typedef  unsigned int  ap_uint32_t;
                                               
typedef  __int64           ap_int64_t;
typedef  unsigned __int64  ap_uint64_t;

typedef  int         ap_size_t;
typedef  int         ap_ssize_t;
typedef  _off_t      ap_off_t;
typedef  int         pid_t;

/* Definitions that APR programs need to work properly. */

#define API_THREAD_FUNC          __stdcall
#define API_EXPORT(type)         type
#define API_EXPORT_NONSTD(type)  type
#define API_VAR_IMPORT           extern _declspec(dllimport)

/* struct iovec is needed to emulate Unix writev */
struct iovec {
    char* iov_base;
    int   iov_len;
};
#endif /* APRWIN_H */
#endif /* WIN32 */
