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

#ifndef APR_POOLS_H
#define APR_POOLS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file apr_pools.h
 * @brief APR memory allocation
 *
 * Resource allocation routines...
 *
 * designed so that we don't have to keep track of EVERYTHING so that
 * it can be explicitly freed later (a fundamentally unsound strategy ---
 * particularly in the presence of die()).
 *
 * Instead, we maintain pools, and allocate items (both memory and I/O
 * handlers) from the pools --- currently there are two, one for per
 * transaction info, and one for config info.  When a transaction is over,
 * we can delete everything in the per-transaction apr_pool_t without fear, 
 * and without thinking too hard about it either.
 */
/** 
 * @defgroup APR_Pool Pool Allocation Functions
 * @ingroup APR
 * @{ 
 */
#include "apr.h"
#include "apr_errno.h"
#define APR_WANT_MEMFUNC
#include "apr_want.h"

/* Memory allocation/Pool debugging options... 
 *
 * Look in the developer documentation for details of what these do.
 *
 * NB These should ALL normally be commented out unless you REALLY
 * need them!!
 */
/* 
#define APR_POOL_DEBUG
*/

#define APR_POOL_STRINGIZE(x) APR_POOL__STRINGIZE(x)
#define APR_POOL__STRINGIZE(x) #x
#define APR_POOL__FILELINE__ __FILE__ ":" APR_POOL_STRINGIZE(__LINE__)
    
    
/** The fundamental pool type */
typedef struct apr_pool_t apr_pool_t;

/** A function that is called when allocation fails. */
typedef int (*apr_abortfunc_t)(int retcode);

/** Pool creation flags */

#define APR_POOL_FDEFAULT       0x0
#define APR_POOL_FNEW_ALLOCATOR 0x1
#define APR_POOL_FLOCK          0x2


/*
 * APR memory structure manipulators (pools, tables, and arrays).
 */

/*
 * Initialization
 */

/**
 * Setup all of the internal structures required to use pools
 * @remark Programs do NOT need to call this directly.  APR will call this
 *      automatically from apr_initialize. 
 * @internal
 */
APR_DECLARE(apr_status_t) apr_pool_initialize(void);

/**
 * Tear down all of the internal structures required to use pools
 * @remark Programs do NOT need to call this directly.  APR will call this
 *      automatically from apr_terminate. 
 * @internal
 */
APR_DECLARE(void) apr_pool_terminate(void); 


/*
 * Pool creation/destruction
 */

/**
 * Create a new pool.
 * @param newpool The pool we have just created.
 * @param parent The parent pool.  If this is NULL, the new pool is a root
 *        pool.  If it is non-NULL, the new pool will inherit all
 *        of its parent pool's attributes, except the apr_pool_t will 
 *        be a sub-pool.
 * @param apr_abort A function to use if the pool cannot allocate more memory.
 * @param flags Flags indicating how the pool should be created:
 *        - POOL_FNEW_ALLOCATOR  will create a new allocator for the pool
 *          instead of using the allocator of the parent.
 *        - POOL_FLOCK will create a mutex for the newly created allocator
 *          (this flag only makes sense in combination with POOL_FNEW_ALLOCATOR)
 *
 */
#if defined(APR_POOL_DEBUG)
#define apr_pool_create_ex( newpool, parent, abort_fn, flag)  \
    apr_pool_create_ex_dbg( newpool, parent, abort_fn, flag,__FILE__,__LINE__)  

APR_DECLARE(apr_status_t) apr_pool_create_ex_dbg(apr_pool_t **newpool,
                                             apr_pool_t *parent,
                                             apr_abortfunc_t abort_fn,
                                             apr_uint32_t flags,
                                             const char *file,
                                             int line);
#
#else
APR_DECLARE(apr_status_t) apr_pool_create_ex(apr_pool_t **newpool,
                                             apr_pool_t *parent,
                                             apr_abortfunc_t abort_fn,
                                             apr_uint32_t flags);
#endif
/**
 * Create a new pool.
 * @param newpool The pool we have just created.
 * @param parent The parent pool.  If this is NULL, the new pool is a root
 *        pool.  If it is non-NULL, the new pool will inherit all
 *        of its parent pool's attributes, except the apr_pool_t will 
 *        be a sub-pool.
 */
#if defined(DOXYGEN)
APR_DECLARE(apr_status_t) apr_pool_create(apr_pool_t **newpool,
                                          apr_pool_t *parent);
#else
#if defined(APR_POOL_DEBUG)
#define apr_pool_create(newpool, parent) \
    apr_pool_create_ex_dbg(newpool, parent, NULL, APR_POOL_FDEFAULT,__FILE__,__LINE__)
#else
#define apr_pool_create(newpool, parent) \
    apr_pool_create_ex(newpool, parent, NULL, APR_POOL_FDEFAULT)
