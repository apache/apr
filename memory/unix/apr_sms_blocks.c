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

#include "apr.h"
#include "apr_general.h"
#include "apr_private.h"
#include "apr_sms.h"
#include "apr_sms_blocks.h"
#include "apr_lock.h"
#include <stdlib.h>
#include "apr_portable.h"
#define APR_WANT_MEMFUNC
#include "apr_want.h"

static const char *module_identity = "BLOCKS";
#define MIN_ALLOC     8 * 1024 /* don't allocate in smaller blocks than 8Kb */

/*
 * Simple bucket memory system
 */

/* INTERNALLY USED STRUCTURES */
typedef struct block_t {
    char *nxt;
} block_t;

typedef struct apr_sms_blocks_t
{
    apr_sms_t            header;
    apr_size_t           block_sz;
    apr_size_t           alloc_sz; 
    block_t             *alloc_list;
    block_t             *free_list;
    char                *ptr;
    char                *endp;
    char                *self_endp;
} apr_sms_blocks_t;

/* Various defines we'll use */
#define SIZEOF_SMS_BLOCKS_T         APR_ALIGN_DEFAULT(sizeof(apr_sms_blocks_t))
#define SIZEOF_BLOCK_T              APR_ALIGN_DEFAULT(sizeof(block_t))

#define SMS_BLOCKS_T(sms)           ((apr_sms_blocks_t *)sms)
#define BLOCK_T(mem)                ((block_t *)mem)

static void *apr_sms_blocks_malloc(apr_sms_t *sms,
                                   apr_size_t size)
{
    void *mem;

    if (size > SMS_BLOCKS_T(sms)->block_sz)
        return NULL;

    if ((mem = SMS_BLOCKS_T(sms)->free_list) != NULL) {
        SMS_BLOCKS_T(sms)->free_list = BLOCK_T(BLOCK_T(mem)->nxt);
        return mem;
    }

    mem = SMS_BLOCKS_T(sms)->ptr;
    if ((SMS_BLOCKS_T(sms)->ptr = (char *)mem + SMS_BLOCKS_T(sms)->block_sz)
                               <= SMS_BLOCKS_T(sms)->endp)
        return mem;
        
    /* OK, we've run out of memory.  Let's get more :) */
    mem = apr_sms_malloc(sms->parent, SMS_BLOCKS_T(sms)->alloc_sz);
    if (!mem) {
        /* make safe and return */
        SMS_BLOCKS_T(sms)->ptr = SMS_BLOCKS_T(sms)->endp;
        return NULL;
    }

    /* Insert our new bit of memory at the start of the list */
    BLOCK_T(mem)->nxt = (char*)SMS_BLOCKS_T(sms)->alloc_list;
    SMS_BLOCKS_T(sms)->alloc_list = mem;
    SMS_BLOCKS_T(sms)->endp = (char *)mem + SMS_BLOCKS_T(sms)->alloc_sz;
    (char *)mem += SIZEOF_BLOCK_T;
    SMS_BLOCKS_T(sms)->ptr = (char *)mem + SMS_BLOCKS_T(sms)->block_sz;

    if (SMS_BLOCKS_T(sms)->alloc_list->nxt) {
        /* we're not the first block being added, so we increase our
         * size.
         */
        SMS_BLOCKS_T(sms)->alloc_sz <<= 1;
    }

    
    return mem;
}
    
