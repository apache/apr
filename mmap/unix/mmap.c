/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>

#include "apr.h"
#include "apr_private.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_arch_file_io.h"
#include "apr_portable.h"

/* System headers required for the mmap library */
#ifdef BEOS
#include <kernel/OS.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h>  /* for sysconf() */
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if APR_HAS_MMAP || defined(BEOS)

#ifndef BEOS
struct mm_layout {
    apr_mmap_t mm;
    apr_off_t poffset;
};

static APR_INLINE
apr_mmap_t *alloc_with_poffset(apr_pool_t *p)
{
    struct mm_layout *layout = apr_pcalloc(p, sizeof(*layout));
    return &layout->mm;
}

static APR_INLINE
void set_poffset(apr_mmap_t *mm, apr_off_t poffset)
{
    struct mm_layout *layout = (struct mm_layout *)mm;
    layout->poffset = poffset;
}

static APR_INLINE
apr_off_t get_poffset(apr_mmap_t *mm)
{
    struct mm_layout *layout = (struct mm_layout *)mm;
    return layout->poffset;
}
#endif

static apr_status_t mmap_cleanup(void *themmap)
{
    apr_mmap_t *mm = themmap;
    apr_mmap_t *next = APR_RING_NEXT(mm,link);
    int rv = 0;

    /* we no longer refer to the mmaped region */
    APR_RING_REMOVE(mm,link);
    APR_RING_NEXT(mm,link) = NULL;
    APR_RING_PREV(mm,link) = NULL;

    if (next != mm) {
        /* more references exist, so we're done */
        return APR_SUCCESS;
    }

#ifdef BEOS
    rv = delete_area(mm->area);
#else
    rv = munmap((char *)mm->mm - get_poffset(mm), mm->size + get_poffset(mm));
#endif
    mm->mm = (void *)-1;

    if (rv == 0) {
        return APR_SUCCESS;
    }
    return errno;
}

APR_DECLARE(apr_status_t) apr_mmap_create(apr_mmap_t **new, 
                                          apr_file_t *file, apr_off_t offset, 
                                          apr_size_t size, apr_int32_t flag, 
                                          apr_pool_t *cont)
{
    void *mm;
#ifdef BEOS
    area_id aid = -1;
    uint32 pages = 0;
#else
    static long psize;
    apr_off_t poffset = 0;
    apr_int32_t native_flags = 0;
#endif

#if APR_HAS_LARGE_FILES && defined(HAVE_MMAP64)
#define mmap mmap64
#elif APR_HAS_LARGE_FILES && SIZEOF_OFF_T == 4
    /* LFS but no mmap64: check for overflow */
    if ((apr_int64_t)offset + size > INT_MAX)
        return APR_EINVAL;
#endif

    if (size == 0)
        return APR_EINVAL;
    
    if (file == NULL || file->filedes == -1 || file->buffered)
        return APR_EBADF;

#ifdef BEOS

    *new = (apr_mmap_t *)apr_pcalloc(cont, sizeof(apr_mmap_t));

    /* XXX: mmap shouldn't really change the seek offset */
    apr_file_seek(file, APR_SET, &offset);

    /* There seems to be some strange interactions that mean our area must
     * be set as READ & WRITE or writev will fail!  Go figure...
     * So we ignore the value in flags and always ask for both READ and WRITE
     */
    pages = (size + B_PAGE_SIZE -1) / B_PAGE_SIZE;
    aid = create_area("apr_mmap", &mm , B_ANY_ADDRESS, pages * B_PAGE_SIZE,
        B_NO_LOCK, B_WRITE_AREA|B_READ_AREA);

    if (aid < B_NO_ERROR) {
        /* we failed to get an area we can use... */
        *new = NULL;
        return APR_ENOMEM;
    }

    if (aid >= B_NO_ERROR)
        read(file->filedes, mm, size);
    
    (*new)->area = aid;

#else
    *new = alloc_with_poffset(cont);

    if (flag & APR_MMAP_WRITE) {
        native_flags |= PROT_WRITE;
    }
    if (flag & APR_MMAP_READ) {
        native_flags |= PROT_READ;
    }

#if defined(_SC_PAGESIZE)
    if (psize == 0) {
        psize = sysconf(_SC_PAGESIZE);
        /* the page size should be a power of two */
        assert(psize > 0 && (psize & (psize - 1)) == 0);
    }
    poffset = offset & (apr_off_t)(psize - 1);
    set_poffset(*new, poffset);
#endif

#ifdef HAVE_MAP_POPULATE
    mm = mmap(NULL, size + poffset,
              native_flags, MAP_SHARED | MAP_POPULATE,
              file->filedes, offset - poffset);
#else
    mm = mmap(NULL, size + poffset,
              native_flags, MAP_SHARED,
              file->filedes, offset - poffset);
#endif

    if (mm == (void *)-1) {
        /* we failed to get an mmap'd file... */
        *new = NULL;
        return errno;
    }

    mm = (char *)mm + poffset;
#endif

    (*new)->mm = mm;
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
                                       apr_pool_t *p)
{
#ifdef BEOS
    *new_mmap = (apr_mmap_t *)apr_pmemdup(p, old_mmap, sizeof(apr_mmap_t));
#else
    *new_mmap = (apr_mmap_t *)apr_pmemdup(p, old_mmap,
                                          sizeof(struct mm_layout));
#endif
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
