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

#if APR_HAS_MMAP

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

    rv = munmap((char *)mm->mm - mm->poffset, mm->size + mm->poffset);
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
    static long psize;
    apr_off_t poffset = 0;
    apr_int32_t native_flags = 0;

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
    (*new) = (apr_mmap_t *)apr_pcalloc(cont, sizeof(apr_mmap_t));

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
    (*new)->poffset = poffset;
#endif

    mm = mmap(NULL, size + poffset,
              native_flags, MAP_SHARED,
              file->filedes, offset - poffset);

    if (mm == (void *)-1) {
        /* we failed to get an mmap'd file... */
        *new = NULL;
        return errno;
    }

    mm = (char *)mm + poffset;

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
