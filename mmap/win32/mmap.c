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

#include "apr.h"
#include "apr_private.h"
#include "apr_general.h"
#include "apr_mmap.h"
#include "apr_errno.h"
#include "fileio.h"
#include "apr_portable.h"

#if APR_HAS_MMAP

static apr_status_t mmap_cleanup(void *themmap)
{
    apr_mmap_t *mm = themmap;
    apr_status_t rv = 0;
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
    DWORD fmaccess = 0, mvaccess = 0;

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
    (*new)->psize = (offset % memblock) + size;
    (*new)->poffset = offset - (*new)->pstart;

    (*new)->mhandle = CreateFileMapping(file->filehand, NULL, fmaccess,
                                        0, (*new)->psize, NULL);
    if (!(*new)->mhandle || (*new)->mhandle == INVALID_HANDLE_VALUE)
    {
        *new = NULL;
        return apr_get_os_error();
    }

    (*new)->mv = MapViewOfFile((*new)->mhandle, mvaccess, 0,
                               (*new)->poffset, (*new)->psize);
    if (!(*new)->mv)
    {
        apr_status_t rv = apr_get_os_error();
        CloseHandle((*new)->mhandle);
        *new = NULL;
        return rv;
    }

    (*new)->mm = (char*)((*new)->mv) + offset;
    (*new)->size = size;
    (*new)->cntxt = cont;

    /* register the cleanup... */
    apr_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
                         apr_null_cleanup);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_mmap_delete(apr_mmap_t *mmap)
{
    apr_status_t rv;

    if ((rv = mmap_cleanup(mmap)) == APR_SUCCESS) {
        apr_kill_cleanup(mmap->cntxt, mmap, mmap_cleanup);
        return APR_SUCCESS;
    }
    return rv;
}

#endif
