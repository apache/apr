/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include "atime.h"

#if APR_HAVE_ERRNO_H
#include <errno.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif


static apr_status_t dir_cleanup(void *thedir)
{
    apr_dir_t *dir = thedir;
    if (dir->dirhand != INVALID_HANDLE_VALUE && !FindClose(dir->dirhand)) {
        return apr_get_os_error();
    }
    dir->dirhand = INVALID_HANDLE_VALUE;
    return APR_SUCCESS;
} 

APR_DECLARE(apr_status_t) apr_dir_open(apr_dir_t **new, const char *dirname,
                                       apr_pool_t *cont)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
#endif
    int len = strlen(dirname);
    (*new) = apr_pcalloc(cont, sizeof(apr_dir_t));
    /* Leave room here to add and pop the '*' wildcard for FindFirstFile 
     * and double-null terminate so we have one character to change.
     */
    (*new)->dirname = apr_palloc(cont, len + 3);
    memcpy((*new)->dirname, dirname, len);
    if (len && (*new)->dirname[len - 1] != '/') {
    	(*new)->dirname[len++] = '/';
    }
    (*new)->dirname[len++] = '\0';
    (*new)->dirname[len] = '\0';

#if APR_HAS_UNICODE_FS
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT)
    {
        /* Create a buffer for the longest file name we will ever see 
         */
        (*new)->w.entry = apr_pcalloc(cont, sizeof(WIN32_FIND_DATAW));
        (*new)->name = apr_pcalloc(cont, APR_FILE_MAX * 3 + 1);        
    }
    else
#endif
    {
        /* Note that we won't open a directory that is greater than MAX_PATH,
         * including the trailing /* wildcard suffix.  If a * won't fit, then
         * neither will any other file name within the directory.
         * The length not including the trailing '*' is stored as rootlen, to
         * skip over all paths which are too long.
         */
        if (len >= APR_PATH_MAX) {
            (*new) = NULL;
            return APR_ENAMETOOLONG;
        }
        (*new)->n.entry = apr_pcalloc(cont, sizeof(WIN32_FIND_DATAW));
    }
    (*new)->rootlen = len - 1;
    (*new)->cntxt = cont;
    (*new)->dirhand = INVALID_HANDLE_VALUE;
    apr_pool_cleanup_register((*new)->cntxt, (void *)(*new), dir_cleanup,
                        apr_pool_cleanup_null);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_dir_close(apr_dir_t *dir)
{
    apr_pool_cleanup_kill(dir->cntxt, dir, dir_cleanup);
    return dir_cleanup(dir);
}

