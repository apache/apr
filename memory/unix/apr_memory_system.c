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
#include "apr_memory_system.h"
#include <stdlib.h>
#include <assert.h>

#include <memory.h> /* strikerXXX: had to add this for windows to stop 
                     * complaining, please autoconf the include stuff
                     */

/*
 * private structure defenitions
 */
struct apr_memory_system_cleanup
{
    struct apr_memory_system_cleanup *next;
    void *data;
    apr_status_t (*cleanup_fn)(void *);
};

/* 
 * memory allocation functions
 */

APR_DECLARE(void *)
apr_memory_system_malloc(apr_memory_system_t *memory_system,
                         apr_size_t size)
{
    assert(memory_system != NULL);
    assert(memory_system->malloc_fn != NULL);
 
    if (size == 0)
        return NULL;

    return memory_system->malloc_fn(memory_system, size);
}

APR_DECLARE(void *)
apr_memory_system_realloc(apr_memory_system_t *memory_system,
                          void *mem,
                          apr_size_t size)
{
    assert(memory_system != NULL);
    assert(memory_system->realloc_fn != NULL);

    if (mem == NULL)
        return apr_memory_system_malloc(memory_system, size);

    if (size == 0)
    {
        apr_memory_system_free(memory_system, mem);
        return NULL;
    }

    return memory_system->realloc_fn(memory_system, mem, size);
}

APR_DECLARE(apr_status_t)
apr_memory_system_free(apr_memory_system_t *memory_system,
                       void *mem)
{
    assert(memory_system != NULL);
  
    if (mem == NULL)
        return APR_EINVAL; /* Hmm, is this an error??? */

    if (memory_system->free_fn != NULL)
        memory_system->free_fn(memory_system, mem);  

#ifdef APR_MEMORY_SYSTEM_DEBUG
    else /* assume this is a tracking memory system */
    {
        /* issue a warning: 
         * WARNING: Called apr_memory_system_free() on a tracking memory system
         */
    }
#endif /* APR_MEMORY_SYSTEM_DEBUG */
    return APR_SUCCESS;
}

/*
 * memory system functions
 */

static int apr_memory_system_is_tracking(apr_memory_system_t *memory_system)
{
    /*
     * The presense of a reset function gives us the clue that this is a 
     * tracking memory system.
     */
    return memory_system->reset_fn != NULL;
}

APR_DECLARE(apr_memory_system_t *)
apr_memory_system_create(void *memory, 
                         apr_memory_system_t *parent_memory_system)
{
    apr_memory_system_t *memory_system;

    if (memory == NULL)
        return NULL;

    /* Just typecast it, and clear it */
    memory_system = (apr_memory_system_t *)memory;
    memset(memory_system, '\0', sizeof(apr_memory_system_t));

    /* Initialize the parent and accounting memory system pointers */
    memory_system->parent_memory_system = parent_memory_system;
    memory_system->accounting_memory_system = memory_system;

    if (parent_memory_system != NULL)
    {
        if ((memory_system->sibling_memory_system = 
           parent_memory_system->child_memory_system) != NULL)
        {
            memory_system->sibling_memory_system->ref_memory_system = 
              &memory_system->sibling_memory_system;
        }
        memory_system->ref_memory_system = 
          &parent_memory_system->child_memory_system;
        parent_memory_system->child_memory_system = memory_system;
    }
    /* This seems a bit redundant, but we're not taking chances */
    else
    {
        memory_system->ref_memory_system = NULL;
        memory_system->sibling_memory_system = NULL;
        memory_system->child_memory_system = NULL;
    }

    return memory_system;
}

