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
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef WIN32
#include "apr_winconfig.h"
#include <windows.h>
#endif
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_lib.h"
#include "apr_portable.h"

ap_status_t dir_cleanup(void *thedir)
{
    struct dir_t *dir = thedir;
    if (!CloseHandle(dir->dirhand)) {
        return GetLastError();
    }
    return APR_SUCCESS;
} 

ap_status_t ap_opendir(struct dir_t **new, const char *dirname, ap_context_t *cont)
{
    char * temp;
    (*new) = ap_palloc(cont, sizeof(struct dir_t));
    (*new)->cntxt = cont;
    (*new)->entry = NULL;
    temp = canonical_filename((*new)->cntxt, dirname);
    if (temp[strlen(temp)] == '/') {
    	(*new)->dirname = ap_pstrcat(cont, dirname, "*", NULL);
    }
    else {
        (*new)->dirname = ap_pstrcat(cont, dirname, "/*", NULL);
    }
    (*new)->dirhand = INVALID_HANDLE_VALUE;
    ap_register_cleanup((*new)->cntxt, (void *)(*new), dir_cleanup,
                        ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_closedir(struct dir_t *thedir)
{
    if (!FindClose(thedir->dirhand)) {
        return GetLastError();   
    }
    ap_kill_cleanup(thedir->cntxt, thedir, dir_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_readdir(struct dir_t *thedir)
{
    if (thedir->dirhand == INVALID_HANDLE_VALUE) {
        thedir->entry = ap_palloc(thedir->cntxt, sizeof(WIN32_FIND_DATA));
        thedir->dirhand = FindFirstFile(thedir->dirname, thedir->entry);
        if (thedir->dirhand == INVALID_HANDLE_VALUE) {
            return GetLastError();
        }
        return APR_SUCCESS;
    }
    if (!FindNextFile(thedir->dirhand, thedir->entry)) {
        return GetLastError();
    }
    return APR_SUCCESS;
}

ap_status_t ap_rewinddir(struct dir_t *thedir)
{
    ap_status_t stat;
    ap_context_t *cont = thedir->cntxt;
    char *temp = ap_pstrdup(cont, thedir->dirname);
    temp[strlen(temp) - 2] = '\0';   /*remove the \* at the end */
    if (thedir->dirhand == INVALID_HANDLE_VALUE) {
        return APR_SUCCESS;
    }
    if ((stat = ap_closedir(thedir)) == APR_SUCCESS) {
        if ((stat = ap_opendir(&thedir, temp, cont)) == APR_SUCCESS) {
            ap_readdir(thedir);
            return APR_SUCCESS;
        }
    }
    return stat;	
}

ap_status_t ap_make_dir(const char *path, ap_fileperms_t perm, ap_context_t *cont)
{
    if (!CreateDirectory(path, NULL)) {
        return GetLastError();
    }
    return APR_SUCCESS;
}

ap_status_t ap_remove_dir(const char *path, ap_context_t *cont)
{
    char *temp = canonical_filename(cont, path);
    if (!RemoveDirectory(temp)) {
        return GetLastError();
    }
    return APR_SUCCESS;
}

ap_status_t ap_dir_entry_size(ap_ssize_t *size, struct dir_t *thedir)
{
    if (thedir == NULL || thedir->entry == NULL) {
        return APR_ENODIR;
    }
    (*size) = (thedir->entry->nFileSizeHigh * MAXDWORD) + 
        thedir->entry->nFileSizeLow;
    return APR_SUCCESS;
}

ap_status_t ap_dir_entry_mtime(time_t *time, struct dir_t *thedir)
{
    if (thedir == NULL || thedir->entry == NULL) {
        return APR_ENODIR;
    }
    
    *time = WinTimeToUnixTime(&thedir->entry->ftLastWriteTime);
 
    return APR_SUCCESS;
}
 
ap_status_t ap_dir_entry_ftype(ap_filetype_e *type, struct dir_t *thedir)
{
    switch(thedir->entry->dwFileAttributes) {
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

ap_status_t ap_get_dir_filename(char **new, struct dir_t *thedir)
{
    (*new) = ap_pstrdup(thedir->cntxt, thedir->entry->cFileName);
    return APR_SUCCESS;
}

ap_status_t ap_get_os_dir(ap_os_dir_t **thedir, struct dir_t *dir)
{
    if (dir == NULL) {
        return APR_ENODIR;
    }
    *thedir = dir->dirhand;
    return APR_SUCCESS;
}

ap_status_t ap_put_os_dir(struct dir_t **dir, ap_os_dir_t *thedir, ap_context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if ((*dir) == NULL) {
        (*dir) = (struct dir_t *)ap_palloc(cont, sizeof(struct dir_t));
        (*dir)->cntxt = cont;
    }
    (*dir)->dirhand = thedir;
    return APR_SUCCESS;
}