APR_DECLARE(apr_status_t) apr_dir_read(apr_finfo_t *finfo, apr_int32_t wanted,
                                       apr_dir_t *thedir)
{
    apr_status_t rv;
    char *fname;
    /* The while loops below allow us to skip all invalid file names, so that
     * we aren't reporting any files where their absolute paths are too long.
     */
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    apr_wchar_t wdirname[APR_PATH_MAX];
    apr_wchar_t *eos = NULL;
    if (!apr_get_oslevel(thedir->cntxt, &os_level) && os_level >= APR_WIN_NT)
    {
        if (thedir->dirhand == INVALID_HANDLE_VALUE) 
        {
            apr_status_t rv;
            if (rv = utf8_to_unicode_path(wdirname, sizeof(wdirname) 
                                                     / sizeof(apr_wchar_t), 
                                          thedir->dirname)) {
                return rv;
            }
            eos = wcschr(wdirname, '\0');
            eos[0] = '*';
            eos[1] = '\0';
            thedir->dirhand = FindFirstFileW(wdirname, thedir->w.entry);
            eos[0] = '\0';
            if (thedir->dirhand == INVALID_HANDLE_VALUE) {
                return apr_get_os_error();
            }
        }
        else if (!FindNextFileW(thedir->dirhand, thedir->w.entry)) {
            return apr_get_os_error();
        }
        while (thedir->rootlen &&
               thedir->rootlen + wcslen(thedir->w.entry->cFileName) >= APR_PATH_MAX)
        {
            if (!FindNextFileW(thedir->dirhand, thedir->w.entry)) {
                return apr_get_os_error();
            }
        }
        if (rv = unicode_to_utf8_path(thedir->name, APR_FILE_MAX * 3 + 1, 
                                      thedir->w.entry->cFileName))
            return rv;
        fname = thedir->name;
    }
    else
#endif
    {
        char *eop = strchr(thedir->dirname, '\0');
        if (thedir->dirhand == INVALID_HANDLE_VALUE) {
            /* '/' terminated, so add the '*' and pop it when we finish */
            eop[0] = '*';
            eop[1] = '\0';
            thedir->dirhand = FindFirstFileA(thedir->dirname, 
                                             thedir->n.entry);
            eop[0] = '\0';
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
        fname = thedir->n.entry->cFileName;
    }

    fillin_fileinfo(finfo, (WIN32_FILE_ATTRIBUTE_DATA *) thedir->w.entry, 
                    0, wanted);
    finfo->cntxt = thedir->cntxt;

    finfo->valid |= APR_FINFO_NAME;
    finfo->name = fname;

    if (wanted &= ~finfo->valid) {
        /* Go back and get more_info if we can't answer the whole inquiry
         */
#if APR_HAS_UNICODE_FS
        if (os_level >= APR_WIN_NT) {
            /* Almost all our work is done.  Tack on the wide file name
             * to the end of the wdirname (already / delimited)
             */
            if (!eos)
                eos = wcschr(wdirname, '\0');
            wcscpy(eos, thedir->w.entry->cFileName);
            rv = more_finfo(finfo, wdirname, wanted, MORE_OF_WFSPEC, os_level);
            eos[0] = '\0';
            return rv;
        }
        else {
            /* Don't waste stack space on a second buffer, the one we set
             * aside for the wide directory name is twice what we need.
             */
            char *fspec = (char*)wdirname;
#else /* !APR_HAS_UNICODE_FS */
        {
            char fspec[APR_PATH_MAX];
#endif
            int dirlen = strlen(thedir->dirname);
            if (dirlen >= sizeof(fspec))
                dirlen = sizeof(fspec) - 1;
            apr_cpystrn(fspec, thedir->dirname, sizeof(fspec));
            apr_cpystrn(fspec + dirlen, fname, sizeof(fspec) - dirlen);
            return more_finfo(finfo, fspec, wanted, MORE_OF_FSPEC, os_level);
        }
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_dir_rewind(apr_dir_t *dir)
{
    /* this will mark the handle as invalid and we'll open it
     * again if apr_dir_read() is subsequently called
     */
    return dir_cleanup(dir);
}

APR_DECLARE(apr_status_t) apr_dir_make(const char *path, apr_fileperms_t perm,
                                       apr_pool_t *cont)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT) 
    {
        apr_wchar_t wpath[APR_PATH_MAX];
        apr_status_t rv;
        if (rv = utf8_to_unicode_path(wpath, sizeof(wpath) 
                                              / sizeof(apr_wchar_t), path)) {
            return rv;
        }
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

APR_DECLARE(apr_status_t) apr_dir_remove(const char *path, apr_pool_t *cont)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT) 
    {
        apr_wchar_t wpath[APR_PATH_MAX];
        apr_status_t rv;
        if (rv = utf8_to_unicode_path(wpath, sizeof(wpath) 
                                              / sizeof(apr_wchar_t), path)) {
            return rv;
        }
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

APR_DECLARE(apr_status_t) apr_os_dir_get(apr_os_dir_t **thedir,
                                         apr_dir_t *dir)
{
    if (dir == NULL) {
        return APR_ENODIR;
    }
    *thedir = dir->dirhand;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_dir_put(apr_dir_t **dir,
                                         apr_os_dir_t *thedir,
                                         apr_pool_t *cont)
{
    return APR_ENOTIMPL;
}
