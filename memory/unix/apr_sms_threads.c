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
#include "apr_general.h"
#include "apr_private.h"
#include "apr_sms.h"
#include "apr_sms_threads.h"
#include "apr_lock.h"
#include "sms_private.h"

#if APR_HAS_THREADS
 
static const char *module_identity = "THREADS";
static const char *module_acct_identity = "THREADS_ACCT";

/*
 * Simple thread memory system
 */

/* INTERNALLY USED STRUCTURES */

typedef struct block_t
{
    struct node_t  *node;
} block_t;

typedef struct node_t
{
    struct node_t  *next;
    struct node_t  *prev;
    char           *first_avail;
    apr_size_t      avail_size;
    apr_uint16_t    count;
} node_t;

typedef struct thread_node_t
{
    struct thread_node_t  *next;
    struct thread_node_t **ref;
    apr_os_thread_t        thread;
    node_t                 used_sentinel;
    node_t                 free_sentinel;
    node_t                *self;
    apr_size_t             max_free;
} thread_node_t;

/*
 * Primes just below a power of 2:
 * 3, 7, 13, 31, 61, 127, 251, 509, 1021,
 * 2039, 4093, 8191, 16381, 32749, 65521
 */

#define HASH_SIZE 1021
#define SIZEOF_HASHTABLE APR_ALIGN_DEFAULT(HASH_SIZE)
#define THREAD_HASH(tid) \
(*(int *)&(tid) % HASH_SIZE)

typedef struct apr_sms_threads_t
{
    apr_sms_t            sms_hdr;
    node_t               used_sentinel;
    node_t               free_sentinel;
    node_t              *self;
    char                *first_avail;
    thread_node_t       *hashtable[SIZEOF_HASHTABLE];
    apr_size_t           min_free;
    apr_size_t           min_alloc;
    apr_size_t           max_free;
    apr_size_t           thread_max_free;
    apr_lock_t          *lock;
} apr_sms_threads_t;

typedef struct apr_sms_threads_acct_t
{
    apr_sms_t            sms_hdr;
    apr_sms_threads_t   *tms;
} apr_sms_threads_acct_t;

#define SIZEOF_BLOCK_T       APR_ALIGN_DEFAULT(sizeof(block_t))
#define SIZEOF_NODE_T        APR_ALIGN_DEFAULT(sizeof(node_t))
#define SIZEOF_THREAD_NODE_T APR_ALIGN_DEFAULT(sizeof(thread_node_t))
#define SIZEOF_SMS_THREADS_T APR_ALIGN_DEFAULT(sizeof(apr_sms_threads_t))
#define SIZEOF_SMS_ACCT_T    APR_ALIGN_DEFAULT(sizeof(apr_sms_threads_acct_t))

#define BLOCK_T(mem)         ((block_t *)(mem))
#define NODE_T(mem)          ((node_t *)(mem))
#define THREAD_NODE_T(mem)   ((thread_node_t *)(mem))
#define SMS_THREADS_T(sms)   ((apr_sms_threads_t *)(sms))
#define SMS_ACCT_T(sms)      ((apr_sms_threads_acct_t *)(sms))

/* Magic numbers :) */

#define MIN_ALLOC       0x2000
#define MIN_FREE        0x1000
#define MAX_FREE        0x100000
#define THREAD_MAX_FREE 0x80000

