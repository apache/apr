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

#include "fileio.h"
#include "apr_file_io.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include <string.h>

ap_status_t apr_file_cleanup(void *thefile)
{
    ap_file_t *file = thefile;
    return ap_close(file);
}



ap_status_t ap_open(ap_file_t **new, const char *fname, ap_int32_t flag,  ap_fileperms_t perm, ap_pool_t *cntxt)
{
    int oflags = 0;
    int mflags = OPEN_FLAGS_FAIL_ON_ERROR|OPEN_SHARE_DENYNONE;
    int rv;
    ULONG action;
    ap_file_t *dafile = (ap_file_t *)ap_palloc(cntxt, sizeof(ap_file_t));

    *new = dafile;
    dafile->cntxt = cntxt;
    dafile->isopen = FALSE;
    dafile->eof_hit = FALSE;
    dafile->buffer = NULL;
    dafile->flags = flag;
    
    if ((flag & APR_READ) && (flag & APR_WRITE)) {
        mflags |= OPEN_ACCESS_READWRITE;
    } else if (flag & APR_READ) {
        mflags |= OPEN_ACCESS_READONLY;
    } else if (flag & APR_WRITE) {
        mflags |= OPEN_ACCESS_WRITEONLY;
    } else {
        dafile->filedes = -1;
        return APR_EACCES;
    }

    dafile->buffered = (flag & APR_BUFFERED) > 0;

    if (dafile->buffered) {
        dafile->buffer = ap_palloc(cntxt, APR_FILE_BUFSIZE);
        rv = ap_create_lock(&dafile->mutex, APR_MUTEX, APR_INTRAPROCESS, NULL, cntxt);

        if (rv)
            return rv;
    }

    if (flag & APR_CREATE) {
        oflags |= OPEN_ACTION_CREATE_IF_NEW; 
        if (!(flag & APR_EXCL)) {
            if (flag & APR_APPEND)
                oflags |= OPEN_ACTION_OPEN_IF_EXISTS;
            else
                oflags |= OPEN_ACTION_REPLACE_IF_EXISTS;
        }
    }
    
    if ((flag & APR_EXCL) && !(flag & APR_CREATE))
        return APR_EACCES;

    if (flag & APR_TRUNCATE) {
        oflags |= OPEN_ACTION_REPLACE_IF_EXISTS;
    } else if ((oflags & 0xF) == 0) {
        oflags |= OPEN_ACTION_OPEN_IF_EXISTS;
    }
    
    rv = DosOpen(fname, &(dafile->filedes), &action, 0, 0, oflags, mflags, NULL);
    
    if (rv == 0 && (flag & APR_APPEND)) {
        ULONG newptr;
        rv = DosSetFilePtr(dafile->filedes, 0, FILE_END, &newptr );
        
        if (rv)
            DosClose(dafile->filedes);
    }
    
    if (rv != 0)
        return APR_OS2_STATUS(rv);
    
    dafile->isopen = TRUE;
    dafile->fname = ap_pstrdup(cntxt, fname);
    dafile->filePtr = 0;
    dafile->bufpos = 0;
    dafile->dataRead = 0;
    dafile->direction = 0;
    dafile->pipe = FALSE;

    ap_register_cleanup(dafile->cntxt, dafile, apr_file_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
}



ap_status_t ap_close(ap_file_t *file)
{
    ULONG rc;
    ap_status_t status;
    
    if (file && file->isopen) {
        ap_flush(file);
        rc = DosClose(file->filedes);
    
        if (rc == 0) {
            file->isopen = FALSE;
            status = APR_SUCCESS;

            if (file->flags & APR_DELONCLOSE) {
                status = APR_OS2_STATUS(DosDelete(file->fname));
            }
        } else {
            return APR_OS2_STATUS(rc);
        }
    }

    if (file->buffered)
        ap_destroy_lock(file->mutex);

    return APR_SUCCESS;
}



ap_status_t ap_remove_file(const char *path, ap_pool_t *cntxt)
{
    ULONG rc = DosDelete(path);
    return APR_OS2_STATUS(rc);
}



ap_status_t ap_rename_file(const char *from_path, const char *to_path,
                           ap_pool_t *p)
{
    ULONG rc = DosMove(from_path, to_path);

    if (rc == ERROR_ACCESS_DENIED) {
        rc = DosDelete(to_path);

        if (rc == 0 || rc == ERROR_FILE_NOT_FOUND) {
            rc = DosMove(from_path, to_path);
        }
    }

    return APR_OS2_STATUS(rc);
}



ap_status_t ap_get_os_file(ap_os_file_t *thefile, ap_file_t *file)
{
    if (file == NULL) {
        return APR_ENOFILE;
    }

    *thefile = file->filedes;
    return APR_SUCCESS;
}



ap_status_t ap_put_os_file(ap_file_t **file, ap_os_file_t *thefile, ap_pool_t *cont)
{
    ap_os_file_t *dafile = thefile;
    if ((*file) == NULL) {
        (*file) = (ap_file_t *)ap_palloc(cont, sizeof(ap_file_t));
        (*file)->cntxt = cont;
    }
    (*file)->filedes = *dafile;
    (*file)->isopen = TRUE;
    (*file)->buffered = FALSE;
    (*file)->eof_hit = FALSE;
    (*file)->flags = 0;
    (*file)->pipe = FALSE;
    return APR_SUCCESS;
}    



ap_status_t ap_eof(ap_file_t *fptr)
{
    if (!fptr->isopen || fptr->eof_hit == 1) {
        return APR_EOF;
    }
    return APR_SUCCESS;
}   



ap_status_t ap_open_stderr(ap_file_t **thefile, ap_pool_t *cont)
{
    (*thefile) = ap_palloc(cont, sizeof(ap_file_t));
    if ((*thefile) == NULL) {
        return APR_ENOMEM;
    }
    (*thefile)->cntxt = cont;
    (*thefile)->filedes = 2;
    (*thefile)->fname = NULL;
    (*thefile)->isopen = TRUE;
    (*thefile)->buffered = FALSE;
    (*thefile)->eof_hit = FALSE;
    (*thefile)->flags = 0;
    (*thefile)->pipe = FALSE;

    return APR_SUCCESS;
}
