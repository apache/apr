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

/* This code kindly donated to APR by 
 *    Elrond  <elrond@samba-tng.org>
 *    Luke Kenneth Casson Leighton <lkcl@samba-tng.org>
 *    Sander Striker <striker@samba-tng.org>
 *
 * May 2001
 */
 
#ifndef APR_SMS_H
#define APR_SMS_H

typedef struct apr_sms_t    apr_sms_t;

#include "apr.h"
#include "apr_errno.h"
#ifndef APR_POOLS_ARE_SMS
#include "apr_pools.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef APR_POOLS_ARE_SMS
typedef struct apr_sms_t           apr_pool_t;
typedef int (*apr_abortfunc_t)(int retcode);
#endif

/**********************************************************************
 ** Defines 
 **********************************************************************/

/* The various types of cleanup's we can offer */
#define APR_ALL_CLEANUPS         0x0000
#define APR_GENERAL_CLEANUP      0x0001
#define APR_CHILD_CLEANUP        0x0002

/* Alignment macro's 
 *
 * APR_ALIGN is only to be used to align on a power of 2 boundary
 */
#define APR_ALIGN(size, boundary) \
    (((size) + ((boundary) - 1)) & ~((boundary) - 1))

#define APR_ALIGN_DEFAULT(size) APR_ALIGN(size, 8)

/**********************************************************************
 ** Debug options
 **********************************************************************/
/* 
 * One of the aims of SMS is to provide a large range of debugging
 * options.
 *
 * The options are normally turned off by having the define commented out.
 * To use, simply remove the define and rebuild!
 *
 * Function definitions are at the end of the file...
 */

/* APR_DEBUG_TO_FILE
 * This will put all debug output into a file, that you can name
 * using the APR_DEBUG_FILE define.  Normally this is set to setdout
 * and the output is simply printed there.
 */
#define APR_DEBUG_TO_FILE             0
#define APR_DEBUG_FILE   "/tmp/sms_debug"

/* APR_DEBUG_SHOW_STRUCTURE
 * This turns on a print of the ancestory of the SMS when
 * creating/destroying an SMS so it's place in the world can be seen.
 */
#define APR_DEBUG_SHOW_STRUCTURE      0

/* APR_DEBUG_SHOW_FUNCTIONS
 * This turns on debug printing of every call to 
 *    apr_sms_create
 *    apr_sms_destroy
 *    apr_sms_reset
 *
 * Format of output is
 *    CREATE - sms 0x0000000 [STANDARD] has been created
 */
#define APR_DEBUG_SHOW_FUNCTIONS      0

/* APR_DEBUG_TAG_SMS
 * Turn on the ability to give an SMS a "tag" that can be used to identify
 * it.
 */
#define APR_DEBUG_TAG_SMS             0

/* APR_DEBUG_ALLOCATIONS
 * This will record ALL calls made to
 *     apr_sms_malloc
 *     apr_sms_calloc
 *     apr_sms_realloc
 *     apr_sms_free
 * Details are put into the file specified in the APR_DEBUG_ALLOC_FILE
 * define
 */
#define APR_DEBUG_ALLOCATIONS         0
#define APR_DEBUG_ALLOC_FILE "/tmp/sms_alloc"

/**
 * @package APR memory system
 */

struct apr_sms_cleanup;

/*
 * memory allocation functions 
 */

/**
 * Allocate a block of memory using a certain memory system
 * @param sms The memory system to use
 * @param size The (minimal required) size of the block to be allocated
 * @return pointer to a newly allocated block of memory, NULL if insufficient
 *         memory available
 */
APR_DECLARE(void *) apr_sms_malloc(apr_sms_t *sms, apr_size_t size);

/**
 * Allocate a block of zeroed memory using a certain memory system
 * @param sms The memory system to use
 * @param size The (minimal required) size of the block to be allocated
 * @return pointer to a newly allocated block of memory, NULL if insufficient
 *         memory available
 */
APR_DECLARE(void *) apr_sms_calloc(apr_sms_t *sms, apr_size_t size);

