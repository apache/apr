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
#ifdef WIN32
#include "apr_win.h"
#endif
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

ap_status_t file_cleanup(void *thefile)
{
    struct file_t *file = thefile;
    if (!CloseHandle(file->filehand)) {
        return GetLastError();
    }
    file->filehand = INVALID_HANDLE_VALUE;
    return APR_SUCCESS;
}

ap_status_t ap_open(struct file_t **dafile, const char *fname, 
                    ap_int32_t flag, ap_fileperms_t perm, ap_context_t *cont)
{
    DWORD oflags = 0;
    DWORD createflags = 0;
    DWORD theerror;
    /*mode_t mode = get_fileperms(perm);*/

    (*dafile) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));

    (*dafile)->cntxt = cont;

    if (flag & APR_READ) {
        oflags |= GENERIC_READ;
    }
    if (flag & APR_WRITE) {
        oflags |= GENERIC_WRITE;
    }
    if (!(flag & APR_READ) && !(flag & APR_WRITE)) {
        (*dafile)->filehand = INVALID_HANDLE_VALUE;
        return APR_EACCES;
    }

    if (flag & APR_BUFFERED) {
        (*dafile)->buffered = TRUE;
    } else {
        (*dafile)->buffered = FALSE;
    }
    (*dafile)->fname = strdup(fname);

    (*dafile)->demonfname = canonical_filename((*dafile)->cntxt, fname);
    (*dafile)->lowerdemonfname = strlwr((*dafile)->demonfname);
    
    createflags = OPEN_ALWAYS;     
    if (flag & APR_CREATE) {
        if (flag & APR_EXCL) {
            createflags = CREATE_NEW;
        }
    }
    if ((flag & APR_EXCL) && !(flag & APR_CREATE)) {
        (*dafile)->filehand = INVALID_HANDLE_VALUE;
        return APR_EACCES;
    }   

    if (flag & APR_APPEND) {
        (*dafile)->append = 1;
    }
    else {
        (*dafile)->append = 0;
    }

    if (flag & APR_TRUNCATE) {
        createflags = TRUNCATE_EXISTING;
    }
 
    (*dafile)->filehand = CreateFile(fname, oflags, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     NULL, createflags, FILE_ATTRIBUTE_NORMAL, 0);

    if ((*dafile)->filehand == INVALID_HANDLE_VALUE) {
        theerror = GetLastError();
        return APR_EEXIST;
    }
    (*dafile)->stated = 0;  /* we haven't called stat for this file yet. */
    (*dafile)->eof_hit = 0;
    ap_register_cleanup((*dafile)->cntxt, (void *)(*dafile), file_cleanup,
                        ap_null_cleanup);
    return APR_SUCCESS;
}

ap_status_t ap_close(struct file_t *file)
{
    ap_status_t stat;
    if ((stat = file_cleanup(file)) == APR_SUCCESS) {
        ap_kill_cleanup(file->cntxt, file, file_cleanup);
        return APR_SUCCESS;
    }
    return stat;
}

ap_status_t ap_remove_file(char *path, ap_context_t *cont)
{
    char *temp = canonical_filename(cont, path);

    if (DeleteFile(temp)) {
        return APR_SUCCESS;
    }
    else {
        return APR_EEXIST;
    }
}

ap_status_t ap_get_os_file(ap_os_file_t *thefile, struct file_t *file)
{
    if (file == NULL) {
        return APR_ENOFILE;
    }
    *thefile = file->filehand;
    return APR_SUCCESS;
}

ap_status_t ap_put_os_file(struct file_t **file, ap_os_file_t *thefile, 
                           ap_context_t *cont)
{
    if ((*file) == NULL) {
        if (cont == NULL) {
            return APR_ENOCONT;
        }
        (*file) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));
        (*file)->cntxt = cont;
    }
    (*file)->filehand = *thefile;
    return APR_SUCCESS;
}    

ap_status_t ap_eof(ap_file_t *fptr)
{
    if (fptr->eof_hit == 1) {
        return APR_EOF;
    }
    APR_SUCCESS;
}   
