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
#include "apr_portable.h"

static ap_status_t dir_cleanup(void *thedir)
{
    ap_dir_t *dir = thedir;
    if (closedir(dir->dirstruct) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
} 

ap_status_t ap_opendir(ap_dir_t **new, const char *dirname, ap_context_t *cont)
{
    if (new == NULL)
        return APR_EBADARG;

    (*new) = (ap_dir_t *)ap_palloc(cont, sizeof(ap_dir_t));

    (*new)->cntxt = cont;
    (*new)->dirname = ap_pstrdup(cont, dirname);
    (*new)->dirstruct = opendir(dirname);
    (*new)->entry = ap_pcalloc(cont, sizeof(struct dirent));

    if ((*new)->dirstruct == NULL) {
        return errno;
    }    
    else {
        ap_register_cleanup((*new)->cntxt, (void *)(*new), dir_cleanup,
	                    ap_null_cleanup);
        return APR_SUCCESS;
    }
}

ap_status_t ap_closedir(ap_dir_t *thedir)
{
    ap_status_t rv;

    if (thedir == NULL)
        return APR_EBADARG;

    if ((rv = dir_cleanup(thedir)) == APR_SUCCESS) {
        ap_kill_cleanup(thedir->cntxt, thedir, dir_cleanup);
        return APR_SUCCESS;
    }
    return rv;
}

ap_status_t ap_readdir(ap_dir_t *thedir)
{
#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS) \
    && !defined(READDIR_IS_THREAD_SAFE)
    ap_status_t ret;
#endif

    if (thedir == NULL)
        return APR_EBADARG;

#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS) \
    && !defined(READDIR_IS_THREAD_SAFE)

    ret = readdir_r(thedir->dirstruct, thedir->entry, &thedir->entry);
    /* Avoid the Linux problem where at end-of-directory thedir->entry
     * is set to NULL, but ret = APR_SUCCESS.
     */
    return (ret == APR_SUCCESS && thedir->entry == NULL) ? APR_ENOENT : ret;
#else

    thedir->entry = readdir(thedir->dirstruct);
    if (thedir->entry == NULL) {
        /* If NULL was returned, this can NEVER be a success. Can it?! */
        if (errno == APR_SUCCESS) {
            return APR_ENOENT;
        }
        return errno;
    }
    return APR_SUCCESS;
#endif
}

ap_status_t ap_rewinddir(ap_dir_t *thedir)
{
    if (thedir == NULL)
        return APR_EBADARG;

    rewinddir(thedir->dirstruct);
    return APR_SUCCESS;
}

ap_status_t ap_make_dir(const char *path, ap_fileperms_t perm, ap_context_t *cont)
{
    mode_t mode = ap_unix_get_fileperms(perm);

    if (mkdir(path, mode) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

ap_status_t ap_remove_dir(const char *path, ap_context_t *cont)
{
    if (rmdir(path) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

ap_status_t ap_dir_entry_size(ap_ssize_t *size, ap_dir_t *thedir)
{
    struct stat filestat;
    char *fname = NULL;    

    if (size == NULL || thedir == NULL)
        return APR_EBADARG;

    if (thedir->entry == NULL) {
        *size = -1;
        return APR_ENOFILE;
    }
    fname = ap_pstrcat(thedir->cntxt, thedir->dirname, "/", 
                       thedir->entry->d_name, NULL);
    if (stat(fname, &filestat) == -1) {
        *size = -1;
        return APR_ENOSTAT;
    }
    
    *size = filestat.st_size;
    return APR_SUCCESS;
}

ap_status_t ap_dir_entry_mtime(ap_time_t *mtime, ap_dir_t *thedir)
{
    struct stat filestat;
    char *fname = NULL;

    if (mtime == NULL || thedir == NULL)
        return APR_EBADARG;

    if (thedir->entry == NULL) {
        *mtime = -1;
        return APR_ENOFILE;
    }

    fname = ap_pstrcat(thedir->cntxt, thedir->dirname, "/", 
                       thedir->entry->d_name, NULL);
    if (stat(fname, &filestat) == -1) {
        *mtime = -1;
        return APR_ENOSTAT;
    }
    
    ap_ansi_time_to_ap_time(mtime, filestat.st_mtime);
    return APR_SUCCESS;
}
 
ap_status_t ap_dir_entry_ftype(ap_filetype_e *type, ap_dir_t *thedir)
{
    struct stat filestat;
    char *fname = NULL;

    if (type == NULL || thedir == NULL)
        return APR_EBADARG;

    if (thedir->entry == NULL) {
        *type = APR_REG;
        return APR_ENOFILE;
    }

    fname = ap_pstrcat(thedir->cntxt, thedir->dirname, "/", 
                       thedir->entry->d_name, NULL);
    if (stat(fname, &filestat) == -1) {
        *type = APR_REG;
        return APR_ENOSTAT;
    }

    if (S_ISREG(filestat.st_mode))
        *type = APR_REG;    
    if (S_ISDIR(filestat.st_mode))
        *type = APR_DIR;    
    if (S_ISCHR(filestat.st_mode))
        *type = APR_CHR;    
    if (S_ISBLK(filestat.st_mode))
        *type = APR_BLK;    
    if (S_ISFIFO(filestat.st_mode))
        *type = APR_PIPE;    
    if (S_ISLNK(filestat.st_mode))
        *type = APR_LNK;    
#ifndef BEOS
    if (S_ISSOCK(filestat.st_mode))
        *type = APR_SOCK;    
#endif
    return APR_SUCCESS;
}

ap_status_t ap_get_dir_filename(char **new, ap_dir_t *thedir)
{
    if (new == NULL)
        return APR_EBADARG;

    /* Detect End-Of-File */
    if (thedir == NULL || thedir->entry == NULL) {
        *new = NULL;
        return APR_ENOENT;
    }
    (*new) = ap_pstrdup(thedir->cntxt, thedir->entry->d_name);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_dir(ap_os_dir_t **thedir, ap_dir_t *dir)
 *    convert the dir from apr type to os specific type.
 * arg 1) The os specific dir we are converting to
 * arg 2) The apr dir to convert.
 */   
ap_status_t ap_get_os_dir(ap_os_dir_t **thedir, ap_dir_t *dir)
{
    if (dir == NULL) {
        return APR_ENODIR;
    }
    *thedir = dir->dirstruct;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_dir(ap_dir_t **dir, ap_os_dir_t *thedir, 
 *                           ap_context_t *cont)
 *    convert the dir from os specific type to apr type.
 * arg 1) The apr dir we are converting to.
 * arg 2) The os specific dir to convert
 * arg 3) The context to use when creating to apr directory.
 */
ap_status_t ap_put_os_dir(ap_dir_t **dir, ap_os_dir_t *thedir,
                          ap_context_t *cont)
{
    if ((*dir) == NULL) {
        (*dir) = (ap_dir_t *)ap_palloc(cont, sizeof(ap_dir_t));
        (*dir)->cntxt = cont;
    }
    (*dir)->dirstruct = thedir;
    return APR_SUCCESS;
}

  
