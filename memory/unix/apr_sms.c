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
#include "apr_sms.h"
#include <stdlib.h>
#include <assert.h>

#include <memory.h> /* strikerXXX: had to add this for windows to stop 
                     * complaining, please autoconf the include stuff
                     */

/*
 * private structure defenitions
 */
struct apr_sms_cleanup
{
    struct apr_sms_cleanup *next;
    apr_int32_t type;
    void *data;
    apr_status_t (*cleanup_fn)(void *);
};

/* 
 * memory allocation functions
 */

APR_DECLARE(void *) apr_sms_malloc(apr_sms_t *mem_sys,
                                   apr_size_t size)
{
#ifdef APR_ASSERT_MEMORY
    assert(mem_sys);
    assert(mem_sys->malloc_fn);
#endif

    if (!mem_sys || !mem_sys->malloc_fn)
        return NULL;

    if (size == 0)
        return NULL;

    return mem_sys->malloc_fn(mem_sys, size);
}

APR_DECLARE(void *) apr_sms_calloc(apr_sms_t *mem_sys,
                                   apr_size_t size)
{
#ifdef APR_ASSERT_MEMORY
    assert(mem_sys);
    assert(mem_sys->malloc_fn);
#endif

    if (size == 0)
        return NULL;

    if (!mem_sys->calloc_fn){
        /* Assumption - if we don't have calloc we have
         * malloc, might be bogus...
         */
        void *mem = mem_sys->malloc_fn(mem_sys, size);
        memset(mem, '\0', size);
        return mem;
    } else
        return mem_sys->calloc_fn(mem_sys, size);
}

APR_DECLARE(void *) apr_sms_realloc(apr_sms_t *mem_sys, void *mem,
                                    apr_size_t size)
{
#ifdef APR_ASSERT_MEMORY
    assert(mem_sys);
    assert(mem_sys->realloc_fn);
#endif
   
    if (!mem)
        return apr_sms_malloc(mem_sys, size);

    if (size == 0)
    {
        apr_sms_free(mem_sys, mem);
        return NULL;
    }

    return mem_sys->realloc_fn(mem_sys, mem, size);
}

APR_DECLARE(apr_status_t) apr_sms_free(apr_sms_t *mem_sys,
                                       void *mem)
{
    if (!mem_sys)
        return APR_EMEMSYS;
       
#ifdef APR_ASSERT_MEMORY
    assert(mem_sys->free_fn);
#endif

    if (!mem)
        return APR_EINVAL;

    if (!mem_sys->free_fn)
        return mem_sys->free_fn(mem_sys, mem);  

    return APR_EMEMFUNC;
}

/*
 * memory system functions
 */

static int apr_sms_is_tracking(apr_sms_t *mem_sys)
{
    /*
     * The presense of a reset function gives us the clue that this is a 
     * tracking memory system.
     */
    return mem_sys->reset_fn != NULL;
}

APR_DECLARE(apr_sms_t *) apr_sms_create(void *memory, 
                                        apr_sms_t *parent_mem_sys)
{
    apr_sms_t *mem_sys;

    if (!memory)
        return NULL;

    /* Just typecast it, and clear it */
    mem_sys = (apr_sms_t *)memory;
    memset(mem_sys, '\0', sizeof(apr_sms_t));

    /* Initialize the parent and accounting memory system pointers */
    mem_sys->parent_mem_sys = parent_mem_sys;
    mem_sys->accounting_mem_sys = mem_sys;

    if (parent_mem_sys != NULL){
        if ((mem_sys->sibling_mem_sys = parent_mem_sys->child_mem_sys)){
            mem_sys->sibling_mem_sys->ref_mem_sys = &mem_sys->sibling_mem_sys;
        }
        mem_sys->ref_mem_sys = &parent_mem_sys->child_mem_sys;
        parent_mem_sys->child_mem_sys = mem_sys;
    }
    /* This seems a bit redundant, but we're not taking chances */
    else
    {
        mem_sys->ref_mem_sys = NULL;
        mem_sys->sibling_mem_sys = NULL;
        mem_sys->child_mem_sys = NULL;
    }

    return mem_sys;
}

