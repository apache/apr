/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "apr_general.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "misc.h"
#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif
#include <signal.h>
#include <errno.h>
#include <string.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

/* ***APRDOC********************************************************
 * ap_status_t ap_create_context(ap_context_t **, ap_context_t *)
 *    Create a new context.
 * arg 1) The parent context.  If this is NULL, the new context is a root
 *        context.  If it is non-NULL, the new context will inherit all
 *        of it's parent context's attributes, except the ap_context_t will be a
 *        sub-pool.
 * arg 2) The context we have just created.
 */
ap_status_t ap_create_context(struct context_t **newcont, struct context_t *cont)
{
    struct context_t *new;
    ap_pool_t *pool;

    if (cont) {
        pool = ap_make_sub_pool(cont->pool, cont->apr_abort);
    }
    else {
        pool = ap_init_alloc();;
    }
        
    if (pool == NULL) {
        return APR_ENOPOOL;
    }   

    new = (struct context_t *)ap_palloc(cont, sizeof(struct context_t));

    new->pool = pool;
    new->prog_data = NULL;
    new->apr_abort = NULL;
 
    *newcont = new;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_destroy_context(ap_context_t *)
 *    Free the context and all of it's child contexts'.
 * arg 1) The context to free.
 */
ap_status_t ap_destroy_context(struct context_t *cont)
{
    ap_destroy_pool(cont);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_userdata(ap_context_t *, void *, char *,
                               ap_status_t (*cleanup) (void *))
 *    Set the data associated with the current context.
 * arg 1) The current context.
 * arg 2) The user data associated with the context.
 * arg 3) The key to use for association
 * arg 4) The cleanup program to use to cleanup the data;
 * NOTE:  The data to be attached to the context should have the same
 *        life span as the context it is being attached to.
 *        
 *        Users of APR must take EXTREME care when choosing a key to
 *        use for their data.  It is possible to accidentally overwrite
 *        data by choosing a key that another part of the program is using
 *        It is advised that steps are taken to ensure that a unique
 *        key is used at all times.
 */
ap_status_t ap_set_userdata(void *data, char *key,
                            ap_status_t (*cleanup) (void *),
                            struct context_t *cont)
{
    datastruct *dptr = NULL, *dptr2 = NULL;
    if (cont) { 
        dptr = cont->prog_data;
        while (dptr) {
            if (!strcmp(dptr->key, key))
                break;
            dptr2 = dptr;
            dptr = dptr->next;
        }
        if (dptr == NULL) {
            dptr = ap_palloc(cont, sizeof(datastruct));
            dptr->next = dptr->prev = NULL;
            dptr->key = ap_pstrdup(cont, key);
            if (dptr2) {
                dptr2->next = dptr;
                dptr->prev = dptr2;
            }
            else {
                cont->prog_data = dptr;
            }
        }
        dptr->data = data;
        ap_register_cleanup(cont, dptr->data, cleanup, cleanup);
        return APR_SUCCESS;
    }
    return APR_ENOCONT;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_userdata(void **, ap_context_t *)
 *    Return the data associated with the current context.
 * arg 1) The current context.
 * arg 2) The key for the data to retrieve
 * arg 3) The user data associated with the context.
 */
ap_status_t ap_get_userdata(void **data, char *key, struct context_t *cont)
{
    datastruct *dptr = NULL;
    if (cont) { 
        dptr = cont->prog_data;
        while (dptr) {
            if (!strcmp(dptr->key, key)) {
                break;
            }
            dptr = dptr->next;
        }
        if (dptr) {
            (*data) = dptr->data;
        }
        else {
            (*data) = NULL;
        }
        return APR_SUCCESS;
    }
    return APR_ENOCONT;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_initialize()
 *    Setup any APR internal data structures.  This MUST be the first
 *    function called for any APR program.
 */
ap_status_t ap_initialize(void)
{
#ifdef HAVE_PTHREAD_SIGMASK 
    sigset_t sigset;

    sigfillset(&sigset);
    /*@@@ FIXME: This should *NOT* be called for the prefork MPM,
     * even if HAVE_PTHREAD_SIGMASK is defined!!!!       MnKr
     */
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);
#endif
    return APR_SUCCESS;
}

ap_status_t ap_set_abort(int (*apr_abort)(int retcode), struct context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    else {
        cont->apr_abort = apr_abort;
        return APR_SUCCESS;
    }
}
 
