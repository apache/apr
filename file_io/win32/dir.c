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

#include "apr.h"
#include "win32/fileio.h"
#include "apr_file_io.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include "atime.h"

#if APR_HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

apr_status_t dir_cleanup(void *thedir)
{
    apr_dir_t *dir = thedir;
    if (dir->dirhand != INVALID_HANDLE_VALUE && !FindClose(dir->dirhand)) {
        return apr_get_os_error();
    }
    dir->dirhand = INVALID_HANDLE_VALUE;
    return APR_SUCCESS;
} 

apr_status_t apr_dir_open(apr_dir_t **new, const char *dirname, apr_pool_t *cont)
{
    /* Note that we won't open a directory that is greater than MAX_PATH,
     * including the trailing /* wildcard suffix.  If a * won't fit, then
     * neither will any other file name within the directory.
     * The length not including the trailing '*' is stored as rootlen, to
     * skip over all paths which are too long.
     */
    int len = strlen(dirname);
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT)
    {
        /* While there is now a generic accessor to convert to Unicode,
         * we do something special here to provide a few extra wchars
         * for the /* (really \*) suffix
         *
         * Note that the \\?\ form only works for local drive paths, and
         * not for UNC paths.
         */
        int srcremains = len;
        int dirremains = len;
        apr_wchar_t *wch;
        (*new) = apr_pcalloc(cont, sizeof(apr_dir_t));
        (*new)->w.entry = apr_pcalloc(cont, sizeof(WIN32_FIND_DATAW));
        if (dirname[1] == ':' && dirname[2] == '/') {
            (*new)->w.dirname = apr_palloc(cont, (dirremains + 7) * 2);
            wcscpy((*new)->w.dirname, L"\\\\?\\");
            wch = (*new)->w.dirname + 4;
        }
        else if (dirname[0] == '/' && dirname[1] == '/') {
            /* Skip the leading slashes */
            dirname += 2;
            srcremains = dirremains = (len -= 2);
            (*new)->w.dirname = apr_palloc(cont, (dirremains + 11) * 2);
            wcscpy ((*new)->w.dirname, L"\\\\?\\UNC\\");
            wch = (*new)->w.dirname + 8;
        }
        else
            wch = (*new)->w.dirname = apr_palloc(cont, (dirremains + 3) * 2);
        
        if (conv_utf8_to_ucs2(dirname, &srcremains,
                              wch, &dirremains) || srcremains) {
            (*new) = NULL;
            return APR_ENAMETOOLONG;
        }
        len -= dirremains;
        if (len && wch[len - 1] != '/') {
    	    wch[len++] = L'/';
        }
        wch[len++] = L'*';
        wch[len] = L'\0';
        if (wch != (*new)->w.dirname)
        {
            if (len >= MAX_PATH ) {
                (*new) = NULL;
                return APR_ENAMETOOLONG;
            }
            (*new)->rootlen = len - 1;
        }
        else /* we don't care, since the path isn't limited in length */
            (*new)->rootlen = 0;
        for (; *wch; ++wch)
            if (*wch == L'/')
                *wch = L'\\';
    }
    else
#endif
    {
        (*new) = apr_pcalloc(cont, sizeof(apr_dir_t));
        (*new)->n.entry = apr_pcalloc(cont, sizeof(WIN32_FIND_DATA));
        (*new)->n.dirname = apr_palloc(cont, len + 3);
        memcpy((*new)->n.dirname, dirname, len);
        if (len && (*new)->n.dirname[len - 1] != '/') {
    	    (*new)->n.dirname[len++] = '/';
        }
        (*new)->n.dirname[len++] = '*';
        (*new)->n.dirname[len] = '\0';
        (*new)->rootlen = len - 1;
        if (len >= MAX_PATH ){
            (*new) = NULL;
            return APR_ENAMETOOLONG;
        }
    }
    (*new)->cntxt = cont;
    (*new)->dirhand = INVALID_HANDLE_VALUE;
    apr_register_cleanup((*new)->cntxt, (void *)(*new), dir_cleanup,
                        apr_null_cleanup);
    return APR_SUCCESS;
}

apr_status_t apr_closedir(apr_dir_t *dir)
{
    if (dir->dirhand != INVALID_HANDLE_VALUE && !FindClose(dir->dirhand)) {
        return apr_get_os_error();
    }
    dir->dirhand = INVALID_HANDLE_VALUE;
    return APR_SUCCESS;
}

apr_status_t apr_readdir(apr_dir_t *thedir)
{
    /* The while loops below allow us to skip all invalid file names, so that
     * we aren't reporting any files where their absolute paths are too long.
     */
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(thedir->cntxt, &os_level) && os_level >= APR_WIN_NT)
    {
        if (thedir->dirhand == INVALID_HANDLE_VALUE) {
            thedir->dirhand = FindFirstFileW(thedir->w.dirname, thedir->w.entry);
            if (thedir->dirhand == INVALID_HANDLE_VALUE) {
                return apr_get_os_error();
            }
        }
        else if (!FindNextFileW(thedir->dirhand, thedir->w.entry)) {
            return apr_get_os_error();
        }
        while (thedir->rootlen &&
               thedir->rootlen + wcslen(thedir->w.entry->cFileName) >= MAX_PATH)
        {
            if (!FindNextFileW(thedir->dirhand, thedir->w.entry)) {
                return apr_get_os_error();
            }
        }
    }
    else
