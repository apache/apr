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


// BeOS port by David Reid 23 Feb 1999

#include <errno.h>
#include <support/SupportDefs.h>
#include <kernel/OS.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"

ap_status_t file_cleanup(void *thefile)
{
    struct file_t *file = thefile;
    if (close(file->filedes) == 0) {
        file->filedes = -1;
        return APR_SUCCESS;
    }
    else {
        return errno;
	/* Are there any error conditions other than EINTR or EBADF? */
    }
}

ap_status_t ap_open(ap_context_t *cont, char *fname, ap_int32_t flag,  ap_fileperms_t perm, struct file_t **new)
{
    int oflags = 0;
    struct stat info;
    mode_t mode = get_fileperms(perm);

    (*new) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));
    
    (*new)->cntxt = cont;
    
    if ((flag & APR_READ) && (flag & APR_WRITE)) {
        oflags = B_READ_WRITE;
    }
    else if (flag & APR_READ) {
        oflags = B_READ_ONLY;
    }
    else if (flag & APR_WRITE) {
        oflags = B_WRITE_ONLY;
    }
    else {
        (*new)->filedes = -1;
		return APR_EACCES;
    }
    if (flag & APR_BUFFERED) {
       (*new)->buffered = TRUE;
    }

    (*new)->fname = (char*)strdup(fname);
    if (flag & APR_CREATE) {
        oflags |= B_CREATE_FILE; 
		if (flag & APR_EXCL) {
		    oflags |= B_FAIL_IF_EXISTS;
		}
    }
    if ((flag & APR_EXCL) && !(flag & APR_CREATE)) {
        (*new)->filedes = -1;
		return APR_EACCES;
    }   

    if (flag & APR_APPEND) {
        oflags |= B_OPEN_AT_END;
    }
    if (flag & APR_TRUNCATE) {
        oflags |= B_ERASE_FILE;
    }

    (*new)->filedes = open(fname, oflags, mode);
    
    if ((*new)->filedes < 0) {
        (*new)->filedes = -1;
        return errno;
    }
    if (ap_updatefileinfo(*new) == APR_SUCCESS) {
		ap_register_cleanup((*new)->cntxt, (void *)(*new),
		                    file_cleanup, NULL);
		return APR_SUCCESS;
    }
    else {
        (*new)->filedes = -1;
		return APR_ENOSTAT;
    }
}

ap_status_t ap_close(struct file_t * file)
{
    if (file_cleanup(file) == APR_SUCCESS) {
        ap_kill_cleanup(file->cntxt, file, file_cleanup);
        return APR_SUCCESS;
    }
    else {
        return errno;
	/* Are there any error conditions other than EINTR or EBADF? */
    }
}

ap_status_t ap_remove_file(ap_context_t *cont, char *path) 
{ 
    if (unlink(path) == 0) { 
        return APR_SUCCESS; 
    } 
    else { 
        return errno; 
    } 
} 