#endif
#endif

/**
 * This function is deprecated.  Use apr_pool_create_ex.
 * @param newpool The new sub-pool
 * @param parent The pool to use as a parent pool
 * @param apr_abort A function to use if the pool cannot allocate more memory.
 * @deffunc void apr_pool_sub_make(apr_pool_t **p, apr_pool_t *parent, int (*apr_abort)(int retcode), const char *created)
 * @remark The @a apr_abort function provides a way to quit the program if the
 *      machine is out of memory.  By default, APR will return on error.
 */
#if defined(DOXYGEN)
APR_DECLARE(void) apr_pool_sub_make(apr_pool_t **newpool, 
                                    apr_pool_t *parent,
                                    int (*apr_abort)(int retcode));
#else
#if defined(APR_POOL_DEBUG)
#define apr_pool_sub_make(newpool, parent, abort_fn) \
    (void)apr_pool_create_ex_dbg(newpool, parent, abort_fn, APR_POOL_FDEFAULT,__FILE__,__LINE__);
#else
#define apr_pool_sub_make(newpool, parent, abort_fn) \
    (void)apr_pool_create_ex(newpool, parent, abort_fn, APR_POOL_FDEFAULT);
#endif
#endif

/**
 * Destroy the pool. This takes similar action as apr_pool_clear() and then
 * frees all the memory.
 * @param p The pool to destroy
 * @remark This will actually free the memory
 */
#if defined(APR_POOL_DEBUG)
#define apr_pool_destroy(p) \
    apr_pool_destroy_dbg(p, __FILE__,__LINE__)

APR_DECLARE(void) apr_pool_destroy_dbg(apr_pool_t *p, const char *file, int line);
#else
APR_DECLARE(void) apr_pool_destroy(apr_pool_t *p);
#endif

/*
 * Memory allocation
 */

/**
 * Allocate a block of memory from a pool
 * @param p The pool to allocate from 
 * @param reqsize The amount of memory to allocate 
 * @return The allocated memory
 */
APR_DECLARE(void *) apr_palloc(apr_pool_t *p, apr_size_t reqsize);

/**
 * Allocate a block of memory from a pool and set all of the memory to 0
 * @param p The pool to allocate from 
 * @param size The amount of memory to allocate 
 * @return The allocated memory
 */
APR_DECLARE(void *) apr_pcalloc(apr_pool_t *p, apr_size_t size);

/**
 * Clear all memory in the pool and run all the cleanups. This also clears all
 * subpools.
 * @param p The pool to clear
 * @remark  This does not actually free the memory, it just allows the pool
 *       to re-use this memory for the next allocation.
 * @see apr_pool_destroy()
 */
#if defined(APR_POOL_DEBUG)
#define apr_pool_clear(p) \
    apr_pool_clear_dbg(p, __FILE__,__LINE__)

APR_DECLARE(void) apr_pool_clear_dbg(apr_pool_t *p, const char*file, int line);
#else
APR_DECLARE(void) apr_pool_clear(apr_pool_t *p);
#endif

/*
 * Pool Properties
 */

/**
 * Set the function to be called when an allocation failure occurs.
 * @tip If the program wants APR to exit on a memory allocation error,
 *      then this function can be called to set the callback to use (for
 *      performing cleanup and then exiting). If this function is not called,
 *      then APR will return an error and expect the calling program to
 *      deal with the error accordingly.
 * @deffunc apr_status_t apr_pool_set_abort(apr_abortfunc_t abortfunc, apr_pool_t *pool)
 */
APR_DECLARE(void) apr_pool_set_abort(apr_abortfunc_t abortfunc,
                                     apr_pool_t *pool);

/**
 * Get the abort function associated with the specified pool.
 * @param pool The pool for retrieving the abort function.
 * @return The abort function for the given pool.
 * @deffunc apr_abortfunc_t apr_pool_get_abort(apr_pool_t *pool)
 */
APR_DECLARE(apr_abortfunc_t) apr_pool_get_abort(apr_pool_t *pool);

/**
 * Get the parent pool of the specified pool.
 * @param pool The pool for retrieving the parent pool.
 * @return The parent of the given pool.
 * @deffunc apr_pool_t * apr_pool_get_parent(apr_pool_t *pool)
 */
APR_DECLARE(apr_pool_t *) apr_pool_get_parent(apr_pool_t *pool);

/**
 * Determine if pool a is an ancestor of pool b
 * @param a The pool to search 
 * @param b The pool to search for
 * @return True if a is an ancestor of b, NULL is considered an ancestor
 *         of all pools.
 */
APR_DECLARE(int) apr_pool_is_ancestor(apr_pool_t *a, apr_pool_t *b);

/**
 * Tag a pool (give it a name)
 * @param pool The pool to tag
 * @param tag  The tag
 */
