/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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

#include "misc.h"
#include "../../locks/unix/locks.h"

/* ***APRDOC********************************************************
 * ap_status_t ap_create_context(ap_context_t **newcont, ap_context_t *cont)
 *    Create a new context.
 * arg 1) The context we have just created.
 * arg 2) The parent context.  If this is NULL, the new context is a root
 *        context.  If it is non-NULL, the new context will inherit all
 *        of it's parent context's attributes, except the ap_context_t will be a
 *        sub-pool.
 */
ap_status_t ap_create_context(struct ap_context_t **newcont, struct ap_context_t *cont)
{
    struct ap_context_t *new;
    ap_pool_t *pool;

    if (cont) {
        pool = ap_make_sub_pool(cont->pool, cont->apr_abort);
    }
    else {
        pool = ap_make_sub_pool(NULL, NULL);
    }
        
    if (pool == NULL) {
        return APR_ENOPOOL;
    }   

    new = (struct ap_context_t *)ap_palloc(cont, sizeof(struct ap_context_t));

    new->pool = pool;
    new->prog_data = NULL;
    new->apr_abort = NULL;
 
    *newcont = new;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_destroy_context(ap_context_t *cont)
 *    Free the context and all of it's child contexts'.
 * arg 1) The context to free.
 */
ap_status_t ap_destroy_context(struct ap_context_t *cont)
{
    ap_destroy_pool(cont);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_userdata(void *data, char *key, 
 *                             ap_status_t (*cleanup) (void *),
 *                             ap_context_t *cont)
 *    Set the data associated with the current context.
 * arg 1) The user data associated with the context.
 * arg 2) The key to use for association
 * arg 3) The cleanup program to use to cleanup the data;
 * arg 4) The current context.
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
                            struct ap_context_t *cont)
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
 * ap_status_t ap_get_userdata(void **data, char *key, ap_context_t *cont)
 *    Return the data associated with the current context.
 * arg 1) The key for the data to retrieve
 * arg 2) The user data associated with the context.
 * arg 3) The current context.
 */
ap_status_t ap_get_userdata(void **data, char *key, struct ap_context_t *cont)
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
 * ap_status_t ap_initialize(void)
 *    Setup any APR internal data structures.  This MUST be the first
 *    function called for any APR program.
 */
ap_status_t ap_initialize(void)
{
    ap_status_t status;
    setup_lock();
    status = ap_init_alloc();
    return status;
}

/* ***APRDOC*******************************************************
 * void ap_terminate(void)
 *    Tear down any APR internal data structures which aren't
 *    torn down automatically.  An APR program must call this
 *    function at termination once it has stopped using APR
 *    services.
 */
void ap_terminate(void)
{
    ap_term_alloc();
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_abort(int (*apr_abort)(int retcode), ap_context_t *cont)
 *    Set the APR_ABORT function.
 * NOTE:  This is in for backwards compatability.  If the program using
 *        APR wants APR to exit on a memory allocation error, then this
 *        function should be called to set the function to use in order
 *        to actually exit the program.  If this function is not called,
 *        then APR will return an error and expect the calling program to
 *        deal with the error accordingly.
 */
ap_status_t ap_set_abort(int (*apr_abort)(int retcode), struct ap_context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    else {
        cont->apr_abort = apr_abort;
        return APR_SUCCESS;
    }
}
 
