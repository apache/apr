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
#include "apr_strings.h"
#include <string.h>

#define INCL_DOS
#include <os2.h>

static apr_status_t dir_cleanup(void *thedir)
{
    apr_dir_t *dir = thedir;
    return apr_closedir(dir);
}



apr_status_t apr_dir_open(apr_dir_t **new, const char *dirname, apr_pool_t *cntxt)
{
    apr_dir_t *thedir = (apr_dir_t *)apr_palloc(cntxt, sizeof(apr_dir_t));
    
    if (thedir == NULL)
        return APR_ENOMEM;
    
    thedir->cntxt = cntxt;
    thedir->dirname = apr_pstrdup(cntxt, dirname);

    if (thedir->dirname == NULL)
        return APR_ENOMEM;

    thedir->handle = 0;
    thedir->validentry = FALSE;
    *new = thedir;
    apr_register_cleanup(cntxt, thedir, dir_cleanup, apr_null_cleanup);
    return APR_SUCCESS;
}



apr_status_t apr_closedir(apr_dir_t *thedir)
{
    int rv = 0;
    
    if (thedir->handle) {
        rv = DosFindClose(thedir->handle);
        
        if (rv == 0) {
            thedir->handle = 0;
        }
    }

    return APR_OS2_STATUS(rv);
} 



apr_status_t apr_readdir(apr_dir_t *thedir)
{
    int rv;
    ULONG entries = 1;
    
    if (thedir->handle == 0) {
        thedir->handle = HDIR_CREATE;
        rv = DosFindFirst(apr_pstrcat(thedir->cntxt, thedir->dirname, "/*", NULL), &thedir->handle, 
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
        return APR_OS2_STATUS(rv);
    
    return APR_ENOENT;
}



apr_status_t apr_rewinddir(apr_dir_t *thedir)
{
    return apr_closedir(thedir);
}



apr_status_t apr_make_dir(const char *path, apr_fileperms_t perm, apr_pool_t *cont)
{
    return APR_OS2_STATUS(DosCreateDir(path, NULL));
}



apr_status_t apr_remove_dir(const char *path, apr_pool_t *cont)
{
    return APR_OS2_STATUS(DosDeleteDir(path));
}



apr_status_t apr_dir_entry_size(apr_size_t *size, apr_dir_t *thedir)
{
    if (thedir->validentry) {
        *size = thedir->entry.cbFile;
        return APR_SUCCESS;
    }
    
    return APR_ENOFILE;
}



apr_status_t apr_dir_entry_mtime(apr_time_t *time, apr_dir_t *thedir)
{
    if (thedir->validentry) {
        apr_os2_time_to_apr_time(time, thedir->entry.fdateLastWrite, 
                                 thedir->entry.ftimeLastWrite);
        return APR_SUCCESS;
    }

    return APR_ENOFILE;
}



apr_status_t apr_dir_entry_ftype(apr_filetype_e *type, apr_dir_t *thedir)
{
    int rc;
    HFILE hFile;
    ULONG action, Type, Attr;
    apr_filetype_e typemap[8] = { APR_REG, APR_CHR, APR_PIPE };

    if (thedir->validentry) {
        if (thedir->entry.attrFile & FILE_DIRECTORY) {
            *type = APR_DIR;
            return APR_SUCCESS;
        } else {
            rc = DosOpen(apr_pstrcat(thedir->cntxt, thedir->dirname, "/", thedir->entry.achName, NULL) ,
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

            return APR_OS2_STATUS(rc);
        }
    }

    return APR_ENOFILE;
}



apr_status_t apr_get_dir_filename(char **new, apr_dir_t *thedir)
{
    if (thedir->validentry) {
        *new = thedir->entry.achName;
        return APR_SUCCESS;
    }

    return APR_ENOFILE;
}
