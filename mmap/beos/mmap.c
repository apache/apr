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
    int fd;
    void *mm;
    struct mmap_t *next;
    area_id aid;
    char *areaname;
    uint32 size;
    int len;               

    /* We really should check to see that we haven't already mmap'd
     * this file before.  Cycling the linked list will allow this.
     */

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

    /* generate a unique name for this area */
    len = strlen(fname) > 22 ? 22 : strlen(fname);
    areaname = malloc(sizeof(char) * 32);
    strncpy(areaname, "beos_mmap:\0", 11);
    strncat(areaname, fname + (strlen(fname)-len), len);
    
    aid = create_area(areaname, mm, B_ANY_ADDRESS, size * B_PAGE_SIZE, 
        B_FULL_LOCK, B_READ_AREA|B_WRITE_AREA);
    free(areaname);
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
           
    /* register the cleanup... */ 
    ap_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
             ap_null_cleanup);

    return APR_SUCCESS;
}

ap_status_t ap_mmap_delete(struct mmap_t *mmap)
{
    ap_status_t rv;
    if (mm->area == -1)
        return APR_ENOENT;
         
    if ((rv = mmap_cleanup(mmap)) == APR_SUCCESS) {
        ap_kill_cleanup(mmap->cntxt, mmap, mmap_cleanup);
        return APR_SUCCESS;
    }
    return rv;
}
