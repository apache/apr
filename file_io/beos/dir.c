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

#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_lib.h"

ap_status_t dir_cleanup(void *thedir)
{
	struct dir_t *dir = thedir;
	if (closedir(dir->dirstruct) ==0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
} 

ap_status_t ap_opendir(ap_context_t *cont, const char *dirname, ap_dir_t ** new)
{
    (*new) = (struct dir_t *)ap_palloc(cont,sizeof(struct dir_t));

    (*new)->cntxt = cont;
    (*new)->dirname = strdup(dirname);
    (*new)->dirstruct = opendir(dirname);
    (*new)->entry = NULL;

    if ((*new)->dirstruct == NULL) {
        (*new)->dirstruct = NULL;
        return errno;
    }    
    else {
    	ap_register_cleanup((*new)->cntxt, (void*)(*new), dir_cleanup, NULL);
        return APR_SUCCESS;
    }
}

ap_status_t ap_closedir(struct dir_t *thedir)
{
    if (dir_cleanup(thedir) == APR_SUCCESS) {
        ap_kill_cleanup(thedir->cntxt, thedir, dir_cleanup);
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
} 

ap_status_t ap_readdir(struct dir_t *thedir)
{
    thedir->entry = readdir(thedir->dirstruct);
 	if (thedir->entry == NULL){
 		return errno;
    }
    return APR_SUCCESS;
}

ap_status_t ap_rewinddir(struct dir_t *thedir)
{
    rewinddir(thedir->dirstruct);
    return APR_SUCCESS;
}

ap_status_t ap_make_dir(ap_context_t *cont, const char *path, ap_fileperms_t mode)
{
    if (mkdir(path, mode) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

ap_status_t ap_remove_dir(ap_context_t *cont, const char *path)
{
    if (rmdir(path) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

ap_status_t ap_dir_entry_size(struct dir_t *thedir, ap_ssize_t *size)
{
    struct stat filestat;
    char *fname = NULL;    

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

ap_status_t ap_dir_entry_mtime(struct dir_t *thedir, time_t *time)
{
    struct stat filestat;
    char *fname = NULL;

    if (thedir->entry == NULL) {
        *time = -1;
        return APR_ENOFILE;
    }

    fname = ap_pstrcat(thedir->cntxt, thedir->dirname, "/", 
                       thedir->entry->d_name, NULL);
    if (stat(fname, &filestat) == -1) {
        *time = -1;
        return APR_ENOSTAT;
    }
    
    *time = filestat.st_mtime;
    return APR_SUCCESS;
}
 
ap_status_t ap_dir_entry_ftype(struct dir_t *thedir, ap_filetype_e *type)
{
    struct stat filestat;
    char *fname = NULL;

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
    /*if (S_ISSOCK(filestat.st_mode))
        *type = APR_SOCK; */   
    return APR_SUCCESS;
}

ap_status_t ap_get_dir_filename(struct dir_t *thedir, char **new)
{
    (*new) = ap_pstrdup(thedir->cntxt, thedir->entry->d_name);
    return APR_SUCCESS;
}

