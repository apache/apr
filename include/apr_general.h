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
#include "apr_win.h"
#include <windows.h>
#else
#include "apr_config.h"
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include "apr_errno.h"

#ifndef APR_GENERAL_H
#define APR_GENERAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TRUE 1
#define FALSE 0

#define MAXIMUM_WAIT_OBJECTS 64

#if (SIZEOF_SHORT == 2)
typedef short                  ap_int16_t;
typedef unsigned short         ap_uint16_t;
#endif

#if (SIZEOF_INT == 4)
typedef int                    ap_int32_t;
typedef unsigned int           ap_uint32_t;
#endif

#if (SIZEOF_LONG == 8)
typedef long                   ap_int64_t;
typedef unsigned long          ap_uint64_t;
#elif (SIZEOF_LONG_LONG == 8)
typedef long long              ap_int64_t;
typedef unsigned long long     ap_uint64_t;
#elif (SIZEOF_LONG_DOUBLE == 8)
typedef long double            ap_int64_t;
typedef unsigned long double   ap_uint64_t;
#elif (SIZEOF_LONGLONG == 8)
typedef __int64                ap_int64_t;
typedef unsigned __int64       ap_uint64_t;
#endif

typedef size_t                 ap_size_t;
#ifdef ssize_t
typedef ssize_t                ap_ssize_t;
#else
typedef int                    ap_ssize_t;
#endif
typedef off_t                  ap_off_t;

typedef struct context_t  ap_context_t;
typedef int               ap_signum_t;

#ifdef SIGHUP
#define APR_SIGHUP SIGHUP
#endif
#ifdef SIGINT
#define APR_SIGINT SIGINT
#endif
#ifdef SIGQUIT
#define APR_SIGQUIT SIGQUIT
#endif
#ifdef SIGILL
#define APR_SIGILL SIGILL
#endif
#ifdef SIGTRAP
#define APR_SIGTRAP SIGTRAP
#endif
#ifdef SIGABRT
#define APR_SIGABRT SIGABRT
#endif
#ifdef SIGIOT
#define APR_SIGIOT SIGIOT
#endif
#ifdef SIGBUS
#define APR_SIGBUS SIGBUS
#endif
#ifdef SIGFPE
#define APR_SIGFPE SIGFPE
#endif
#ifdef SIGKILL
#define APR_SIGKILL SIGKILL
#endif
#ifdef SIGUSR1
#define APR_SIGUSR1 SIGUSR1
#endif
#ifdef SIGSEGV
#define APR_SIGSEGV SIGSEGV
#endif
#ifdef SIGUSR2
#define APR_SIGUSR2 SIGUSR2
#endif
#ifdef SIGPIPE
#define APR_SIGPIPE SIGPIPE
#endif
#ifdef SIGALRM
#define APR_SIGALRM SIGALRM
#endif
#ifdef SIGTERM
#define APR_SIGTERM SIGTERM
#endif
#ifdef SIGSTKFLT
#define APR_SIGSTKFLT SIGSTKFLT
#endif
#ifdef SIGCHLD
#define APR_SIGCHLD SIGCHLD
#endif
#ifdef SIGCONT
#define APR_SIGCONT SIGCONT
#endif
#ifdef SIGSTOP
#define APR_SIGSTOP SIGSTOP
#endif
#ifdef SIGTSTP
#define APR_SIGTSTP SIGTSTP
#endif
#ifdef SIGTTIN
#define APR_SIGTTIN SIGTTIN
#endif
#ifdef SIGTTOU
#define APR_SIGTTOU SIGTTOU
#endif
#ifdef SIGURG
#define APR_SIGURG SIGURG
#endif
#ifdef SIGXCPU
#define APR_SIGXCPU SIGXCPU
#endif
#ifdef SIGXFSZ
#define APR_SIGXFSZ SIGXFSZ
#endif
#ifdef SIGVTALRM
#define APR_SIGVTALRM SIGVTALRM
#endif
#ifdef SIGPROF
#define APR_SIGPROF SIGPROF
#endif
#ifdef SIGWINCH
#define APR_SIGWINCH SIGWINCH
#endif
#ifdef SIGIO
#define APR_SIGIO SIGIO
#endif

#ifdef WIN32
#define APR_INLINE __inline
#elif defined(__GNUC__) && defined(__GNUC__) && \
      __GNUC__ >= 2 && __GNUC_MINOR__ >= 7
#define APR_INLINE __inline__
#else
#define APR_INLINE /*nop*/
#endif

/* Context functions */
ap_status_t ap_create_context(ap_context_t **, ap_context_t *);
ap_status_t ap_destroy_context(struct context_t *cont);
ap_status_t ap_exit(ap_context_t *);
ap_status_t ap_set_userdata(void *, char *, 
                            ap_status_t (*cleanup) (void *), ap_context_t *);
ap_status_t ap_get_userdata(void **, char *, ap_context_t *);
ap_status_t ap_initialize(void);

ap_status_t ap_create_signal(ap_context_t *, ap_signum_t);
ap_status_t ap_send_signal(ap_context_t *, ap_signum_t);
ap_status_t ap_setup_signal(ap_context_t *, ap_signum_t, Sigfunc *);

ap_status_t ap_getopt(ap_context_t *, ap_int32_t, char *const *, const char *,
                      ap_int32_t *); 

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_GENERAL_H */
