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

#include "mmap_h.h"

ap_status_t mmap_cleanup(void *themmap)
{
    ap_mmap_t *mm = themmap;
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

ap_status_t ap_mmap_create(ap_mmap_t **new, ap_file_t *file, ap_off_t offset, ap_size_t size,
                ap_context_t *cont)
{
    void *mm;
    area_id aid = -1;
    char *areaname = "apr_mmap\0";
    uint32 pages = 0;

    if (file == NULL || file->buffered || file->filedes == -1)
        return APR_EBADF;
    (*new) = (ap_mmap_t *)ap_palloc(cont, sizeof(ap_mmap_t));
    
    pages = ((size -1) / B_PAGE_SIZE) + 1;

    ap_seek(file, APR_SET, &offset);
     
    aid = create_area(areaname, &mm , B_ANY_ADDRESS, pages * B_PAGE_SIZE, 
        B_FULL_LOCK, B_READ_AREA|B_WRITE_AREA);

    if (aid < B_NO_ERROR) {
        /* we failed to get an mmap'd file... */
        return APR_ENOMEM;
    }
      
    if (aid >= B_NO_ERROR)
        read(file->filedes, mm, size);    

    (*new)->mm = mm;
    (*new)->size = size;
    (*new)->area = aid;
    (*new)->cntxt = cont;
               
    /* register the cleanup... */ 
    ap_register_cleanup((*new)->cntxt, (void*)(*new), mmap_cleanup,
             ap_null_cleanup);

    return APR_SUCCESS;
}

ap_status_t ap_mmap_delete(ap_mmap_t *mmap)
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
