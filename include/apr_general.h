/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
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
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#ifndef APR_GENERAL_H
#define APR_GENERAL_H

#ifdef WIN32
#include "apr_win.h"
#else
#include "apr.h"
#endif

#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if APR_HAVE_SIGNAL_H
#include <signal.h>
#endif
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TRUE 1
#define FALSE 0

#define MAXIMUM_WAIT_OBJECTS 64

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

/* Finding offsets of elements within structures.
 * Taken from the X code... they've sweated portability of this stuff
 * so we don't have to.  Sigh...
 */

#if defined(CRAY) || (defined(__arm) && !defined(LINUX))
#ifdef __STDC__
#define XtOffset(p_type,field) _Offsetof(p_type,field)
#else
#ifdef CRAY2
#define XtOffset(p_type,field) \
        (sizeof(int)*((unsigned int)&(((p_type)NULL)->field)))

#else /* !CRAY2 */

#define XtOffset(p_type,field) ((unsigned int)&(((p_type)NULL)->field))

#endif /* !CRAY2 */
#endif /* __STDC__ */
#else /* ! (CRAY || __arm) */

#define XtOffset(p_type,field) \
        ((long) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))

#endif /* !CRAY */

#ifdef offsetof
#define XtOffsetOf(s_type,field) offsetof(s_type,field)
#else
#define XtOffsetOf(s_type,field) XtOffset(s_type*,field)
#endif



/* Context functions */
ap_status_t ap_create_context(ap_context_t **newcont, ap_context_t *cont);
ap_status_t ap_destroy_context(struct context_t *cont);
ap_status_t ap_exit(ap_context_t *);
ap_status_t ap_set_userdata(void *data, char *key, 
                            ap_status_t (*cleanup) (void *), 
                            ap_context_t *cont);
ap_status_t ap_get_userdata(void **, char *key, ap_context_t *cont);
ap_status_t ap_initialize(void);
ap_status_t ap_set_abort(int (*apr_abort)(int retcode), ap_context_t *cont);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_GENERAL_H */
