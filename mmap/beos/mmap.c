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

#include "mmap_h.h"
#include "apr_mmap.h"
#include "apr_general.h"
#include "apr_portable.h"
#include "apr_lib.h"
#include "fileio.h"
#include <kernel/OS.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

ap_status_t mmap_cleanup(void *themmap)
{
    struct mmap_t *mm = themmap;
    int rv;
    rv = delete_area(mm->area);

    if (rv == 0) {
        mm->mm = 0;
        mm->area = -1;
        return APR_SUCCESS;
    }
    else
        return errno;
}

ap_status_t ap_mmap_create(struct mmap_t **new, const char *fname,
     ap_context_t *cont)
{
    struct stat st;
    int fd = -1;
    void *mm;
    area_id aid = -1;
    char *areaname = "apr_mmap\0";
    uint32 size = 0;

    (*new) = (struct mmap_t *)ap_palloc(cont, sizeof(struct mmap_t));
    
    if (stat(fname, &st) == -1) { 
        /* we couldn't stat the file...probably doesn't exist! */
        return APR_ENOFILE;
    }
    if ((st.st_mode & S_IFMT) != S_IFREG) {
        /* oh dear, we're only doing regular files at present... */
        return APR_EBADF;
    } 
    size = ((st.st_size -1) / B_PAGE_SIZE) + 1;

    if ((fd = open(fname, O_RDONLY, 0)) == -1) {
        return APR_EBADF;
    }
  
    aid = create_area(areaname, &mm , B_ANY_ADDRESS, size * B_PAGE_SIZE, 
        B_FULL_LOCK, B_READ_AREA|B_WRITE_AREA);

    if (aid >= B_NO_ERROR)
        read(fd, mm, st.st_size);    
    
    close (fd);
    if (aid < B_NO_ERROR) {
        /* we failed to get an mmap'd file... */
        return APR_ENOMEM;
    }  

    (*new)->filename = ap_pstrdup(cont, fname);
    (*new)->mm = mm;
    (*new)->sinfo = st;
    (*new)->size = st.st_size;
    (*new)->area = aid;
    (*new)->cntxt = cont;
    (*new)->statted = 1;
               
    /* register the cleanup... */ 
    ap_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
             ap_null_cleanup);

    return APR_SUCCESS;
}

ap_status_t ap_mmap_open_create(struct mmap_t **new, ap_file_t *file, 
               ap_context_t *cont)
{
    char *mm;
    area_id aid = -1;
    char *areaname = "apr_mmap\0";
    uint32 size;           
    
    if (file->buffered)
        /* we don't yet mmap buffered files... */
        return APR_EBADF;
    if (file->filedes == -1)
        /* there isn't a file handle so how can we mmap?? */
        return APR_EBADF;
    (*new) = (struct mmap_t*)ap_palloc(cont, sizeof(struct mmap_t));
    
    if (!file->stated) {
        /* hmmmm... we need to stat the file now */
        struct stat st;
        if (stat(file->fname, &st) == -1) {
            /* hmm, is this fatal?? */
            return APR_EBADF;
        }
        file->stated = 1;
        file->size = st.st_size;
        file->atime = st.st_atime;
        file->mtime = st.st_mtime;
        file->ctime = st.st_ctime;
        (*new)->sinfo = st;
    }
    
    size = ((file->size -1) / B_PAGE_SIZE) + 1;

    aid = create_area(areaname, (void*)&mm, B_ANY_ADDRESS, size * B_PAGE_SIZE, 
        B_FULL_LOCK, B_READ_AREA|B_WRITE_AREA);
    free(areaname);
    
    if (aid < B_OK) {
        /* we failed to get an mmap'd file... */
        return APR_ENOMEM;
    }  
    if (aid >= B_OK)
        read(file->filedes, mm, file->size);    

    (*new)->filename = ap_pstrdup(cont, file->fname);
    (*new)->mm = mm;
    (*new)->size = file->size;
    (*new)->area = aid;
    (*new)->cntxt = cont;
    (*new)->statted = 1;
    
    /* register the cleanup... */ 
    ap_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
             ap_null_cleanup);

    return APR_SUCCESS;
}

ap_status_t ap_mmap_size_create(ap_mmap_t **new, ap_file_t *file, ap_size_t mmapsize,
                                ap_context_t *cont)
{
    char *mm;
    area_id aid = -1;
    char *areaname = "apr_mmap\0";
    uint32 size;           
    
    if (file->buffered)
        return APR_EBADF;
    if (file->filedes == -1)
        return APR_EBADF;
    (*new) = (struct mmap_t*)ap_palloc(cont, sizeof(struct mmap_t));
      
    size = ((mmapsize -1) / B_PAGE_SIZE) + 1;

    aid = create_area(areaname, (void*)&mm, B_ANY_ADDRESS, size * B_PAGE_SIZE, 
        B_FULL_LOCK, B_READ_AREA|B_WRITE_AREA);
    free(areaname);
    
    if (aid < B_OK) {
        /* we failed to get an mmap'd file... */
        return APR_ENOMEM;
    }  
    if (aid >= B_OK)
        read(file->filedes, mm, mmapsize);    

    (*new)->filename = ap_pstrdup(cont, file->fname);
    (*new)->mm = mm;
    (*new)->size = mmapsize;
    (*new)->area = aid;
    (*new)->cntxt = cont;
    (*new)->statted = 0;

    /* register the cleanup... */ 
    ap_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
             ap_null_cleanup);

    return APR_SUCCESS;
}

ap_status_t ap_mmap_delete(struct mmap_t *mmap)
{
    ap_status_t rv;
    if (mmap->area == -1)
        return APR_ENOENT;
         
    if ((rv = mmap_cleanup(mmap)) == APR_SUCCESS) {
        ap_kill_cleanup(mmap->cntxt, mmap, mmap_cleanup);
        return APR_SUCCESS;
    }
    return rv;
}
