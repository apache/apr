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
#include "apr_private.h"
#include "apr_general.h"
#include "apr_memory_system.h"
#include <stdlib.h>
#include <assert.h>

#include <memory.h> /* strikerXXX: had to add this for windows to stop 
                     * complaining, please autoconf the include stuff
		     */

/*
 * standard memory system
 */

static 
void *
apr_standard_memory_system_malloc(apr_memory_system_t *memory_system,
                                  size_t size)
{
  return malloc(size);
}

static 
void *
apr_standard_memory_system_realloc(apr_memory_system_t *memory_system,
                                   void *mem,
                                   size_t size)
{
  return realloc(mem, size);
}

static 
void
apr_standard_memory_system_free(apr_memory_system_t *memory_system,
                                void *mem)
{
  free(mem);
}

APR_DECLARE(apr_status_t)
apr_standard_memory_system_create(apr_memory_system_t **memory_system)
{
  apr_memory_system_t *new_memory_system;

  assert(memory_system != NULL);

  *memory_system = NULL;
  new_memory_system = apr_memory_system_create(
    malloc(sizeof(apr_memory_system_t)), NULL);

  if (new_memory_system == NULL)
    return APR_ENOMEM;

  new_memory_system->malloc_fn = apr_standard_memory_system_malloc;
  new_memory_system->realloc_fn = apr_standard_memory_system_realloc;
  new_memory_system->free_fn = apr_standard_memory_system_free;

  apr_memory_system_assert(new_memory_system);

  *memory_system = new_memory_system;
  return APR_SUCCESS;
}
