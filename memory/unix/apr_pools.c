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

#include "apr.h"
#include "apr_private.h"

#include "apr_portable.h" /* for get_os_proc */
#include "apr_strings.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_lib.h"
#include "apr_thread_mutex.h"
#include "apr_hash.h"
#include "apr_time.h"
#define APR_WANT_MEMFUNC
#include "apr_want.h"

#if APR_HAVE_STDLIB_H
#include <stdlib.h>     /* for malloc and free */
#endif

/*
 * Magic numbers
 */

#define MIN_ALLOC 8192
#define MAX_INDEX   20

#define BOUNDARY_INDEX 12
#define BOUNDARY_SIZE (1 << BOUNDARY_INDEX)

 
/*
 * Macros and defines
 */

/* APR_ALIGN() is only to be used to align on a power of 2 boundary */
#define APR_ALIGN(size, boundary) \
    (((size) + ((boundary) - 1)) & ~((boundary) - 1))

#define APR_ALIGN_DEFAULT(size) APR_ALIGN(size, 8)

/*
 * This option prints out the pool creation info
 * (and info about its children)
 * when the pool is destroyed.
 */
/*
#define APR_POOL_DEBUG_VERBOSE
*/
    
/*
 * Structures
 */

typedef struct cleanup_t cleanup_t;

#if !defined(APR_POOL_DEBUG)
typedef struct allocator_t allocator_t;
typedef struct node_t node_t;

struct node_t {
    node_t      *next;
    apr_uint32_t index;
    char        *first_avail;
    char        *endp;
};

struct allocator_t {
    apr_uint32_t        max_index;
#if APR_HAS_THREADS
    apr_thread_mutex_t *mutex;
#endif
    apr_pool_t         *owner;
    node_t             *free[MAX_INDEX];
};

#define SIZEOF_NODE_T       APR_ALIGN_DEFAULT(sizeof(node_t))
#define SIZEOF_ALLOCATOR_T  APR_ALIGN_DEFAULT(sizeof(allocator_t))

#else /* !defined(APR_POOL_DEBUG) */

typedef struct debug_node_t debug_node_t;

struct debug_node_t {
    debug_node_t *next;
    apr_uint32_t  index;
    void         *beginp[64];
    void         *endp[64];
};

#define SIZEOF_DEBUG_NODE_T APR_ALIGN_DEFAULT(sizeof(debug_node_t))

#endif /* !defined(APR_POOL_DEBUG) */

/* The ref field in the apr_pool_t struct holds a
 * pointer to the pointer referencing this pool.
 * It is used for parent, child, sibling management.
 * Look at apr_pool_create_ex() and apr_pool_destroy()
 * to see how it is used.
 */
struct apr_pool_t {
    apr_pool_t           *parent;
    apr_pool_t           *child;
    apr_pool_t           *sibling;
    apr_pool_t          **ref;
    cleanup_t            *cleanups;
    struct process_chain *subprocesses;
    apr_abortfunc_t       abort_fn;
    apr_hash_t           *user_data;
    const char           *tag;

#if !defined(APR_POOL_DEBUG)
    allocator_t          *allocator;
    node_t               *active;
    node_t               *self; /* The node containing the pool itself */
    char                 *self_first_avail;

#else /* !defined(APR_POOL_DEBUG) */
    debug_node_t         *nodes;
    const char           *file;
    int                   line;
#if APR_HAS_THREADS
    apr_thread_mutex_t *mutex;
#endif
#endif /* !defined(APR_POOL_DEBUG) */
};

#define SIZEOF_POOL_T       APR_ALIGN_DEFAULT(sizeof(apr_pool_t))


/*
 * Variables
 */

static apr_byte_t   apr_pools_initialized = 0;
static apr_pool_t  *global_pool = NULL;

#if !defined(APR_POOL_DEBUG)
static allocator_t  global_allocator = { 
    0,          /* max_index */
#if APR_HAS_THREADS
    NULL,       /* mutex */
#endif
    NULL,       /* owner */
    { NULL }    /* free[0] */
};
#endif /* !defined(APR_POOL_DEBUG) */


/*
 * Local functions
 */

static void run_cleanups(cleanup_t *c);
static void run_child_cleanups(cleanup_t *c);
static void free_proc_chain(struct process_chain *procs);


#if !defined(APR_POOL_DEBUG)
/*
 * Initialization
 */

APR_DECLARE(apr_status_t) apr_pool_initialize(void)
{
    apr_status_t rv;

    if (apr_pools_initialized++)
        return APR_SUCCESS;
    
    memset(&global_allocator, 0, SIZEOF_ALLOCATOR_T);

    if ((rv = apr_pool_create_ex(&global_pool, NULL, NULL, APR_POOL_FDEFAULT)) != APR_SUCCESS) {
        return rv;
    }
    
#if APR_HAS_THREADS    
    if ((rv = apr_thread_mutex_create(&global_allocator.mutex, 
                  APR_THREAD_MUTEX_DEFAULT, global_pool)) != APR_SUCCESS) {
        return rv;
    }
#endif

    global_allocator.owner = global_pool;
    apr_pools_initialized = 1;

    return APR_SUCCESS;
}

