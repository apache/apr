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

#include "apr_general.h"
#include "apr_shmem.h"
#include "apr_errno.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include <stdio.h>
#include <stdlib.h>
#include <kernel/OS.h>

struct block_t {
    apr_pool_t *p;
    void *addr;
    apr_size_t size;
    void *nxt;
    void *prev;
};

typedef struct apr_shmem_t {
    apr_pool_t *p;
    void *memblock;
    void *ptr;
    apr_size_t avail;
    area_id aid;
    struct block_t *uselist;
    struct block_t *freelist;
} apr_shmem_t;

#define MIN_BLK_SIZE 128


void add_block(struct block_t **list, struct block_t *blk);
void split_block(struct block_t **list, struct block_t *blk, apr_size_t size);

static struct block_t * find_block_by_addr(struct block_t *list, void *addr)
{
    struct block_t *b = list;   
    do {
        if (b->addr == addr)
            return b;
    }
    while ((b = b->nxt) != NULL);

    return NULL;
}

static struct block_t *find_block_of_size(struct block_t *list, apr_size_t size)
{
    struct block_t *b = list, *rv = NULL;
    apr_ssize_t diff = -1;

    if (!list)
        return NULL;
        
    do {
        if (b->size == size)
            return b;
        if (b->size > size){
            if (diff == -1)
                diff = b->size;
            if ((b->size - size) < diff){
                diff = b->size - size;
                rv = b;
            }
        }
    }
    while ((b = b->nxt) != list);

    if (diff > MIN_BLK_SIZE) {
        split_block(&list, rv, size);       
    }

    if (rv)
        return rv;          

    return NULL;
}

void add_block(struct block_t **list, struct block_t *blk)
{   
    if ((*list) == NULL)
        *list = blk;
                  
    if (blk == (*list)){
        blk->prev = blk;
        blk->nxt = blk;
    } else {
        ((struct block_t*)(*list)->prev)->nxt = blk;
        blk->prev = ((struct block_t *)(*list))->prev;
        blk->nxt = (*list);
        (*list)->prev = blk;
    }  
}

void split_block(struct block_t **list, struct block_t *blk, apr_size_t size)
{
        apr_size_t nsz = blk->size - size;
        struct block_t *b = (struct block_t*)apr_pcalloc(blk->p, sizeof(struct block_t));
        b->p = blk->p;
        b->size = nsz;
        blk->size = size;
        b->addr = blk->addr + size;
        add_block(list, b);     
}

static void remove_block(struct block_t **list, struct block_t *blk)
{
    if (((*list) == blk) && (blk->nxt == blk) && (blk == blk->prev)){
        *list = NULL;
        blk->nxt = NULL;
        blk->prev = NULL;
    } else {
        ((struct block_t*)(blk->nxt))->prev = blk->prev;
        ((struct block_t*)(blk->prev))->nxt = blk->nxt;
        if (*list == blk)
            *list = blk->nxt;
        blk->nxt = NULL;
        blk->prev = NULL;   
    }
}

/* puts a used block onto the free list for it to be reused... */
static void free_block(apr_shmem_t *m, void *entity)
{
    struct block_t *b;
    if ((b = find_block_by_addr(m->uselist, entity)) != NULL){
        remove_block(&(m->uselist), b);
        add_block(&(m->freelist), b);
        m->avail += b->size;
    }
}

/* assigns a block of our memory and puts an entry on the uselist */
static struct block_t *alloc_block(apr_shmem_t *m, apr_size_t size)
{
    struct block_t *b = NULL;
    if (m->avail < size)
        return NULL; 

    if ((b = find_block_of_size(m->freelist, size)) != NULL){
        remove_block(&(m->freelist), b);
    } else {
        b = (struct block_t*)apr_pcalloc(m->p, sizeof(struct block_t));   
        b->p = m->p;
        b->addr = m->ptr;
        b->size = size;
        m->ptr += size;
    }
    m->avail -= b->size; /* actual size may be different if we're reusing a block */
    add_block(&(m->uselist), b);

    return b;
}

APR_DECLARE(apr_status_t) apr_shm_init(apr_shmem_t **m, apr_size_t reqsize, const char *file, 
                                       apr_pool_t *p)
{
    apr_size_t pagesize;
    area_id newid;
    char *addr;

    (*m) = (apr_shmem_t *)apr_pcalloc(p, sizeof(apr_shmem_t));
    /* we MUST allocate in pages, so calculate how big an area we need... */
    pagesize = ((reqsize + B_PAGE_SIZE - 1) / B_PAGE_SIZE) * B_PAGE_SIZE;
    
    newid = create_area("apr_shm", (void*)&addr, B_ANY_ADDRESS,
                        pagesize, B_CONTIGUOUS, B_READ_AREA|B_WRITE_AREA);

    if (newid < 0)
        return errno;

    (*m)->p = p;
    (*m)->aid = newid;
    (*m)->memblock = addr;
    (*m)->ptr = (void*)addr;
    (*m)->avail = pagesize; /* record how big an area we actually created... */
    (*m)->uselist = NULL;
    (*m)->freelist = NULL;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_destroy(apr_shmem_t *m)
{
    delete_area(m->aid);
    m->avail = 0;
    m->freelist = NULL;
    m->uselist = NULL;
    m->memblock = NULL;
    return APR_SUCCESS;
}

APR_DECLARE(void *) apr_shm_malloc(apr_shmem_t *m, apr_size_t reqsize)
{
    struct block_t *b;
    if ((b = alloc_block(m, reqsize)) != NULL)
        return b->addr;
    return NULL;
}

APR_DECLARE(void *) apr_shm_calloc(apr_shmem_t *m, apr_size_t reqsize)
{
    struct block_t *b; 
    if ((b = alloc_block(m, reqsize)) != NULL){  
        memset(b->addr, 0, reqsize);
        return b->addr;
    }
    return NULL;
}

APR_DECLARE(apr_status_t) apr_shm_free(apr_shmem_t *m, void *entity)
{
    free_block(m, entity);   
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_name_get(apr_shmem_t *c, apr_shm_name_t **name)
{
    *name = NULL;
    return APR_ANONYMOUS;
}

APR_DECLARE(apr_status_t) apr_shm_name_set(apr_shmem_t *c, apr_shm_name_t *name)
{
    return APR_ANONYMOUS;
}

APR_DECLARE(apr_status_t) apr_shm_open(apr_shmem_t *m)
{
    /* If we've forked we need a clone of the original area or we
     * will only have access to a one time copy of the data made when
     * the fork occurred.  This strange bit of code fixes that problem!
     */
    thread_info ti;
    area_info ai;
    area_id deleteme = area_for(m->memblock);
    
    /* we need to check which team we're in, so we need to get
     * the appropriate info structures for the current thread and
     * the area we're using.
     */
    get_area_info(m->aid, &ai);   
    get_thread_info(find_thread(NULL), &ti);

    if (ti.team != ai.team){
        area_id nai;
        /* if we are in a child then we need to delete the system
         * created area as it's a one time copy and won't be a clone
         * which is not good.
         */
        delete_area(deleteme);
        /* now we make our own clone and use that from now on! */
        nai = clone_area(ai.name, &(ai.address), B_CLONE_ADDRESS,
            B_READ_AREA | B_WRITE_AREA, ai.area);
        get_area_info(nai, &ai);
       m->memblock = ai.address;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_avail(apr_shmem_t *m, apr_size_t *size)
{
    *size = m->avail;
    if (m->avail == 0)
        return APR_ENOSHMAVAIL;
    return APR_SUCCESS;
        
}
