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

#include "mm.h"
#include "apr_general.h"
#include "apr_errno.h"

struct shmem_t {
    MM *mm;
    ap_context_t *cntxt;
};

ap_status_t ap_shm_create(ap_context_t *cont, ap_size_t size, const char *file,
                          struct shmem_t **new)
{
    MM *mm = mm_create(size, file);

    if (mm == NULL) {
        return APR_ENOMEM;
    }
    (*new) = (struct shmem_t *)mm_malloc(mm, sizeof(struct shmem_t));
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->mm = mm;
    (*new)->cntxt = cont;
    return APR_SUCCESS;
}

ap_status_t ap_shm_destroy(struct shmem_t *shared)
{
    mm_destroy(shared->mm);
    shared->mm = NULL;
    return APR_SUCCESS;
}

ap_status_t ap_shm_malloc(struct shmem_t *shared, ap_size_t size, void **entity)
{
    entity = mm_malloc(shared->mm, size);
    if (entity == NULL) {
        return APR_ENOMEM;
    }
    return APR_SUCCESS;
}

ap_status_t ap_shm_calloc(struct shmem_t *shared, ap_size_t num, 
                          ap_size_t size, void **entity)
{
    entity = mm_calloc(shared->mm, num, size);
    if (entity == NULL) {
        return APR_ENOMEM;
    }
    return APR_SUCCESS;
}

ap_status_t ap_shm_realloc(struct shmem_t *shared, ap_size_t size, void **entity)
{
    void *new;

    new = mm_realloc(shared->mm, *entity, size);
    if (new == NULL) {
        return APR_ENOMEM;
    }

    (*entity) = new;
    return APR_SUCCESS;
}

ap_status_t apr_shm_free(struct shmem_t *shared, void *entity)
{
    mm_free(shared->mm, entity);
    return APR_SUCCESS;
}

ap_status_t ap_shm_strdup(struct shmem_t *shared, const char *old, char **new)
{
    (*new) = mm_strdup(shared->mm, old);
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    return APR_SUCCESS;
}

ap_status_t ap_shm_sizeof(struct shmem_t *shared, const void *ent, 
                          ap_size_t *size)
{
    *size = mm_sizeof(shared->mm, ent);
    if ((*size) == -1) {
        return APR_EINVAL;
    }
    return APR_SUCCESS;
}

ap_status_t ap_shm_maxsize(ap_size_t *size)
{
    (*size) = mm_maxsize();
    if ((*size) <= 0) {
        return APR_ENOMEM;
    }
    return APR_SUCCESS;
}

ap_status_t ap_shm_available(struct shmem_t *shared, ap_size_t *size)
{
    (*size) = mm_available(shared->mm);
    if ((*size) <= 0) {
        return APR_ENOMEM;
    }
    return APR_SUCCESS;
}

ap_status_t ap_shm_child_create(ap_context_t *cont, const char *fname, 
                                struct shmem_t **shared)
{
    return APR_SUCCESS;
}


