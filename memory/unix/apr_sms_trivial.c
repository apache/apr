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
#include "apr_sms_trivial.h"
#include "apr_lock.h"
#include "sms_private.h"
#include <stddef.h>

static const char *module_identity = "TRIVIAL";

/*
 * Simple trivial memory system
 *
 * The goal of this SMS is to make malloc and reset operations as efficient 
 * as possible.
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
    apr_uint32_t    count;
} node_t;

typedef struct apr_sms_trivial_t
{
    apr_sms_t            sms_hdr;
    node_t               used_sentinel;
    node_t               free_sentinel;
    node_t              *self;
    apr_size_t           min_alloc;
    apr_size_t           min_free;
    apr_size_t           max_free;
    apr_lock_t          *lock;
} apr_sms_trivial_t;

#define SIZEOF_BLOCK_T       APR_ALIGN_DEFAULT(sizeof(block_t))
#define SIZEOF_NODE_T        APR_ALIGN_DEFAULT(sizeof(node_t))
#define SIZEOF_SMS_TRIVIAL_T APR_ALIGN_DEFAULT(sizeof(apr_sms_trivial_t))

#define BLOCK_T(mem)         ((block_t *)(mem))
#define NODE_T(mem)          ((node_t *)(mem))
#define SMS_TRIVIAL_T(sms)   ((apr_sms_trivial_t *)(sms))

/* Magic numbers :)
 * MIN_ALLOC defines the floor of how many bytes we will ask our parent for
 * MIN_FREE  defines how many extra bytes we will allocate when asking the
 *           the system for memory.
 * MAX_FREE  defines how many bytes the SMS may hold at one time.  If it
 *           exceeds this value, it will return memory to the parent SMS.
 *           (note that this implementation counts down to 0)
 */
#define MIN_ALLOC  0x2000 
#define MIN_FREE   0x1000
#define MAX_FREE  0x80000

static void *apr_sms_trivial_malloc(apr_sms_t *sms,
                                    apr_size_t size)
{
    node_t *node, *sentinel;
    apr_size_t node_size;
    void *mem;

    /* Round up the size to the next 8 byte boundary */
    size = APR_ALIGN_DEFAULT(size) + SIZEOF_BLOCK_T;

    if (SMS_TRIVIAL_T(sms)->lock)
        apr_lock_acquire(SMS_TRIVIAL_T(sms)->lock);
    
    node = SMS_TRIVIAL_T(sms)->used_sentinel.prev;

    if (node->avail_size >= size) {
        mem = node->first_avail;
        node->avail_size  -= size;
        node->first_avail += size;
        node->count++;

        if (SMS_TRIVIAL_T(sms)->lock)
            apr_lock_release(SMS_TRIVIAL_T(sms)->lock);
    
        BLOCK_T(mem)->node = node;
        mem = (char *)mem + SIZEOF_BLOCK_T;

        return mem;
    }

    /* reset the 'last' block, it will be replaced soon */
    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;

    /* browse the free list for a useable block.  Note that we set the 
     * sentinel to be the size we are looking for - so, we'll have to 
     * stop when we hit the sentinel again. 
     */
    sentinel = &SMS_TRIVIAL_T(sms)->free_sentinel;
    sentinel->avail_size = size;

    node = sentinel->next;
    while (node->avail_size < size)
        node = node->next;

    if (node != sentinel) {
        /* Remove from chain of free nodes and add it to used chain */
        node->prev->next = node->next;
        node->next->prev = node->prev;

        sentinel = &SMS_TRIVIAL_T(sms)->used_sentinel;
        node->prev = sentinel->prev;
        node->prev->next = node;
        node->next = sentinel;
        sentinel->prev = node;
       
        /* We are no longer free, so increase available free mem. */
        if (node != SMS_TRIVIAL_T(sms)->self)
            SMS_TRIVIAL_T(sms)->max_free += node->avail_size;

        mem = node->first_avail;
        node->avail_size  -= size;
        node->first_avail += size;
        node->count = 1;

        if (SMS_TRIVIAL_T(sms)->lock)
            apr_lock_release(SMS_TRIVIAL_T(sms)->lock);

        BLOCK_T(mem)->node = node;
        mem = (char *)mem + SIZEOF_BLOCK_T;

        return mem;
    }
   
    /* We couldn't find any used or free node that had enough space,
     * so we have to allocate a new block from our parent. 
     */ 
    node_size = size + SMS_TRIVIAL_T(sms)->min_free;
    if (node_size < SMS_TRIVIAL_T(sms)->min_alloc)
        node_size = SMS_TRIVIAL_T(sms)->min_alloc;
    
    node = apr_sms_malloc(sms->parent, node_size);
    if (!node) {
        /* restore the 'last' node, so next allocation will not segfault */
        node = SMS_TRIVIAL_T(sms)->used_sentinel.prev;
        node->first_avail += node->avail_size;
        node->avail_size   = 0;
    
        if (SMS_TRIVIAL_T(sms)->lock)
            apr_lock_release(SMS_TRIVIAL_T(sms)->lock);
        
        return NULL;
    }

    /* Add the new node to the used chain. */
    sentinel = &SMS_TRIVIAL_T(sms)->used_sentinel;
    node->prev = sentinel->prev;
    node->prev->next = node;
    node->next = sentinel;
    sentinel->prev = node;
    
    mem = node->first_avail = (char *)node + SIZEOF_NODE_T;
    node->first_avail += size;
    node->avail_size = node_size - (node->first_avail - (char *)node);
    node->count = 1;

    if (SMS_TRIVIAL_T(sms)->lock)
        apr_lock_release(SMS_TRIVIAL_T(sms)->lock);

    BLOCK_T(mem)->node = node;
    mem = (char *)mem + SIZEOF_BLOCK_T;
    
    return mem;
}