static void *apr_sms_threads_malloc(apr_sms_t *sms,
                                    apr_size_t size)
{
    thread_node_t *thread_node;
    apr_os_thread_t thread;
    node_t *node, *sentinel;
    apr_size_t node_size;
    apr_uint16_t hash;
    void *mem;

    /* Round up the size to the next 8 byte boundary */
    size = APR_ALIGN_DEFAULT(size) + SIZEOF_BLOCK_T;

    thread = apr_os_thread_current();
    hash = THREAD_HASH(thread);
   
    /* If the thread wasn't registered before, we will segfault */ 
    thread_node = SMS_THREADS_T(sms)->hashtable[hash];
    while (thread_node->thread != thread)
        thread_node = thread_node->next;
   
    node = thread_node->used_sentinel.prev;

    if (node->avail_size >= size) {
        mem = node->first_avail;
        node->avail_size -= size;
        node->first_avail += size;
        node->count++;

        BLOCK_T(mem)->node = node;
        mem = (char *)mem + SIZEOF_BLOCK_T;

        return mem;
    }

    /* reset the 'last' block, it will be replaced soon */
    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;

    /* browse the free list for a useable block */
    sentinel = &thread_node->free_sentinel;
    sentinel->avail_size = size;

    node = sentinel->next;
    while (node->avail_size < size)
        node = node->next;

    if (node != sentinel) {
        node->prev->next = node->next;
        node->next->prev = node->prev;

        sentinel = &thread_node->used_sentinel;
        node->prev = sentinel->prev;
        node->prev->next = node;
        node->next = sentinel;
        sentinel->prev = node;
        
        if (node != thread_node->self)
            thread_node->max_free += node->avail_size;
        
        mem = node->first_avail;
        node->avail_size  -= size;
        node->first_avail += size;
        node->count = 1;

        BLOCK_T(mem)->node = node;
        mem = (char *)mem + SIZEOF_BLOCK_T;

        return mem;
    }
    
    if (SMS_THREADS_T(sms)->lock)
        apr_lock_acquire(SMS_THREADS_T(sms)->lock);
    
    /* browse the shared free list for a useable block */
    sentinel = &SMS_THREADS_T(sms)->free_sentinel;
    sentinel->avail_size = size;

    node = sentinel->next;
    while (node->avail_size < size)
        node = node->next;

    if (node != sentinel) {
        node->prev->next = node->next;
        node->next->prev = node->prev;

        if (SMS_THREADS_T(sms)->lock)
            apr_lock_release(SMS_THREADS_T(sms)->lock);

        sentinel = &thread_node->used_sentinel;
        node->prev = sentinel->prev;
        node->prev->next = node;
        node->next = sentinel;
        sentinel->prev = node;
       
        SMS_THREADS_T(sms)->max_free += node->avail_size;
        
        mem = node->first_avail;
        node->avail_size  -= size;
        node->first_avail += size;
        node->count = 1;

        BLOCK_T(mem)->node = node;
        mem = (char *)mem + SIZEOF_BLOCK_T;

        return mem;
    }
    
    if (SMS_THREADS_T(sms)->lock)
        apr_lock_release(SMS_THREADS_T(sms)->lock);
    
    /* we have to allocate a new block from our parent */
    node_size = size + SMS_THREADS_T(sms)->min_free;
    if (node_size < SMS_THREADS_T(sms)->min_alloc)
        node_size = SMS_THREADS_T(sms)->min_alloc;
    
    node = apr_sms_malloc(sms->parent, node_size);
    if (!node) {
        /* restore the 'last' node to prevent the
         * next allocation from segfaulting
         */
        node = thread_node->used_sentinel.prev;
        node->first_avail += node->avail_size;
        node->avail_size = 0;
        
        return NULL;
    }

    sentinel = &thread_node->used_sentinel;
    node->prev = sentinel->prev;
    node->prev->next = node;
    node->next = sentinel;
    sentinel->prev = node;
    
    mem = node->first_avail = (char *)node + SIZEOF_NODE_T;
    node->first_avail += size;
    node->avail_size = node_size - (node->first_avail - (char *)node);
    node->count = 1;

    BLOCK_T(mem)->node = node;
    mem = (char *)mem + SIZEOF_BLOCK_T;
    
    return mem;
}

