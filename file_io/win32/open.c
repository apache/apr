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
#include "win32/fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include <errno.h>
#include <winbase.h>
#include <string.h>
#include <sys/stat.h>
#include "misc.h"

#if APR_HAS_UNICODE_FS
apr_status_t utf8_to_unicode_path(apr_wchar_t* retstr, apr_size_t retlen, 
                                  const char* srcstr)
{
    /* TODO: The computations could preconvert the string to determine
     * the true size of the retstr, but that's a memory over speed
     * tradeoff that isn't appropriate this early in development.
     *
     * Allocate the maximum string length based on leading 4 
     * characters of \\?\ (allowing nearly unlimited path lengths) 
     * plus the trailing null, then transform /'s into \\'s since
     * the \\?\ form doesn't allow '/' path seperators.
     *
     * Note that the \\?\ form only works for local drive paths, and
     * \\?\UNC\ is needed UNC paths.
     */
    int srcremains = strlen(srcstr) + 1;
    int retremains = srcremains;
    apr_wchar_t *t = retstr;
    apr_status_t rv;
    if (srcstr[1] == ':' && srcstr[2] == '/') {
        wcscpy (retstr, L"\\\\?\\");
        retlen -= 4;
        t += 4;
    }
    else if (srcstr[0] == '/' && srcstr[1] == '/') {
        /* Skip the slashes */
        srcstr += 2;
        wcscpy (retstr, L"\\\\?\\UNC\\");
        retlen -= 8;
        t += 8;
    }

    if (rv = conv_utf8_to_ucs2(srcstr, &srcremains, t, &retremains)) {
        return rv;
    }
    if (srcremains) {
        return APR_ENAMETOOLONG;
    }
    for (; *t; ++t)
        if (*t == L'/')
            *t = L'\\';
    return APR_SUCCESS;
}

apr_status_t unicode_to_utf8_path(char* retstr, apr_size_t retlen,
                                  const apr_wchar_t* srcstr)
{
    /* Skip the leading 4 characters if the path begins \\?\, or substitute
     * // for the \\?\UNC\ path prefix, allocating the maximum string
     * length based on the remaining string, plus the trailing null.
     * then transform \\'s back into /'s since the \\?\ form never
     * allows '/' path seperators, and APR always uses '/'s.
     */
    int srcremains = wcslen(srcstr) + 1;
    apr_status_t rv;
    char *t = retstr;
    if (srcstr[0] == L'\\' && srcstr[1] == L'\\' && 
        srcstr[2] == L'?'  && srcstr[3] == L'\\') {
        if (srcstr[4] == L'U' && srcstr[5] == L'N' && 
            srcstr[6] == L'C' && srcstr[7] == L'\\') {
            srcremains -= 8;
            srcstr += 8;
            strcpy(retstr, "//");
            retlen -= 2;
            t += 2;
        }
        else {
            srcremains -= 4;
            srcstr += 4;
        }
    }
        
    if (rv = conv_ucs2_to_utf8(srcstr, &srcremains, t, &retlen)) {
        return rv;
    }
    if (srcremains) {
        return APR_ENAMETOOLONG;
    }
    for (; *t; ++t)
        if (*t == L'/')
            *t = L'\\';
    return APR_SUCCESS;
}
#endif

