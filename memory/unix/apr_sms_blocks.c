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

static const char *module_identity = "BLOCKS";
#define SIZE_TO_MALLOC 8 * 1024

/*
 * Simple bucket memory system
 */

/* INTERNALLY USED STRUCTURES */
typedef struct block_t block_t;
struct block_t {
    void *nxt;
};

typedef struct apr_sms_blocks_t apr_sms_blocks_t;
struct apr_sms_blocks_t
{
    apr_sms_t            header;
    apr_size_t           block_sz;
    void                *ptr;
    void                *endp;
    block_t             *free_list;
    apr_lock_t          *lock;
};

#define SIZEOF_BLOCKS_T (sizeof(apr_sms_blocks_t) + \
                        ((0x8 - (sizeof(apr_sms_blocks_t) & 0x7)) & 0x7))
#define BLOCKS_T(sms)   ((apr_sms_blocks_t *)sms)

static void *apr_sms_blocks_malloc(apr_sms_t *sms,
                                   apr_size_t size)
{
    void *mem;
    
    if (size > BLOCKS_T(sms)->block_sz)
        return NULL;

    if ((mem = BLOCKS_T(sms)->free_list) != NULL) {
        BLOCKS_T(sms)->free_list = ((block_t*)mem)->nxt;
        return mem;
    }
    
    mem = BLOCKS_T(sms)->ptr;
    BLOCKS_T(sms)->ptr += BLOCKS_T(sms)->block_sz;

    if (BLOCKS_T(sms)->ptr > BLOCKS_T(sms)->endp)
        return NULL;

    return mem;
}
    
static void *apr_sms_blocks_calloc(apr_sms_t *sms, 
                                   apr_size_t size)
{
    void *mem;
    
    if (size > BLOCKS_T(sms)->block_sz)
        return NULL;

    if ((mem = BLOCKS_T(sms)->free_list) != NULL) {
        BLOCKS_T(sms)->free_list = ((block_t*)mem)->nxt;
        return mem;
    }
    
    mem = BLOCKS_T(sms)->ptr;
    BLOCKS_T(sms)->ptr += BLOCKS_T(sms)->block_sz;

    if (BLOCKS_T(sms)->ptr > BLOCKS_T(sms)->endp)
        return NULL;

    memset(mem, '\0', BLOCKS_T(sms)->block_sz);
    
    return mem;
}

static apr_status_t apr_sms_blocks_free(apr_sms_t *sms,
                                        void *mem)
{
    ((block_t *)mem)->nxt = BLOCKS_T(sms)->free_list;
    BLOCKS_T(sms)->free_list = (block_t*)mem;

    return APR_SUCCESS;
}

static apr_status_t apr_sms_blocks_reset(apr_sms_t *sms)
{
    BLOCKS_T(sms)->ptr = (char *)sms + SIZEOF_BLOCKS_T;
    BLOCKS_T(sms)->free_list = NULL;
    
    return APR_SUCCESS;
}

static apr_status_t apr_sms_blocks_destroy(apr_sms_t *sms)
{
    return apr_sms_free(sms->parent, sms);
}

APR_DECLARE(apr_status_t) apr_sms_blocks_create(apr_sms_t **sms, 
                                                apr_sms_t *parent,
                                                apr_size_t block_size)
{
    apr_sms_t *new_sms;
    apr_status_t rv;

    *sms = NULL;
    new_sms = apr_sms_calloc(parent, SIZE_TO_MALLOC);

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

    BLOCKS_T(new_sms)->ptr = (char *)new_sms + SIZEOF_BLOCKS_T;
    BLOCKS_T(new_sms)->endp = (char *)new_sms + SIZE_TO_MALLOC;
    
    BLOCKS_T(new_sms)->block_sz = block_size +
                                 ((0x8 - (block_size & 0x7)) & 0x7);

    /* We are normally single threaded so no lock */
    
    apr_sms_assert(new_sms);

    *sms = new_sms;
    return APR_SUCCESS;
}