#ifdef APR_MEMORY_ASSERT
APR_DECLARE(void) apr_sms_assert(apr_sms_t *mem_sys)
{
    apr_sms_t *parent;

    /*
     * A memory system without a malloc won't do us much good
     */
    assert(mem_sys->malloc_fn != NULL);

    /* 
     * Check to see if this is either a non-tracking or
     * a tracking memory system. It has to have at least a free
     * or destroy function. And to avoid half implementations
     * we require reset to be present when destroy is.
     */
    assert(mem_sys->free_fn || (mem_sys->destroy_fn && mem_sys->reset_fn));

    assert(!mem_sys->destroy_fn || mem_sys->reset_fn);
  
    assert(!mem_sys->reset_fn || mem_sys->destroy_fn);

    /*
     * Make sure all accounting memory dies with the memory system.
     * To be more specific, make sure the accounting memort system
     * is either the memory system itself or a direct child.
     */
    assert(mem_sys->accounting_mem_sys == mem_sys ||
	 mem_sys->accounting_mem_sys->parent_mem_sys ==  mem_sys);

    /*
     * A non-tracking memory system can be the child of
     * another non-tracking memory system if there are no
     * tracking ancestors, but in that specific case we issue a
     * warning.
     */
    if (mem_sys->parent_mem_sys == NULL)
        return;

    parent = mem_sys
    while (parent){
        if (apr_sms_is_tracking(parent))
            return; /* Tracking memory system found, return satisfied ;-) */

        parent = parent->parent_mem_sys;
    }

    /* issue a warning: 
     * WARNING: A non-tracking memory system was created without a tracking 
     * parent.
     */
}
#endif /* APR_MEMORY_ASSERT */

/*
 * LOCAL FUNCTION used in:
 *  - apr_sms_do_child_cleanups
 *  - apr_sms_reset
 *  - apr_sms_destroy
 *
 * Call all the cleanup routines registered with a memory system.
 */
static void apr_sms_do_cleanups(apr_sms_t *mem_sys)
{
    struct apr_sms_cleanup *cleanup;

    cleanup = mem_sys->cleanups;
    while (cleanup)
    {
        cleanup->cleanup_fn(cleanup->data);
        cleanup = cleanup->next;
    }
}

/*
 * LOCAL FUNCTION used in:
 *  - apr_sms_reset
 *  - apr_sms_destroy
 *
 * This not only calls do_cleanups, but also calls the pre_destroy(!)
 */
static void apr_sms_do_child_cleanups(apr_sms_t *mem_sys)
{
    if (!mem_sys)
        return;

    mem_sys = mem_sys->child_mem_sys;
    while (mem_sys){
        apr_sms_do_child_cleanups(mem_sys);
        apr_sms_do_cleanups(mem_sys);

        if (mem_sys->pre_destroy_fn != NULL)
            mem_sys->pre_destroy_fn(mem_sys);

        mem_sys = mem_sys->sibling_mem_sys;
    }
}

APR_DECLARE(apr_status_t) apr_sms_reset(apr_sms_t *mem_sys)
{
    if (!mem_sys)
        return APR_EMEMSYS;

    if (!mem_sys->reset_fn)
        return APR_EMEMFUNC;

    /* 
     * Run the cleanups of all child memory systems _including_
     * the accounting memory system.
     */
    apr_sms_do_child_cleanups(mem_sys);

    /* Run all cleanups, the memory will be freed by the reset */
    apr_sms_do_cleanups(mem_sys);
    mem_sys->cleanups = NULL;

    /* We don't have any child memory systems after the reset */
    mem_sys->child_mem_sys = NULL;

    /* Reset the accounting memory system to ourselves, any
     * child memory system _including_ the accounting memory
     * system will be destroyed by the reset 
     * strikerXXX: Maybe this should be the responsibility of
     *             the reset function(?).
     */
    mem_sys->accounting_mem_sys = mem_sys;

    /* Let the memory system handle the actual reset */
    return mem_sys->reset_fn(mem_sys);
}

