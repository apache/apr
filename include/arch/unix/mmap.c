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

#if HAVE_MMAP

#include "mmap_h.h"
#include "fileio.h"
#include "apr_mmap.h"
#include "apr_general.h"
#include "apr_portable.h"
#include "apr_lib.h"
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

ap_status_t mmap_cleanup(void *themmap)
{
    struct mmap_t *mm = themmap;
    int rv;
    rv = munmap(mm->mm, mm->size);

    if (rv == 0) {
        mm->mm = (caddr_t)-1;
        return APR_SUCCESS;
    }
    else
        return errno;
}

ap_status_t ap_mmap_create(ap_mmap_t **new, const char * fname,
     ap_context_t *cont)
{
    struct stat st;
    int fd;
    caddr_t mm;
   
    (*new) = (struct mmap_t *)ap_palloc(cont, sizeof(struct mmap_t));
    
    if (stat(fname, &st) == -1) {
        /* we couldn't stat the file...probably doesn't exist! */
        return APR_ENOFILE;
    }
    if ((st.st_mode & S_IFMT) != S_IFREG) {
        /* oh dear, we're only doing regular files at present... */
        return APR_EBADF;
    } 
    if ((fd = open(fname, O_RDONLY, 0)) == -1) {
        return APR_EBADF;
    }
    mm = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd ,0);
    close (fd);
    if (mm == (caddr_t)-1) {
        /* we failed to get an mmap'd file... */
        return APR_ENOMEM;
    }
    (*new)->filename = ap_pstrdup(cont, fname);
    (*new)->mm = mm;
    (*new)->sinfo = st;
    (*new)->size = st.st_size;
    (*new)->cntxt = cont;

    /* register the cleanup... */
    ap_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
             ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_mmap_open_create(struct mmap_t **new, ap_file_t *file, 
               ap_context_t *cont)
{
    caddr_t mm;

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

    mm = mmap(NULL, file->size, PROT_READ, MAP_SHARED, file->filedes ,0);
    if (mm == (caddr_t)-1) {
        /* we failed to get an mmap'd file... */
        return APR_ENOMEM;
    }

    (*new)->filename = ap_pstrdup(cont, file->fname);
    (*new)->mm = mm;
    (*new)->size = file->size;
    (*new)->cntxt = cont;
           
    /* register the cleanup... */
    ap_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
             ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_mmap_size_create(ap_mmap_t **new, ap_file_t *file, ap_size_t mmapsize,
                                ap_context_t *cont)
{
    caddr_t mm;

    if (file->buffered)
        return APR_EBADF;
    if (file->filedes == -1)
        return APR_EBADF;

    (*new) = (struct mmap_t*)ap_palloc(cont, sizeof(struct mmap_t));
    
    mm = mmap(NULL, mmapsize, PROT_READ, MAP_SHARED, file->filedes ,0);
    if (mm == (caddr_t)-1) {
        return APR_ENOMEM;
    }

    (*new)->filename = ap_pstrdup(cont, file->fname);
    (*new)->mm = mm;
    (*new)->size = mmapsize;
    (*new)->cntxt = cont;
           
    /* register the cleanup... */
    ap_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
             ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_mmap_delete(struct mmap_t *mmap)
{
    ap_status_t rv;

    if (mmap->mm == (caddr_t) -1)
        return APR_ENOENT;
      
    if ((rv = mmap_cleanup(mmap)) == APR_SUCCESS) {
        ap_kill_cleanup(mmap->cntxt, mmap, mmap_cleanup);
        return APR_SUCCESS;
    }
    return rv;
}

#endif