APR_DECLARE(void) apr_pool_terminate(void)
{
    if (!apr_pools_initialized)
        return;

    apr_pools_initialized = 0;
    
    apr_pool_destroy(global_pool); /* This will also destroy the mutex */
    global_pool = NULL;

    memset(&global_allocator, 0, SIZEOF_ALLOCATOR_T);
}


/*
 * Memory allocation
 */

static APR_INLINE node_t *node_malloc(allocator_t *allocator, apr_size_t size)
{
    node_t *node, **ref;
    apr_uint32_t i, index, max_index; 

    /* Round up the block size to the next boundary, but always
     * allocate at least a certain size (MIN_ALLOC).
     */
    size = APR_ALIGN(size + SIZEOF_NODE_T, BOUNDARY_SIZE);
    if (size < MIN_ALLOC)
        size = MIN_ALLOC;

    /* Find the index for this node size by
     * deviding its size by the boundary size
     */
    index = (size >> BOUNDARY_INDEX) - 1;

    /* First see if there are any nodes in the area we know
     * our node will fit into.
     */
    if (index <= allocator->max_index) {
#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_lock(allocator->mutex);
#endif
        
        /* Walk the free list to see if there are
         * any nodes on it of the requested size
         *
         * NOTE: an optimization would be to check
         * allocator->free[index] first and if no
         * node is present, directly use 
         * allocator->free[max_index].  This seems
         * like overkill though and could cause
         * memory waste.
         */
        max_index = allocator->max_index;
        ref = &allocator->free[index];
        i = index;
        while (*ref == NULL && i < max_index) {
           ref++;
           i++;
        }

        if ((node = *ref) != NULL) {
            /* If we have found a node and it doesn't have any
             * nodes waiting in line behind it _and_ we are on
             * the highest available index, find the new highest
             * available index
             */
            if ((*ref = node->next) == NULL && i >= max_index) {
                do {
                    ref--;
                    max_index--;
                }
                while (*ref == NULL && max_index > 0);

                allocator->max_index = max_index;
            }
            
            node->next = NULL;

#if APR_HAS_THREADS
            if (allocator->mutex)
                apr_thread_mutex_unlock(allocator->mutex);
#endif

            return node;
        }

#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_unlock(allocator->mutex);
#endif
    }

    /* If we found nothing, seek the sink (at index 0), if
     * it is not empty.
     */
    else if (allocator->free[0]) {
#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_lock(allocator->mutex);
#endif

        /* Walk the free list to see if there are
         * any nodes on it of the requested size
         */
        ref = &allocator->free[0];
        while ((node = *ref) != NULL && index > node->index)
            ref = &node->next;

        if (node) {
            *ref = node->next;
            node->next = NULL;
            
#if APR_HAS_THREADS
            if (allocator->mutex)
                apr_thread_mutex_unlock(allocator->mutex);
#endif

            return node;
        }
        
#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_unlock(allocator->mutex);
#endif
    }
    
    /* If we haven't got a suitable node, malloc a new one
     * and initialize it.
     */
    if ((node = malloc(size)) == NULL)
        return NULL;

    node->next = NULL;
    node->index = index;
    node->first_avail = (char *)node + SIZEOF_NODE_T;
    node->endp = (char *)node + size;

    return node;
}

static APR_INLINE void node_free(allocator_t *allocator, node_t *node)
{
    node_t *next;
    apr_uint32_t index, max_index;

#if APR_HAS_THREADS
    if (allocator->mutex)
        apr_thread_mutex_lock(allocator->mutex);
#endif

    max_index = allocator->max_index;

    /* Walk the list of submitted nodes and free them one by one,
     * shoving them in the right 'size' buckets as we go.
     */
    do {
        next = node->next;
        index = node->index;

        if (index < MAX_INDEX) {
            /* Add the node to the appropiate 'size' bucket.  Adjust
             * the max_index when appropiate.
             */
            if ((node->next = allocator->free[index]) == NULL && index > max_index) {
                 max_index = index;
            }
            allocator->free[index] = node;
        }
        else {
            /* This node is too large to keep in a specific size bucket,
             * just add it to the sink (at index 0).
             */
            node->next = allocator->free[0];
            allocator->free[0] = node;
        }
    }
    while ((node = next) != NULL);

    allocator->max_index = max_index;

#if APR_HAS_THREADS
    if (allocator->mutex)
        apr_thread_mutex_unlock(allocator->mutex);
#endif
}

APR_DECLARE(void *) apr_palloc(apr_pool_t *pool, apr_size_t size)
{
    node_t *active, *node;
    void *mem;
    char *endp;

    size = APR_ALIGN_DEFAULT(size);
    active = pool->active;

    /* If the active node has enough bytes left, use it. */
    endp = active->first_avail + size;
    if (endp < active->endp) {
        mem = active->first_avail;
        active->first_avail = endp;
        
        return mem;
    }

    /* Reset the active node, get ourselves a new one and activate it. */
    active->first_avail = (char *)active + SIZEOF_NODE_T;

    if ((node = node_malloc(pool->allocator, size)) == NULL) {
        active->first_avail = active->endp;

        if (pool->abort_fn)
            pool->abort_fn(APR_ENOMEM);

        return NULL;
    }

    active->next = pool->active = node; 

    mem = node->first_avail;
    node->first_avail += size;
    
    return mem;
}