/**
 * Change the size of a previously allocated block of memory
 * @param sms The memory system to use (should be the same as the
 *        one that returned the block)
 * @param mem Pointer to the previously allocated block. If NULL, this
 *        function acts like apr_sms_malloc.
 * @param size The (minimal required) size of the block to be allocated
 * @return pointer to a newly allocated block of memory, NULL if insufficient
 *         memory available
 */
APR_DECLARE(void *) apr_sms_realloc(apr_sms_t *sms, void *mem, apr_size_t size);

/**
 * Free a block of memory
 * @param sms The memory system to use (should be the same as the
 *        one that returned the block)
 * @param mem The block of memory to be freed
 */
APR_DECLARE(apr_status_t) apr_sms_free(apr_sms_t *sms, void *mem);

/*
 * memory system functions
 */

#ifdef APR_ASSERT_MEMORY

/**
 * Check if a memory system is obeying all rules. 
 * @caution Call this function as the last statement before returning a new
 *          memory system from your apr_xxx_sms_create.
 */
APR_DECLARE(void) apr_sms_assert(apr_sms_t *sms);
#else
#ifdef apr_sms_assert
#undef apr_sms_assert
#endif
#define apr_sms_assert(sms)
#endif /* APR_ASSERT_MEMORY */

/**
 * Reset a memory system so it can be reused. 
 * This will also run all cleanup functions associated with the memory system
 * @warning This function will fail if there is no reset function available
 *          for the given memory system (i.e. the memory system is non-
 *          tracking).
 * @param sms The memory system to be reset
 */
APR_DECLARE(apr_status_t) apr_sms_reset(apr_sms_t *sms);

/**
 * Destroy a memory system, effectively freeing all of its memory, and itself. 
 * This will also run all cleanup functions associated with the memory system.
 * @caution Be carefull when using this function with a non-tracking memory
 *          system
 * @param sms The memory system to be destroyed
 */
APR_DECLARE(apr_status_t) apr_sms_destroy(apr_sms_t *sms);

/**
 * Perform thread-safe locking required whilst this memory system is modified
 * @param sms The memory system to be locked for thread-safety
 */
APR_DECLARE(apr_status_t) apr_sms_lock(apr_sms_t *sms);

/**
 * Release thread-safe locking required whilst this memory system was
 * being modified
 * @param sms The memory system to be released from thread-safety
 */
APR_DECLARE(apr_status_t) apr_sms_unlock(apr_sms_t *sms);

/**
 * Determine if memory system a is an ancestor of memory system b
 * @param a The memory system to search
 * @param b The memory system to search for
 * @return APR_SUCCESS if a is an ancestor of b, 1 if it isn't
 */
APR_DECLARE(apr_status_t) apr_sms_is_ancestor(apr_sms_t *a, apr_sms_t *b);

/** 
 * Get the memory_system identity
 * @param sms The memory system to identify
 */
APR_DECLARE(const char *) apr_sms_get_identity(apr_sms_t *sms);

/**
 * Get the parent sms
 * @param sms the memory system to get the parent from
 */
APR_DECLARE(apr_sms_t *) apr_sms_get_parent(apr_sms_t *sms);

/**
 * Set the abort function.  This is called when we fail to
 * fulfil an allocation request.
 * @tip If an app wants APR to exit on a memory allocation error
 *      then this function can be called to set the callback to 
 *      use.  If this function is not called then APR will return an error
 *      and expect the app to deal with it.
 * @param abortfunc The function to call
 * @param sms The sms to register the function with
 */
APR_DECLARE(void) apr_sms_set_abort(apr_abortfunc_t abortfunc,
                                    apr_sms_t *sms);

/**
 * Get the abort function registered with the sms.
 * @param sms The sms to get the function for
 * @return The abort function registered
 */
APR_DECLARE(apr_abortfunc_t) apr_sms_get_abort(apr_sms_t *sms);

/**
 * Set user data into the current sms.
 * @param data The user data to insert
 * @param key The key to assign to this data
 * @param cleanup The cleanup program to use when cleaning up the data
 * @param sms The sms to inert the data into
 */
APR_DECLARE(apr_status_t) apr_sms_userdata_set(const void *data,
                                               const char *key,
                                               apr_status_t (*cleanup)(void*),
                                               apr_sms_t *sms);

