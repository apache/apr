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
 * ap_status_t ap_get_filename(ap_file_t *, char **)
 *    return the file name of the current file.
 * arg 1) The currently open file.
 * arg 2) The path of the file.  
 */                     
ap_status_t ap_get_filename(struct file_t *thefile, char **new)
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
 * ap_status_t ap_get_filesize(ap_file_t *, ap_ssize_t *)
 *    Return the size of the current file.
 * arg 1) The currently open file.
 * arg 2) The size of the file.  
 */                     
ap_status_t ap_get_filesize(struct file_t *file, ap_ssize_t *size)
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
 * ap_status_t ap_get_fileperms(ap_file_t *, ap_fileperms_t *)
 *    Return the permissions of the current file.
 * arg 1) The currently open file.
 * arg 2) The permissions of the file.  
 */                     
ap_status_t ap_get_fileperms(struct file_t *file, ap_fileperms_t *perm)
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
 * ap_status_t ap_get_fileatime(ap_file_t *, time_t *)
 *    Return the last access time of the current file.
 * arg 1) The currently open file.
 * arg 2) The last access time of the file.  
 */                     
ap_status_t ap_get_fileatime(struct file_t *file, time_t *time)
{    
    if (file != NULL) {
        if (!file->stated) {
            ap_getfileinfo(file);
        }
        *time = file->atime;
        return APR_SUCCESS;
    }
    else {
        *time = -1;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_filectime(ap_file_t *, time_t *)
 *    Return the time of the last change to the current file.
 * arg 1) The currently open file.
 * arg 2) The last change time of the file.  
 */                     
ap_status_t ap_get_filectime(struct file_t *file, time_t *time)
{    
    if (file != NULL) {
        if (!file->stated) {
            ap_getfileinfo(file);
        }
        *time = file->ctime;
        return APR_SUCCESS;
    }
    else {
        *time = -1;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_filemtime(ap_file_t *, time_t *)
 *    Return the last modified time of the current file.
 * arg 1) The currently open file.
 * arg 2) The last modified time of the file.  
 */                     
ap_status_t ap_get_filemtime(struct file_t *file, time_t *time)
{    
    if (file != NULL) {
        if (!file->stated) {
            ap_getfileinfo(file);
        }
        *time = file->mtime;
        return APR_SUCCESS;
    }
    else {
        *time = -1;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_filedata(ap_file_t *, void *)
 *    Return the data associated with the current file.
 * arg 1) The currently open file.
 * arg 2) The user data associated with the file.  
 */                     
ap_status_t ap_get_filedata(struct file_t *file, void *data)
{    
    if (file != NULL) {
        return ap_get_userdata(file->cntxt, &data);
    }
    else {
        data = NULL;
        return APR_ENOFILE;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_filedata(ap_file_t *, void *)
 *    Set the data associated with the current file.
 * arg 1) The currently open file.
 * arg 2) The user data to associate with the file.  
 */                     
ap_status_t ap_set_filedata(struct file_t *file, void *data)
{    
    if (file != NULL) {
        return ap_set_userdata(file->cntxt, data);
    }
    else {
        data = NULL;
        return APR_ENOFILE;
    }
}