APR_DECLARE(void *) apr_pcalloc(apr_pool_t *pool, apr_size_t size)
{
    node_t *active, *node;
    void *mem;
    char *endp;

    size = APR_ALIGN_DEFAULT(size);
    active = pool->active;

    /* If the active node has enough bytes left, use it. */
    endp = active->first_avail + size;
    if (endp < active->endp) {
        mem = active->first_avail;
        active->first_avail = endp;

        memset(mem, 0, size);
        
        return mem;
    }

    /* Reset the active node, get ourselves a new one and activate it. */
    active->first_avail = (char *)active + SIZEOF_NODE_T;

    if ((node = node_malloc(pool->allocator, size)) == NULL) {
        active->first_avail = active->endp;

        if (pool->abort_fn)
            pool->abort_fn(APR_ENOMEM);

        return NULL;
    }

    active->next = pool->active = node; 

    mem = node->first_avail;
    node->first_avail += size;
    
    memset(mem, 0, size);
    
    return mem;
}


/*
 * Pool creation/destruction
 */

APR_DECLARE(void) apr_pool_clear(apr_pool_t *pool)
{
    node_t *active;

    /* Destroy the subpools.  The subpools will detach themselves from 
     * this pool thus this loop is safe and easy.
     */
    while (pool->child)
        apr_pool_destroy(pool->child);

    /* Run cleanups */
    run_cleanups(pool->cleanups);
    pool->cleanups = NULL;

    /* Free subprocesses */
    free_proc_chain(pool->subprocesses);
    pool->subprocesses = NULL;

    /* Clear the user data. */
    pool->user_data = NULL;

    /* Reset the active node */
    if ((active = pool->active) == pool->self) {
        active->first_avail = pool->self_first_avail;
        return;
    }

    active->first_avail = (char *)active + SIZEOF_NODE_T;

    /* Find the node attached to the pool structure, make
     * it the active node and free the rest of the nodes.
     */
    active = pool->active = pool->self; 
    active->first_avail = pool->self_first_avail;
    node_free(pool->allocator, active->next);
    active->next = NULL;
}

APR_DECLARE(void) apr_pool_destroy(apr_pool_t *pool)
{
    node_t *node, *active, **ref;
    allocator_t *allocator;
    apr_uint32_t index;

    /* Destroy the subpools.  The subpools will detach themselve from 
     * this pool thus this loop is safe and easy.
     */
    while (pool->child)
        apr_pool_destroy(pool->child);

    /* Run cleanups */
    run_cleanups(pool->cleanups);

    /* Free subprocesses */
    free_proc_chain(pool->subprocesses);

    /* Remove the pool from the parents child list */
    if (pool->parent) {
#if APR_HAS_THREADS
        apr_thread_mutex_t *mutex;

        if ((mutex = pool->parent->allocator->mutex) != NULL)
            apr_thread_mutex_lock(mutex);
#endif

        if ((*pool->ref = pool->sibling) != NULL)
            pool->sibling->ref = pool->ref;

#if APR_HAS_THREADS
        if (mutex)
            apr_thread_mutex_unlock(mutex);
#endif
    }
    
    /* Reset the active block */
    active = pool->active;
    active->first_avail = (char *)active + SIZEOF_NODE_T;

    /* Find the block attached to the pool structure.  Save a copy of the
     * allocator pointer, because the pool struct soon will be no more.
     */
    allocator = pool->allocator;
    active = pool->self;
    active->first_avail = (char *)active + SIZEOF_NODE_T;

    /* If this pool happens to be the owner of the allocator, free 
     * everything in the allocator (that includes the pool struct
     * and the allocator).  Don't worry about destroying the optional mutex
     * in the allocator, it will have been destroyed by the cleanup function.
     */
    if (allocator->owner == pool) {
        for (index = 0; index < MAX_INDEX; index++) {
            ref = &allocator->free[index];
            while ((node = *ref) != NULL) {
                *ref = node->next;
                free(node);
            }
        }
            
        ref = &active;
        while ((node = *ref) != NULL) {
            *ref = node->next;
            free(node);
        }
            
        return;
    }

    /* Free all the nodes in the pool (including the node holding the
     * pool struct), by giving them back to the allocator.
     */
    node_free(allocator, active);
}