static apr_status_t apr_sms_threads_free(apr_sms_t *sms, void *mem)
{
    node_t *node, *sentinel;
    thread_node_t *thread_node;
    apr_os_thread_t thread;
    apr_uint16_t hash;

    node = BLOCK_T((char *)mem - SIZEOF_BLOCK_T)->node;

    node->count--;
    if (node->count)
        return APR_SUCCESS;

    thread = apr_os_thread_current();
    hash = THREAD_HASH(thread);
    
    thread_node = SMS_THREADS_T(sms)->hashtable[hash];
    while (thread_node->thread != thread)
        thread_node = thread_node->next;

    node->avail_size += node->first_avail - 
                        ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;
   
    if (thread_node->used_sentinel.prev == node)
        return APR_SUCCESS;

    node->next->prev = node->prev;
    node->prev->next = node->next;

    if (thread_node->max_free >= node->avail_size ||
        node == thread_node->self) {
        sentinel = &thread_node->free_sentinel; 
        node->prev = sentinel->prev;
        node->prev->next = node;
        node->next = sentinel;
        sentinel->prev = node;

        if (node != thread_node->self)
            thread_node->max_free -= node->avail_size;
        
        return APR_SUCCESS;
    }

    if (sms->parent->free_fn && 
        node->avail_size > SMS_THREADS_T(sms)->max_free)
        return apr_sms_free(sms->parent, node);

    if (SMS_THREADS_T(sms)->lock)
        apr_lock_acquire(SMS_THREADS_T(sms)->lock);
    
    sentinel = &SMS_THREADS_T(sms)->free_sentinel;
    node->prev = sentinel->prev;
    node->prev->next = node;
    node->next = sentinel;
    sentinel->prev = node;

    SMS_THREADS_T(sms)->max_free -= node->avail_size;
   
    if (SMS_THREADS_T(sms)->lock)
        apr_lock_release(SMS_THREADS_T(sms)->lock);
    
    return APR_SUCCESS;
}

static apr_status_t apr_sms_threads_thread_register(apr_sms_t *sms,
                                                    apr_os_thread_t thread);

static apr_status_t apr_sms_threads_reset(apr_sms_t *sms)
{
    node_t *node, *prev, *used_sentinel, *free_sentinel, *free_list;
    thread_node_t *thread_node;
    apr_uint16_t hash;
    apr_size_t min_alloc, max_free;

    used_sentinel = &SMS_THREADS_T(sms)->used_sentinel;
    free_sentinel = &SMS_THREADS_T(sms)->free_sentinel;

    free_list = NULL;
    
    if (SMS_THREADS_T(sms)->lock)
        apr_lock_acquire(SMS_THREADS_T(sms)->lock);
    
    /* Actually, the thread creation function should have installed
     * a cleanup which effectively kills all the threads created using
     * this sms. Therefor it is not wise to call reset from another
     * thread than the 'master' thread.
     */
    for (hash = 0; hash < HASH_SIZE; hash++) {
        while ((thread_node = SMS_THREADS_T(sms)->hashtable[hash]) != NULL) {
            if ((*thread_node->ref = thread_node->next) != NULL)
                thread_node->next->ref = thread_node->ref;

            node = thread_node->self;
            node->prev->next = node->next;
            node->next->prev = node->prev;
            node->avail_size += node->first_avail - 
                                ((char *)node + SIZEOF_NODE_T);

            node = thread_node->used_sentinel.prev;
            node->avail_size += node->first_avail -
                                ((char *)node + SIZEOF_NODE_T);
            node->first_avail = (char *)node + SIZEOF_NODE_T;
            node->next = thread_node->free_sentinel.next;
            node->next->prev = node;

            node = thread_node->free_sentinel.prev;
            node->next = used_sentinel->next;
            node->next->prev = node;

            used_sentinel->next = thread_node->used_sentinel.next;
            used_sentinel->next->prev = used_sentinel;

            node = NODE_T(thread_node);
            node->avail_size = thread_node->self->avail_size + 
                               SIZEOF_THREAD_NODE_T;
            node->first_avail = (char *)node + SIZEOF_NODE_T;

            node->next = used_sentinel->next;
            node->next->prev = node;
            used_sentinel->next = node;
            node->prev = used_sentinel;
        }
    }
    
    node = SMS_THREADS_T(sms)->self;
    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;
    node->count = 0;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    
    node = used_sentinel->prev;
    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;

    if (sms->parent->free_fn) {
        min_alloc = SMS_THREADS_T(sms)->min_alloc;
        max_free  = SMS_THREADS_T(sms)->max_free;
        used_sentinel->avail_size = min_alloc;
        while (max_free > min_alloc) {
            if (node->avail_size <= max_free) {
                if (node == used_sentinel)
                    break;

                max_free -= node->avail_size;
                node->prev->next = node->next;
                node->next->prev = node->prev;
                
                prev = node->prev;
                
                node->next = free_sentinel->next;
                free_sentinel->next = node;
                node->next->prev = node;
                node->prev = free_sentinel;
                
                node = prev;
            }
            else
                node = node->prev;
        }
        SMS_THREADS_T(sms)->max_free = max_free;

        used_sentinel->prev->next = NULL;
        free_list = used_sentinel->next;
    }
    else {
        node = used_sentinel->prev;
        node->next = free_sentinel->next;
        node->next->prev = node;

        node = used_sentinel->next;
        node->prev = free_sentinel;
        free_sentinel->next = node;
    }
    
    node = SMS_THREADS_T(sms)->self;
    node->next = node->prev = used_sentinel;
    used_sentinel->next = used_sentinel->prev = node;

    sms->accounting = (apr_sms_t *)((char *)sms + SIZEOF_SMS_THREADS_T);
    
    apr_sms_threads_thread_register(sms, apr_os_thread_current());

    if (SMS_THREADS_T(sms)->lock)
        apr_lock_release(SMS_THREADS_T(sms)->lock);

    while ((node = free_list) != NULL) {
        free_list = node->next;
        apr_sms_free(sms->parent, node);
    }
    
    return APR_SUCCESS;    
}

