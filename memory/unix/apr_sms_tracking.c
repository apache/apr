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
#include "apr_sms_tracking.h"
#include <stdlib.h>
#include <assert.h>

static const char *module_identity = "TRACKING";

/*
 * Simple tracking memory system
 */

/* INTERNALLY USED STRUCTURES */
typedef struct apr_track_node_t
{
    struct apr_track_node_t  *next;
    struct apr_track_node_t **ref;
} apr_track_node_t;

typedef struct apr_sms_tracking_t
{
    apr_sms_t            header;
    apr_track_node_t    *nodes;
} apr_sms_tracking_t;

static void *apr_sms_tracking_malloc(apr_sms_t *mem_sys,
                                     apr_size_t size)
{
    apr_sms_tracking_t *tms;
    apr_track_node_t *node;
  
    assert(mem_sys);

    tms = (apr_sms_tracking_t *)mem_sys;
    node = apr_sms_malloc(mem_sys->parent_mem_sys,
                          size + sizeof(apr_track_node_t));
    if (!node)
        return NULL;

    node->next = tms->nodes;
    tms->nodes = node;
    node->ref = &tms->nodes;
    if (node->next)
        node->next->ref = &node->next;

    node++;

    return (void *)node;
}

static void *apr_sms_tracking_calloc(apr_sms_t *mem_sys, 
                                     apr_size_t size)
{
    apr_sms_tracking_t *tms;
    apr_track_node_t *node;
  
    assert(mem_sys);

    tms = (apr_sms_tracking_t *)mem_sys;
    node = apr_sms_calloc(mem_sys->parent_mem_sys,
                          size + sizeof(apr_track_node_t));
    if (!node)
        return NULL;

    node->next = tms->nodes;
    tms->nodes = node;
    node->ref = &tms->nodes;
    if (node->next)
        node->next->ref = &node->next;

    node++;

    return (void *)node;
}

static void *apr_sms_tracking_realloc(apr_sms_t *mem_sys,
                                      void *mem, apr_size_t size)
{
    apr_sms_tracking_t *tms;
    apr_track_node_t *node;

    assert(mem_sys);

    tms = (apr_sms_tracking_t *)mem_sys;
    node = (apr_track_node_t *)mem;

    if (node) {
        node--;
        *(node->ref) = node->next;
    }

    node = apr_sms_realloc(mem_sys->parent_mem_sys,
                           node, size + sizeof(apr_track_node_t));
    if (!node)
        return NULL;

    node->next = tms->nodes;
    tms->nodes = node;
    node->ref = &tms->nodes;
    if (node->next)
        node->next->ref = &node->next;

    node++;

    return (void *)node;
}

static apr_status_t apr_sms_tracking_free(apr_sms_t *mem_sys,
                                          void *mem)
{
    apr_track_node_t *node;
    
    assert(mem_sys);
    assert(mem);

    node = (apr_track_node_t *)mem;
    node--;

    *(node->ref) = node->next;
    if (node->next)
        node->next->ref = node->ref;
          
    return apr_sms_free(mem_sys->parent_mem_sys, node);
}

static apr_status_t apr_sms_tracking_reset(apr_sms_t *mem_sys)
{
    apr_sms_tracking_t *tms;
    apr_track_node_t *node;
    apr_status_t rv;
 
    assert(mem_sys);

    tms = (apr_sms_tracking_t *)mem_sys;

    while (tms->nodes) {
        node = tms->nodes;
        *(node->ref) = node->next;
        if (node->next)
            node->next->ref = node->ref;
        if ((rv = apr_sms_free(mem_sys->parent_mem_sys, 
                               node)) != APR_SUCCESS)
            return rv;
    }
    
    return APR_SUCCESS;
}

static apr_status_t apr_sms_tracking_destroy(apr_sms_t *mem_sys)
{
    apr_status_t rv;
    
    /* If this is NULL we won't blow up as it should be caught at the
     * next level down and then passed back to us...
     */
#ifdef APR_ASSERT_MEMORY
    assert(mem_sys->parent_mem_sys);
#endif
    
    if (!mem_sys)
        return APR_EMEMSYS;

    if ((rv = apr_sms_tracking_reset(mem_sys)) != APR_SUCCESS)
        return rv;
    
    return apr_sms_free(mem_sys->parent_mem_sys, mem_sys);
}

APR_DECLARE(apr_status_t) apr_sms_tracking_create(apr_sms_t **mem_sys, 
                                                  apr_sms_t *pms)
{
    apr_sms_t *new_mem_sys;
    apr_sms_tracking_t *tms;
    apr_status_t rv;

    assert(mem_sys);
    assert(pms);
    
    *mem_sys = NULL;
    /* We're not a top level module, ie we have a parent, so
     * we allocate the memory for the structure from our parent.
     * This is safe as we shouldn't outlive our parent...
     */
    new_mem_sys = apr_sms_calloc(pms, sizeof(apr_sms_tracking_t));

    if (!new_mem_sys)
        return APR_ENOMEM;

    if ((rv = apr_sms_init(new_mem_sys, pms)) != APR_SUCCESS)
        return rv;

    new_mem_sys->malloc_fn  = apr_sms_tracking_malloc;
    new_mem_sys->calloc_fn  = apr_sms_tracking_calloc;
    new_mem_sys->realloc_fn = apr_sms_tracking_realloc;
    new_mem_sys->free_fn    = apr_sms_tracking_free;
    new_mem_sys->reset_fn   = apr_sms_tracking_reset;
    new_mem_sys->destroy_fn = apr_sms_tracking_destroy;
    new_mem_sys->identity   = module_identity;

    tms = (apr_sms_tracking_t *)new_mem_sys;
    tms->nodes = NULL;

    apr_sms_assert(new_mem_sys);

    *mem_sys = new_mem_sys;
    return APR_SUCCESS;
}