/**
 * Get user data from an sms using the key it was registered with
 * @param data A pointer to the returned data
 * @param key The key to use to identify the data
 * @param sms The sms to get the data from
 */
APR_DECLARE(apr_status_t) apr_sms_userdata_get(void **data, const char *key,
                                               apr_sms_t *sms);

/*
 * memory system cleanup management functions
 */

/**
 * Register a function to be called when a memory system is reset or destroyed
 * @param sms The memory system to register the cleanup function with
 * @param type The type of cleanup to register
 * @param data The data to pass to the cleanup function
 * @param cleanup_fn The function to call when the memory system is reset or
 *        destroyed
 */
APR_DECLARE(apr_status_t) apr_sms_cleanup_register(apr_sms_t *sms, 
                                                   apr_int32_t type,
                                                   const void *data, 
                                                   apr_status_t (*cleanup_fn)(void *));

/**
 * Unregister a previously registered cleanup function
 * @param sms The memory system the cleanup function is registered
 *        with
 * @param type The type of the cleanup to unregister
 * @param type The type of cleanup to run
 * @param data The data associated with the cleanup function
 * @param cleanup_fn The registered cleanup function
 */
APR_DECLARE(apr_status_t) apr_sms_cleanup_unregister(apr_sms_t *sms, 
                                                     apr_int32_t type,
                                                     const void *data,
                                                     apr_status_t (*cleanup)(void *));

/**
 * Unregister all previously registered cleanup functions of the specified type
 * @param sms The memory system the cleanup functions are registered with
 * @param type The type associated with the cleanup function. Pass 0 to 
 *        unregister all cleanup functions.
 */
APR_DECLARE(apr_status_t) apr_sms_cleanup_unregister_type(apr_sms_t *sms,
                                                           apr_int32_t type);

/**
 * Run the specified cleanup function immediately and unregister it
 * @param sms The memory system the cleanup function is registered
 *        with
 * @param sms The memory system the cleanup function is registered with
 * @param type The type associated with the cleanup function. Pass 0 to ignore type.
 * @param data The data associated with the cleanup function
 * @param cleanup The registered cleanup function
 */
APR_DECLARE(apr_status_t) apr_sms_cleanup_run(apr_sms_t *sms, 
                                              apr_int32_t type, 
                                              const void *data,
                                              apr_status_t (*cleanup)(void *));

/**
 * Run the specified type of cleanup functions immediately and unregister them
 * @param sms The memory system the cleanup functions are registered with
 * @param type The category of cleanup functions to run. Pass 0 to run all
 *        cleanup functions.
 */
APR_DECLARE(apr_status_t) apr_sms_cleanup_run_type(apr_sms_t *sms, 
                                                   apr_int32_t type);

/**********************************************************************
 ** Standard SMS module
 **********************************************************************/

/**
 * Create a standard malloc/realloc/free memory system
 * @param sms A pointer to the created apr_sms_t*
 */
APR_DECLARE(apr_status_t) apr_sms_std_create(apr_sms_t **sms);


/**********************************************************************
 ** Debug routines
 **********************************************************************/

/* NB These are only available if the debugging option has been turned on. */

#ifdef APR_DEBUG_SHOW_STRUCTURE
/**
 * Show the heirachy of the sms
 * @param sms The sms to show the information for
 * @param direction Do we show up (to parent) or down (to children)
 */
APR_DECLARE(void) apr_sms_show_structure(apr_sms_t *sms, int direction);
#endif /* APR_DEBUG_SHOW_STRUCTURE */

#ifdef APR_DEBUG_TAG_SMS
/**
 * Set the debugging tag for an sms
 * @param tag The tag to give the sms
 * @param sms The sms to apply the tag to
 */
APR_DECLARE(void) apr_sms_tag(apr_sms_t *sms, const char *tag);
#endif /* APR_DEBUG_TAG_SMS */

#if APR_SMS_ALLOC_STATS
APR_DECLARE(void) apr_sms_dump_stats(apr_sms_t *sms);
#endif

#ifdef __cplusplus
}
#endif

#endif /* !APR_SMS_H */