static apr_status_t apr_sms_threads_pre_destroy(apr_sms_t *sms)
{
    /* This function WILL always be called.  However, be aware that the
     * main sms destroy function knows that it's not wise to try and destroy
     * the same piece of memory twice, so the destroy function in a child won't
     * neccesarily be called.  To guarantee we destroy the lock it's therefore
     * destroyed here.
     */
        
    if (SMS_THREADS_T(sms)->lock) {
        apr_lock_acquire(SMS_THREADS_T(sms)->lock);
        apr_lock_destroy(SMS_THREADS_T(sms)->lock);
        SMS_THREADS_T(sms)->lock = NULL;
    }
    
    return APR_SUCCESS;    
}

static apr_status_t apr_sms_threads_destroy(apr_sms_t *sms)
{
    node_t *node, *next, *used_sentinel, *free_sentinel;
    thread_node_t *thread_node;
    apr_uint16_t hash;

    for (hash = 0; hash < HASH_SIZE; hash++) {
        while ((thread_node = SMS_THREADS_T(sms)->hashtable[hash]) != NULL) {
            if ((*thread_node->ref = thread_node->next) != NULL)
                thread_node->next->ref = thread_node->ref;

            node = thread_node->self;
            node->prev->next = node->next;
            node->next->prev = node->prev;

            used_sentinel = &thread_node->used_sentinel;
            free_sentinel = &thread_node->free_sentinel;
            
            free_sentinel->prev->next = NULL;
            used_sentinel->prev->next = free_sentinel->next;

            node = used_sentinel->next;
            while (node) {
                next = node->next;
                apr_sms_free(sms->parent, node);
                node = next;
            }

            apr_sms_free(sms->parent, thread_node);
        }
    }

    node = SMS_THREADS_T(sms)->self;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    
    used_sentinel = &SMS_THREADS_T(sms)->used_sentinel;
    free_sentinel = &SMS_THREADS_T(sms)->free_sentinel;

    free_sentinel->prev->next = NULL;
    used_sentinel->prev->next = free_sentinel->next;

    node = used_sentinel->next;
    while (node) {
        next = node->next;
        apr_sms_free(sms->parent, node);
        node = next;
    }

    return apr_sms_free(sms->parent, sms);
}

