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
#include "fileio.h"
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
    if (!CloseHandle(dir->dirhand)) {
        return apr_get_os_error();
    }
    return APR_SUCCESS;
} 

apr_status_t apr_opendir(apr_dir_t **new, const char *dirname, apr_pool_t *cont)
{
    (*new) = apr_pcalloc(cont, sizeof(apr_dir_t));
    (*new)->cntxt = cont;
    (*new)->entry = NULL;
    if (dirname[strlen(dirname)] == '/') {
    	(*new)->dirname = apr_pstrcat(cont, dirname, "*", NULL);
    }
    else {
        (*new)->dirname = apr_pstrcat(cont, dirname, "/*", NULL);
    }
    (*new)->dirhand = INVALID_HANDLE_VALUE;
    apr_register_cleanup((*new)->cntxt, (void *)(*new), dir_cleanup,
                        apr_null_cleanup);
    return APR_SUCCESS;
}

apr_status_t apr_closedir(apr_dir_t *thedir)
{
    if (!FindClose(thedir->dirhand)) {
        return apr_get_os_error();   
    }
    apr_kill_cleanup(thedir->cntxt, thedir, dir_cleanup);
    return APR_SUCCESS;
}

apr_status_t apr_readdir(apr_dir_t *thedir)
{
    if (thedir->dirhand == INVALID_HANDLE_VALUE) {
        thedir->entry = apr_pcalloc(thedir->cntxt, sizeof(WIN32_FIND_DATA));
        thedir->dirhand = FindFirstFile(thedir->dirname, thedir->entry);
        if (thedir->dirhand == INVALID_HANDLE_VALUE) {
            return apr_get_os_error();
        }
        return APR_SUCCESS;
    }
    if (!FindNextFile(thedir->dirhand, thedir->entry)) {
        return apr_get_os_error();
    }
    return APR_SUCCESS;
}

apr_status_t apr_rewinddir(apr_dir_t *thedir)
{
    apr_status_t stat;
    apr_pool_t *cont = thedir->cntxt;
    char *temp = apr_pstrdup(cont, thedir->dirname);
    temp[strlen(temp) - 2] = '\0';   /*remove the \* at the end */
    if (thedir->dirhand == INVALID_HANDLE_VALUE) {
        return APR_SUCCESS;
    }
    if ((stat = apr_closedir(thedir)) == APR_SUCCESS) {
        if ((stat = apr_opendir(&thedir, temp, cont)) == APR_SUCCESS) {
            apr_readdir(thedir);
            return APR_SUCCESS;
        }
    }
    return stat;	
}

apr_status_t apr_make_dir(const char *path, apr_fileperms_t perm, apr_pool_t *cont)
{
    if (!CreateDirectory(path, NULL)) {
        return apr_get_os_error();
    }
    return APR_SUCCESS;
}

apr_status_t apr_remove_dir(const char *path, apr_pool_t *cont)
{
    if (!RemoveDirectory(path)) {
        return apr_get_os_error();
    }
    return APR_SUCCESS;
}

apr_status_t apr_dir_entry_size(apr_ssize_t *size, apr_dir_t *thedir)
{
    if (thedir == NULL || thedir->entry == NULL) {
        return APR_ENODIR;
    }
    (*size) = (thedir->entry->nFileSizeHigh * MAXDWORD) + 
        thedir->entry->nFileSizeLow;
    return APR_SUCCESS;
}

apr_status_t apr_dir_entry_mtime(apr_time_t *time, apr_dir_t *thedir)
{
    if (thedir == NULL || thedir->entry == NULL) {
        return APR_ENODIR;
    }
    FileTimeToAprTime(time, &thedir->entry->ftLastWriteTime);
    return APR_SUCCESS;
}
 
apr_status_t apr_dir_entry_ftype(apr_filetype_e *type, apr_dir_t *thedir)
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

apr_status_t apr_get_dir_filename(char **new, apr_dir_t *thedir)
{
    (*new) = apr_pstrdup(thedir->cntxt, thedir->entry->cFileName);
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

apr_status_t apr_put_os_dir(apr_dir_t **dir, apr_os_dir_t *thedir, apr_pool_t *cont)
{
    if (cont == NULL) {
        return APR_ENOPOOL;
    }
    if ((*dir) == NULL) {
        (*dir) = (apr_dir_t *)apr_pcalloc(cont, sizeof(apr_dir_t));
        (*dir)->cntxt = cont;
    }
    (*dir)->dirhand = thedir;
    return APR_SUCCESS;
}
