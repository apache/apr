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

#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <errno.h>
#include <string.h>
#include <sys/types.h>

/* A file to put ALL of the accessor functions for struct file_t types. */

/* ***APRDOC********************************************************
 * ap_status_t ap_get_filename(char **, ap_file_t *)
 *    return the file name of the current file.
 * arg 1) The currently open file.
 * arg 2) The path of the file.  
 */                     
ap_status_t ap_get_filename(char **new, struct file_t *thefile)
{
    if (thefile != NULL) {
        *new = ap_pstrdup(thefile->cntxt, thefile->fname);
        return APR_SUCCESS;
    }
    else {
        *new = NULL;
        return APR_ENOFILE;
    }
}

mode_t get_fileperms(ap_fileperms_t mode)
{
    mode_t rv = 0;

    if (mode & APR_UREAD)
        rv |= S_IRUSR;
    if (mode & APR_UWRITE)
        rv |= S_IWUSR;
    if (mode & APR_UEXECUTE)
        rv |= S_IXUSR;

    if (mode & APR_GREAD)
        rv |= S_IRGRP;
    if (mode & APR_GWRITE)
        rv |= S_IWGRP;
    if (mode & APR_GEXECUTE)
        rv |= S_IXGRP;

    if (mode & APR_WREAD)
        rv |= S_IROTH;
    if (mode & APR_WWRITE)
        rv |= S_IWOTH;
    if (mode & APR_WEXECUTE)
        rv |= S_IXOTH;

    return rv;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_filesize(ap_ssize_t *, ap_file_t *)
 *    Return the size of the current file.
 * arg 1) The currently open file.
 * arg 2) The size of the file.  
 */                     
ap_status_t ap_get_filesize(ap_ssize_t *size, struct file_t *file)
{
    if (file != NULL) {
        if (!file->stated) {
            ap_getfileinfo(file);
        }
        *size = file->size;
        return APR_SUCCESS;
    }
    else {
        *size = -1;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_fileperms(ap_fileperms_t *, ap_file_t *)
 *    Return the permissions of the current file.
 * arg 1) The currently open file.
 * arg 2) The permissions of the file.  
 */                     
ap_status_t ap_get_fileperms(ap_fileperms_t *perm, struct file_t *file)
{
    if (file != NULL) {
        if (!file->stated) {
            ap_getfileinfo(file);
        }
        *perm = file->protection;
        return APR_SUCCESS;
    }
    else {
        *perm = -1;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_fileatime(time_t *, ap_file_t *)
 *    Return the last access time of the current file.
 * arg 1) The currently open file.
 * arg 2) The last access time of the file.  
 */                     
ap_status_t ap_get_fileatime(time_t *atime, struct file_t *file)
{    
    if (file != NULL) {
        if (!file->stated) {
            ap_getfileinfo(file);
        }
        *atime = file->atime;
        return APR_SUCCESS;
    }
    else {
        *atime = -1;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_filectime(time_t *, ap_file_t *)
 *    Return the time of the last change to the current file.
 * arg 1) The currently open file.
 * arg 2) The last change time of the file.  
 */                     
ap_status_t ap_get_filectime(time_t *ptime, struct file_t *file)
{    
    if (file != NULL) {
        if (!file->stated) {
            ap_getfileinfo(file);
        }
        *ptime = file->ctime;
        return APR_SUCCESS;
    }
    else {
        *ptime = -1;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_filemtime(time_t *, ap_file_t *)
 *    Return the last modified time of the current file.
 * arg 1) The currently open file.
 * arg 2) The last modified time of the file.  
 */                     
ap_status_t ap_get_filemtime(time_t *mtime, struct file_t *file)
{    
    if (file != NULL) {
        if (!file->stated) {
            ap_getfileinfo(file);
        }
        *mtime = file->mtime;
        return APR_SUCCESS;
    }
    else {
        *mtime = -1;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_filetype(ap_filetype_e, ap_file_t *)
 *    Return the type of the current file.
 * arg 1) The currently open file.
 * arg 2) The file type
 */                     
ap_status_t ap_get_filetype(ap_filetype_e *type, struct file_t *file)
{    
    if (file != NULL) {
        if (!file->stated) {
            ap_getfileinfo(file);
        }
        if (S_ISREG(file->protection))
            *type = APR_REG;
        if (S_ISDIR(file->protection))
            *type = APR_DIR;
        if (S_ISCHR(file->protection))
            *type = APR_CHR;
        if (S_ISBLK(file->protection))
            *type = APR_BLK;
        if (S_ISFIFO(file->protection))
            *type = APR_PIPE;
        if (S_ISLNK(file->protection))
            *type = APR_LNK;
#ifndef BEOS
        if (S_ISSOCK(file->protection))
            *type = APR_SOCK;
#endif
        return APR_SUCCESS;
    }
    else {
        *type = APR_REG;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_filedata(void *, char *key, ap_file_t *)
 *    Return the data associated with the current file.
 * arg 1) The currently open file.
 * arg 2) The key to use for retreiving data associated with this file.
 * arg 3) The user data associated with the file.  
 */                     
ap_status_t ap_get_filedata(void *data, char *key, struct file_t *file)
{    
    if (file != NULL) {
        return ap_get_userdata(&data, file->cntxt, key);
    }
    else {
        data = NULL;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_filedata(ap_file_t *, void *, char *key,
                               ap_status (*cleanup) (void *))
 *    Set the data associated with the current file.
 * arg 1) The currently open file.
 * arg 2) The user data to associate with the file.  
 * arg 3) The key to use for assocaiteing data with the file.
 * arg 4) The cleanup routine to use when the file is destroyed.
 */                     
ap_status_t ap_set_filedata(struct file_t *file, void *data, char *key,
                            ap_status_t (*cleanup) (void *))
{    
    if (file != NULL) {
        return ap_set_userdata(file->cntxt, data, key, cleanup);
    }
    else {
        data = NULL;
        return APR_ENOFILE;
    }
}