static apr_status_t apr_sms_threads_thread_register(apr_sms_t *sms,
                                                    apr_os_thread_t thread)
{
    thread_node_t *thread_node;
    node_t *node, *sentinel;
    apr_uint16_t hash;

    hash = THREAD_HASH(thread);
    
    if (sms->threads > 1 && !SMS_THREADS_T(sms)->lock) {
        apr_lock_create(&SMS_THREADS_T(sms)->lock,
                        APR_MUTEX, APR_LOCKALL,
                        NULL, sms->pool);
    }
    
    if (SMS_THREADS_T(sms)->lock)
        apr_lock_acquire(SMS_THREADS_T(sms)->lock);

    sentinel = &SMS_THREADS_T(sms)->free_sentinel;
    if ((node = sentinel->next) != sentinel) {
        node->prev->next = node->next;
        node->next->prev = node->prev;

        thread_node = THREAD_NODE_T(node);
        node = NODE_T((char *)thread_node + SIZEOF_THREAD_NODE_T);
        node->avail_size = NODE_T(thread_node)->avail_size - 
                           SIZEOF_THREAD_NODE_T;
        node->first_avail = (char *)node + SIZEOF_NODE_T;
    }
    else {
        thread_node = apr_sms_malloc(sms->parent, SMS_THREADS_T(sms)->min_alloc);
        if (!thread_node) {
            if (SMS_THREADS_T(sms)->lock)
                apr_lock_release(SMS_THREADS_T(sms)->lock);

            return APR_ENOMEM;
        }

        node = NODE_T((char *)thread_node + SIZEOF_THREAD_NODE_T);
        node->first_avail = (char *)node + SIZEOF_NODE_T;
        node->avail_size = SMS_THREADS_T(sms)->min_alloc -
                           (node->first_avail - (char *)thread_node);
    }
    node->count = 0;
    
    thread_node->self = node;
    thread_node->max_free = SMS_THREADS_T(sms)->thread_max_free;

    sentinel = &thread_node->used_sentinel;
    node->prev = node->next = sentinel;
    sentinel->prev = sentinel->next = node;

    sentinel = &thread_node->free_sentinel;
    sentinel->prev = sentinel->next = sentinel;

    thread_node->thread = thread;
    if ((thread_node->next = SMS_THREADS_T(sms)->hashtable[hash]) != NULL)
        thread_node->next->ref = &thread_node->next;
    thread_node->ref = &SMS_THREADS_T(sms)->hashtable[hash];
    SMS_THREADS_T(sms)->hashtable[hash] = thread_node;

    if (SMS_THREADS_T(sms)->lock)
        apr_lock_release(SMS_THREADS_T(sms)->lock);

    return APR_SUCCESS;
}

static apr_status_t apr_sms_threads_thread_unregister(apr_sms_t *sms,
                                                      apr_os_thread_t thread)
{
    thread_node_t *thread_node;
    node_t *node, *prev, *free_list, *used_sentinel, *free_sentinel;
    apr_uint16_t hash;
    apr_size_t min_alloc, max_free;

    hash = THREAD_HASH(thread);
    free_list = NULL;
    
    if (SMS_THREADS_T(sms)->lock)
        apr_lock_acquire(SMS_THREADS_T(sms)->lock);

    thread_node = SMS_THREADS_T(sms)->hashtable[hash];
    while (thread_node->thread != thread)
        thread_node = thread_node->next;

    if ((*thread_node->ref = thread_node->next) != NULL)
        thread_node->next->ref = thread_node->ref;

    node = thread_node->self;
    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->prev->next = node->next;
    node->next->prev = node->prev;
    
    used_sentinel = &thread_node->used_sentinel;
    free_sentinel = &SMS_THREADS_T(sms)->free_sentinel;
    
    node = used_sentinel->prev;
    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;
   
    node->next = thread_node->free_sentinel.next;
    node->next->prev = node;

    node = thread_node->free_sentinel.prev;
    node->next = used_sentinel;
    used_sentinel->prev = node;

    min_alloc = SMS_THREADS_T(sms)->min_alloc;
    max_free  = SMS_THREADS_T(sms)->max_free;
    used_sentinel->avail_size = min_alloc;
    while (max_free > min_alloc) {
        if (node->avail_size <= max_free) {
            if (node == used_sentinel)
                break;
    
            max_free -= node->avail_size;
            node->prev->next = node->next;
            node->next->prev = node->prev;
            prev = node->prev;

            node->next = free_sentinel->next;
            free_sentinel->next = node;
            node->next->prev = node;
            node->prev = free_sentinel;
            
            node = prev;
        }
        else
            node = node->prev;
    }
                            
    used_sentinel->prev->next = NULL;
    free_list = used_sentinel->next;
                    
    node = NODE_T(thread_node);
    node->avail_size = thread_node->self->avail_size + 
                       SIZEOF_THREAD_NODE_T;
    node->first_avail = (char *)node + SIZEOF_NODE_T;

    if (max_free >= node->avail_size) {
        max_free -= node->avail_size;

        node->next = free_sentinel->next;
        node->next->prev = node;
        free_sentinel->next = node;
        node->prev = free_sentinel;
    }
    else {
        node->next = free_list;
        free_list = node;
    }
    
    SMS_THREADS_T(sms)->max_free = max_free;
    
    if (SMS_THREADS_T(sms)->lock)
        apr_lock_release(SMS_THREADS_T(sms)->lock);

    while ((node = free_list) != NULL) {
        free_list = node->next;
        apr_sms_free(sms->parent, node);
    }
    
    return APR_SUCCESS;
}

