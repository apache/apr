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

#include "mm.h"
#include "apr_general.h"
#include "apr_shmem.h"
#include "apr_errno.h"

struct shmem_t {
    MM *mm;
};

ap_status_t ap_shm_init(struct shmem_t **m, ap_size_t reqsize, const char *file, ap_pool_t *cont)
{
    MM *newmm = mm_create(reqsize, file);
    if (newmm == NULL) {
        return errno;
    }
    (*m) = mm_malloc(newmm, sizeof(struct shmem_t));
    (*m)->mm = newmm;
    return APR_SUCCESS;
}

ap_status_t ap_shm_destroy(struct shmem_t *m)
{
    mm_destroy(m->mm);
    return APR_SUCCESS;
}

void *ap_shm_malloc(struct shmem_t *c, ap_size_t reqsize)
{
    if (c->mm == NULL) {
        return NULL;
    }
    return mm_malloc(c->mm, reqsize);
}

void *ap_shm_calloc(struct shmem_t *shared, ap_size_t size) 
{
    if (shared == NULL) {
        return NULL;
    }
    return mm_calloc(shared->mm, 1, size);
}

ap_status_t ap_shm_free(struct shmem_t *shared, void *entity)
{
    mm_free(shared->mm, entity);
    return APR_SUCCESS;
}

ap_status_t ap_get_shm_name(ap_shmem_t *c, ap_shm_name_t **name)
{
#if APR_USES_ANONYMOUS_SHM
    name = NULL;
    return APR_ANONYMOUS;
/* Currently, we are not supporting name based shared memory on Unix
 * systems.  This may change in the future however, so I will leave
 * this in here for now.  Plus, this gives other platforms a good idea
 * of how to proceed.
 */
#elif APR_USES_FILEBASED_SHM
#elif APR_USES_KEYBASED_SHM
#endif
}

ap_status_t ap_set_shm_name(ap_shmem_t *c, ap_shm_name_t *name)
{
#if APR_USES_ANONYMOUS_SHM
    return APR_ANONYMOUS;
/* Currently, we are not supporting name based shared memory on Unix
 * systems.  This may change in the future however, so I will leave
 * this in here for now.  Plus, this gives other platforms a good idea
 * of how to proceed.
 */
#elif APR_USES_FILEBASED_SHM
#elif APR_USES_KEYBASED_SHM
#endif
}

ap_status_t ap_open_shmem(struct shmem_t *c)
{
#if APR_USES_ANONYMOUS_SHM
/* When using MM, we don't need to open shared memory segments in child
 * segments, so just return immediately.
 */
    return APR_SUCCESS;
/* Currently, we are not supporting name based shared memory on Unix
 * systems.  This may change in the future however, so I will leave
 * this in here for now.  Plus, this gives other platforms a good idea
 * of how to proceed.
 */
#elif APR_USES_FILEBASED_SHM
#elif APR_USES_KEYBASED_SHM
#endif
}

ap_status_t ap_shm_avail(struct shmem_t *c, ap_size_t *size)
{
    *size = mm_available(c);
    if (*size == 0) {
        return APR_ESHMLOCK;
    }
    return APR_SUCCESS;
}
