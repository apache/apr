/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "apr_general.h"
#include "apr_shm.h"
#include "apr_errno.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_portable.h"

struct apr_shm_t {
    apr_pool_t *pool;
    void *memblock;
};

APR_DECLARE(apr_status_t) apr_shm_create(apr_shm_t **m,
                                         apr_size_t reqsize,
                                         const char *filename,
                                         apr_pool_t *pool)
{
    int rc;
    apr_shm_t *newm = (apr_shm_t *)apr_palloc(pool, sizeof(apr_shm_t));
    char *name = NULL;
    ULONG flags = PAG_COMMIT|PAG_READ|PAG_WRITE;

    newm->pool = pool;

    if (filename) {
        name = apr_pstrcat(pool, "\\SHAREMEM\\", filename, NULL);
    }

    if (name == NULL) {
        flags |= OBJ_GETTABLE;
    }

    rc = DosAllocSharedMem(&(newm->memblock), name, reqsize, flags);

    if (rc) {
        return APR_OS2_STATUS(rc);
    }

    *m = newm;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_destroy(apr_shm_t *m)
{
    DosFreeMem(m->memblock);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_attach(apr_shm_t **m,
                                         const char *filename,
                                         apr_pool_t *pool)
{
    int rc;
    apr_shm_t *newm = (apr_shm_t *)apr_palloc(pool, sizeof(apr_shm_t));
    char *name = NULL;
    ULONG flags = PAG_READ|PAG_WRITE;

    newm->pool = pool;
    name = apr_pstrcat(pool, "\\SHAREMEM\\", filename, NULL);

    rc = DosGetNamedSharedMem(&(newm->memblock), name, flags);

    if (rc) {
        return APR_FROM_OS_ERROR(rc);
    }

    *m = newm;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_detach(apr_shm_t *m)
{
    int rc = 0;

    if (m->memblock) {
        rc = DosFreeMem(m->memblock);
    }

    return APR_FROM_OS_ERROR(rc);
}

APR_DECLARE(void *) apr_shm_baseaddr_get(const apr_shm_t *m)
{
    return m->memblock;
}

APR_DECLARE(apr_size_t) apr_shm_size_get(const apr_shm_t *m)
{
    ULONG flags, size = 0x1000000;
    DosQueryMem(m->memblock, &size, &flags);
    return size;
}

APR_POOL_IMPLEMENT_ACCESSOR(shm)

APR_DECLARE(apr_status_t) apr_os_shm_get(apr_os_shm_t *osshm,
                                         apr_shm_t *shm)
{
    *osshm = shm->memblock;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_shm_put(apr_shm_t **m,
                                         apr_os_shm_t *osshm,
                                         apr_pool_t *pool)
{
    int rc;
    apr_shm_t *newm = (apr_shm_t *)apr_palloc(pool, sizeof(apr_shm_t));
    ULONG flags = PAG_COMMIT|PAG_READ|PAG_WRITE;

    newm->pool = pool;

    rc = DosGetSharedMem(&(newm->memblock), flags);

    if (rc) {
        return APR_FROM_OS_ERROR(rc);
    }

    *m = newm;
    return APR_SUCCESS;
}    