static void *apr_sms_threads_acct_malloc(apr_sms_t *sms, apr_size_t size)
{
    apr_sms_threads_t *tms;
    node_t *node, *sentinel;
    apr_size_t node_size;
    void *mem;

    tms = SMS_ACCT_T(sms)->tms;

    /* Round up the size to the next 8 byte boundary */
    size = APR_ALIGN_DEFAULT(size) + SIZEOF_BLOCK_T;

    if (tms->lock)
        apr_lock_acquire(tms->lock);
    
    node = tms->used_sentinel.prev;

    if (node->avail_size >= size) {
        mem = node->first_avail;
        node->avail_size -= size;
        node->first_avail += size;
        node->count++;

        if (tms->lock)
            apr_lock_release(tms->lock);
    
        BLOCK_T(mem)->node = node;
        mem = (char *)mem + SIZEOF_BLOCK_T;

        return mem;
    }

    /* reset the 'last' block, it will be replaced soon */
    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;

    /* browse the free list for a useable block */
    sentinel = &tms->free_sentinel;
    sentinel->avail_size = size;

    node = sentinel->next;
    while (node->avail_size < size)
        node = node->next;

    if (node != sentinel) {
        node->prev->next = node->next;
        node->next->prev = node->prev;

        sentinel = &tms->used_sentinel;
        node->prev = sentinel->prev;
        node->prev->next = node;
        node->next = sentinel;
        sentinel->prev = node;
        
        if (node != tms->self)
            tms->max_free += node->avail_size;

        mem = node->first_avail;
        node->avail_size  -= size;
        node->first_avail += size;
        node->count = 1;

        if (tms->lock)
            apr_lock_release(tms->lock);

        BLOCK_T(mem)->node = node;
        mem = (char *)mem + SIZEOF_BLOCK_T;

        return mem;
    }
    
    /* we have to allocate a new block from our parent */
    node_size = size + tms->min_free;
    if (node_size < tms->min_alloc)
        node_size = tms->min_alloc;
    
    node = apr_sms_malloc(sms->parent, node_size);
    if (!node) {
        /* restore the 'last' node, so the next allocation
         * will not segfault
         */
        node = tms->used_sentinel.prev;
        node->first_avail += node->avail_size;
        node->avail_size = 0;

        if (tms->lock)
            apr_lock_release(tms->lock);
        
        return NULL;
    }

    sentinel = &tms->used_sentinel;
    node->prev = sentinel->prev;
    node->prev->next = node;
    node->next = sentinel;
    sentinel->prev = node;
    
    mem = node->first_avail = (char *)node + SIZEOF_NODE_T;
    node->first_avail += size;
    node->avail_size = node_size - (node->first_avail - (char *)node);
    node->count = 1;

    if (tms->lock)
        apr_lock_release(tms->lock);

    BLOCK_T(mem)->node = node;
    mem = (char *)mem + SIZEOF_BLOCK_T;
    
    return mem;
}

