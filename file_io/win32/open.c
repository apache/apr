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

#ifdef WIN32
#include "apr_winconfig.h"
#endif
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include <errno.h>
#include <winbase.h>
#include <string.h>
#include <sys/stat.h>

ap_status_t file_cleanup(void *thefile)
{
    struct file_t *file = thefile;
    if (!CloseHandle(file->filehand)) {
        return GetLastError();
    }
    file->filehand = INVALID_HANDLE_VALUE;
    return APR_SUCCESS;
}

ap_status_t ap_open(struct file_t **dafile, const char *fname, 
                    ap_int32_t flag, ap_fileperms_t perm, ap_context_t *cont)
{
    DWORD oflags = 0;
    DWORD createflags = 0;
    DWORD attributes = 0;
    DWORD sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    ap_oslevel_e level;

    (*dafile) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));

    (*dafile)->cntxt = cont;

    if (flag & APR_READ) {
        oflags |= GENERIC_READ;
    }
    if (flag & APR_WRITE) {
        oflags |= GENERIC_WRITE;
    }
    if (!(flag & APR_READ) && !(flag & APR_WRITE)) {
        (*dafile)->filehand = INVALID_HANDLE_VALUE;
        return APR_EACCES;
    }

    if (flag & APR_BUFFERED) {
        (*dafile)->buffered = TRUE;
    } else {
        (*dafile)->buffered = FALSE;
    }
    (*dafile)->fname = ap_pstrdup(cont, fname);

    (*dafile)->demonfname = canonical_filename((*dafile)->cntxt, fname);
    (*dafile)->lowerdemonfname = strlwr((*dafile)->demonfname);
 
    if (ap_get_oslevel(cont, &level) == APR_SUCCESS && level == APR_WIN_NT) {
        sharemode |= FILE_SHARE_DELETE;
    }

    if (flag & APR_CREATE) {
        if (flag & APR_EXCL) {
            /* only create new if file does not already exist */
            createflags = CREATE_NEW;
        } else if (flag & APR_TRUNCATE) {
            /* truncate existing file or create new */
            createflags = CREATE_ALWAYS;
        } else {
            /* open existing but create if necessary */
            createflags = OPEN_ALWAYS;
        }
    } else if (flag & APR_TRUNCATE) {
        /* only truncate if file already exists */
        createflags = TRUNCATE_EXISTING;
    } else {
        /* only open if file already exists */
        createflags = OPEN_EXISTING;
    }

    if ((flag & APR_EXCL) && !(flag & APR_CREATE)) {
        (*dafile)->filehand = INVALID_HANDLE_VALUE;
        return APR_EACCES;
    }   

    if (flag & APR_APPEND) {
        (*dafile)->append = 1;
    }
    else {
        (*dafile)->append = 0;
    }

    if (flag & APR_TRUNCATE) {
        createflags = TRUNCATE_EXISTING;
    }
 
    attributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
    if (flag & APR_DELONCLOSE) {
        attributes |= FILE_FLAG_DELETE_ON_CLOSE;
    }

    (*dafile)->filehand = CreateFile(fname, oflags, sharemode,
                                     NULL, createflags, attributes, 0);

    if ((*dafile)->filehand == INVALID_HANDLE_VALUE) {
        return GetLastError();
    }
    if (flag & APR_APPEND) {
        SetFilePointer((*dafile)->filehand, 0, NULL, FILE_END);
    }

    (*dafile)->stated = 0;  /* we haven't called stat for this file yet. */
    (*dafile)->eof_hit = 0;
    ap_register_cleanup((*dafile)->cntxt, (void *)(*dafile), file_cleanup,
                        ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_close(struct file_t *file)
{
    ap_status_t stat;
    if ((stat = file_cleanup(file)) == APR_SUCCESS) {
        ap_kill_cleanup(file->cntxt, file, file_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

ap_status_t ap_remove_file(char *path, ap_context_t *cont)
{
    char *temp = canonical_filename(cont, path);

    if (DeleteFile(temp)) {
        return APR_SUCCESS;
    }
    else {
        return APR_EEXIST;
    }
}

ap_status_t ap_get_os_file(ap_os_file_t *thefile, struct file_t *file)
{
    if (file == NULL) {
        return APR_ENOFILE;
    }
    *thefile = file->filehand;
    return APR_SUCCESS;
}

ap_status_t ap_put_os_file(struct file_t **file, ap_os_file_t *thefile, 
                           ap_context_t *cont)
{
    if ((*file) == NULL) {
        if (cont == NULL) {
            return APR_ENOCONT;
        }
        (*file) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));
        (*file)->cntxt = cont;
    }
    (*file)->filehand = *thefile;
    return APR_SUCCESS;
}    

ap_status_t ap_eof(ap_file_t *fptr)
{
    if (fptr->eof_hit == 1) {
        return APR_EOF;
    }
    return APR_SUCCESS;
}   

ap_status_t ap_open_stderr(struct file_t **thefile, ap_context_t *cont)
{
    (*thefile) = ap_pcalloc(cont, sizeof(struct file_t));
    if ((*thefile) == NULL) {
        return APR_ENOMEM;
    }
    (*thefile)->filehand = GetStdHandle(STD_ERROR_HANDLE);
    (*thefile)->cntxt = cont;
    (*thefile)->fname = NULL;
    (*thefile)->stated = 0;
    (*thefile)->buffered = 0;
    (*thefile)->eof_hit = 0;

    return APR_SUCCESS;
}