APR_DECLARE(apr_status_t) apr_pool_create_ex(apr_pool_t **newpool, 
                                             apr_pool_t *parent,
                                             apr_abortfunc_t abort_fn,
                                             apr_uint32_t flags)
{
    apr_pool_t *pool;
    node_t *node;
    allocator_t *allocator, *new_allocator;

    *newpool = NULL;

    if (!parent)
        parent = global_pool;

    if (!abort_fn && parent)
        abort_fn = parent->abort_fn;

    allocator = parent ? parent->allocator : &global_allocator;
    if ((node = node_malloc(allocator, MIN_ALLOC - SIZEOF_NODE_T)) == NULL) {
        if (abort_fn)
            abort_fn(APR_ENOMEM);

        return APR_ENOMEM;
    }

    if ((flags & APR_POOL_FNEW_ALLOCATOR) == APR_POOL_FNEW_ALLOCATOR) {
        new_allocator = (allocator_t *)node->first_avail;
        pool = (apr_pool_t *)((char *)new_allocator + SIZEOF_ALLOCATOR_T);
        node->first_avail = pool->self_first_avail = (char *)pool + SIZEOF_POOL_T;
        
        memset(new_allocator, 0, SIZEOF_ALLOCATOR_T);
        new_allocator->owner = pool;

        pool->allocator = new_allocator;
        pool->active = pool->self = node;
        pool->abort_fn = abort_fn;
        pool->child = NULL;
        pool->cleanups = NULL;
        pool->subprocesses = NULL;
        pool->user_data = NULL;
        pool->tag = NULL;

#if APR_HAS_THREADS
        if ((flags & APR_POOL_FLOCK) == APR_POOL_FLOCK) {
            apr_status_t rv;

            if ((rv = apr_thread_mutex_create(&allocator->mutex, 
                    APR_THREAD_MUTEX_DEFAULT, pool)) != APR_SUCCESS) {
                node_free(allocator, node);
                return rv;
            }
        }
#endif
    }
    else {
        pool = (apr_pool_t *)node->first_avail;
        node->first_avail = pool->self_first_avail = (char *)pool + SIZEOF_POOL_T;
    
        pool->allocator = allocator;
        pool->active = pool->self = node;
        pool->abort_fn = abort_fn;
        pool->child = NULL;
        pool->cleanups = NULL;
        pool->subprocesses = NULL;
        pool->user_data = NULL;
        pool->tag = NULL;
    }

    if ((pool->parent = parent) != NULL) {
#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_lock(allocator->mutex);
#endif
        if ((pool->sibling = parent->child) != NULL)
            pool->sibling->ref = &pool->sibling;

        parent->child = pool;
        pool->ref = &parent->child;

#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_unlock(allocator->mutex);
#endif
    }
    else {
        pool->sibling = NULL;
        pool->ref = NULL;
    }

    *newpool = pool;

    return APR_SUCCESS;
}


/*
 * "Print" functions
 */

/*
 * apr_psprintf is implemented by writing directly into the current
 * block of the pool, starting right at first_avail.  If there's
 * insufficient room, then a new block is allocated and the earlier
 * output is copied over.  The new block isn't linked into the pool
 * until all the output is done.
 *
 * Note that this is completely safe because nothing else can
 * allocate in this apr_pool_t while apr_psprintf is running.  alarms are
 * blocked, and the only thing outside of apr_pools.c that's invoked
 * is apr_vformatter -- which was purposefully written to be
 * self-contained with no callouts.
 */

struct psprintf_data {
    apr_vformatter_buff_t vbuff;
    node_t               *node;
    allocator_t          *allocator;
    apr_byte_t            got_a_new_node;
    node_t               *free;
};

static int psprintf_flush(apr_vformatter_buff_t *vbuff)
{
    struct psprintf_data *ps = (struct psprintf_data *)vbuff;
    node_t *node, *active;
    apr_size_t cur_len;
    char *strp;
    allocator_t *allocator;

    allocator = ps->allocator;
    node = ps->node;
    strp = ps->vbuff.curpos;
    cur_len = strp - node->first_avail;

    if ((active = node_malloc(allocator, cur_len << 1)) == NULL)
        return -1;

    memcpy(active->first_avail, node->first_avail, cur_len);

    /* Reset the previous active node */
    node->first_avail = (char *)node + SIZEOF_NODE_T;

    if (ps->got_a_new_node) {
        node->next = ps->free;
        ps->free = node; 
    }

    ps->node = active;
    ps->vbuff.curpos = active->first_avail + cur_len;
    ps->vbuff.endpos = active->endp - 1; /* Save a byte for NUL terminator */
    ps->got_a_new_node = 1;

    return 0;
}

APR_DECLARE(char *) apr_pvsprintf(apr_pool_t *pool, const char *fmt, va_list ap)
{
    struct psprintf_data ps;
    char *strp;
    apr_size_t size;
    node_t *active;

    ps.node = active = pool->active;
    ps.allocator = pool->allocator;
    ps.vbuff.curpos  = ps.node->first_avail;

    /* Save a byte for the NUL terminator */
    ps.vbuff.endpos = ps.node->endp - 1;
    ps.got_a_new_node = 0;
    ps.free = NULL;

    if (apr_vformatter(psprintf_flush, &ps.vbuff, fmt, ap) == -1) {
        if (pool->abort_fn)
            pool->abort_fn(APR_ENOMEM);

        return NULL;
    }

    strp = ps.vbuff.curpos;
    *strp++ = '\0';

    size = strp - ps.node->first_avail;
    size = APR_ALIGN_DEFAULT(size);
    strp = ps.node->first_avail;
    ps.node->first_avail += size;

    /* 
     * Link the node in if it's a new one 
     */
    if (ps.got_a_new_node) {
        active->next = pool->active = ps.node;
    }

    if (ps.free)
        node_free(ps.allocator, ps.free);

    return strp;
}


