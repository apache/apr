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

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define MAXIMUM_WAIT_OBJECTS 64

typedef int               apr_signum_t;

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

/*
 * String and memory functions
 */

#if (!APR_HAVE_MEMMOVE)
#define memmove(a,b,c) bcopy(b,a,c)
#endif

#if (!APR_HAVE_BZERO)
#define bzero(a,b) memset(a,0,b)
#endif

/**
 * package APR Random Functions
 */

#if APR_HAS_RANDOM
/**
 * Generate a string of random bytes.
 * @param buf Random bytes go here
 * @param length size of the buffer
 */
/* TODO: I'm not sure this is the best place to put this prototype...*/
apr_status_t apr_generate_random_bytes(unsigned char * buf, int length);
#endif

/* Memory allocation/Pool debugging options... 
 *
 * Look in the developer documentation for details of what these do.
 *
 * NB These should ALL normally be commented out unless you REALLY
 * need them!!
 */
 
/*
#define ALLOC_DEBUG
#define POOL_DEBUG
#define ALLOC_USE_MALLOC
#define MAKE_TABLE_PROFILE
#define ALLOC_STATS
*/

typedef struct apr_pool_t apr_pool_t;

/** The memory allocation structure
 */
struct apr_pool_t {
    /** The first block in this pool. */
    union block_hdr *first;
    /** The last block in this pool. */
    union block_hdr *last;
    /** The list of cleanups to run on pool cleanup. */
    struct cleanup *cleanups;
    /** A list of processes to kill when this pool is cleared */
    struct process_chain *subprocesses;
    /** The first sub_pool of this pool */
    struct apr_pool_t *sub_pools;
    /** The next sibling pool */
    struct apr_pool_t *sub_next;
    /** The previous sibling pool */
    struct apr_pool_t *sub_prev;
    /** The parent pool of this pool */
    struct apr_pool_t *parent;
    /** The first free byte in this pool */
    char *free_first_avail;
#ifdef ALLOC_USE_MALLOC
    /** The allocation list if using malloc */
    void *allocation_list;
#endif
#ifdef POOL_DEBUG
    /** a list of joined pools 
     *  @defvar apr_pool_t *joined */
    struct apr_pool_t *joined;
#endif
    /** A function to control how pools behave when they receive ENOMEM
     *  @deffunc int apr_abort(int retcode) */
    int (*apr_abort)(int retcode);
    /** A place to hand user data associated with this pool 
     *  @defvar datastruct *prog_data */
    struct datastruct *prog_data;
};
 
/* pool functions */

/**
 * Create a new pool.
 * @param newcont The pool we have just created.
 * @param cont The parent pool.  If this is NULL, the new pool is a root
 *        pool.  If it is non-NULL, the new pool will inherit all
 *        of it's parent pool's attributes, except the apr_pool_t will 
 *        be a sub-pool.
 */
apr_status_t apr_create_pool(apr_pool_t **newcont, apr_pool_t *cont);

/**
 * Set the data associated with the current pool
 * @param data The user data associated with the pool.
 * @param key The key to use for association
 * @param cleanup The cleanup program to use to cleanup the data;
 * @param cont The current pool.
 * @tip The data to be attached to the pool should have the same
 *      life span as the pool it is being attached to.
 *
 *      Users of APR must take EXTREME care when choosing a key to
 *      use for their data.  It is possible to accidentally overwrite
 *      data by choosing a key that another part of the program is using
 *      It is advised that steps are taken to ensure that a unique
 *      key is used at all times.
 */
apr_status_t apr_set_userdata(const void *data, const char *key, 
                            apr_status_t (*cleanup) (void *), 
                            apr_pool_t *cont);

/**
 * Return the data associated with the current pool.
 * @param data The key for the data to retrieve
 * @param key The user data associated with the pool.
 * @param cont The current pool.
 */
apr_status_t apr_get_userdata(void **data, const char *key, apr_pool_t *cont);

/**
 * Setup any APR internal data structures.  This MUST be the first function 
 * called for any APR program.
 */
apr_status_t apr_initialize(void);

/**
 * Tear down any APR internal data structures which aren't torn down 
 * automatically.
 * @tip An APR program must call this function at termination once it 
 *      has stopped using APR services.
 */
void apr_terminate(void);

/**
 * Set the APR_ABORT function.
 * @tip This is in for backwards compatability.  If the program using
 *      APR wants APR to exit on a memory allocation error, then this
 *      function should be called to set the function to use in order
 *      to actually exit the program.  If this function is not called,
 *      then APR will return an error and expect the calling program to
 *      deal with the error accordingly.
 */
apr_status_t apr_set_abort(int (*apr_abort)(int retcode), apr_pool_t *cont);

/**
 * Return a human readable string describing the specified error.
 * @param statcode The error code the get a string for.
 * @param buf A buffer to hold the error string.
 * @param bufsize Size of the buffer to hold the string.
 */
char *apr_strerror(apr_status_t statcode, char *buf, apr_size_t bufsize);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_GENERAL_H */
