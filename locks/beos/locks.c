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

#include "apr_lock.h"
#include "apr_general.h"
#include "locks.h"
#include <strings.h>
#include <stdio.h>

ap_status_t ap_create_lock(ap_context_t *cont, ap_locktype_e type, char *fname, struct lock_t **lock)
{
    struct lock_t *new;
    ap_status_t stat;
    
    new = (struct lock_t *)ap_palloc(cont, sizeof(struct lock_t));
    if (new == NULL){
        return APR_ENOMEM;
    }
    
    new->cntxt = cont;
    if (new->cntxt == NULL){
        printf ("null pool\n");
        return APR_ENOMEM;
    }
    new->type = type;
    new->fname = strdup(fname);

    if (type != APR_CROSS_PROCESS) {
        if ((stat = create_intra_lock(new)) != APR_SUCCESS) {
            return stat;
        }
    }
    if (type != APR_INTRAPROCESS) {
        if ((stat = create_inter_lock(new)) != APR_SUCCESS) {
            return stat;
        }
    }
    (*lock) = new;
    return APR_SUCCESS;
}

ap_status_t ap_lock(ap_lock_t *lock)
{
    ap_status_t stat;
    
    if (lock->type != APR_CROSS_PROCESS) {
        if ((stat = lock_intra(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    if (lock->type != APR_INTRAPROCESS) {
        if ((stat = lock_inter(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_unlock(ap_lock_t *lock)
{
    ap_status_t stat;
    if (lock->type != APR_CROSS_PROCESS) {
        if ((stat = unlock_intra(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    if (lock->type != APR_INTRAPROCESS) {
        if ((stat = unlock_inter(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}

ap_status_t ap_destroy_lock(ap_lock_t *lock)
{
    ap_status_t stat; 
    if (lock->type != APR_CROSS_PROCESS) {
        if ((stat = destroy_intra_lock(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    if (lock->type != APR_INTRAPROCESS) {
        if ((stat = destroy_inter_lock(lock)) != APR_SUCCESS) {
            return stat;
        }
    }
    return APR_SUCCESS;
}


