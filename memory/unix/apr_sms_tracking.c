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
#include "apr_lock.h"
#include <stddef.h> /* for NULL */
#include "sms_private.h"

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
    apr_lock_t          *lock;
} apr_sms_tracking_t;

#define INSERT_NODE(node, tms) \
    if (tms->lock) \
        apr_lock_acquire(tms->lock); \
    \
    node->ref = &tms->nodes; \
    if ((node->next = tms->nodes) != NULL) \
        node->next->ref = &node->next; \
    tms->nodes = node; \
    \
    if (tms->lock) \
        apr_lock_release(tms->lock);
    
#define REMOVE_NODE(node, tms) \
        if (tms->lock) \
            apr_lock_acquire(tms->lock); \
        \
        *(node->ref) = node->next; \
        if ((*(node->ref) = node->next) != NULL) \
            node->next->ref = node->ref; \
        \
        if (tms->lock) \
            apr_lock_release(tms->lock);
    
static void *apr_sms_tracking_malloc(apr_sms_t *sms,
                                     apr_size_t size)
{
    apr_sms_tracking_t *tms;
    apr_track_node_t *node;

    node = apr_sms_malloc(sms->parent,
                          size + sizeof(apr_track_node_t));
    if (!node)
        return NULL;

    tms = (apr_sms_tracking_t *)sms;

    INSERT_NODE(node, tms)
    
    node++;

    return (void *)node;
}

static void *apr_sms_tracking_calloc(apr_sms_t *sms, 
                                     apr_size_t size)
{
    apr_sms_tracking_t *tms;
    apr_track_node_t *node;
  
    node = apr_sms_calloc(sms->parent,
                          size + sizeof(apr_track_node_t));
    if (!node)
        return NULL;

    tms = (apr_sms_tracking_t *)sms;

    INSERT_NODE(node, tms)

    node++;

    return (void *)node;
}

static void *apr_sms_tracking_realloc(apr_sms_t *sms,
                                      void *mem, apr_size_t size)
{
    apr_sms_tracking_t *tms;
    apr_track_node_t *node;

    tms = (apr_sms_tracking_t *)sms;
    node = (apr_track_node_t *)mem;

    if (node) {
        node--;

        REMOVE_NODE(node, tms)
    }

    node = apr_sms_realloc(sms->parent,
                           node, size + sizeof(apr_track_node_t));
    if (!node)
        return NULL;

    INSERT_NODE(node, tms)

    node++;

    return (void *)node;
}

static apr_status_t apr_sms_tracking_free(apr_sms_t *sms,
                                          void *mem)
{
    apr_track_node_t *node;
    apr_sms_tracking_t *tms;
   
    node = (apr_track_node_t *)mem;
    tms = (apr_sms_tracking_t *)sms;

    node--;

    REMOVE_NODE(node, tms);

    return apr_sms_free(sms->parent, node);
}

static apr_status_t apr_sms_tracking_reset(apr_sms_t *sms)
{
    apr_sms_tracking_t *tms;
    apr_track_node_t *node;
    apr_status_t rv;
 
    tms = (apr_sms_tracking_t *)sms;

    if (tms->lock)
        apr_lock_acquire(tms->lock);
    
    while (tms->nodes) {
        node = tms->nodes;
        if ((*(node->ref) = node->next) != NULL)
            node->next->ref = node->ref;
        if ((rv = apr_sms_free(sms->parent, 
                               node)) != APR_SUCCESS) {
            if (tms->lock) {
                apr_lock_release(tms->lock);
            }
            return rv;
        }
    }
    
    if (tms->lock)
        apr_lock_release(tms->lock);

    return APR_SUCCESS;
}

static apr_status_t apr_sms_tracking_pre_destroy(apr_sms_t *sms)
{
    /* This function WILL alwways be called.  However, be aware that the
     * main sms destroy function knows that it's not wise to try and destroy
     * the same piece of memory twice, so the destroy function in a child won't
     * neccesarily be called.  To guarantee we destroy the lock it's therefore
     * destroyed here.
     */
    apr_sms_tracking_t *tms;
 
    tms = (apr_sms_tracking_t *)sms;

    if (tms->lock) {
        apr_lock_acquire(tms->lock);
        apr_lock_destroy(tms->lock);
        tms->lock = NULL;
    }
    
    return APR_SUCCESS;    
}

static apr_status_t apr_sms_tracking_destroy(apr_sms_t *sms)
{
    apr_status_t rv;

    if ((rv = apr_sms_reset(sms)) != APR_SUCCESS)
        return rv;
    
    return apr_sms_free(sms->parent, sms);
}


APR_DECLARE(apr_status_t) apr_sms_tracking_create(apr_sms_t **sms, 
                                                  apr_sms_t *pms)
{
    apr_sms_t *new_sms;
    apr_sms_tracking_t *tms;
    apr_status_t rv;

    *sms = NULL;
    /* We're not a top level module, ie we have a parent, so
     * we allocate the memory for the structure from our parent.
     * This is safe as we shouldn't outlive our parent...
     */
    new_sms = apr_sms_calloc(pms, sizeof(apr_sms_tracking_t));

    if (!new_sms)
        return APR_ENOMEM;

    if ((rv = apr_sms_init(new_sms, pms)) != APR_SUCCESS)
        return rv;

    new_sms->malloc_fn      = apr_sms_tracking_malloc;
    new_sms->calloc_fn      = apr_sms_tracking_calloc;
    new_sms->realloc_fn     = apr_sms_tracking_realloc;
    new_sms->free_fn        = apr_sms_tracking_free;
    new_sms->reset_fn       = apr_sms_tracking_reset;
    new_sms->pre_destroy_fn = apr_sms_tracking_pre_destroy;
    new_sms->destroy_fn     = apr_sms_tracking_destroy;
    new_sms->identity       = module_identity;
    
    tms = (apr_sms_tracking_t *)new_sms;
    tms->nodes = NULL;

    apr_sms_post_init(new_sms);

    *sms = new_sms;
    return APR_SUCCESS;
}