static apr_status_t apr_sms_trivial_free(apr_sms_t *sms, void *mem)
{
    node_t *node, *sentinel;

    node = BLOCK_T((char *)mem - SIZEOF_BLOCK_T)->node;

    if (SMS_TRIVIAL_T(sms)->lock)
        apr_lock_acquire(SMS_TRIVIAL_T(sms)->lock);

    node->count--;

    if (node->count) {
        if (SMS_TRIVIAL_T(sms)->lock)
            apr_lock_release(SMS_TRIVIAL_T(sms)->lock);

        return APR_SUCCESS;
    }

    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;
    
    if (SMS_TRIVIAL_T(sms)->used_sentinel.prev != node) {
        node->next->prev = node->prev;
        node->prev->next = node->next;

        if (sms->parent->free_fn &&
            node->avail_size > SMS_TRIVIAL_T(sms)->max_free &&
            node != SMS_TRIVIAL_T(sms)->self) {
            if (SMS_TRIVIAL_T(sms)->lock)
                apr_lock_release(SMS_TRIVIAL_T(sms)->lock);

            return apr_sms_free(sms->parent, node);
        }
        
        sentinel = &SMS_TRIVIAL_T(sms)->free_sentinel;
        node->prev = sentinel->prev;
        node->prev->next = node;
        node->next = sentinel;
        sentinel->prev = node;

        if (node != SMS_TRIVIAL_T(sms)->self)
            SMS_TRIVIAL_T(sms)->max_free -= node->avail_size;
    }

    if (SMS_TRIVIAL_T(sms)->lock)
        apr_lock_release(SMS_TRIVIAL_T(sms)->lock);

    return APR_SUCCESS;
}

static void *apr_sms_trivial_realloc(apr_sms_t *sms, void *mem, apr_size_t reqsize)
{
    void *new_mem;
    apr_size_t size;
    node_t *node;
    char *endp;

    reqsize = APR_ALIGN_DEFAULT(reqsize);

    new_mem = apr_sms_trivial_malloc(sms, reqsize);
 
    if (new_mem) {
        node = BLOCK_T((char *)mem - SIZEOF_BLOCK_T)->node;

        endp = node->first_avail;
        if (endp == (char *)node + SIZEOF_NODE_T)
            endp += node->avail_size;

        size = endp - (char *)mem;
        if (size > reqsize)
            size = reqsize;

        memcpy(new_mem, mem, size);
    }

    apr_sms_trivial_free(sms, mem);

    return new_mem;
}