apr_status_t file_cleanup(void *thefile)
{
    apr_file_t *file = thefile;
    if (file->filehand != INVALID_HANDLE_VALUE) {
        CloseHandle(file->filehand);
        file->filehand = INVALID_HANDLE_VALUE;
    }
    if (file->pOverlapped) {
        CloseHandle(file->pOverlapped->hEvent);
        file->pOverlapped = NULL;
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_open(apr_file_t **new, const char *fname,
                                   apr_int32_t flag, apr_fileperms_t perm,
                                   apr_pool_t *cont)
{
    /* XXX: The default FILE_FLAG_SEQUENTIAL_SCAN is _wrong_ for
     *      sdbm and any other random files!  We _must_ rethink
     *      this approach.
     */
    HANDLE handle = INVALID_HANDLE_VALUE;
    DWORD oflags = 0;
    DWORD createflags = 0;
    DWORD attributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
    DWORD sharemode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    apr_oslevel_e os_level;
    apr_status_t rv;

    if (flag & APR_READ) {
        oflags |= GENERIC_READ;
    }
    if (flag & APR_WRITE) {
        oflags |= GENERIC_WRITE;
    }
    
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT) 
        sharemode |= FILE_SHARE_DELETE;
    else
        os_level = 0;

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
        return APR_EACCES;
    }   
    
    if (flag & APR_DELONCLOSE) {
        attributes |= FILE_FLAG_DELETE_ON_CLOSE;
    }
    if (flag & APR_OPENLINK) {
        attributes |= FILE_FLAG_OPEN_REPARSE_POINT;
    }
    if (!(flag & (APR_READ | APR_WRITE)) && (os_level >= APR_WIN_NT)) {
        /* We once failed here, but this is how one opens 
         * a directory as a file under winnt.  Accelerate
         * further by not hitting storage, we don't need to.
         */
        attributes |= FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_NO_RECALL;
    }
    if (flag & APR_XTHREAD) {
        /* This win32 specific feature is required 
         * to allow multiple threads to work with the file.
         */
        attributes |= FILE_FLAG_OVERLAPPED;
    }

#if APR_HAS_UNICODE_FS
    if (os_level >= APR_WIN_NT) {
        apr_wchar_t wfname[8192];
        if (rv = utf8_to_unicode_path(wfname, sizeof(wfname) 
                                               / sizeof(apr_wchar_t), fname))
            return rv;
        handle = CreateFileW(wfname, oflags, sharemode,
                             NULL, createflags, attributes, 0);
    }
    else
#endif
        handle = CreateFileA((*new)->fname, oflags, sharemode,
                             NULL, createflags, attributes, 0);

    if (handle == INVALID_HANDLE_VALUE) {
        return apr_get_os_error();
    }

    (*new) = (apr_file_t *)apr_pcalloc(cont, sizeof(apr_file_t));
    (*new)->cntxt = cont;
    (*new)->filehand = handle;
    (*new)->fname = apr_pstrdup(cont, fname);

    if (flag & APR_APPEND) {
        (*new)->append = 1;
        SetFilePointer((*new)->filehand, 0, NULL, FILE_END);
    }
    else {
        (*new)->append = 0;
    }

    if (flag & APR_BUFFERED) {
        (*new)->buffered = 1;
        (*new)->buffer = apr_palloc(cont, APR_FILE_BUFSIZE);
        rv = apr_create_lock(&(*new)->mutex, APR_MUTEX, APR_INTRAPROCESS, NULL, cont);

        if (rv) {
            if (file_cleanup(*new) == APR_SUCCESS) {
                apr_kill_cleanup(cont, *new, file_cleanup);
            }
            return rv;
        }
    }
    else {
        (*new)->buffered = 0;
        (*new)->buffer = apr_palloc(cont, APR_FILE_BUFSIZE);
        (*new)->mutex = NULL;
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

APR_DECLARE(apr_status_t) apr_close(apr_file_t *file)
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

APR_DECLARE(apr_status_t) apr_remove_file(const char *path, apr_pool_t *cont)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT) 
    {
        apr_wchar_t wpath[8192];
        apr_status_t rv;
        if (rv = utf8_to_unicode_path(wpath, sizeof(wpath) 
                                              / sizeof(apr_wchar_t), path)) {
            return rv;
        }
        if (DeleteFileW(wpath))
            return APR_SUCCESS;
    }
    else
#endif
        if (DeleteFile(path))
            return APR_SUCCESS;
    return apr_get_os_error();
}

APR_DECLARE(apr_status_t) apr_rename_file(const char *frompath,
                                          const char *topath,
                                          apr_pool_t *cont)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT) 
    {
        apr_wchar_t wfrompath[8192], wtopath[8192];
        apr_status_t rv;
        if (rv = utf8_to_unicode_path(wfrompath, sizeof(wfrompath) 
                                           / sizeof(apr_wchar_t), frompath)) {
            return rv;
        }
        if (rv = utf8_to_unicode_path(wtopath, sizeof(wtopath) 
                                             / sizeof(apr_wchar_t), topath)) {
            return rv;
        }
        if (MoveFileExW(wfrompath, wtopath, MOVEFILE_REPLACE_EXISTING |
                                            MOVEFILE_COPY_ALLOWED))
            return APR_SUCCESS;
    }
    else
#endif
        if (MoveFileEx(frompath, topath, MOVEFILE_REPLACE_EXISTING |
                                         MOVEFILE_COPY_ALLOWED))
            return APR_SUCCESS;
    return apr_get_os_error();
}

APR_DECLARE(apr_status_t) apr_get_os_file(apr_os_file_t *thefile,
                                          apr_file_t *file)
{
    if (file == NULL) {
        return APR_ENOFILE;
    }
    *thefile = file->filehand;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_put_os_file(apr_file_t **file,
                                          apr_os_file_t *thefile,
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

APR_DECLARE(apr_status_t) apr_eof(apr_file_t *fptr)
{
    if (fptr->eof_hit == 1) {
        return APR_EOF;
    }
    return APR_SUCCESS;
}   

APR_DECLARE(apr_status_t) apr_open_stderr(apr_file_t **thefile, apr_pool_t *cont)
{
    (*thefile) = apr_pcalloc(cont, sizeof(apr_file_t));
    if ((*thefile) == NULL) {
        return APR_ENOMEM;
    }
    (*thefile)->filehand = GetStdHandle(STD_ERROR_HANDLE);
    if ((*thefile)->filehand == INVALID_HANDLE_VALUE)
        return apr_get_os_error();
    (*thefile)->cntxt = cont;
    (*thefile)->fname = "\0"; // What was this??? : "STD_ERROR_HANDLE"; */
    (*thefile)->eof_hit = 0;

    return APR_SUCCESS;
}

