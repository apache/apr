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
#include "sms_private.h"
#include "apr_portable.h"

#ifdef APR_ASSERT_MEMORY
#include <assert.h>
#endif
#include <stdio.h>

#include <memory.h> /* strikerXXX: had to add this for windows to stop 
                     * complaining, please autoconf the include stuff
                     */

FILE *dbg_file;

/*
 * private structure defenitions
 */
struct apr_sms_cleanup
{
    struct apr_sms_cleanup *next;
    apr_int32_t type;
    const void *data;
    apr_status_t (*cleanup_fn)(void *);
};

#if APR_DEBUG_ALLOCATIONS
FILE *alloc_file = NULL;

static void _record_(apr_sms_t *sms, const char *what, apr_size_t size,
                     void *ptr)
{
    if (!alloc_file)
        return;

    if (ptr) {
#if APR_DEBUG_TAG_SMS
        fprintf(alloc_file, "%10s %p '%9s' [%9s] @ %p\n",
                what, sms, sms->tag, sms->identity, ptr);
#else
        fprintf(alloc_file, "%10s %p             [%9s] @ %p\n",
                what, sms, sms->identity, ptr);
#endif
    } else {
#if APR_DEBUG_TAG_SMS
        fprintf(alloc_file, "%10s %p '%9s' [%9s] %6" APR_SIZE_T_FMT " bytes\n",
                what, sms, sms->tag, sms->identity, size);
#else
        fprintf(alloc_file, "%10s %p             [%9s] %6" APR_SIZE_T_FMT " bytes\n",
                what, sms, sms->identity, size);
#endif
    }
    fflush(alloc_file);
}
#endif


/* 
 * memory allocation functions
 */

APR_DECLARE(void *) apr_sms_malloc(apr_sms_t *sms,
                                   apr_size_t size)
{
    if (size == 0)
        return NULL;

#if APR_DEBUG_ALLOCATIONS
    _record_(sms, "MALLOC", size, NULL);
#endif

    return sms->malloc_fn(sms, size);
}

APR_DECLARE(void *) apr_sms_calloc(apr_sms_t *sms,
                                   apr_size_t size)
{
    if (size == 0)
        return NULL;

#if APR_DEBUG_ALLOCATIONS
    _record_(sms, "CALLOC", size, NULL);
#endif

    return sms->calloc_fn(sms, size);
}

APR_DECLARE(void *) apr_sms_realloc(apr_sms_t *sms, void *mem,
                                    apr_size_t size)
{
#if APR_DEBUG_ALLOCATIONS
    _record_(sms, "REALLOC", size, NULL);
#endif

    if (!mem)
        return apr_sms_malloc(sms, size);

    if (size == 0) {
        apr_sms_free(sms, mem);
        return NULL;
    }

    if (sms->realloc_fn)
        return sms->realloc_fn(sms, mem, size);

    /* XXX - should we free the block passed in ??? */
    return NULL;
}

APR_DECLARE(apr_status_t) apr_sms_free(apr_sms_t *sms,
                                       void *mem)
{

#if APR_DEBUG_ALLOCATIONS
    _record_(sms, "FREE", 0, mem);
#endif

    if (sms->free_fn)
        return sms->free_fn(sms, mem);  

    /*
     * If there is no free_fn, this sms is a tracking memory
     * system by definition.  In other words, it is ok
     * to return APR_SUCCESS because the memory will be
     * free()d by the reset or destroy.
     */
    return APR_SUCCESS;
}

/*
 * default allocation functions
 */

static void *apr_sms_default_calloc(apr_sms_t *sms,
                                    apr_size_t size)
{
    void *mem;

    mem = sms->malloc_fn(sms, size);
    if (mem)
        memset(mem, '\0', size);

    return mem;
}

/*
 * memory system functions
 */

static int apr_sms_is_tracking(apr_sms_t *sms)
{
    /*
     * The presense of a reset function gives us the clue that this is a 
     * tracking memory system.
     */
    return sms->reset_fn != NULL;
}

