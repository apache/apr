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
#include "../../file_io/unix/fileio.h"
#include "apr_portable.h"

/* System headers required for the mmap library */
#ifdef BEOS
#include <kernel/OS.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#if APR_HAS_MMAP || defined(BEOS)

static apr_status_t mmap_cleanup(void *themmap)
{
    apr_mmap_t *mm = themmap;
    int rv;
#ifdef BEOS
    rv = delete_area(mm->area);

    if (rv == 0) {
        mm->mm = (caddr_t)-1;
        return APR_SUCCESS;
    }
#else
    rv = munmap(mm->mm, mm->size);

    if (rv == 0) {
        mm->mm = (caddr_t)-1;
        return APR_SUCCESS;
    }
#endif
    return errno;
}

apr_status_t apr_mmap_create(apr_mmap_t **new, apr_file_t *file, apr_off_t offset, 
       apr_size_t size, apr_pool_t *cont)
{
#ifdef BEOS
    void *mm;
    area_id aid = -1;
    char *areaname = "apr_mmap\0";
    uint32 pages = 0;
#else
    caddr_t mm;
#endif
   
    if (file == NULL || file->filedes == -1 || file->buffered)
        return APR_EBADF;
    (*new) = (apr_mmap_t *)apr_pcalloc(cont, sizeof(apr_mmap_t));
    
#ifdef BEOS
    /* XXX: mmap shouldn't really change the seek offset */
    apr_seek(file, APR_SET, &offset);
    pages = ((size -1) / B_PAGE_SIZE) + 1;

    aid = create_area(areaname, &mm , B_ANY_ADDRESS, pages * B_PAGE_SIZE,
        B_FULL_LOCK, B_READ_AREA|B_WRITE_AREA);

    if (aid < B_NO_ERROR) {
        /* we failed to get an mmap'd file... */
        return APR_ENOMEM;
    }

    if (aid >= B_NO_ERROR)
        read(file->filedes, mm, size);
    (*new)->area = aid;
#else

    mm = mmap(NULL, size, PROT_READ, MAP_SHARED, file->filedes, offset);

    if (mm == (caddr_t)-1) {
        /* we failed to get an mmap'd file... */
        return APR_ENOMEM;
    }
#endif

    (*new)->mm = mm;
    (*new)->size = size;
    (*new)->cntxt = cont;

    /* register the cleanup... */
    apr_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
             apr_null_cleanup);
    return APR_SUCCESS;
}

apr_status_t apr_mmap_delete(apr_mmap_t *mmap)
{
    apr_status_t rv;

    if (mmap->mm == (caddr_t) -1)
        return APR_ENOENT;
      
    if ((rv = mmap_cleanup(mmap)) == APR_SUCCESS) {
        apr_kill_cleanup(mmap->cntxt, mmap, mmap_cleanup);
        return APR_SUCCESS;
    }
    return rv;
}

#endif