static void *apr_sms_blocks_calloc(apr_sms_t *sms, 
                                   apr_size_t size)
{
    void *mem;

    if (size > SMS_BLOCKS_T(sms)->block_sz)
        return NULL;

    if ((mem = SMS_BLOCKS_T(sms)->free_list) != NULL) {
        SMS_BLOCKS_T(sms)->free_list = BLOCK_T(BLOCK_T(mem)->nxt);
        memset(mem, '\0', SMS_BLOCKS_T(sms)->block_sz);
        return mem;
    }

    mem = SMS_BLOCKS_T(sms)->ptr;
    if ((SMS_BLOCKS_T(sms)->ptr = (char *)mem + 
        SMS_BLOCKS_T(sms)->block_sz) <= SMS_BLOCKS_T(sms)->endp) {
        memset(mem, '\0', SMS_BLOCKS_T(sms)->block_sz);
        return mem;
    }
        
    /* probably quicker to just grab malloc memory, then memset as 
     * required.
     */
    mem = apr_sms_malloc(sms->parent, SMS_BLOCKS_T(sms)->alloc_sz);
    if (!mem) {
        SMS_BLOCKS_T(sms)->ptr = SMS_BLOCKS_T(sms)->endp;
        return NULL;
    }

    /* Insert at the start of the list */
    BLOCK_T(mem)->nxt = (char*)SMS_BLOCKS_T(sms)->alloc_list;
    SMS_BLOCKS_T(sms)->alloc_list = mem;
    SMS_BLOCKS_T(sms)->endp = (char *)mem + SMS_BLOCKS_T(sms)->alloc_sz;
    (char *)mem += SIZEOF_BLOCK_T;
    SMS_BLOCKS_T(sms)->ptr = (char *)mem + SMS_BLOCKS_T(sms)->block_sz;

    if (SMS_BLOCKS_T(sms)->alloc_list->nxt) {
        /* we're not the first block being added, so we increase our
         * size.
         */
        SMS_BLOCKS_T(sms)->alloc_sz <<= 1;
    }

    memset(mem, '\0', SMS_BLOCKS_T(sms)->block_sz);

    return mem;
}

static apr_status_t apr_sms_blocks_free(apr_sms_t *sms,
                                        void *mem)
{
    BLOCK_T(mem)->nxt = (char*)SMS_BLOCKS_T(sms)->free_list;
    SMS_BLOCKS_T(sms)->free_list = mem;
    return APR_SUCCESS;
}

static apr_status_t apr_sms_blocks_reset(apr_sms_t *sms)
{
    block_t *block;

    SMS_BLOCKS_T(sms)->ptr = (char *)sms + SIZEOF_SMS_BLOCKS_T;
    SMS_BLOCKS_T(sms)->endp = SMS_BLOCKS_T(sms)->self_endp;
    SMS_BLOCKS_T(sms)->free_list = NULL;

    while ((block = SMS_BLOCKS_T(sms)->alloc_list) != NULL) {
        SMS_BLOCKS_T(sms)->alloc_list = BLOCK_T(block->nxt);
        apr_sms_free(sms->parent, block);
    }

    return APR_SUCCESS;
}

static apr_status_t apr_sms_blocks_destroy(apr_sms_t *sms)
{
    block_t *block;
    
    while ((block = SMS_BLOCKS_T(sms)->alloc_list) != NULL) {
        SMS_BLOCKS_T(sms)->alloc_list = BLOCK_T(block->nxt);
        apr_sms_free(sms->parent, block);
    }
    
    return apr_sms_free(sms->parent, sms);
}

APR_DECLARE(apr_status_t) apr_sms_blocks_create(apr_sms_t **sms, 
                                                apr_sms_t *parent,
                                                apr_size_t block_size)
{
    apr_sms_t *new_sms;
    apr_status_t rv;
    apr_size_t alloc_size;
    apr_sms_blocks_t *bms;
       
    *sms = NULL;
    if (block_size == 0)
        return APR_EINVAL;

    block_size = APR_ALIGN_DEFAULT(block_size);
    alloc_size = block_size << 2;
    if (alloc_size < MIN_ALLOC)
        alloc_size = MIN_ALLOC;

    new_sms = apr_sms_calloc(parent, alloc_size);

    if (!new_sms)
        return APR_ENOMEM;

    if ((rv = apr_sms_init(new_sms, parent)) != APR_SUCCESS)
        return rv;

    new_sms->malloc_fn      = apr_sms_blocks_malloc;
    new_sms->calloc_fn      = apr_sms_blocks_calloc;
    new_sms->free_fn        = apr_sms_blocks_free;
    new_sms->reset_fn       = apr_sms_blocks_reset;
    new_sms->destroy_fn     = apr_sms_blocks_destroy;
    new_sms->identity       = module_identity;

    bms = SMS_BLOCKS_T(new_sms);
    bms->block_sz = block_size;
    bms->alloc_sz = alloc_size;
    bms->ptr = (char*)new_sms + SIZEOF_SMS_BLOCKS_T;
    bms->endp = bms->self_endp = (char*)new_sms + alloc_size;

    /* We are normally single threaded so no lock */

    apr_sms_assert(new_sms);

    *sms = new_sms;
    return APR_SUCCESS;
}