static apr_status_t apr_sms_threads_acct_free(apr_sms_t *sms, void *mem)
{
    apr_sms_threads_t *tms;
    node_t *node, *sentinel;

    tms = SMS_ACCT_T(sms)->tms;
    node = BLOCK_T((char *)mem - SIZEOF_BLOCK_T)->node;

    if (tms->lock)
        apr_lock_acquire(tms->lock);

    node->count--;

    if (node->count) {
        if (tms->lock)
            apr_lock_release(tms->lock);

        return APR_SUCCESS;
    }

    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;
    
    if (tms->used_sentinel.prev != node) {
        node->next->prev = node->prev;
        node->prev->next = node->next;

        if (sms->parent->free_fn &&
            node->avail_size > tms->max_free &&
            node != tms->self) {
            if (tms->lock)
                apr_lock_release(tms->lock);

            return apr_sms_free(sms->parent, node);
        }
        
        sentinel = &tms->free_sentinel;
        node->prev = sentinel->prev;
        node->prev->next = node;
        node->next = sentinel;
        sentinel->prev = node;

        if (node != tms->self)
            tms->max_free -= node->avail_size;
    }

    if (tms->lock)
        apr_lock_release(tms->lock);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_sms_threads_create(apr_sms_t **sms, 
                                                 apr_sms_t *pms)
{
    return apr_sms_threads_create_ex(sms, pms, MIN_ALLOC, MIN_FREE, MAX_FREE);    
}

APR_DECLARE(apr_status_t) apr_sms_threads_create_ex(apr_sms_t **sms, 
                                                    apr_sms_t *pms,
                                                    apr_size_t min_alloc,
                                                    apr_size_t min_free,
                                                    apr_size_t max_free)
{
    apr_sms_t *new_sms, *ams;
    apr_sms_threads_t *tms;
    node_t *node;
    apr_status_t rv;

    *sms = NULL;

    min_alloc = APR_ALIGN_DEFAULT(min_alloc);
    min_free  = APR_ALIGN_DEFAULT(min_free);
    
    /* We're not a top level module, ie we have a parent, so
     * we allocate the memory for the structure from our parent.
     * This is safe as we shouldn't outlive our parent...
     */
    new_sms = apr_sms_calloc(pms, min_alloc);
    if (!new_sms)
        return APR_ENOMEM;

    if ((rv = apr_sms_init(new_sms, pms)) != APR_SUCCESS)
        return rv;

    new_sms->malloc_fn            = apr_sms_threads_malloc;
    new_sms->free_fn              = apr_sms_threads_free;
    new_sms->reset_fn             = apr_sms_threads_reset;
    new_sms->pre_destroy_fn       = apr_sms_threads_pre_destroy;
    new_sms->destroy_fn           = apr_sms_threads_destroy;
    new_sms->thread_register_fn   = apr_sms_threads_thread_register;
    new_sms->thread_unregister_fn = apr_sms_threads_thread_unregister;
    new_sms->identity             = module_identity;

    ams = (apr_sms_t *)((char *)new_sms + SIZEOF_SMS_THREADS_T);
    
    node = NODE_T((char *)ams + SIZEOF_SMS_ACCT_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;
    node->avail_size  = min_alloc - (node->first_avail - (char *)new_sms);
    node->count       = 0;
    
    tms = SMS_THREADS_T(new_sms);
    tms->min_alloc       = min_alloc;
    tms->min_free        = min_free;
    tms->max_free        = max_free;
    tms->thread_max_free = THREAD_MAX_FREE;
    tms->self            = node;
    
    node->next = node->prev = &tms->used_sentinel;
    tms->used_sentinel.next = tms->used_sentinel.prev = node;
    tms->free_sentinel.next = tms->free_sentinel.prev = &tms->free_sentinel;

    apr_sms_init(ams, new_sms);
    ams->malloc_fn = apr_sms_threads_acct_malloc;
    ams->free_fn   = apr_sms_threads_acct_free;
    ams->identity  = module_acct_identity;
    SMS_ACCT_T(ams)->tms = tms;
    apr_sms_post_init(ams);
    
    new_sms->accounting = ams;
    
    apr_sms_post_init(new_sms);
    
    *sms = new_sms;
    return APR_SUCCESS;
}

#endif /* APR_HAS_THREADS */