#endif
    {
        if (thedir->dirhand == INVALID_HANDLE_VALUE) {
            thedir->dirhand = FindFirstFile(thedir->n.dirname, thedir->n.entry);
            if (thedir->dirhand == INVALID_HANDLE_VALUE) {
                return apr_get_os_error();
            }
        }
        else if (!FindNextFile(thedir->dirhand, thedir->n.entry)) {
            return apr_get_os_error();
        }
        while (thedir->rootlen &&
               thedir->rootlen + strlen(thedir->n.entry->cFileName) >= MAX_PATH)
        {
            if (!FindNextFileW(thedir->dirhand, thedir->w.entry)) {
                return apr_get_os_error();
            }
        }
    }
    return APR_SUCCESS;
}

apr_status_t apr_rewinddir(apr_dir_t *dir)
{
    dir_cleanup(dir);
    if (!FindClose(dir->dirhand)) {
        return apr_get_os_error();
    }    
    dir->dirhand = INVALID_HANDLE_VALUE;
    return APR_SUCCESS;
}

apr_status_t apr_make_dir(const char *path, apr_fileperms_t perm, apr_pool_t *cont)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT) 
    {
        apr_wchar_t *wpath = utf8_to_unicode_path(path, cont);
        if (!wpath)
            return APR_ENAMETOOLONG;
        if (!CreateDirectoryW(wpath, NULL)) {
            return apr_get_os_error();
        }
    }
    else
#endif
        if (!CreateDirectory(path, NULL)) {
            return apr_get_os_error();
        }
    return APR_SUCCESS;
}

apr_status_t apr_remove_dir(const char *path, apr_pool_t *cont)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT) 
    {
        apr_wchar_t *wpath = utf8_to_unicode_path(path, cont);
        if (!wpath)
            return APR_ENAMETOOLONG;
        if (!RemoveDirectoryW(wpath)) {
            return apr_get_os_error();
        }
    }
    else
#endif
        if (!RemoveDirectory(path)) {
            return apr_get_os_error();
        }
    return APR_SUCCESS;
}

apr_status_t apr_dir_entry_size(apr_ssize_t *size, apr_dir_t *thedir)
{
    if (thedir == NULL || thedir->n.entry == NULL) {
        return APR_ENODIR;
    }
    (*size) = (thedir->n.entry->nFileSizeHigh * MAXDWORD) + 
        thedir->n.entry->nFileSizeLow;
    return APR_SUCCESS;
}

apr_status_t apr_dir_entry_mtime(apr_time_t *time, apr_dir_t *thedir)
{
    if (thedir == NULL || thedir->n.entry == NULL) {
        return APR_ENODIR;
    }
    FileTimeToAprTime(time, &thedir->n.entry->ftLastWriteTime);
    return APR_SUCCESS;
}
 
apr_status_t apr_dir_entry_ftype(apr_filetype_e *type, apr_dir_t *thedir)
{
    switch(thedir->n.entry->dwFileAttributes) {
        case FILE_ATTRIBUTE_DIRECTORY: {
            (*type) = APR_DIR;
            return APR_SUCCESS;
        }
        case FILE_ATTRIBUTE_NORMAL: {
            (*type) = APR_REG;
            return APR_SUCCESS;
        }
        default: {
            (*type) = APR_REG;     /* As valid as anything else.*/
            return APR_SUCCESS;
        }
    }
}

apr_status_t apr_get_dir_filename(char **new, apr_dir_t *thedir)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(thedir->cntxt, &os_level) && os_level >= APR_WIN_NT)
    {
        (*new) = unicode_to_utf8_path(thedir->w.entry->cFileName, 
                                      thedir->cntxt);
        if (!*new)
            return APR_ENAMETOOLONG;
    }
    else
#endif
        (*new) = apr_pstrdup(thedir->cntxt, thedir->n.entry->cFileName);
    return APR_SUCCESS;
}

apr_status_t apr_get_os_dir(apr_os_dir_t **thedir, apr_dir_t *dir)
{
    if (dir == NULL) {
        return APR_ENODIR;
    }
    *thedir = dir->dirhand;
    return APR_SUCCESS;
}

/* XXX: This is sort of blinkin stupid on win32... consider,
 * our open doesn't open the dir, it sets up the apr_dir_t,
 * and on the first apr_readdir it actually does a FindFirstFile
 * if the handle is closed, or else a FindNextFile that is based 
 * on cached info that we simply don't have our hands on when
 * we use this function.  Maybe APR_ENOTIMPL would be better?
 */
apr_status_t apr_put_os_dir(apr_dir_t **dir, apr_os_dir_t *thedir, apr_pool_t *cont)
{
    if (cont == NULL) {
        return APR_ENOPOOL;
    }
    if ((*dir) == NULL) {
        (*dir) = (apr_dir_t *)apr_pcalloc(cont, sizeof(apr_dir_t));
        (*dir)->cntxt = cont;
    }
    else
        (*dir)->rootlen = 0; /* We don't know, don't care */
    (*dir)->dirhand = thedir;
    return APR_SUCCESS;
}
