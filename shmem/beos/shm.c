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
#include <stdio.h>
#include <stdlib.h>
#include <kernel/OS.h>
#include "apr_portable.h"

struct apr_shm_t {
    apr_pool_t *pool;
    void *memblock;
    void *ptr;
    apr_size_t reqsize;
    apr_size_t avail;
    area_id aid;
};

APR_DECLARE(apr_status_t) apr_shm_create(apr_shm_t **m, 
                                         apr_size_t reqsize, 
                                         const char *file, 
                                         apr_pool_t *p)
{
    apr_size_t pagesize;
    area_id newid;
    char *addr;
    
    (*m) = (apr_shm_t *)apr_pcalloc(p, sizeof(apr_shm_t));
    /* we MUST allocate in pages, so calculate how big an area we need... */
    pagesize = ((reqsize + B_PAGE_SIZE - 1) / B_PAGE_SIZE) * B_PAGE_SIZE;
     
    newid = create_area("apr_shmem", (void*)&addr, B_ANY_ADDRESS,
                        pagesize, B_CONTIGUOUS, B_READ_AREA|B_WRITE_AREA);

    if (newid < 0)
        return errno;

    (*m)->pool = p;
    (*m)->aid = newid;
    (*m)->memblock = addr;
    (*m)->ptr = (void*)addr;
    (*m)->avail = pagesize; /* record how big an area we actually created... */
    (*m)->reqsize = reqsize;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_destroy(apr_shm_t *m)
{
    delete_area(m->aid);
    m->avail = 0;
    m->memblock = NULL;
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_shm_attach(apr_shm_t **m,
                                         const char *filename,
                                         apr_pool_t *pool)
{
    area_info ai;
    thread_info ti;
    area_id deleteme;
    apr_shm_t *new_m;
    
    deleteme = find_area(filename);
    if (deleteme == B_NAME_NOT_FOUND)
        return APR_EINVAL;

    new_m = (apr_shm_t*)apr_palloc(pool, sizeof(apr_shm_t*));
    if (new_m == NULL)
        return APR_ENOMEM;
    new_m->pool = pool;

    get_area_info(deleteme, &ai);
    get_thread_info(find_thread(NULL), &ti);

    if (ti.team != ai.team) {
        area_id narea;
        
        narea = clone_area(ai.name, &(ai.address), B_CLONE_ADDRESS,
                           B_READ_AREA|B_WRITE_AREA, ai.area);

        if (narea < B_OK)
            return narea;
            
        get_area_info(narea, &ai);
        new_m->aid = narea;
        new_m->memblock = ai.address;
        new_m->ptr = (void*)ai.address;
        new_m->avail = ai.size;
        new_m->reqsize = ai.size;
    }

    (*m) = new_m;
    
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_shm_detach(apr_shm_t *m)
{
    delete_area(m->aid);
    return APR_SUCCESS;
}

APR_DECLARE(void *) apr_shm_baseaddr_get(const apr_shm_t *m)
{
    return m->memblock;
}

APR_DECLARE(apr_size_t) apr_shm_size_get(const apr_shm_t *m)
{
    return m->reqsize;
}

APR_POOL_IMPLEMENT_ACCESSOR(shm)

APR_DECLARE(apr_status_t) apr_os_shm_get(apr_os_shm_t *osshm,
                                         apr_shm_t *shm)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_os_shm_put(apr_shm_t **m,
                                         apr_os_shm_t *osshm,
                                         apr_pool_t *pool)
{
    return APR_ENOTIMPL;
}    

