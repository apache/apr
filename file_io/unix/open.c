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
#include "apr_portable.h"
#include "apr_lib.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>

ap_status_t file_cleanup(void *thefile)
{
    struct file_t *file = thefile;
    int rv;
    if (file->buffered) {
        rv = fclose(file->filehand);
    }
    else {
        rv = close(file->filedes);
    }

    if (rv == 0) {
        file->filedes = -1;
        file->filehand = NULL;
        return APR_SUCCESS;
    }
    else {
        return errno;
	/* Are there any error conditions other than EINTR or EBADF? */
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_open(ap_context_t *, char *, ap_int32, 
 *                     ap_fileperms, ap_file_t **)
 *    Open the specified file.
 * arg 1) The context to use.
 * arg 2) The full path to the file (using / on all systems)
 * arg 3) Or'ed value of:
 *          APR_READ             open for reading
 *          APR_WRITE            open for writing
 *          APR_CREATE           create the file if not there
 *          APR_APPEND           file ptr is set to end prior to all writes
 *          APR_TRUNCATE         set length to zero if file exists
 *          APR_BINARY           not a text file (This flag is ignored on UNIX because it has no meaning)
 *          APR_BUFFERED         buffer the data.  Default is non-buffered
 *          APR_EXCL             return error if APR_CREATE and file exists
 * arg 4) Access permissions for file.
 * arg 5) The opened file descriptor.
 * NOTE:  If mode is APR_OS_DEFAULT, the system open command will be 
 *        called without any mode parameters.
 */
ap_status_t ap_open(struct file_t **new, const char *fname, ap_int32_t flag,  ap_fileperms_t perm, ap_context_t *cont)
{
    int oflags = 0;
    char *buf_oflags;

    (*new) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));

    (*new)->cntxt = cont;
    (*new)->oflags = oflags;
    (*new)->filedes = -1;
    (*new)->filehand = NULL;

    if ((flag & APR_READ) && (flag & APR_WRITE)) {
        buf_oflags = ap_pstrdup(cont, "r+");
        oflags = O_RDWR;
    }
    else if (flag & APR_READ) {
        buf_oflags = ap_pstrdup(cont, "r");
        oflags = O_RDONLY;
    }
    else if (flag & APR_WRITE) {
        buf_oflags = ap_pstrdup(cont, "w");
        oflags = O_WRONLY;
    }
    else {
        return APR_EACCES; 
    }

    if (flag & APR_BUFFERED) {
        (*new)->buffered = TRUE;
    }
    else {
        (*new)->buffered = FALSE;
    }
    (*new)->fname = ap_pstrdup(cont, fname);

    if (flag & APR_CREATE) {
        oflags |= O_CREAT; 
	if (flag & APR_EXCL) {
	    oflags |= O_EXCL;
	}
    }
    if ((flag & APR_EXCL) && !(flag & APR_CREATE)) {
        return APR_EACCES;
    }   

    if (flag & APR_APPEND) {
        buf_oflags[0] = 'a';
        oflags |= O_APPEND;
    }
    if (flag & APR_TRUNCATE) {
        oflags |= O_TRUNC;
    }
    
    if ((*new)->buffered) {
        (*new)->filehand = fopen(fname, buf_oflags);
    }
    else { 
        if (perm == APR_OS_DEFAULT) {
            (*new)->filedes = open(fname, oflags);
        }
        else {
            (*new)->filedes = open(fname, oflags, get_fileperms(perm));
        }    
    }

    if ((*new)->filedes < 0 && (*new)->filehand == NULL) {
       (*new)->filedes = -1;
       (*new)->eof_hit = 1;
        return errno;
    }

    (*new)->stated = 0;  /* we haven't called stat for this file yet. */
    (*new)->eof_hit = 0;
    ap_register_cleanup((*new)->cntxt, (void *)(*new), file_cleanup,
                        ap_null_cleanup);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_close(ap_file_t *)
 *    Close the specified file.
 * arg 1) The file descriptor to close.
 */
ap_status_t ap_close(struct file_t *file)
{
    ap_status_t rv;
  
    if ((rv = file_cleanup(file)) == APR_SUCCESS) {
        ap_kill_cleanup(file->cntxt, file, file_cleanup);
        return APR_SUCCESS;
    }
    return rv;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_remove_file(ap_context_t *, char *) 
 *    delete the specified file.
 * arg 1) The context to use.
 * arg 2) The full path to the file (using / on all systems)
 * NOTE: If the file is open, it won't be removed until all instances are
 *       closed.
 */
ap_status_t ap_remove_file(char *path, ap_context_t *cont)
{
    if (unlink(path) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_file(ap_file_t *, ap_os_file_t *) 
 *    convert the file from apr type to os specific type.
 * arg 1) The apr file to convert.
 * arg 2) The os specific file we are converting to
 * NOTE:  On Unix, it is only possible to get a file descriptor from 
 *        an apr file type.
 */
ap_status_t ap_get_os_file(ap_os_file_t *thefile, struct file_t *file)
{
    if (file == NULL) {
        return APR_ENOFILE;
    }

    if (file->buffered) {
        *thefile = fileno(file->filehand);
    }
    else {
        *thefile = file->filedes;
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_file(ap_context_t *, ap_file_t *, ap_os_file_t *) 
 *    convert the file from os specific type to apr type.
 * arg 1) The context to use if it is needed.
 * arg 2) The apr file we are converting to.
 * arg 3) The os specific file to convert
 * NOTE:  On Unix, it is only possible to put a file descriptor into
 *        an apr file type.
 */
ap_status_t ap_put_os_file(struct file_t **file, ap_os_file_t *thefile,
                           ap_context_t *cont)
{
    int *dafile = thefile;
    if ((*file) == NULL) {
        (*file) = ap_pcalloc(cont, sizeof(struct file_t));
        (*file)->cntxt = cont;
    }
    (*file)->filedes = *dafile;
    return APR_SUCCESS;
}    

/* ***APRDOC********************************************************
 * ap_status_t ap_eof(ap_file_t *) 
 *    Are we at the end of the file
 * arg 1) The apr file we are testing.
 * NOTE:  Returns APR_EOF if we are at the end of file, APR_SUCCESS otherwise.
 */
ap_status_t ap_eof(ap_file_t *fptr)
{
    if (fptr->buffered) {
        if (feof(fptr->filehand) == 0) {
            return APR_SUCCESS;
        }
        return APR_EOF;
    }
    if (fptr->eof_hit == 1) {
        return APR_EOF;
    }
    return APR_SUCCESS;
}   

/* ***APRDOC********************************************************
 * ap_status_t ap_ferror(ap_file_t *) 
 *    Is there an error on the stream?
 * arg 1) The apr file we are testing.
 * NOTE:  Returns -1 if the error indicator is set, APR_SUCCESS otherwise.
 */
ap_status_t ap_ferror(ap_file_t *fptr)
{
    if (ferror(fptr->filehand)) {
        return (-1);
    }

    return APR_SUCCESS;
}   
