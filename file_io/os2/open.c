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

#include "fileio.h"
#include "apr_file_io.h"
#include "apr_lib.h"
#include <string.h>

#define INCL_DOS
#include <os2.h>

ap_status_t file_cleanup(void *thefile)
{
    struct file_t *file = thefile;
    return ap_close(file);
}



ap_status_t ap_open(ap_context_t *cntxt, char *fname, ap_int32_t flag,  ap_fileperms_t perm, struct file_t **new)
{
    int oflags = 0;
    int mflags = OPEN_FLAGS_FAIL_ON_ERROR|OPEN_SHARE_DENYNONE;
    int rv;
    ULONG action;
    ap_file_t *dafile = (struct file_t *)ap_palloc(cntxt, sizeof(struct file_t));

    *new = dafile;
    dafile->cntxt = cntxt;
    dafile->isopen = FALSE;
    dafile->validstatus = FALSE;
    
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
    }
    
    rv = DosOpen(fname, (HFILE *)&(dafile->filedes), &action, 0, 0, oflags, mflags, NULL);
    
    if (rv == 0 && (flag & APR_APPEND)) {
        ULONG newptr;
        rv = DosSetFilePtr(dafile->filedes, 0, FILE_END, &newptr );
        
        if (rv)
            DosClose(dafile->filedes);
    }
    
    if (rv != 0)
        return os2errno(rv);
    
    dafile->isopen = TRUE;
    dafile->fname = ap_pstrdup(cntxt, fname);
    ap_register_cleanup(dafile->cntxt, dafile, file_cleanup, NULL);
    return APR_SUCCESS;
}



ap_status_t ap_close(ap_file_t *file)
{
    ULONG rc; 
    
    if (file && file->isopen) {
        rc = DosClose(file->filedes);
    
        if (rc == 0) {
            file->isopen = FALSE;
            return APR_SUCCESS;
        } else {
            return os2errno(rc);
        }
    }
    
    return APR_SUCCESS;
}



ap_status_t ap_remove_file(ap_context_t *cntxt, char *path)
{
    ULONG rc = DosDelete(path);
    return os2errno(rc);
}