static apr_status_t apr_sms_trivial_reset(apr_sms_t *sms)
{
    node_t *node, *prev, *used_sentinel, *free_sentinel, *free_list;
    apr_size_t min_alloc, max_free;
 
    used_sentinel = &SMS_TRIVIAL_T(sms)->used_sentinel;
    free_sentinel = &SMS_TRIVIAL_T(sms)->free_sentinel;
    
    free_list = NULL;
    
    if (SMS_TRIVIAL_T(sms)->lock)
        apr_lock_acquire(SMS_TRIVIAL_T(sms)->lock);

    /* Always reset our base node as this can't be reclaimed. */
    node = SMS_TRIVIAL_T(sms)->self;
    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;
    node->count = 0;
    node->prev->next = node->next;
    node->next->prev = node->prev;
   
    /* used_sentinel->prev may be currently "active", so disable it. */
    node = used_sentinel->prev;
    node->avail_size += node->first_avail - ((char *)node + SIZEOF_NODE_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;
    
    if (sms->parent->free_fn) {
        /* We only reserve max_free bytes. The rest will be passed to the 
         * parent SMS to be freed. 
         */
        min_alloc = SMS_TRIVIAL_T(sms)->min_alloc;
        max_free  = SMS_TRIVIAL_T(sms)->max_free;

        used_sentinel->avail_size = min_alloc;

        while (max_free > min_alloc) {
            if (node->avail_size <= max_free) {
                if (node == used_sentinel)
                    break;
                /* These are the nodes that will NOT be freed, but
                 * placed on the free list for later reuse. 
                 */
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
                node = node->prev; /* Will be reclaimed. */
        }

        /* Remember that when sms->max_free hits zero, we free everything. */
        SMS_TRIVIAL_T(sms)->max_free = max_free;

        /* Anything remaining on the used_sentinel list will be freed. */
        used_sentinel->prev->next = NULL;
        free_list = used_sentinel->next;
    }
    else {
        /* Everything we have allocated goes into free_sentinel. */
        node = used_sentinel->prev;
        node->next = free_sentinel->next;
        node->next->prev = node;

        node = used_sentinel->next;
        node->prev = free_sentinel;
        free_sentinel->next = node;
    }
       
    /* Reset used_sentinel to just be the originally allocated node. */ 
    node = SMS_TRIVIAL_T(sms)->self;
    node->next = node->prev = used_sentinel;
    used_sentinel->next = used_sentinel->prev = node;

    if (SMS_TRIVIAL_T(sms)->lock)
        apr_lock_release(SMS_TRIVIAL_T(sms)->lock);

    while ((node = free_list) != NULL) {
        free_list = node->next;
        apr_sms_free(sms->parent, node);
    }
    
    return APR_SUCCESS;
}

static apr_status_t apr_sms_trivial_pre_destroy(apr_sms_t *sms)
{
    /* This function WILL always be called.  However, be aware that the
     * main sms destroy function knows that it's not wise to try and destroy
     * the same piece of memory twice, so the destroy function in a child won't
     * neccesarily be called.  To guarantee we destroy the lock it's therefore
     * destroyed here.
     */
        
    if (SMS_TRIVIAL_T(sms)->lock) {
        apr_lock_acquire(SMS_TRIVIAL_T(sms)->lock);
        apr_lock_destroy(SMS_TRIVIAL_T(sms)->lock);
        SMS_TRIVIAL_T(sms)->lock = NULL;
    }
    
    return APR_SUCCESS;    
}

static apr_status_t apr_sms_trivial_destroy(apr_sms_t *sms)
{
    apr_sms_trivial_t *tms;
    node_t *node, *next;

    tms = SMS_TRIVIAL_T(sms);
    node = tms->self;
    node->next->prev = node->prev;
    node->prev->next = node->next;

    tms->free_sentinel.prev->next = NULL;
    tms->used_sentinel.prev->next = tms->free_sentinel.next;
    
    node = tms->used_sentinel.next;
    while (node) {
        next = node->next;

        apr_sms_free(sms->parent, node);
        node = next;
    }

    apr_sms_free(sms->parent, sms);

    return APR_SUCCESS;
}

#if APR_HAS_THREADS
static apr_status_t apr_sms_trivial_thread_register(apr_sms_t *sms,
                                                    apr_os_thread_t thread)
{
    if (!SMS_TRIVIAL_T(sms)->lock && sms->threads > 1)
        return apr_lock_create(&SMS_TRIVIAL_T(sms)->lock,
                               APR_MUTEX, APR_LOCKALL,
                               NULL, sms->pool);

    return APR_SUCCESS;
}

static apr_status_t apr_sms_trivial_thread_unregister(apr_sms_t *sms,
                                                      apr_os_thread_t thread)
{
    return APR_SUCCESS;
}
#endif /* APR_HAS_THREADS */

APR_DECLARE(apr_status_t) apr_sms_trivial_create_ex(apr_sms_t **sms, 
                                                    apr_sms_t *pms,
                                                    apr_size_t min_alloc,
                                                    apr_size_t min_free,
                                                    apr_size_t max_free)
{
    apr_sms_t *new_sms;
    apr_sms_trivial_t *tms;
    node_t *node;
    apr_status_t rv;

    *sms = NULL;
    
    min_alloc = APR_ALIGN_DEFAULT(min_alloc);
    min_free  = APR_ALIGN_DEFAULT(min_free);
    if (min_free < SIZEOF_NODE_T)
        min_free = SIZEOF_NODE_T;
    
    /* We're not a top level module, ie we have a parent, so
     * we allocate the memory for the structure from our parent.
     * This is safe as we shouldn't outlive our parent...
     */

    new_sms = apr_sms_calloc(pms, min_alloc);
    if (!new_sms)
        return APR_ENOMEM;

    if ((rv = apr_sms_init(new_sms, pms)) != APR_SUCCESS)
        return rv;

    new_sms->malloc_fn            = apr_sms_trivial_malloc;
    new_sms->realloc_fn           = apr_sms_trivial_realloc;
    new_sms->free_fn              = apr_sms_trivial_free;
    new_sms->reset_fn             = apr_sms_trivial_reset;
    new_sms->pre_destroy_fn       = apr_sms_trivial_pre_destroy;
    new_sms->destroy_fn           = apr_sms_trivial_destroy;
#if APR_HAS_THREADS
    new_sms->thread_register_fn   = apr_sms_trivial_thread_register;
    new_sms->thread_unregister_fn = apr_sms_trivial_thread_unregister;
#endif /* APR_HAS_THREADS */
    new_sms->identity             = module_identity;

    node = NODE_T((char *)new_sms + SIZEOF_SMS_TRIVIAL_T);
    node->first_avail = (char *)node + SIZEOF_NODE_T;
    node->avail_size  = min_alloc - SIZEOF_SMS_TRIVIAL_T - SIZEOF_NODE_T;
    node->count       = 0;

    tms = SMS_TRIVIAL_T(new_sms);
    tms->min_alloc = min_alloc;
    tms->min_free  = min_free;
    tms->max_free  = max_free;
    tms->self      = node;
    
    node->next = node->prev = &tms->used_sentinel;
    tms->used_sentinel.next = tms->used_sentinel.prev = node;
    tms->free_sentinel.next = tms->free_sentinel.prev = &tms->free_sentinel;
   
    apr_sms_post_init(new_sms);

    *sms = new_sms;
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_sms_trivial_create(apr_sms_t **sms, 
                                                  apr_sms_t *pms)
{
    return apr_sms_trivial_create_ex(sms, pms, MIN_ALLOC, MIN_FREE, MAX_FREE);
}

