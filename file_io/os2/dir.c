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

ap_status_t dir_cleanup(void *thedir)
{
    struct dir_t *dir = thedir;
    return ap_closedir(dir);
}



ap_status_t ap_opendir(struct dir_t **new, const char *dirname, ap_context_t *cntxt)
{
    struct dir_t *thedir = (struct dir_t *)ap_palloc(cntxt, sizeof(struct dir_t));
    
    if (thedir == NULL)
        return APR_ENOMEM;
    
    thedir->cntxt = cntxt;
    thedir->dirname = ap_pstrdup(cntxt, dirname);
    thedir->handle = 0;
    thedir->validentry = FALSE;
    *new = thedir;
    return APR_SUCCESS;
}



ap_status_t ap_closedir(struct dir_t *thedir)
{
    int rv = 0;
    
    if (thedir->handle) {
        rv = DosFindClose(thedir->handle);
        
        if (rv == 0) {
            thedir->handle = 0;
        }
    }

    return os2errno(rv);
} 



ap_status_t ap_readdir(struct dir_t *thedir)
{
    int rv;
    ULONG entries = 1;
    
    if (thedir->handle == 0) {
        thedir->handle = HDIR_CREATE;
        rv = DosFindFirst(ap_pstrcat(thedir->cntxt, thedir->dirname, "/*", NULL), &thedir->handle, 
                          FILE_ARCHIVED|FILE_DIRECTORY|FILE_SYSTEM|FILE_HIDDEN|FILE_READONLY, 
                          &thedir->entry, sizeof(thedir->entry), &entries, FIL_STANDARD);
    } else {
        rv = DosFindNext(thedir->handle, &thedir->entry, sizeof(thedir->entry), &entries);
    }

    if (rv == 0 && entries == 1) {
        thedir->validentry = TRUE;
        return APR_SUCCESS;
    }
        
    thedir->validentry = FALSE;
    
    if (rv)
        return os2errno(rv);
    
    return APR_ENOENT;
}



ap_status_t ap_rewinddir(struct dir_t *thedir)
{
    return ap_closedir(thedir);
}



ap_status_t ap_make_dir(const char *path, ap_fileperms_t perm, ap_context_t *cont)
{
    return os2errno(DosCreateDir(path, NULL));
}



ap_status_t ap_remove_dir(const char *path, ap_context_t *cont)
{
    return os2errno(DosDeleteDir(path));
}



ap_status_t ap_dir_entry_size(ap_ssize_t *size, struct dir_t *thedir)
{
    if (thedir->validentry) {
        *size = thedir->entry.cbFile;
        return APR_SUCCESS;
    }
    
    return APR_ENOFILE;
}



ap_status_t ap_dir_entry_mtime(time_t *time, struct dir_t *thedir)
{
    if (thedir->validentry) {
        *time = os2date2unix(thedir->entry.fdateLastWrite, thedir->entry.ftimeLastWrite);
        return APR_SUCCESS;
    }

    return APR_ENOFILE;
}



ap_status_t ap_dir_entry_ftype(ap_filetype_e *type, struct dir_t *thedir)
{
    int rc;
    HFILE hFile;
    ULONG action, Type, Attr;
    ap_filetype_e typemap[8] = { APR_REG, APR_CHR, APR_PIPE };

    if (thedir->validentry) {
        if (thedir->entry.attrFile & FILE_DIRECTORY) {
            *type = APR_DIR;
            return APR_SUCCESS;
        } else {
            rc = DosOpen(ap_pstrcat(thedir->cntxt, thedir->dirname, "/", thedir->entry.achName, NULL) ,
                         &hFile, &action, 0, 0,
                         OPEN_ACTION_FAIL_IF_NEW|OPEN_ACTION_OPEN_IF_EXISTS, OPEN_SHARE_DENYNONE|OPEN_ACCESS_READONLY,
                         NULL);

            if ( rc == 0 ) {
                rc = DosQueryHType( hFile, &Type, &Attr );

                if ( rc == 0 ) {
            *type = typemap[(Type & 0x0007)];
                }
                DosClose( hFile );
            }

            return os2errno(rc);
        }
    }

    return APR_ENOFILE;
}



ap_status_t ap_get_dir_filename(char **new, struct dir_t *thedir)
{
    if (thedir->validentry) {
        *new = thedir->entry.achName;
        return APR_SUCCESS;
    }

    return APR_ENOFILE;
}