#ifdef APR_MEMORY_SYSTEM_DEBUG
APR_DECLARE(void) 
apr_memory_system_assert(apr_memory_system_t *memory_system)
{
    apr_memory_system_t *parent;

    /*
     * A memory system without a malloc won't do us much good
     */
    assert(memory_system->malloc_fn != NULL);

    /* 
     * Check to see if this is either a non-tracking or
     * a tracking memory system. It has to have at least a free
     * or destroy function. And to avoid half implementations
     * we require reset to be present when destroy is.
     */
    assert(memory_system->free_fn != NULL ||
         (memory_system->destroy_fn != NULL &&
          memory_system->reset_fn != NULL));

    assert(memory_system->destroy_fn == NULL ||
         memory_system->reset_fn != NULL);
  
    assert(memory_system->reset_fn == NULL ||
         memory_system->destroy_fn != NULL);

    /*
     * Make sure all accounting memory dies with the memory system.
     * To be more specific, make sure the accounting memort system
     * is either the memory system itself or a direct child.
     */
    assert(memory_system->accounting_memory_system == memory_system ||
	 memory_system->accounting_memory_system->parent_memory_system == 
	   memory_system);

    /*
     * A non-tracking memory system can be the child of
     * another non-tracking memory system if there are no
     * tracking ancestors, but in that specific case we issue a
     * warning.
     */
    if (memory_system->parent_memory_system == NULL)
        return;

    parent = memory_system
    while (parent)
    {
        if (apr_memory_system_is_tracking(parent))
            return; /* Tracking memory system found, return satisfied ;-) */

        parent = parent->parent_memory_system;
    }

    /* issue a warning: 
     * WARNING: A non-tracking memory system was created without a tracking 
     * parent.
     */
}
#endif /* APR_MEMORY_SYSTEM_DEBUG */

/*
 * LOCAL FUNCTION used in:
 *  - apr_memory_system_do_child_cleanups
 *  - apr_memory_system_reset
 *  - apr_memory_system_destroy
 *
 * Call all the cleanup routines registered with a memory system.
 */
static
void
apr_memory_system_do_cleanups(apr_memory_system_t *memory_system)
{
    struct apr_memory_system_cleanup *cleanup;

    cleanup = memory_system->cleanups;
    while (cleanup)
    {
        cleanup->cleanup_fn(cleanup->data);
        cleanup = cleanup->next;
    }
}

/*
 * LOCAL FUNCTION used in:
 *  - apr_memory_system_reset
 *  - apr_memory_system_destroy
 *
 * This not only calls do_cleanups, but also calls the pre_destroy(!)
 */
static 
void 
apr_memory_system_do_child_cleanups(apr_memory_system_t *memory_system)
{
    if (memory_system == NULL)
        return;

    memory_system = memory_system->child_memory_system;
    while (memory_system)
    {
        apr_memory_system_do_child_cleanups(memory_system);
        apr_memory_system_do_cleanups(memory_system);

        if (memory_system->pre_destroy_fn != NULL)
            memory_system->pre_destroy_fn(memory_system);

        memory_system = memory_system->sibling_memory_system;
    }
}

APR_DECLARE(void)
apr_memory_system_reset(apr_memory_system_t *memory_system)
{
    assert(memory_system != NULL);
    /* Assert when called on a non-tracking memory system */
    assert(memory_system->reset_fn != NULL);

    /* 
     * Run the cleanups of all child memory systems _including_
     * the accounting memory system.
     */
    apr_memory_system_do_child_cleanups(memory_system);

    /* Run all cleanups, the memory will be freed by the reset */
    apr_memory_system_do_cleanups(memory_system);
    memory_system->cleanups = NULL;

    /* We don't have any child memory systems after the reset */
    memory_system->child_memory_system = NULL;

    /* Reset the accounting memory system to ourselves, any
     * child memory system _including_ the accounting memory
     * system will be destroyed by the reset 
     * strikerXXX: Maybe this should be the responsibility of
     *             the reset function(?).
     */
    memory_system->accounting_memory_system = memory_system;

    /* Let the memory system handle the actual reset */
    memory_system->reset_fn(memory_system);
}