#else /* !defined(APR_POOL_DEBUG) */
/*
 * Initialization (debug)
 */

APR_DECLARE(apr_status_t) apr_pool_initialize(void)
{
    apr_status_t rv;

    if (apr_pools_initialized++)
        return APR_SUCCESS;
    
    if ((rv = apr_pool_create_ex(&global_pool, NULL, NULL, 
                  APR_POOL_FNEW_ALLOCATOR|APR_POOL_FLOCK)) != APR_SUCCESS) {
        return rv;
    }
    
    apr_pools_initialized = 1;

    return APR_SUCCESS;
}

APR_DECLARE(void) apr_pool_terminate(void)
{
    if (!apr_pools_initialized)
        return;

    apr_pools_initialized = 0;
    
    apr_pool_destroy(global_pool); /* This will also destroy the mutex */
    global_pool = NULL;
}


/*
 * Memory allocation (debug)
 */

APR_DECLARE(void *) apr_palloc(apr_pool_t *pool, apr_size_t size)
{
    debug_node_t *node;
    void *mem;

    if ((mem = malloc(size)) == NULL) {
        if (pool->abort_fn)
            pool->abort_fn(APR_ENOMEM);

        return NULL;
    }

    node = pool->nodes;
    if (node == NULL || node->index == 64) {
        if ((node = malloc(SIZEOF_DEBUG_NODE_T)) == NULL) {
            if (pool->abort_fn)
                pool->abort_fn(APR_ENOMEM);

            return NULL;
        }

        node->next = pool->nodes;
        pool->nodes = node;
        node->index = 0;
    }

    node->beginp[node->index] = mem;
    node->endp[node->index] = (char *)mem + size;
    node->index++;

    return mem;
}

/*
 * (debug)
 */
APR_DECLARE(void *) apr_pcalloc(apr_pool_t *pool, apr_size_t size)
{
    void *mem;
   
    mem = apr_palloc(pool, size);
    memset(mem, 0, size);

    return mem;
}


/*
 * Pool creation/destruction (debug)
 * TODO: printout a line if _VERBOSE is on
 */

APR_DECLARE(void) apr_pool_clear_dbg(apr_pool_t *pool,const char*file, int line)
{
    debug_node_t *node;
    apr_uint32_t index;

    /* Destroy the subpools.  The subpools will detach themselves from
     * this pool thus this loop is safe and easy.
     */
    while (pool->child)
        apr_pool_destroy_dbg(pool->child,file,line);

    /* Run cleanups */
    run_cleanups(pool->cleanups);
    pool->cleanups = NULL;

    /* Free subprocesses */
    free_proc_chain(pool->subprocesses);
    pool->subprocesses = NULL;

    /* Clear the user data. */
    pool->user_data = NULL;

    /* Free the blocks */
    while ((node = pool->nodes) != NULL) {
        pool->nodes = node->next;

        for (index = 0; index < node->index; index++)
            free(node->beginp[index]);

        free(node);
    }
}

/*
 * destroy (debug)
 */
APR_DECLARE(void) apr_pool_destroy_dbg(apr_pool_t *pool,const char*file, int line)
{
#if defined APR_POOL_DEBUG_VERBOSE
    apr_file_t *stderr_log = NULL;
    apr_pool_t *child;

    apr_file_open_stderr(&stderr_log,pool); /* XXX not sure about this one */
    if (stderr_log) {
        apr_file_printf(stderr_log,
            "DEBUG: %s:%d destroy pool tagged %s created %s:%d\n",
            file, line, pool->tag, pool->file, pool->line);
        child= pool->child;
        while (child) {
            apr_file_printf(stderr_log,
                "DEBUG:\tpool child tagged %s created %s:%d\n",
                child->tag, child->file, child->line);
            child = child->sibling;
        }
        apr_file_close(stderr_log);
    }
#endif

    apr_pool_clear_dbg(pool,file,line);

    /* Remove the pool from the parents child list */
    if (pool->parent) {
#if APR_HAS_THREADS
        apr_thread_mutex_t *mutex;

        if ((mutex = pool->parent->mutex) != NULL)
            apr_thread_mutex_lock(mutex);
#endif

        if ((*pool->ref = pool->sibling) != NULL)
            pool->sibling->ref = pool->ref;

#if APR_HAS_THREADS
        if (mutex)
            apr_thread_mutex_unlock(mutex);
#endif
    }

    /* Free the pool itself */
    free(pool);
}

/*
 * create (debug)
 * there is a macro which adds the file/line #
 */
