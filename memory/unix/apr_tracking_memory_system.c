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

/* This code donated to APR by 
 *    Elrond  <elrond@samba-tng.org>
 *    Luke Kenneth Casson Leighton <lkcl@samba-tng.org>
 *    Sander Striker <striker@samba-tng.org>
 */

#include "apr.h"
#include "apr_general.h"
#include "apr_private.h"
#include "apr_tracking_memory_system.h"
#include <stdlib.h>
#include <assert.h>

/*
 * tracking memory system
 */

/* INTERNALLY USED STRUCTURES */
typedef struct apr_track_node_t
{
  struct apr_track_node_t *next;
  struct apr_track_node_t **ref;
} apr_track_node_t;

typedef struct apr_tracking_memory_system_t
{
  apr_memory_system_t  header;
  apr_track_node_t    *nodes;
} apr_tracking_memory_system_t;

static 
void *
apr_tracking_memory_system_malloc(apr_memory_system_t *memory_system,
                                  size_t size)
{
  apr_tracking_memory_system_t *tracking_memory_system;
  apr_track_node_t *node;
  
  assert (memory_system != NULL);

  tracking_memory_system = (apr_tracking_memory_system_t *)memory_system;
  node = apr_memory_system_malloc(memory_system->parent_memory_system,
				  size + sizeof(apr_track_node_t));
  if (node == NULL)
    return NULL;

  node->next = tracking_memory_system->nodes;
  tracking_memory_system->nodes = node;
  node->ref = &tracking_memory_system->nodes;
  if (node->next != NULL)
    node->next->ref = &node->next;

  node++;

  return (void *)node;
}

static 
void *
apr_tracking_memory_system_realloc(apr_memory_system_t *memory_system,
                                   void *mem,
                                   size_t size)
{
  apr_tracking_memory_system_t *tracking_memory_system;
  apr_track_node_t *node;

  assert (memory_system != NULL);

  tracking_memory_system = (apr_tracking_memory_system_t *)memory_system;
  node = (apr_track_node_t *)mem;

  if (node != NULL)
  {
    node--;
    *(node->ref) = node->next;
  }

  node = apr_memory_system_realloc(memory_system->parent_memory_system,
				   node, size + sizeof(apr_track_node_t));
  if (node == NULL)
    return NULL;

  node->next = tracking_memory_system->nodes;
  tracking_memory_system->nodes = node;
  node->ref = &tracking_memory_system->nodes;
  if (node->next != NULL)
    node->next->ref = &node->next;

  node++;

  return (void *)node;
}

static 
apr_status_t
apr_tracking_memory_system_free(apr_memory_system_t *memory_system,
                                void *mem)
{
  apr_track_node_t *node;

  assert (memory_system != NULL);
  assert (mem != NULL);

  node = (apr_track_node_t *)mem;
  node--;

  *(node->ref) = node->next;
  
  return apr_memory_system_free(memory_system->parent_memory_system, node);
}

static
apr_status_t
apr_tracking_memory_system_reset(apr_memory_system_t *memory_system)
{
    apr_tracking_memory_system_t *tracking_memory_system;
    apr_track_node_t *node;
    apr_status_t rv;
    
    assert (memory_system != NULL);

    tracking_memory_system = (apr_tracking_memory_system_t *)memory_system;

    while (tracking_memory_system->nodes != NULL)
    {
        node = tracking_memory_system->nodes;
        *(node->ref) = node->next;
        if ((rv = apr_memory_system_free(memory_system->parent_memory_system,
                                         node)) != APR_SUCCESS)
            return rv;
    }
    return APR_SUCCESS;
}

static
void
apr_tracking_memory_system_destroy(apr_memory_system_t *memory_system)
{
  assert (memory_system != NULL);

  apr_tracking_memory_system_reset(memory_system);
  apr_memory_system_free(memory_system->parent_memory_system, memory_system);
}

APR_DECLARE(apr_status_t)
apr_tracking_memory_system_create(apr_memory_system_t **memory_system,
				  apr_memory_system_t *parent_memory_system)
{
  apr_memory_system_t *new_memory_system;
  apr_tracking_memory_system_t *tracking_memory_system;

  assert (memory_system != NULL);
  assert (parent_memory_system != NULL);

  new_memory_system = apr_memory_system_create(
    apr_memory_system_malloc(parent_memory_system, 
                             sizeof(apr_tracking_memory_system_t)), 
			     parent_memory_system);

  *memory_system = NULL;
  if (new_memory_system == NULL)
    return APR_ENOMEM;

  new_memory_system->malloc_fn = apr_tracking_memory_system_malloc;
  new_memory_system->realloc_fn = apr_tracking_memory_system_realloc;
  new_memory_system->free_fn = apr_tracking_memory_system_free;
  new_memory_system->reset_fn = apr_tracking_memory_system_reset;
  new_memory_system->destroy_fn = apr_tracking_memory_system_destroy;

  tracking_memory_system = (apr_tracking_memory_system_t *)new_memory_system;
  tracking_memory_system->nodes = NULL;

  apr_memory_system_assert(new_memory_system);

  *memory_system = new_memory_system;
  return APR_SUCCESS;
}