APR_DECLARE(void) apr_pool_tag(apr_pool_t *pool, const char *tag);


/*
 * User data management
 */

/**
 * Set the data associated with the current pool
 * @param data The user data associated with the pool.
 * @param key The key to use for association
 * @param cleanup The cleanup program to use to cleanup the data (NULL if none)
 * @param pool The current pool
 * @warning The data to be attached to the pool should have a life span
 *          at least as long as the pool it is being attached to.
 *
 *      Users of APR must take EXTREME care when choosing a key to
 *      use for their data.  It is possible to accidentally overwrite
 *      data by choosing a key that another part of the program is using
 *      It is advised that steps are taken to ensure that a unique
 *      key is used at all times.
 * @bug Specify how to ensure this uniqueness!
 */
APR_DECLARE(apr_status_t) apr_pool_userdata_set(const void *data,
                                                const char *key,
                                                apr_status_t (*cleanup)(void *),
                                                apr_pool_t *pool);

/**
 * Set the data associated with the current pool
 * @param data The user data associated with the pool.
 * @param key The key to use for association
 * @param cleanup The cleanup program to use to cleanup the data (NULL if none)
 * @param pool The current pool
 * @note same as apr_pool_userdata_set(), except that this version doesn't
 *       make a copy of the key (this function is useful, for example, when
 *       the key is a string literal)
 * @warning The key and the data to be attached to the pool should have
 *       a life span at least as long as the pool itself.
 *
 */
APR_DECLARE(apr_status_t) apr_pool_userdata_setn(const void *data,
                                                 const char *key,
                                                 apr_status_t (*cleanup)(void *),
                                                 apr_pool_t *pool);

/**
 * Return the data associated with the current pool.
 * @param data The user data associated with the pool.
 * @param key The key for the data to retrieve
 * @param pool The current pool.
 */
APR_DECLARE(apr_status_t) apr_pool_userdata_get(void **data, const char *key,
                                           apr_pool_t *pool);


/*
 * Cleanup
 */

/**
 * Register a function to be called when a pool is cleared or destroyed
 * @param p The pool register the cleanup with 
 * @param data The data to pass to the cleanup function.
 * @param plain_cleanup The function to call when the pool is cleared 
 *                      or destroyed
 * @param child_cleanup The function to call when a child process is created -
 *                      this function is called in the child, obviously!
 */
APR_DECLARE(void) apr_pool_cleanup_register(apr_pool_t *p, const void *data,
                                       apr_status_t (*plain_cleanup)(void *),
                                       apr_status_t (*child_cleanup)(void *));

/**
 * Remove a previously registered cleanup function
 * @param p The pool remove the cleanup from 
 * @param data The data to remove from cleanup
 * @param cleanup The function to remove from cleanup
 * @remarks For some strange reason only the plain_cleanup is handled by this
 *          function
 */
APR_DECLARE(void) apr_pool_cleanup_kill(apr_pool_t *p, const void *data,
                                   apr_status_t (*cleanup)(void *));

/**
 * Replace the child cleanup of a previously registered cleanup
 * @param p The pool of the registered cleanup
 * @param data The data of the registered cleanup
 * @param plain_cleanup The plain cleanup function of the registered cleanup
 * @param child_cleanup The function to register as the child cleanup
 */
APR_DECLARE(void) apr_pool_child_cleanup_set(apr_pool_t *p, const void *data,
                                      apr_status_t (*plain_cleanup)(void *),
                                      apr_status_t (*child_cleanup)(void *));

/**
 * Run the specified cleanup function immediately and unregister it. Use
 * @a data instead of the data that was registered with the cleanup.
 * @param p The pool remove the cleanup from 
 * @param data The data to remove from cleanup
 * @param cleanup The function to remove from cleanup
 */
APR_DECLARE(apr_status_t) apr_pool_cleanup_run(apr_pool_t *p, void *data,
                                          apr_status_t (*cleanup)(void *));

/**
 * An empty cleanup function 
 * @param data The data to cleanup
 */
APR_DECLARE_NONSTD(apr_status_t) apr_pool_cleanup_null(void *data);

/* Preparing for exec() --- close files, etc., but *don't* flush I/O
 * buffers, *don't* wait for subprocesses, and *don't* free any memory.
 */
/**
 * Run all of the child_cleanups, so that any unnecessary files are 
 * closed because we are about to exec a new program
 */
APR_DECLARE(void) apr_pool_cleanup_for_exec(void);