APR_DECLARE(apr_status_t) apr_sms_init(apr_sms_t *sms, 
                                       apr_sms_t *pms)
{
#if APR_DEBUG_TO_FILE
    if (!dbg_file)
        dbg_file = fopen(APR_DEBUG_FILE, "w");
#else
    dbg_file = stdout;
#endif
#if APR_DEBUG_ALLOCATIONS
    if (!alloc_file)
        alloc_file = fopen(APR_DEBUG_ALLOC_FILE, "w");
#endif

    /* XXX - I've assumed that memory passed in will be zeroed,
     * i.e. calloc'd instead of malloc'd...
     * This may well be a bogus assumption, and if so we either need
     * to memset or have a series of =NULL's at the end.
     * This function is only called by modules, so this isn't as crazy
     * an assumption to make as it sounds :)
     */

    sms->parent = pms;
    sms->accounting = sms;
    sms->child = NULL;

    /*
     * Child memory systems are always linked to their parents.  This works
     * as follows...
     *
     *  parent
     *    |
     *    |
     *  child  --- sibling --- sibling --- sibling
     *
     * To be able to remove a memory system from a list we also need to
     * keep a ref pointer (a pointer to the pointer pointing to the memory
     * system).  To remove a memory system, basically...
     *
     *  *ref = sibling;
     *  if (sibling)
     *      sibling->ref = ref;
     */
     
    if (pms) {
        /*
         * We only need to lock the parent as the only other function that
         * touches the fields we're about to mess with is apr_sms_destroy
         */
        if (pms->sms_lock)
            apr_lock_acquire(pms->sms_lock);
        
        if ((sms->sibling = pms->child) != NULL)
            sms->sibling->ref = &sms->sibling;

        sms->ref = &pms->child;
        pms->child = sms;

        if (pms->sms_lock)
            apr_lock_release(pms->sms_lock);    
    }

    /* Set default functions.  These should NOT be altered by an sms
     * module unless it implements them itself.
     */
    sms->calloc_fn = apr_sms_default_calloc;
    
#if APR_HAS_THREADS
    sms->threads = 1;
#endif /* APR_HAS_THREADS */
    
    /* XXX - This should eventually be removed */
    apr_pool_create(&sms->pool, pms ? pms->pool : NULL);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_sms_post_init(apr_sms_t *sms)
{
    /* We do things here as these may potentially make calls
     * to the sms that we're creating, and if we make the calls
     * in the sms_init phase we haven't yet added the function
     * pointers so we'll segfault!
     */
    apr_status_t rv;

    apr_sms_assert(sms);

    rv = APR_SUCCESS;
    
#if APR_HAS_THREADS
    if (sms->thread_register_fn)
        rv = sms->thread_register_fn(sms, apr_os_thread_current());
#endif /* APR_HAS_THREADS */
    
#if APR_DEBUG_SHOW_FUNCTIONS
    fprintf(dbg_file, "CREATE - sms %p [%s] has been created\n", 
            sms, sms->identity);
#endif
#if APR_DEBUG_SHOW_STRUCTURE
    apr_sms_show_structure(sms, 1);
#endif /* APR_DEBUG_SHOW_STRUCTURE */

    return rv;
}


#ifdef APR_ASSERT_MEMORY
APR_DECLARE(void) apr_sms_assert(apr_sms_t *sms)
{
    apr_sms_t *parent;

    /*
     * A memory system without a malloc won't do us much good
     */
    assert(sms->malloc_fn);

    /* 
     * Check to see if this is either a non-tracking or
     * a tracking memory system. It has to have at least a free
     * or destroy function. And to avoid half implementations
     * we require reset to be present when destroy is.
     */
    assert(sms->free_fn || (sms->destroy_fn && sms->reset_fn));

    assert(!sms->destroy_fn || sms->reset_fn);
  
    assert(!sms->reset_fn || sms->destroy_fn);

    /* Has someone been stupid and NULL'd our default function without
     * providing an implementation of their own... tsch, tsch
     */
    assert(sms->calloc_fn);
    
    /*
     * Make sure all accounting memory dies with the memory system.
     * To be more specific, make sure the accounting memort system
     * is either the memory system itself or a direct child.
     */
    assert(sms->accounting == sms ||
           sms->accounting->parent == sms);

    /*
     * A non-tracking memory system can be the child of
     * another non-tracking memory system if there are no
     * tracking ancestors, but in that specific case we issue a
     * warning.
     */
    if (!sms->parent)
        return;

    parent = sms;
    while (parent) {
        if (apr_sms_is_tracking(parent))
            return; /* Tracking memory system found, return satisfied ;-) */

        parent = parent->parent;
    }

    /* issue a warning: 
     * WARNING: A non-tracking memory system was created without a tracking 
     * parent.
     */
}
#endif /* APR_ASSERT_MEMORY */

/*
 * LOCAL FUNCTION used in:
 *  - apr_sms_do_child_cleanups
 *  - apr_sms_reset
 *  - apr_sms_destroy
 *
 * Call all the cleanup routines registered with a memory system.
 */
static void apr_sms_do_cleanups(struct apr_sms_cleanup *c)
{
    while (c) {
        if (c->type == APR_ALL_CLEANUPS || c->type == APR_GENERAL_CLEANUP) {
            c->cleanup_fn((void*)c->data);
        }
        c = c->next;
    }
}

/*
 * LOCAL FUNCTION used in:
 *  - apr_sms_reset
 *  - apr_sms_destroy
 *
 * This not only calls do_cleanups, but also calls the pre_destroy(!)
 */
static void apr_sms_do_child_cleanups(apr_sms_t *sms)
{
    while (sms) {
        apr_sms_do_child_cleanups(sms->child);
        apr_sms_do_cleanups(sms->cleanups);
        /*
         * We assume that all of our children & their siblings are created
         * from memory we've allocated, and as we're about to nuke it all 
         * we need to run the pre_destroy so things like locks can be 
         * cleaned up so we don't leak.
         * However, we aren't going to call destroy on a reset as we're about
         * to nuke them when we do the reset.  This is why all "leakable"
         * items created in an sms module MUST be cleaned up in the
         * pre_destroy not the destroy.
         */
        if (sms->pre_destroy_fn != NULL)
            sms->pre_destroy_fn(sms);
        sms = sms->sibling;
    }
}

APR_DECLARE(apr_status_t) apr_sms_reset(apr_sms_t *sms)
{
    apr_status_t rv;
    
    if (!sms->reset_fn)
        return APR_ENOTIMPL;

#if APR_DEBUG_SHOW_FUNCTIONS
# if APR_DEBUG_TAG_SMS
    fprintf(dbg_file, "RESET - sms %p '%s' [%s] being reset\n", sms,
            sms->tag, sms->identity);
# else
    fprintf(dbg_file, "RESET - sms %p [%s] being reset\n", sms, sms->identity);
# endif
#endif

    if (sms->sms_lock)
        apr_lock_acquire(sms->sms_lock);
    
    /* 
     * Run the cleanups of all child memory systems _including_
     * the accounting memory system.
     */
    apr_sms_do_child_cleanups(sms->child);

    /* Run all cleanups, the memory will be freed by the reset */
    apr_sms_do_cleanups(sms->cleanups);
    sms->cleanups = NULL;

    /* We don't have any child memory systems after the reset */
    sms->child = NULL;

    /* Reset the accounting memory system to ourselves, any
     * child memory system _including_ the accounting memory
     * system will be destroyed by the reset 
     * strikerXXX: Maybe this should be the responsibility of
     *             the reset function(?).
     */
    sms->accounting = sms;

    /* Let the memory system handle the actual reset */
    rv = sms->reset_fn(sms);

    if (sms->sms_lock)
        apr_lock_release(sms->sms_lock);
    
    return rv;
}

APR_DECLARE(apr_status_t) apr_sms_destroy(apr_sms_t *sms)
{
    apr_sms_t *child;
    apr_sms_t *sibling;
    apr_sms_t *pms;
    struct apr_sms_cleanup *cleanup;
    struct apr_sms_cleanup *next_cleanup;

#if APR_DEBUG_SHOW_FUNCTIONS
    fprintf(dbg_file, "DESTROY - sms %p [%s] being destroyed\n", 
            sms, sms->identity);
#endif /* APR_DEBUG_SHOW_FUNCTIONS */
#if APR_DEBUG_SHOW_STRUCTURE
    fprintf(dbg_file, "The following SMS will be destroyed by this action:\n");
    apr_sms_show_structure(sms, 0);
#endif /* APR_DEBUG_SHOW_STRUCTURE */

    if (sms->sms_lock)
        apr_lock_acquire(sms->sms_lock);
    
    if (apr_sms_is_tracking(sms)) {
        /* 
         * Run the cleanups of all child memory systems _including_
         * the accounting memory system.
         * This also does the pre_destroy functions in the children.
         */
        apr_sms_do_child_cleanups(sms->child);
        
        /* Run all cleanups, the memory will be freed by the destroy */
        apr_sms_do_cleanups(sms->cleanups);
    }
    else {
        if (sms->accounting != sms) {
            child = sms->accounting;
            
            /* 
             * Remove the accounting memory system from the memory systems 
             * child list (we will explicitly destroy it later in this block).
             */
            if (child->sibling != NULL)
                child->sibling->ref = child->ref;

            *child->ref = child->sibling;

            /* Set this fields so destroy will work */
            child->ref = NULL;
            child->sibling = NULL;
        }

        /* Visit all children and destroy them */
        child = sms->child;

        while (child) {
            sibling = child->sibling;
            apr_sms_destroy(child);
            child = sibling;
        }

        /*
         * If the accounting memory system _is_ tracking, we also know that
         * it is not the memory system itself.
         */
        if (apr_sms_is_tracking(sms->accounting)) {
            /* 
             * Run all cleanups, the memory will be freed by the destroying
             * of the accounting memory system.
             */
            apr_sms_do_cleanups(sms->cleanups);

            /* Destroy the accounting memory system */
            apr_sms_destroy(sms->accounting);

            /* 
             * Set the accounting memory system back to the parent memory
             * system just in case...
             */
            sms->accounting = sms;
        }
        else {
            /* Run all cleanups, free'ing memory as we go */
            cleanup = sms->cleanups;

            while (cleanup) {
                if (cleanup->type == APR_GENERAL_CLEANUP)
                    cleanup->cleanup_fn((void*)cleanup->data);

                next_cleanup = cleanup->next;
                apr_sms_free(sms->accounting, cleanup);
                cleanup = next_cleanup;
            }

            if (sms->accounting != sms) {
                /* Destroy the accounting memory system */
                apr_sms_destroy(sms->accounting);
                
                /* 
                 * Set the accounting memory system back to the parent memory
                 * system just in case...
                 */
                sms->accounting = sms;
            }
        }
    }

    pms = sms->parent;
    
    /* Remove the memory system from the parent memory systems child list */
    if (pms) {
        if (pms->sms_lock)
            apr_lock_acquire(pms->sms_lock);
        
        if ((*sms->ref = sms->sibling) != NULL)
            sms->sibling->ref = sms->ref;

        if (pms->sms_lock)
            apr_lock_release(pms->sms_lock);
    }
    
    /* Call the pre-destroy if present */
    if (sms->pre_destroy_fn)
        sms->pre_destroy_fn(sms);

    if (sms->sms_lock)
        apr_lock_destroy(sms->sms_lock);
    
    /* XXX - This should eventually be removed */
    apr_pool_destroy(sms->pool);

    /* 1 - If we have a self destruct, use it */
    if (sms->destroy_fn)
        return sms->destroy_fn(sms);

    /* 2 - If we don't have a parent, free using ourselves */
    if (!pms)
        return sms->free_fn(sms, sms);

    /* 3 - If we do have a parent and it has a free function, use it */
    if (pms->free_fn)
        return apr_sms_free(sms->parent, sms);

    /* 4 - Assume we are the child of a tracking memory system, do nothing */
#ifdef APR_ASSERT_MEMORY
    sms = pms;
    while (sms) {
        if (apr_sms_is_tracking(sms))
            return APR_SUCCESS;

        sms = sms->parent;
    }
    assert(0); /* Made the wrong assumption, so we assert */
#endif /* APR_ASSERT_MEMORY */
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_sms_is_ancestor(apr_sms_t *a,
                                              apr_sms_t *b)
{
#ifdef APR_ASSERT_MEMORY
    assert(a);
#endif
        
    while (b && b != a)
        b = b->parent;

    /* APR_SUCCESS = 0, so if they agree we should return that... */
    return !(b == a); 
}

APR_DECLARE(apr_status_t) apr_sms_lock(apr_sms_t *sms)
{
    /* If we don't have a lock_fn then we probably don't need one,
     * so this is OK and we just return APR_SUCCESS
     */
    if (!sms->lock_fn)
        return APR_SUCCESS;

    return sms->lock_fn(sms);
}

APR_DECLARE(apr_status_t) apr_sms_unlock(apr_sms_t *sms)
{
    /* If we don't have a lock_fn then we probably don't need one,
     * so this is OK and we just return APR_SUCCESS
     */
    if (!sms->unlock_fn)
        return APR_SUCCESS;
        
    return sms->unlock_fn(sms);
}

/*
 * memory system cleanup management functions
 */

APR_DECLARE(apr_status_t) apr_sms_cleanup_register(apr_sms_t *sms, 
                                                   apr_int32_t type,
                                                   const void *data,
                                                   apr_status_t
                                                       (*cleanup_fn)(void *))
{
    struct apr_sms_cleanup *cleanup;

    if (!cleanup_fn)
        return APR_ENOTIMPL;
    
    if (sms->sms_lock)
        apr_lock_acquire(sms->sms_lock);
    
    cleanup = apr_sms_malloc(sms->accounting, sizeof(struct apr_sms_cleanup));

    if (!cleanup) {
        if (sms->sms_lock)
            apr_lock_release(sms->sms_lock);
        
        return APR_ENOMEM;
    }

    cleanup->data = data;
    cleanup->type = type;
    cleanup->cleanup_fn = cleanup_fn;

    cleanup->next = sms->cleanups;
    sms->cleanups = cleanup;

    if (sms->sms_lock)
        apr_lock_release(sms->sms_lock);
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_sms_cleanup_unregister(apr_sms_t *sms,
                                                     apr_int32_t type,
                                                     const void *data,
                                                     apr_status_t
                                                         (*cleanup_fn)(void *))
{
    struct apr_sms_cleanup *cleanup;
    struct apr_sms_cleanup **cleanup_ref;
    apr_status_t rv = APR_EINVAL;
    
    if (sms->sms_lock)
        apr_lock_acquire(sms->sms_lock);
    
    cleanup = sms->cleanups;
    cleanup_ref = &sms->cleanups;
    while (cleanup) {
        if ((type == APR_ALL_CLEANUPS || cleanup->type == type) &&
            cleanup->data == data && cleanup->cleanup_fn == cleanup_fn) {
            *cleanup_ref = cleanup->next;

            sms = sms->accounting;

            if (sms->free_fn)
                apr_sms_free(sms, cleanup);

            cleanup = *cleanup_ref;
            rv = APR_SUCCESS;
        } else {
            cleanup_ref = &cleanup->next;
            cleanup = cleanup->next;
        }
    }

    if (sms->sms_lock)
        apr_lock_release(sms->sms_lock);

    /* The cleanup function must have been registered previously */
    return rv;
}

APR_DECLARE(apr_status_t) apr_sms_cleanup_unregister_type(apr_sms_t *sms, 
                                                          apr_int32_t type)
{
    struct apr_sms_cleanup *cleanup;
    struct apr_sms_cleanup **cleanup_ref;
    apr_status_t rv = APR_EINVAL;

    if (sms->sms_lock)
        apr_lock_acquire(sms->sms_lock);
    
    cleanup = sms->cleanups;
    cleanup_ref = &sms->cleanups;
    sms = sms->accounting;
    while (cleanup) {
        if (type == APR_ALL_CLEANUPS || cleanup->type == type) {
            *cleanup_ref = cleanup->next;

            if (sms->free_fn)
                apr_sms_free(sms, cleanup);

            cleanup = *cleanup_ref;
            rv = APR_SUCCESS;
        }
        else {
            cleanup_ref = &cleanup->next;
            cleanup = cleanup->next;
        }
    }

    if (sms->sms_lock)
        apr_lock_release(sms->sms_lock);
    
    /* The cleanup function must have been registered previously */
    return rv;
}

APR_DECLARE(apr_status_t) apr_sms_cleanup_run(apr_sms_t *sms,
                                              apr_int32_t type,
                                              const void *data, 
                                              apr_status_t
                                                  (*cleanup_fn)(void *))
{
    apr_status_t rv;

    if ((rv = apr_sms_cleanup_unregister(sms, type,
                                         data, cleanup_fn)) != APR_SUCCESS)
        return rv;

    return cleanup_fn((void*)data);
}

APR_DECLARE(apr_status_t) apr_sms_cleanup_run_type(apr_sms_t *sms, 
                                                   apr_int32_t type)
{
    struct apr_sms_cleanup *cleanup;
    struct apr_sms_cleanup **cleanup_ref;
    apr_status_t rv = APR_EINVAL;
    
    if (sms->sms_lock)
        apr_lock_acquire(sms->sms_lock);
    
    cleanup = sms->cleanups;
    cleanup_ref = &sms->cleanups;
    sms = sms->accounting;
    while (cleanup) {
        if (type == APR_ALL_CLEANUPS || cleanup->type == type) {
            *cleanup_ref = cleanup->next;

            cleanup->cleanup_fn((void*)cleanup->data);
            
            if (sms->free_fn)
                apr_sms_free(sms, cleanup);

            cleanup = *cleanup_ref;
            rv = APR_SUCCESS;
        }
        else {
            cleanup_ref = &cleanup->next;
            cleanup = cleanup->next;
        }
    }

    if (sms->sms_lock)
        apr_lock_release(sms->sms_lock);

    /* The cleanup function should have been registered previously */
    return rv;
}

#if APR_HAS_THREADS
APR_DECLARE(apr_status_t) apr_sms_thread_register(apr_sms_t *sms,
                                                  apr_os_thread_t thread)
{
    apr_status_t rv;
    
    do {
        if (!sms->sms_lock) {
            /* Create the sms framework lock we'll use. */
            apr_lock_create(&sms->sms_lock, APR_MUTEX, APR_LOCKALL,
                            NULL, sms->pool);
        }

        apr_lock_acquire(sms->sms_lock);

        sms->threads++;

        /* let the sms know about the thread if it is
         * interested (so it can protect its private
         * data with its own lock)
         *
         * if the sms is doesn't have a thread register
         * function, or it wasn't able to register the
         * thread, we should bomb out!
         * XXX - not sure how to implement the bombing out
         */
        rv = APR_ENOTIMPL;
        if (sms->thread_register_fn)
            rv = sms->thread_register_fn(sms, thread);

        apr_lock_release(sms->sms_lock);

        if (rv != APR_SUCCESS)
            return rv;

        sms = sms->parent;
    } while (sms);

    return APR_SUCCESS;    
}

APR_DECLARE(apr_status_t) apr_sms_thread_unregister(apr_sms_t *sms,
                                                    apr_os_thread_t thread)
{
    if (sms->sms_lock)
        apr_lock_acquire(sms->sms_lock);
    
    sms->threads--;

    /* Even if the thread count hits one, we don't destroy the
     * lock for now
     */

    if (sms->sms_lock)
        apr_lock_release(sms->sms_lock);

    return APR_SUCCESS;
}
#endif /* APR_HAS_THREADS */

APR_DECLARE(const char*) apr_sms_identity(apr_sms_t *sms)
{
    return sms->identity;
}

APR_DECLARE(apr_sms_t *) apr_sms_get_parent(apr_sms_t *sms)
{
    return sms->parent;
}

APR_DECLARE(void) apr_sms_set_abort(apr_abortfunc_t abort, apr_sms_t *sms)
{
    sms->apr_abort = abort;
}

APR_DECLARE(apr_abortfunc_t) apr_sms_get_abort(apr_sms_t *sms)
{
    return sms->apr_abort;
}

#if APR_DEBUG_SHOW_STRUCTURE
static void add_sms(char *a, char *b, char *c, apr_sms_t *sms, 
                    apr_sms_t *caller, int sib)
{
    char tmp[40];
    if (sib == 1) {
        strcat(a, "=");
        strcat(b, " ");
        strcat(c, " ");
    }
    sprintf(tmp, sms == caller ? "**%9p**" : " [%9p] ", sms);
    strcat(a, tmp);
#if APR_DEBUG_TAG_SMS
    sprintf(tmp, " '%9s' ", sms->tag);
#else
    sprintf(tmp, "             ");
#endif
    strcat(b, tmp);
    sprintf(tmp, " [%9s] ", sms->identity);
    strcat(c, tmp);
}

static void add_tab(char *a, char *b, char *c, int level, apr_sms_t *sms) 
{
    char buffer[100];
    int i;
    for(i=0;i < level * 2;i++) 
        buffer[i] = ' ';
    buffer[i] = '\0';
    strcpy(a, buffer);
    strcpy(b, buffer);
    strcpy(c, buffer);
    if (sms->parent)
        fprintf(dbg_file, "%s     |\n", buffer);
    fflush(dbg_file);
}

static void print_structure(char *l1, char *l2, char *l3)
{
    fprintf(dbg_file, "%s\n%s\n%s\n", l1, l2, l3);
    fflush(dbg_file);
}

static void print_depth(int levels)
{
    fprintf(dbg_file, "Showing %d level%s of SMS\n", levels + 1,
            levels == 0 ? "" : "s");
}

APR_DECLARE(void) apr_sms_show_structure(apr_sms_t *sms, int direction)
{
    apr_sms_t *thesms, *sibling;
    int levels = 0, i = 0;
    char l1[256], l2[256], l3[256];
    
    if (direction == 1) {
        /* we're going up! */
        thesms = sms;
        while (thesms->parent != NULL) {
            levels++;
            thesms = thesms->parent;
        }
        print_depth(levels);
        /* thesms now eqauls the top level... so start showing them! */
        while (thesms) {
            add_tab(l1, l2, l3, i++, thesms);

            add_sms(l1, l2, l3, thesms, sms, 0);
            
            sibling = thesms->sibling;
            while (sibling) {
                add_sms(l1, l2, l3, sibling, NULL, 1);
                sibling = sibling->sibling;
            }
            print_structure(l1, l2, l3);

            thesms = thesms->child;
        }
    } else {
        /* we go down!  i.e. show our descendants */
        thesms = sms;
        while (thesms->child) {
            levels++;
            thesms = thesms->child;
        }
        print_depth(levels);
        thesms = sms;
        while (thesms) {
            add_tab(l1, l2, l3, i++, thesms);

            /* add the child... */
            add_sms(l1, l2, l3, thesms, sms, 0);
            /* If we're destroying a sibling, then we won't be destroying
             * the other siblings, just descendants of this SMS, so
             * make sure what we show makes sense!
             */
            if (thesms != sms && thesms->sibling) {
                sibling = thesms->sibling;
                while (sibling) {
                    add_sms(l1, l2, l3, sibling, NULL, 1);
                    sibling = sibling->sibling;
                }
            }
            print_structure(l1, l2, l3);
            thesms = thesms->child;
        }
    }
}
#endif /* APR_DEBUG_SHOW_STRUCTURE */

#if APR_DEBUG_TAG_SMS
APR_DECLARE(void) apr_sms_tag(apr_sms_t *sms, const char *tag)
{
    sms->tag = tag;
}
#endif

