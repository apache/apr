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

#include "apr_private.h"
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include <errno.h>
#include <winbase.h>
#include <string.h>
#include <sys/stat.h>
#include "misc.h"

apr_status_t file_cleanup(void *thefile)
{
    apr_file_t *file = thefile;
    CloseHandle(file->filehand);
    file->filehand = INVALID_HANDLE_VALUE;
    if (file->pOverlapped) {
        CloseHandle(file->pOverlapped->hEvent);
    }
    return APR_SUCCESS;
}

apr_status_t apr_open(apr_file_t **new, const char *fname, 
                    apr_int32_t flag, apr_fileperms_t perm, apr_pool_t *cont)
{
    DWORD oflags = 0;
    DWORD createflags = 0;
    DWORD attributes = 0;
    DWORD sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    apr_oslevel_e level;
    apr_status_t rv;

    (*new) = (apr_file_t *)apr_pcalloc(cont, sizeof(apr_file_t));
    (*new)->cntxt = cont;

    if (flag & APR_READ) {
        oflags |= GENERIC_READ;
    }
    if (flag & APR_WRITE) {
        oflags |= GENERIC_WRITE;
    }
    if (!(flag & APR_READ) && !(flag & APR_WRITE)) {
        (*new)->filehand = INVALID_HANDLE_VALUE;
        return APR_EACCES;
    }

    (*new)->buffered = (flag & APR_BUFFERED) > 0;

    if ((*new)->buffered) {
        (*new)->buffer = apr_palloc(cont, APR_FILE_BUFSIZE);
        rv = apr_create_lock(&(*new)->mutex, APR_MUTEX, APR_INTRAPROCESS, NULL, cont);

        if (rv)
            return rv;
    }

    (*new)->fname = apr_pstrdup(cont, fname);

    (*new)->demonfname = canonical_filename((*new)->cntxt, fname);
    (*new)->lowerdemonfname = strlwr((*new)->demonfname);
 
    if (apr_get_oslevel(cont, &level) == APR_SUCCESS && level >= APR_WIN_NT) {
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
        (*new)->filehand = INVALID_HANDLE_VALUE;
        return APR_EACCES;
    }   

    if (flag & APR_APPEND) {
        (*new)->append = 1;
    }
    else {
        (*new)->append = 0;
    }

    attributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
    if (flag & APR_DELONCLOSE) {
        attributes |= FILE_FLAG_DELETE_ON_CLOSE;
    }

    (*new)->filehand = CreateFile(fname, oflags, sharemode,
                                     NULL, createflags, attributes, 0);

    if ((*new)->filehand == INVALID_HANDLE_VALUE) {
        return apr_get_os_error();
    }
    if (flag & APR_APPEND) {
        SetFilePointer((*new)->filehand, 0, NULL, FILE_END);
    }

    (*new)->pipe = 0;
    (*new)->timeout = -1;
    (*new)->ungetchar = -1;
    (*new)->eof_hit = 0;

    /* Buffered mode fields not initialized above */
    (*new)->bufpos = 0;
    (*new)->dataRead = 0;
    (*new)->direction = 0;
    (*new)->filePtr = 0;

    apr_register_cleanup((*new)->cntxt, (void *)(*new), file_cleanup,
                        apr_null_cleanup);
    return APR_SUCCESS;
}

apr_status_t apr_close(apr_file_t *file)
{
    apr_status_t stat;
    if ((stat = file_cleanup(file)) == APR_SUCCESS) {
        apr_kill_cleanup(file->cntxt, file, file_cleanup);

        if (file->buffered)
            apr_destroy_lock(file->mutex);

        return APR_SUCCESS;
    }
    return stat;
}

apr_status_t apr_remove_file(const char *path, apr_pool_t *cont)
{
    char *temp = canonical_filename(cont, path);

    if (DeleteFile(temp)) {
        return APR_SUCCESS;
    }
    else {
        return apr_get_os_error();
    }
}

apr_status_t apr_rename_file(const char *from_path, const char *to_path,
                           apr_pool_t *p)
{
    const char *from_canon = canonical_filename(p, from_path);
    const char *to_canon = canonical_filename(p, to_path);
    DWORD err;

    /* TODO: would be nice to use MoveFileEx() here, but it isn't available
     * on Win95/98. MoveFileEx could theoretically help prevent the
     * case where we delete the target but don't move the file(!).
     * it can also copy across devices...
     */

    if (MoveFile(from_canon, to_canon)) {
        return APR_SUCCESS;
    }
    err = apr_get_os_error();
    if (APR_STATUS_IS_EEXIST(err)) {
        (void) DeleteFile(to_canon);
        if (MoveFile(from_canon, to_canon))
            return APR_SUCCESS;
        err = apr_get_os_error();
    }
    return err;
}

apr_status_t apr_get_os_file(apr_os_file_t *thefile, apr_file_t *file)
{
    if (file == NULL) {
        return APR_ENOFILE;
    }
    *thefile = file->filehand;
    return APR_SUCCESS;
}

apr_status_t apr_put_os_file(apr_file_t **file, apr_os_file_t *thefile, 
                             apr_pool_t *cont)
{
    if ((*file) == NULL) {
        if (cont == NULL) {
            return APR_ENOPOOL;
        }
        (*file) = (apr_file_t *)apr_pcalloc(cont, sizeof(apr_file_t));
        (*file)->cntxt = cont;
    }
    (*file)->filehand = *thefile;
    (*file)->ungetchar = -1; /* no char avail */
    return APR_SUCCESS;
}    

apr_status_t apr_eof(apr_file_t *fptr)
{
    if (fptr->eof_hit == 1) {
        return APR_EOF;
    }
    return APR_SUCCESS;
}   

apr_status_t apr_open_stderr(apr_file_t **thefile, apr_pool_t *cont)
{
    (*thefile) = apr_pcalloc(cont, sizeof(apr_file_t));
    if ((*thefile) == NULL) {
        return APR_ENOMEM;
    }
    (*thefile)->filehand = GetStdHandle(STD_ERROR_HANDLE);
    if ((*thefile)->filehand == INVALID_HANDLE_VALUE)
        return apr_get_os_error();
    (*thefile)->cntxt = cont;
    (*thefile)->fname = "STD_ERROR_HANDLE";
    (*thefile)->eof_hit = 0;

    return APR_SUCCESS;
}