/**
 * @defgroup PoolDebug Pool Debugging functions.
 *
 * pools have nested lifetimes -- sub_pools are destroyed when the
 * parent pool is cleared.  We allow certain liberties with operations
 * on things such as tables (and on other structures in a more general
 * sense) where we allow the caller to insert values into a table which
 * were not allocated from the table's pool.  The table's data will
 * remain valid as long as all the pools from which its values are
 * allocated remain valid.
 *
 * For example, if B is a sub pool of A, and you build a table T in
 * pool B, then it's safe to insert data allocated in A or B into T
 * (because B lives at most as long as A does, and T is destroyed when
 * B is cleared/destroyed).  On the other hand, if S is a table in
 * pool A, it is safe to insert data allocated in A into S, but it
 * is *not safe* to insert data allocated from B into S... because
 * B can be cleared/destroyed before A is (which would leave dangling
 * pointers in T's data structures).
 *
 * In general we say that it is safe to insert data into a table T
 * if the data is allocated in any ancestor of T's pool.  This is the
 * basis on which the APR_POOL_DEBUG code works -- it tests these ancestor
 * relationships for all data inserted into tables.  APR_POOL_DEBUG also
 * provides tools (apr_find_pool, and apr_pool_is_ancestor) for other
 * folks to implement similar restrictions for their own data
 * structures.
 *
 * However, sometimes this ancestor requirement is inconvenient --
 * sometimes we're forced to create a sub pool (such as through
 * apr_sub_req_lookup_uri), and the sub pool is guaranteed to have
 * the same lifetime as the parent pool.  This is a guarantee implemented
 * by the *caller*, not by the pool code.  That is, the caller guarantees
 * they won't destroy the sub pool individually prior to destroying the
 * parent pool.
 *
 * In this case the caller must call apr_pool_join() to indicate this
 * guarantee to the APR_POOL_DEBUG code.  There are a few examples spread
 * through the standard modules.
 *
 * These functions are only implemented when #APR_POOL_DEBUG is set.
 *
 * @{
 */
#if defined(APR_POOL_DEBUG) || defined(DOXYGEN)
/**
 * Guarantee that a subpool has the same lifetime as the parent.
 * @param p The parent pool
 * @param sub The subpool
 */
APR_DECLARE(void) apr_pool_join(apr_pool_t *p, apr_pool_t *sub);

/**
 * Find a pool from something allocated in it.
 * @param mem The thing allocated in the pool
 * @return The pool it is allocated in
 */
APR_DECLARE(apr_pool_t *) apr_find_pool(const void *mem);

/**
 * Report the number of bytes currently in the pool
 * @param p The pool to inspect
 * @param recurse Recurse/include the subpools' sizes
 * @return The number of bytes
 */
APR_DECLARE(apr_size_t) apr_pool_num_bytes(apr_pool_t *p, int recurse);

/**
 * Report the number of bytes currently in the list of free blocks
 * @return The number of bytes
 */
APR_DECLARE(apr_size_t) apr_pool_free_blocks_num_bytes(void);

/**
 * Lock a pool
 * @param pool The pool to lock
 * @param flag  The flag
 */
APR_DECLARE(void) apr_pool_lock(apr_pool_t *pool, int flag);

/* @} */

#else
#    ifdef apr_pool_join
#        undef apr_pool_join
#    endif
#    define apr_pool_join(a,b)

#    ifdef apr_pool_lock
#        undef apr_pool_lock
#    endif
#    define apr_pool_lock(pool, lock)
#endif


/*
 * Pool accessor functions.
 *
 * These standardized function are used by opaque (APR) data types to return
 * the apr_pool_t that is associated with the data type.
 *
 * APR_POOL_DECLARE_ACCESSOR() is used in a header file to declare the
 * accessor function. A typical usage and result would be:
 *
 *    APR_POOL_DECLARE_ACCESSOR(file);
 * becomes:
 *    APR_DECLARE(apr_pool_t *) apr_file_pool_get(apr_file_t *ob);
 *
 * In the implementation, the APR_POOL_IMPLEMENT_ACCESSOR() is used to
 * actually define the function. It assumes the field is named "pool". For
 * data types with a different field name (e.g. "cont" or "cntxt") the
 * APR_POOL_IMPLEMENT_ACCESSOR_X() macro should be used.
 *
 * Note: the linkage is specified for APR. It would be possible to expand
 *       the macros to support other linkages.
 */
#define APR_POOL_DECLARE_ACCESSOR(typename) \
    APR_DECLARE(apr_pool_t *) apr_##typename##_pool_get \
        (const apr_##typename##_t *ob)

#define APR_POOL_IMPLEMENT_ACCESSOR(typename) \
    APR_POOL_IMPLEMENT_ACCESSOR_X(typename, pool)
#define APR_POOL_IMPLEMENT_ACCESSOR_X(typename, fieldname) \
    APR_DECLARE(apr_pool_t *) apr_##typename##_pool_get \
        (const apr_##typename##_t *ob) { return ob->fieldname; }

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* !APR_POOLS_H */