APR_DECLARE(apr_status_t)
apr_memory_system_destroy(apr_memory_system_t *memory_system)
{
    apr_memory_system_t *child_memory_system;
    apr_memory_system_t *sibling_memory_system;
    struct apr_memory_system_cleanup *cleanup;
    struct apr_memory_system_cleanup *next_cleanup;

    assert(memory_system != NULL);

    if (apr_memory_system_is_tracking(memory_system))
    {
        /* 
         * Run the cleanups of all child memory systems _including_
         * the accounting memory system.
         */
        apr_memory_system_do_child_cleanups(memory_system);

        /* Run all cleanups, the memory will be freed by the destroy */
        apr_memory_system_do_cleanups(memory_system);
    }
    else
    {
        if (memory_system->accounting_memory_system != memory_system)
        {
            child_memory_system = memory_system->accounting_memory_system;
		    
            /* 
             * Remove the accounting memory system from the memory systems 
             * child list (we will explicitly destroy it later in this block).
             */
            if (child_memory_system->sibling_memory_system != NULL)
	            child_memory_system->sibling_memory_system->ref_memory_system =
	              child_memory_system->ref_memory_system;

            *child_memory_system->ref_memory_system = 
            child_memory_system->sibling_memory_system;

            /* Set this fields so destroy will work */
            child_memory_system->ref_memory_system = NULL;
            child_memory_system->sibling_memory_system = NULL;
        }

        /* Visit all children and destroy them */
        child_memory_system = memory_system->child_memory_system;
        while (child_memory_system != NULL)
        {
            sibling_memory_system = child_memory_system->sibling_memory_system;
            apr_memory_system_destroy(child_memory_system);
            child_memory_system = sibling_memory_system;
        }

        /*
         * If the accounting memory system _is_ tracking, we also know that it is
         * not the memory system itself.
         */
        if (apr_memory_system_is_tracking(memory_system->accounting_memory_system))
        {
            /* 
             * Run all cleanups, the memory will be freed by the destroying of the
             * accounting memory system.
             */
            apr_memory_system_do_cleanups(memory_system);

            /* Destroy the accounting memory system */
            apr_memory_system_destroy(memory_system->accounting_memory_system);

            /* 
             * Set the accounting memory system back to the parent memory system
             * just in case...
             */
            memory_system->accounting_memory_system = memory_system;
        }
        else
        {
            /* Run all cleanups, free'ing memory as we go */
            cleanup = memory_system->cleanups;
            while (cleanup)
            {
                cleanup->cleanup_fn(cleanup->data);
                next_cleanup = cleanup->next;
                apr_memory_system_free(memory_system->accounting_memory_system, 
			       cleanup);
                cleanup = next_cleanup;
            }

            if (memory_system->accounting_memory_system != memory_system)
            {
                /* Destroy the accounting memory system */
                apr_memory_system_destroy(memory_system->accounting_memory_system);
                /* 
                 * Set the accounting memory system back to the parent memory system
                 * just in case...
                 */
                memory_system->accounting_memory_system = memory_system;
            }
       }
  }

  /* Remove the memory system from the parent memory systems child list */
  if (memory_system->sibling_memory_system != NULL)
      memory_system->sibling_memory_system->ref_memory_system =
        memory_system->ref_memory_system;
  if (memory_system->ref_memory_system != NULL)
      *memory_system->ref_memory_system = memory_system->sibling_memory_system;

  /* Call the pre-destroy if present */
  if (memory_system->pre_destroy_fn != NULL)
      memory_system->pre_destroy_fn(memory_system);

  /* 1 - If we have a self destruct, use it */
  if (memory_system->destroy_fn != NULL)
      memory_system->destroy_fn(memory_system);

  /* 2 - If we don't have a parent, free using ourselves */
  else if (memory_system->parent_memory_system == NULL)
      memory_system->free_fn(memory_system, memory_system);

  /* 3 - If we do have a parent and it has a free function, use it */
  else if (memory_system->parent_memory_system->free_fn != NULL)
      apr_memory_system_free(memory_system->parent_memory_system, memory_system);

  /* 4 - Assume we are the child of a tracking memory system, and do nothing */
#ifdef APR_MEMORY_SYSTEM_DEBUG
    memory_system = memory_system->parent_memory_system;
    while (memory_system)
    {
        if (apr_memory_system_is_tracking(memory_system))
            return APR_SUCCESS;

        memory_system = memory_system->parent_memory_system;
    }

    assert(0); /* Made the wrong assumption, so we assert */
#endif /* APR_MEMORY_SYSTEM_DEBUG */
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t)
apr_memory_system_is_ancestor(apr_memory_system_t *a,
                              apr_memory_system_t *b)
{
    assert(b != NULL);

    while (b && b != a)
        b = b->parent_memory_system;

    /* strikerXXX: should this be: return b == a ? APR_TRUE : APR_FALSE; */
    /* APR_SUCCESS = 0, so if they agree we should return that... */
    return !(b == a); 
}