APR_DECLARE(apr_status_t) apr_pool_create_ex_dbg(apr_pool_t **newpool, 
                                             apr_pool_t *parent,
                                             apr_abortfunc_t abort_fn,
                                             apr_uint32_t flags, 
                                             const char *file,
                                             int line)
{
    apr_pool_t *pool;

    *newpool = NULL;

    if (!parent)
        parent = global_pool;

    if (!abort_fn && parent)
        abort_fn = parent->abort_fn;

    if ((pool = malloc(SIZEOF_POOL_T)) == NULL) {
        if (abort_fn)
            abort_fn(APR_ENOMEM);

         return APR_ENOMEM;
    }

    memset(pool, 0, SIZEOF_POOL_T);
    
    pool->abort_fn = abort_fn;

    if ((flags & APR_POOL_FNEW_ALLOCATOR) == APR_POOL_FNEW_ALLOCATOR) {
#if APR_HAS_THREADS
        if ((flags & APR_POOL_FLOCK) == APR_POOL_FLOCK) {
            apr_status_t rv;

            if ((rv = apr_thread_mutex_create(&pool->mutex, 
                    APR_THREAD_MUTEX_DEFAULT, pool)) != APR_SUCCESS) {
                free(pool);
                return rv;
            }
        }
#endif
    }
    else {
#if APR_HAS_THREADS
        if (parent)
            pool->mutex = parent->mutex;
#endif
    }

    if ((pool->parent = parent) != NULL) {
#if APR_HAS_THREADS
        if (parent->mutex)
            apr_thread_mutex_lock(parent->mutex);
#endif
        if ((pool->sibling = parent->child) != NULL)
            pool->sibling->ref = &pool->sibling;

        parent->child = pool;
        pool->ref = &parent->child;

#if APR_HAS_THREADS
        if (parent->mutex)
            apr_thread_mutex_unlock(parent->mutex);
#endif
    }
    else {
        pool->sibling = NULL;
        pool->ref = NULL;
    }

    pool->file = file;
    pool->line = line;

    *newpool = pool;

    return APR_SUCCESS;
}


/*
 * "Print" functions (debug)
 */

struct psprintf_data {
    apr_vformatter_buff_t vbuff;
    char      *mem;
    apr_size_t size;
};

static int psprintf_flush(apr_vformatter_buff_t *vbuff)
{
    struct psprintf_data *ps = (struct psprintf_data *)vbuff;
    apr_size_t size;

    size = ps->vbuff.curpos - ps->mem;

    ps->size <<= 1;
    if ((ps->mem = realloc(ps->mem, ps->size)) == NULL)
        return -1;

    ps->vbuff.curpos = ps->mem + size;
    ps->vbuff.endpos = ps->mem + ps->size - 1;

    return 0;
}

APR_DECLARE(char *) apr_pvsprintf(apr_pool_t *pool, const char *fmt, va_list ap)
{
    struct psprintf_data ps;
    debug_node_t *node;

    ps.size = 64;
    ps.mem = malloc(ps.size);
    ps.vbuff.curpos  = ps.mem;

    /* Save a byte for the NUL terminator */
    ps.vbuff.endpos = ps.mem + ps.size - 1;

    if (apr_vformatter(psprintf_flush, &ps.vbuff, fmt, ap) == -1) {
        if (pool->abort_fn)
            pool->abort_fn(APR_ENOMEM);

        return NULL;
    }

    *ps.vbuff.curpos++ = '\0';

    /* 
     * Link the node in
     */
    node = pool->nodes;
    if (node == NULL || node->index == 64) {
        if ((node = malloc(SIZEOF_DEBUG_NODE_T)) == NULL) {
            if (pool->abort_fn)
                pool->abort_fn(APR_ENOMEM);

            return NULL;
        }

        node->next = pool->nodes;
        pool->nodes = node;
        node->index = 0;
    }

    node->beginp[node->index] = ps.mem;
    node->endp[node->index] = ps.mem + ps.size;
    node->index++;

    return ps.mem;
}


/*
 * Debug functions
 */

APR_DECLARE(void) apr_pool_join(apr_pool_t *p, apr_pool_t *sub)
{
}

static apr_pool_t *find_pool(apr_pool_t *pool, const void *mem)
{
    apr_pool_t *found;
    debug_node_t *node;
    apr_uint32_t index;

    while (pool) {
        node = pool->nodes;

        while (node) {
            for (index = 0; index < node->index; index++) {
                if (node->beginp[index] <= mem &&
                    node->endp[index] > mem)
                    return pool;
            }

            node = node->next;
        }

        if ((found = find_pool(pool->child, mem)) != NULL)
            return found;

        pool = pool->sibling;
    }

    return NULL;
}

APR_DECLARE(apr_pool_t *) apr_find_pool(const void *mem)
{
    return find_pool(global_pool, mem);
}

static apr_size_t pool_num_bytes(apr_pool_t *pool)
{
    apr_size_t size = 0;
    debug_node_t *node;
    apr_uint32_t index;

    node = pool->nodes;

    while (node) {
        for (index = 0; index < node->index; index++) {
            size += node->endp[index] - node->beginp[index];
        }

        node = node->next;
    }

    return size;
}

APR_DECLARE(apr_size_t) apr_pool_num_bytes(apr_pool_t *pool, int recurse)
{
    apr_size_t size;

    size = pool_num_bytes(pool);

    if (recurse) {
        pool = pool->child;
        
        while (pool) {
            size += apr_pool_num_bytes(pool, 1);

            pool = pool->sibling;
        }
    }

    return size;
}