APR_DECLARE(apr_status_t) apr_sms_destroy(apr_sms_t *mem_sys)
{
    apr_sms_t *child_mem_sys;
    apr_sms_t *sibling_mem_sys;
    struct apr_sms_cleanup *cleanup;
    struct apr_sms_cleanup *next_cleanup;

    if (!mem_sys)
        return APR_EMEMSYS;

    if (apr_sms_is_tracking(mem_sys)){
        /* 
         * Run the cleanups of all child memory systems _including_
         * the accounting memory system.
         */
        apr_sms_do_child_cleanups(mem_sys);

        /* Run all cleanups, the memory will be freed by the destroy */
        apr_sms_do_cleanups(mem_sys);
    }
    else
    {
        if (mem_sys->accounting_mem_sys != mem_sys)
        {
            child_mem_sys = mem_sys->accounting_mem_sys;
		    
            /* 
             * Remove the accounting memory system from the memory systems 
             * child list (we will explicitly destroy it later in this block).
             */
            if (child_mem_sys->sibling_mem_sys != NULL)
	            child_mem_sys->sibling_mem_sys->ref_mem_sys = child_mem_sys->ref_mem_sys;

            *child_mem_sys->ref_mem_sys = child_mem_sys->sibling_mem_sys;

            /* Set this fields so destroy will work */
            child_mem_sys->ref_mem_sys = NULL;
            child_mem_sys->sibling_mem_sys = NULL;
        }

        /* Visit all children and destroy them */
        child_mem_sys = mem_sys->child_mem_sys;
        while (child_mem_sys != NULL){
            sibling_mem_sys = child_mem_sys->sibling_mem_sys;
            apr_sms_destroy(child_mem_sys);
            child_mem_sys = sibling_mem_sys;
        }

        /*
         * If the accounting memory system _is_ tracking, we also know that it is
         * not the memory system itself.
         */
        if (apr_sms_is_tracking(mem_sys->accounting_mem_sys)){
            /* 
             * Run all cleanups, the memory will be freed by the destroying of the
             * accounting memory system.
             */
            apr_sms_do_cleanups(mem_sys);

            /* Destroy the accounting memory system */
            apr_sms_destroy(mem_sys->accounting_mem_sys);

            /* 
             * Set the accounting memory system back to the parent memory system
             * just in case...
             */
            mem_sys->accounting_mem_sys = mem_sys;
        }
        else
        {
            /* Run all cleanups, free'ing memory as we go */
            cleanup = mem_sys->cleanups;
            while (cleanup){
                cleanup->cleanup_fn(cleanup->data);
                next_cleanup = cleanup->next;
                apr_sms_free(mem_sys->accounting_mem_sys, cleanup);
                cleanup = next_cleanup;
            }

            if (mem_sys->accounting_mem_sys != mem_sys)
            {
                /* Destroy the accounting memory system */
                apr_sms_destroy(mem_sys->accounting_mem_sys);
                /* 
                 * Set the accounting memory system back to the parent memory system
                 * just in case...
                 */
                mem_sys->accounting_mem_sys = mem_sys;
            }
       }
  }

  /* Remove the memory system from the parent memory systems child list */
  if (mem_sys->sibling_mem_sys)
      mem_sys->sibling_mem_sys->ref_mem_sys = mem_sys->ref_mem_sys;

  if (mem_sys->ref_mem_sys)
      *mem_sys->ref_mem_sys = mem_sys->sibling_mem_sys;

  /* Call the pre-destroy if present */
  if (mem_sys->pre_destroy_fn)
      mem_sys->pre_destroy_fn(mem_sys);

  /* 1 - If we have a self destruct, use it */
  if (mem_sys->destroy_fn)
      return mem_sys->destroy_fn(mem_sys);

  /* 2 - If we don't have a parent, free using ourselves */
  else if (!mem_sys->parent_mem_sys)
      return mem_sys->free_fn(mem_sys, mem_sys);

  /* 3 - If we do have a parent and it has a free function, use it */
  else if (mem_sys->parent_mem_sys->free_fn)
      return apr_sms_free(mem_sys->parent_mem_sys, mem_sys);

  /* 4 - Assume we are the child of a tracking memory system, and do nothing */
#ifdef APR_ASSERT_MEMORY
    mem_sys = mem_sys->parent_mem_sys;
    while (mem_sys){
        if (apr_sms_is_tracking(mem_sys))
            return APR_SUCCESS;

        mem_sys = mem_sys->parent_mem_sys;
    }
    assert(0); /* Made the wrong assumption, so we assert */
#endif /* APR_MEMORY_ASSERT */
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_sms_is_ancestor(apr_sms_t *a,
                                              apr_sms_t *b)
{
    if (!b)
        return APR_EMEMSYS;

#ifdef APR_ASSERT_MEMORY
    assert(a != NULL);
    assert(b != NULL);
#endif
        
    while (b && b != a)
        b = b->parent_mem_sys;

    /* APR_SUCCESS = 0, so if they agree we should return that... */
    return !(b == a); 
}

APR_DECLARE(apr_status_t) apr_sms_threadsafe_lock(apr_sms_t *mem_sys)
{
    if (!mem_sys)
        return APR_EMEMSYS;       

    if (!mem_sys->lock_fn)
        return APR_EMEMFUNC;

    if (mem_sys->lock_fn)
        return mem_sys->lock_fn(mem_sys);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_sms_threadsafe_unlock(apr_sms_t *mem_sys)
{
    if (!mem_sys)
        return APR_EMEMSYS;

    if (!mem_sys->unlock_fn)
        return APR_EMEMFUNC;
        
    if (mem_sys->unlock_fn)
        return mem_sys->unlock_fn(mem_sys);

    return APR_SUCCESS;
}

/*
 * memory system cleanup management functions
 */

APR_DECLARE(apr_status_t) apr_sms_cleanup_register(apr_sms_t *mem_sys, 
                                                   apr_int32_t type, void *data,
                                                   apr_status_t (*cleanup_fn)(void *))
{
    struct apr_sms_cleanup *cleanup;

    if (!mem_sys)
        return APR_EMEMSYS;

#ifdef APR_ASSERT_MEMORY
    assert(mem_sys->accounting_mem_sys != NULL);
#endif
    
    if (!cleanup_fn)
        return APR_EMEMFUNC;

    cleanup = (struct apr_sms_cleanup *)
	    apr_sms_malloc(mem_sys->accounting_mem_sys, sizeof(struct apr_sms_cleanup));

    if (!cleanup)
        return APR_ENOMEM;

    cleanup->data = data;
    cleanup->type = type;
    cleanup->cleanup_fn = cleanup_fn;
    cleanup->next = mem_sys->cleanups;
    mem_sys->cleanups = cleanup;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t)
apr_sms_cleanup_unregister(apr_sms_t *mem_sys, apr_int32_t type, void *data,
                           apr_status_t (*cleanup_fn)(void *))
{
    struct apr_sms_cleanup *cleanup;
    struct apr_sms_cleanup **cleanup_ref;

    if (!mem_sys)
        return APR_EMEMSYS;

#ifdef APR_ASSERT_MEMORY
    assert(mem_sys->accounting_mem_sys != NULL);
#endif
        
    cleanup = mem_sys->cleanups;
    cleanup_ref = &mem_sys->cleanups;
    while (cleanup)
    {
        if ((type == 0 || cleanup->cleanup_fn == cleanup_fn) &&
            cleanup->data == data && cleanup->cleanup_fn == cleanup_fn) {
            *cleanup_ref = cleanup->next;

            mem_sys = mem_sys->accounting_mem_sys;

            if (mem_sys->free_fn != NULL)
                apr_sms_free(mem_sys, cleanup);

            return APR_SUCCESS;
        }

        cleanup_ref = &cleanup->next;
        cleanup = cleanup->next;
    }

    /* The cleanup function should have been registered previously */
    return APR_ENOCLEANUP;
}

APR_DECLARE(apr_status_t) apr_sms_cleanup_unregister_type(apr_sms_t *mem_sys, 
                                                          apr_int32_t type)
{
    struct apr_sms_cleanup *cleanup;
    struct apr_sms_cleanup **cleanup_ref;
    apr_status_t rv = APR_ENOCLEANUP;

    if (!mem_sys)
        return APR_EMEMSYS;
        
#ifdef APR_ASSERT_MEMORY
    assert(mem_sys->accounting_mem_sys != NULL);
#endif
    
    cleanup = mem_sys->cleanups;
    cleanup_ref = &mem_sys->cleanups;
    mem_sys = mem_sys->accounting_mem_sys;
    while (cleanup) {
        if (type == 0 || cleanup->type == type) {
            *cleanup_ref = cleanup->next;

            if (mem_sys->free_fn != NULL)
                apr_sms_free(mem_sys, cleanup);

            cleanup = *cleanup_ref;
            rv = APR_SUCCESS;
        }
        else {
            cleanup_ref = &cleanup->next;
            cleanup = cleanup->next;
        }
    }

    /* The cleanup function should have been registered previously */
    return rv;
}

APR_DECLARE(apr_status_t)  apr_sms_cleanup_run(apr_sms_t *mem_sys, apr_int32_t type,
                                               void *data, 
			                                   apr_status_t (*cleanup_fn)(void *))
{
    apr_status_t rv;

    if (!mem_sys)
        return APR_EMEMSYS;

    if ((rv = apr_sms_cleanup_unregister(mem_sys, type, data, cleanup_fn)) != APR_SUCCESS)
        return rv;

     return cleanup_fn(data);
}

APR_DECLARE(apr_status_t) apr_sms_cleanup_run_type(apr_sms_t *mem_sys, 
                                                   apr_int32_t type)
{
    struct apr_sms_cleanup *cleanup;
    struct apr_sms_cleanup **cleanup_ref;
    apr_status_t rv = APR_ENOCLEANUP;

    if (!mem_sys)
        return APR_EMEMSYS;
        
#ifdef APR_ASSERT_MEMORY
    assert(mem_sys->accounting_mem_sys != NULL);
#endif
    
    cleanup = mem_sys->cleanups;
    cleanup_ref = &mem_sys->cleanups;
    mem_sys = mem_sys->accounting_mem_sys;
    while (cleanup)
    {
        if (type == 0 || cleanup->type == type) {
            *cleanup_ref = cleanup->next;

            cleanup->cleanup_fn(cleanup->data);
            
            if (mem_sys->free_fn != NULL)
                apr_sms_free(mem_sys, cleanup);

            cleanup = *cleanup_ref;
            rv = APR_SUCCESS;
        }
        else {
            cleanup_ref = &cleanup->next;
            cleanup = cleanup->next;
        }
    }

    /* The cleanup function should have been registered previously */
    return rv;
}