APR_DECLARE(void)
apr_memory_system_threadsafe_lock(apr_memory_system_t *memory_system)
{
    assert(memory_system != NULL);
    if (memory_system->threadsafe_lock_fn != NULL)
        memory_system->threadsafe_lock_fn(memory_system);
}

APR_DECLARE(void)
apr_memory_system_threadsafe_unlock(apr_memory_system_t *memory_system)
{
    assert(memory_system != NULL);
    if (memory_system->threadsafe_unlock_fn != NULL)
        memory_system->threadsafe_unlock_fn(memory_system);
}

/*
 * memory system cleanup management functions
 */

APR_DECLARE(apr_status_t)
apr_memory_system_cleanup_register(apr_memory_system_t *memory_system,
                                   void *data,
                                   apr_status_t (*cleanup_fn)(void *))
{
    struct apr_memory_system_cleanup *cleanup;

    assert(memory_system != NULL);
    assert(memory_system->accounting_memory_system != NULL);

    /*
     * If someone passes us a NULL cleanup_fn, assert, because the cleanup
     * code can't handle it _and_ it makes no sense.
     */
    cleanup = (struct apr_memory_system_cleanup *)
	    apr_memory_system_malloc(memory_system->accounting_memory_system,
				     sizeof(struct apr_memory_system_cleanup));

    /* See if we actually got the memory */
    if (cleanup == NULL)
        return APR_ENOMEM; /* strikerXXX: Should this become APR_FALSE? */

    cleanup->data = data;
    cleanup->cleanup_fn = cleanup_fn;
    cleanup->next = memory_system->cleanups;
    memory_system->cleanups = cleanup;

    return APR_SUCCESS; /* strikerXXX: Should this become APR_TRUE? */
}

APR_DECLARE(apr_status_t)
apr_memory_system_cleanup_unregister(apr_memory_system_t *memory_system,
                                     void *data,
                                     apr_status_t (*cleanup_fn)(void *))
{
    struct apr_memory_system_cleanup *cleanup;
    struct apr_memory_system_cleanup **cleanup_ref;

    assert(memory_system != NULL);
    assert(memory_system->accounting_memory_system != NULL);

    cleanup = memory_system->cleanups;
    cleanup_ref = &memory_system->cleanups;
    while (cleanup)
    {
        if (cleanup->data == data && cleanup->cleanup_fn == cleanup_fn)
        {
            *cleanup_ref = cleanup->next;

            memory_system = memory_system->accounting_memory_system;

            if (memory_system->free_fn != NULL)
                apr_memory_system_free(memory_system, cleanup);

            return APR_SUCCESS; /* strikerXXX: Should this become APR_TRUE? */
        }

        cleanup_ref = &cleanup->next;
        cleanup = cleanup->next;
    }

    /* The cleanup function should have been registered previously */
    return APR_ENOCLEANUP;
}

APR_DECLARE(apr_status_t)
apr_memory_system_cleanup_run(apr_memory_system_t *memory_system, 
			      void *data, 
			      apr_status_t (*cleanup_fn)(void *))
{
    apr_memory_system_cleanup_unregister(memory_system, data, cleanup_fn);
    return cleanup_fn(data);
}