APR_DECLARE(apr_size_t) apr_pool_free_blocks_num_bytes(void)
{
    /* This really doesn't apply with our current debug code, so: */
    return 0;
}

APR_DECLARE(void) apr_pool_lock(apr_pool_t *pool, int flag)
{
}
#endif /* !defined(APR_POOL_DEBUG) */


/*
 * "Print" functions (common)
 */

APR_DECLARE_NONSTD(char *) apr_psprintf(apr_pool_t *p, const char *fmt, ...)
{
    va_list ap;
    char *res;

    va_start(ap, fmt);
    res = apr_pvsprintf(p, fmt, ap);
    va_end(ap);
    return res;
}

/*
 * Pool Properties
 */

APR_DECLARE(void) apr_pool_set_abort(apr_abortfunc_t abort_fn,
                                     apr_pool_t *pool)
{
    pool->abort_fn = abort_fn;
}

APR_DECLARE(apr_abortfunc_t) apr_pool_get_abort(apr_pool_t *pool)
{
    return pool->abort_fn;
}

APR_DECLARE(apr_pool_t *) apr_pool_get_parent(apr_pool_t *pool)
{
    return pool->parent;
}

/* return TRUE if a is an ancestor of b
 * NULL is considered an ancestor of all pools
 */
APR_DECLARE(int) apr_pool_is_ancestor(apr_pool_t *a, apr_pool_t *b)
{
    if (a == NULL)
        return 1;

    while (b) {
        if (a == b)
            return 1;

        b = b->parent;
    }

    return 0;
}

APR_DECLARE(void) apr_pool_tag(apr_pool_t *pool, const char *tag)
{
    pool->tag = tag;
}


/*
 * User data management
 */

