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

#ifndef APR_GENERAL_H
#define APR_GENERAL_H

#include "apr.h"

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

#if APR_HAS_RANDOM
/* ***APRDOC********************************************************
 * TODO: I'm not sure this is the best place to put this prototype...
 * ap_status_t ap_generate_random_bytes(unsigned char * buf, int length)
 *    Generate a string of random bytes.
 * arg 1) Random bytes go here
 * arg 2) size of the buffer
 */
ap_status_t ap_generate_random_bytes(unsigned char * buf, int length);
#endif

typedef struct ap_pool_t {
    union block_hdr *first;
    union block_hdr *last;
    struct cleanup *cleanups;
    struct process_chain *subprocesses;
    struct ap_pool_t *sub_pools;
    struct ap_pool_t *sub_next;
    struct ap_pool_t *sub_prev;
    struct ap_pool_t *parent;
    char *free_first_avail;
#ifdef ALLOC_USE_MALLOC
    void *allocation_list;
#endif
#ifdef POOL_DEBUG
    ap_pool_t *joined;
#endif
    int (*apr_abort)(int retcode);
    struct datastruct *prog_data;
}ap_pool_t;
 
/* Context functions */
/* ***APRDOC********************************************************
 * ap_status_t ap_create_context(ap_context_t **newcont, ap_context_t *cont)
 *    Create a new context.
 * arg 1) The context we have just created.
 * arg 2) The parent context.  If this is NULL, the new context is a root
 *        context.  If it is non-NULL, the new context will inherit all
 *        of it's parent context's attributes, except the ap_context_t will be a
 *        sub-pool.
 */
ap_status_t ap_create_pool(ap_pool_t **newcont, ap_pool_t *cont);

/* ***APRDOC********************************************************
 * ap_status_t ap_destroy_context(ap_context_t *cont)
 *    Free the context and all of it's child contexts'.
 * arg 1) The context to free.
 */
ap_status_t ap_destroy_context(ap_pool_t *cont);

ap_status_t ap_exit(ap_pool_t *);

/* ***APRDOC********************************************************
 * ap_status_t ap_set_userdata(void *data, char *key, 
 *                             ap_status_t (*cleanup) (void *),
 *                             ap_context_t *cont)
 *    Set the data associated with the current context.
 * arg 1) The user data associated with the context.
 * arg 2) The key to use for association
 * arg 3) The cleanup program to use to cleanup the data;
 * arg 4) The current context.
 * NOTE:  The data to be attached to the context should have the same
 *        life span as the context it is being attached to.
 *        
 *        Users of APR must take EXTREME care when choosing a key to
 *        use for their data.  It is possible to accidentally overwrite
 *        data by choosing a key that another part of the program is using
 *        It is advised that steps are taken to ensure that a unique
 *        key is used at all times.
 */
ap_status_t ap_set_userdata(void *data, char *key, 
                            ap_status_t (*cleanup) (void *), 
                            ap_pool_t *cont);

/* ***APRDOC********************************************************
 * ap_status_t ap_get_userdata(void **data, char *key, ap_context_t *cont)
 *    Return the data associated with the current context.
 * arg 1) The key for the data to retrieve
 * arg 2) The user data associated with the context.
 * arg 3) The current context.
 */
ap_status_t ap_get_userdata(void **, char *key, ap_pool_t *cont);

/* ***APRDOC********************************************************
 * ap_status_t ap_initialize(void)
 *    Setup any APR internal data structures.  This MUST be the first
 *    function called for any APR program.
 */
ap_status_t ap_initialize(void);

/* ***APRDOC*******************************************************
 * void ap_terminate(void)
 *    Tear down any APR internal data structures which aren't
 *    torn down automatically.  An APR program must call this
 *    function at termination once it has stopped using APR
 *    services.
 */
void ap_terminate(void);

/* ***APRDOC********************************************************
 * ap_status_t ap_set_abort(int (*apr_abort)(int retcode), ap_context_t *cont)
 *    Set the APR_ABORT function.
 * NOTE:  This is in for backwards compatability.  If the program using
 *        APR wants APR to exit on a memory allocation error, then this
 *        function should be called to set the function to use in order
 *        to actually exit the program.  If this function is not called,
 *        then APR will return an error and expect the calling program to
 *        deal with the error accordingly.
 */
ap_status_t ap_set_abort(int (*apr_abort)(int retcode), ap_pool_t *cont);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_GENERAL_H */
