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

#include "apr.h"
#include "apr_private.h"
#include "apr_general.h"
#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_arch_file_io.h"
#include "apr_portable.h"
#include "apr_strings.h"

#if APR_HAS_MMAP

static apr_status_t mmap_cleanup(void *themmap)
{
    apr_mmap_t *mm = themmap;
    apr_mmap_t *next = APR_RING_NEXT(mm,link);
    apr_status_t rv = 0;

    /* we no longer refer to the mmaped region */
    APR_RING_REMOVE(mm,link);
    APR_RING_NEXT(mm,link) = NULL;
    APR_RING_PREV(mm,link) = NULL;

    if (next != mm) {
        /* more references exist, so we're done */
        return APR_SUCCESS;
    }

    if (mm->mv) {
        if (!UnmapViewOfFile(mm->mv))
        {
            apr_status_t rv = apr_get_os_error();
            CloseHandle(mm->mhandle);
            mm->mv = NULL;
            mm->mhandle = NULL;
            return rv;
        }
        mm->mv = NULL;
    }
    if (mm->mhandle) 
    {
        if (!CloseHandle(mm->mhandle))
        {
            apr_status_t rv = apr_get_os_error();
            CloseHandle(mm->mhandle);
            mm->mhandle = NULL;
            return rv;
        }
        mm->mhandle = NULL;
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_mmap_create(apr_mmap_t **new, apr_file_t *file,
                                          apr_off_t offset, apr_size_t size,
                                          apr_int32_t flag, apr_pool_t *cont)
{
    static DWORD memblock = 0;
    DWORD fmaccess = 0;
    DWORD mvaccess = 0;
    DWORD offlo;
    DWORD offhi;

    if (size == 0)
        return APR_EINVAL;
    
    if (flag & APR_MMAP_WRITE)
        fmaccess |= PAGE_READWRITE;
    else if (flag & APR_MMAP_READ)
        fmaccess |= PAGE_READONLY;

    if (flag & APR_MMAP_READ)
        mvaccess |= FILE_MAP_READ;
    if (flag & APR_MMAP_WRITE)
        mvaccess |= FILE_MAP_WRITE;

    if (!file || !file->filehand || file->filehand == INVALID_HANDLE_VALUE
        || file->buffered)
        return APR_EBADF;

    if (!memblock)
    {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        memblock = si.dwAllocationGranularity;
    }   
    
    *new = apr_pcalloc(cont, sizeof(apr_mmap_t));
    (*new)->pstart = (offset / memblock) * memblock;
    (*new)->poffset = offset - (*new)->pstart;
    (*new)->psize = (apr_size_t)((*new)->poffset) + size;
    /* The size of the CreateFileMapping object is the current size
     * of the size of the mmap object (e.g. file size), not the size 
     * of the mapped region!
     */

    (*new)->mhandle = CreateFileMapping(file->filehand, NULL, fmaccess,
                                        0, 0, NULL);
    if (!(*new)->mhandle || (*new)->mhandle == INVALID_HANDLE_VALUE)
    {
        *new = NULL;
        return apr_get_os_error();
    }

    offlo = (DWORD)(*new)->pstart;
    offhi = (DWORD)((*new)->pstart >> 32);
    (*new)->mv = MapViewOfFile((*new)->mhandle, mvaccess, offhi,
                               offlo, (*new)->psize);
    if (!(*new)->mv)
    {
        apr_status_t rv = apr_get_os_error();
        CloseHandle((*new)->mhandle);
        *new = NULL;
        return rv;
    }

    (*new)->mm = (char*)((*new)->mv) + (*new)->poffset;
    (*new)->size = size;
    (*new)->cntxt = cont;
    APR_RING_ELEM_INIT(*new, link);

    /* register the cleanup... */
    apr_pool_cleanup_register((*new)->cntxt, (void*)(*new), mmap_cleanup,
                         apr_pool_cleanup_null);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_mmap_dup(apr_mmap_t **new_mmap,
                                       apr_mmap_t *old_mmap,
                                       apr_pool_t *p,
                                       int transfer_ownership)
{
    *new_mmap = (apr_mmap_t *)apr_pmemdup(p, old_mmap, sizeof(apr_mmap_t));
    (*new_mmap)->cntxt = p;

    APR_RING_INSERT_AFTER(old_mmap, *new_mmap, link);

    apr_pool_cleanup_register(p, *new_mmap, mmap_cleanup,
                              apr_pool_cleanup_null);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_mmap_delete(apr_mmap_t *mm)
{
    return apr_pool_cleanup_run(mm->cntxt, mm, mmap_cleanup);
}

#endif