APR_DECLARE(apr_status_t) apr_pool_userdata_set(const void *data, const char *key,
                                                apr_status_t (*cleanup) (void *),
                                                apr_pool_t *pool)
{
    if (pool->user_data == NULL)
        pool->user_data = apr_hash_make(pool);

    if (apr_hash_get(pool->user_data, key, APR_HASH_KEY_STRING) == NULL) {
        char *new_key = apr_pstrdup(pool, key);
        apr_hash_set(pool->user_data, new_key, APR_HASH_KEY_STRING, data);
    } 
    else {
        apr_hash_set(pool->user_data, key, APR_HASH_KEY_STRING, data);
    }

    if (cleanup)
        apr_pool_cleanup_register(pool, data, cleanup, cleanup);
        
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pool_userdata_setn(const void *data, const char *key,
                                                 apr_status_t (*cleanup) (void *),
                                                 apr_pool_t *pool)
{
    if (pool->user_data == NULL)
        pool->user_data = apr_hash_make(pool);

    apr_hash_set(pool->user_data, key, APR_HASH_KEY_STRING, data);

    if (cleanup)
        apr_pool_cleanup_register(pool, data, cleanup, cleanup);
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_pool_userdata_get(void **data, const char *key, apr_pool_t *pool)
{
    if (pool->user_data == NULL)
        *data = NULL;
    else
        *data = apr_hash_get(pool->user_data, key, APR_HASH_KEY_STRING);

    return APR_SUCCESS;
}


/*
 * Cleanup
 */

struct cleanup_t {
    struct cleanup_t *next;
    const void *data;
    apr_status_t (*plain_cleanup_fn)(void *data);
    apr_status_t (*child_cleanup_fn)(void *data);
};

APR_DECLARE(void) apr_pool_cleanup_register(apr_pool_t *p, const void *data,
                      apr_status_t (*plain_cleanup_fn)(void *data),
                      apr_status_t (*child_cleanup_fn)(void *data))
{
    cleanup_t *c;

    if (p != NULL) {
        c = (cleanup_t *) apr_palloc(p, sizeof(cleanup_t));
        c->data = data;
        c->plain_cleanup_fn = plain_cleanup_fn;
        c->child_cleanup_fn = child_cleanup_fn;
        c->next = p->cleanups;
        p->cleanups = c;
    }
}

APR_DECLARE(void) apr_pool_cleanup_kill(apr_pool_t *p, const void *data,
                    apr_status_t (*cleanup_fn)(void *))
{
    cleanup_t *c, **lastp;

    if (p == NULL)
        return;

    c = p->cleanups;
    lastp = &p->cleanups;
    while (c) {
        if (c->data == data && c->plain_cleanup_fn == cleanup_fn) {
            *lastp = c->next;
            break;
        }

        lastp = &c->next;
        c = c->next;
    }
}

APR_DECLARE(void) apr_pool_child_cleanup_set(apr_pool_t *p, const void *data,
                                       apr_status_t (*plain_cleanup_fn) (void *),
                                       apr_status_t (*child_cleanup_fn) (void *))
{
    cleanup_t *c;

    if (p == NULL)
        return;

    c = p->cleanups;
    while (c) {
        if (c->data == data && c->plain_cleanup_fn == plain_cleanup_fn) {
            c->child_cleanup_fn = child_cleanup_fn;
            break;
        }

        c = c->next;
    }
}

APR_DECLARE(apr_status_t) apr_pool_cleanup_run(apr_pool_t *p, void *data,
                                       apr_status_t (*cleanup_fn) (void *))
{
    apr_pool_cleanup_kill(p, data, cleanup_fn);
    return (*cleanup_fn)(data);
}

static void run_cleanups(cleanup_t *c)
{
    while (c) {
        (*c->plain_cleanup_fn)((void *)c->data);
        c = c->next;
    }
}

static void run_child_cleanups(cleanup_t *c)
{
    while (c) {
        (*c->child_cleanup_fn)((void *)c->data);
        c = c->next;
    }
}

static void cleanup_pool_for_exec(apr_pool_t *p)
{
    run_child_cleanups(p->cleanups);
    p->cleanups = NULL;

    for (p = p->child; p; p = p->sibling)
        cleanup_pool_for_exec(p);
}

APR_DECLARE(void) apr_pool_cleanup_for_exec(void)
{
#if !defined(WIN32) && !defined(OS2)
    /*
     * Don't need to do anything on NT or OS/2, because I
     * am actually going to spawn the new process - not
     * exec it. All handles that are not inheritable, will
     * be automajically closed. The only problem is with
     * file handles that are open, but there isn't much
     * I can do about that (except if the child decides
     * to go out and close them
     */
    cleanup_pool_for_exec(global_pool);
#endif /* !defined(WIN32) && !defined(OS2) */
}

APR_DECLARE_NONSTD(apr_status_t) apr_pool_cleanup_null(void *data)
{
    /* do nothing cleanup routine */
    return APR_SUCCESS;
}

/* Subprocesses don't use the generic cleanup interface because
 * we don't want multiple subprocesses to result in multiple
 * three-second pauses; the subprocesses have to be "freed" all
 * at once.  If other resources are introduced with the same property,
 * we might want to fold support for that into the generic interface.
 * For now, it's a special case.
 */
APR_DECLARE(void) apr_pool_note_subprocess(apr_pool_t *pool, apr_proc_t *pid,
                                    enum kill_conditions how)
{
    struct process_chain *pc = apr_palloc(pool, sizeof(struct process_chain));

    pc->pid = pid;
    pc->kill_how = how;
    pc->next = pool->subprocesses;
    pool->subprocesses = pc;
}

static void free_proc_chain(struct process_chain *procs)
{
    /* Dispose of the subprocesses we've spawned off in the course of
     * whatever it was we're cleaning up now.  This may involve killing
     * some of them off...
     */
    struct process_chain *pc;
    int need_timeout = 0;

    if (!procs)
        return; /* No work.  Whew! */

    /* First, check to see if we need to do the SIGTERM, sleep, SIGKILL
     * dance with any of the processes we're cleaning up.  If we've got
     * any kill-on-sight subprocesses, ditch them now as well, so they
     * don't waste any more cycles doing whatever it is that they shouldn't
     * be doing anymore.
     */

#ifndef NEED_WAITPID
    /* Pick up all defunct processes */
    for (pc = procs; pc; pc = pc->next) {
        if (apr_proc_wait(pc->pid, NULL, NULL, APR_NOWAIT) != APR_CHILD_NOTDONE)
            pc->kill_how = kill_never;
    }
#endif

    for (pc = procs; pc; pc = pc->next) {
        if ((pc->kill_how == kill_after_timeout) ||
            (pc->kill_how == kill_only_once)) {
            /*
             * Subprocess may be dead already.  Only need the timeout if not.
             * Note: apr_proc_kill on Windows is TerminateProcess(), which is 
             * similar to a SIGKILL, so always give the process a timeout
             * under Windows before killing it.
             */
#ifdef WIN32
            need_timeout = 1;
#else
            if (apr_proc_kill(pc->pid, SIGTERM) == APR_SUCCESS)
                need_timeout = 1;
#endif
        }
        else if (pc->kill_how == kill_always) {
            apr_proc_kill(pc->pid, SIGKILL);
        }
    }

    /* Sleep only if we have to... */
    if (need_timeout)
        apr_sleep(3 * APR_USEC_PER_SEC);

    /* OK, the scripts we just timed out for have had a chance to clean up
     * --- now, just get rid of them, and also clean up the system accounting
     * goop...
     */
    for (pc = procs; pc; pc = pc->next) {
        if (pc->kill_how == kill_after_timeout)
            apr_proc_kill(pc->pid, SIGKILL);
    }

    /* Now wait for all the signaled processes to die */
    for (pc = procs; pc; pc = pc->next) {
        if (pc->kill_how != kill_never)
            (void)apr_proc_wait(pc->pid, NULL, NULL, APR_WAIT);
    }

#ifdef WIN32
    /* 
     * XXX: Do we need an APR function to clean-up a proc_t?
     * Well ... yeah ... but we can't since it's scope is ill defined.
     * We can't dismiss the handle until the apr_proc_wait above is
     * finished with the proc_t.
     */
    {
        for (pc = procs; pc; pc = pc->next) {
            if (pc->pid->hproc) {
                CloseHandle(pc->pid->hproc);
                pc->pid->hproc = NULL;
            }
        }
    }
#endif /* WIN32 */
}
