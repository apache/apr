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

#define INCL_DOS
#define INCL_DOSERRORS
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_lib.h"
#include <sys/time.h>
#include <os2.h>


static void FS3_to_finfo(apr_finfo_t *finfo, FILESTATUS3 *fstatus)
{
    finfo->protection = (fstatus->attrFile & FILE_READONLY) ? 0555 : 0777;

    if (fstatus->attrFile & FILE_DIRECTORY)
        finfo->filetype = APR_DIR;
    else
        finfo->filetype = APR_REG;

    finfo->user = 0;
    finfo->group = 0;
    finfo->inode = 0;
    finfo->device = 0;
    finfo->size = fstatus->cbFile;
    ap_os2_time_to_ap_time(&finfo->atime, fstatus->fdateLastAccess, fstatus->ftimeLastAccess );
    ap_os2_time_to_ap_time(&finfo->mtime, fstatus->fdateLastWrite,  fstatus->ftimeLastWrite );
    ap_os2_time_to_ap_time(&finfo->ctime, fstatus->fdateCreation,   fstatus->ftimeCreation );
}



static apr_status_t handle_type(ap_filetype_e *ftype, HFILE file)
{
    ULONG filetype, fileattr, rc;

    rc = DosQueryHType(file, &filetype, &fileattr);

    if (rc == 0) {
        switch (filetype & 0xff) {
        case 0:
            *ftype = APR_REG;
            break;

        case 1:
            *ftype = APR_CHR;
            break;

        case 2:
            *ftype = APR_PIPE;
            break;
        }

        return APR_SUCCESS;
    }
    return APR_OS2_STATUS(rc);
}



apr_status_t apr_getfileinfo(apr_finfo_t *finfo, apr_file_t *thefile)
{
    ULONG rc;
    FILESTATUS3 fstatus;

    if (thefile->isopen)
        rc = DosQueryFileInfo(thefile->filedes, FIL_STANDARD, &fstatus, sizeof(fstatus));
    else
        rc = DosQueryPathInfo(thefile->fname, FIL_STANDARD, &fstatus, sizeof(fstatus));

    if (rc == 0) {
        FS3_to_finfo(finfo, &fstatus);

        if (finfo->filetype == APR_REG) {
            if (thefile->isopen) {
                return handle_type(&finfo->filetype, thefile->filedes);
            }
        } else {
            return APR_SUCCESS;
        }
    }

    finfo->protection = 0;
    finfo->filetype = APR_NOFILE;
    return APR_OS2_STATUS(rc);
}

apr_status_t apr_setfileperms(const char *fname, apr_fileperms_t perms)
{
    return APR_ENOTIMPL;
}


apr_status_t apr_stat(apr_finfo_t *finfo, const char *fname, apr_pool_t *cont)
{
    ULONG rc;
    FILESTATUS3 fstatus;
    
    finfo->protection = 0;
    finfo->filetype = APR_NOFILE;
    rc = DosQueryPathInfo(fname, FIL_STANDARD, &fstatus, sizeof(fstatus));
    
    if (rc == 0) {
        FS3_to_finfo(finfo, &fstatus);
        return APR_SUCCESS;
    } else if (rc == ERROR_INVALID_ACCESS) {
        memset(finfo, 0, sizeof(apr_finfo_t));
        finfo->protection = 0444;
        finfo->filetype = APR_CHR;
        return APR_SUCCESS;
    }
    
    return APR_OS2_STATUS(rc);
}
