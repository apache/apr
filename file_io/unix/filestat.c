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
#include "apr_errno.h"

/* ***APRDOC********************************************************
 * ap_status_t ap_getfileinfo(ap_file_t *)
 *    get the specified file's stats..
 * arg 1) The file to get information about. 
 */ 
ap_status_t ap_getfileinfo(struct file_t *thefile)
{
    struct stat info;
    int rv = stat(thefile->fname, &info);

    if (rv == 0) {
        thefile->protection = info.st_mode;
        thefile->user = info.st_uid;
        thefile->group = info.st_gid;
        thefile->size = info.st_size;
        thefile->atime = info.st_atime;
        thefile->mtime = info.st_mtime;
        thefile->ctime = info.st_ctime;
        thefile->stated = 1; 
        return APR_SUCCESS;
    }
    else {
        return APR_ENOSTAT;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_stat(ap_file_t **, char *, ap_context_t *)
 *    get the specified file's stats.  The file is specified by filename,
 *    instead of using a pre-opened file.
 * arg 1) Where to store the information about the file.
 * arg 2) The name of the file to stat.
 * arg 3) the context to use to allocate the new file. 
 */ 
ap_status_t ap_stat(struct file_t **thefile, const char *fname, ap_context_t *cont)
{
    struct stat info;
    int rv = stat(fname, &info);

    if (rv == 0) {
        if ((*thefile) == NULL) {
            /* Only allocate more space and initialize the object if it is
             * NULL when passed in.
             */ 
            (*thefile) = ap_pcalloc(cont, sizeof(struct file_t));
            if ((*thefile) == NULL) {
                return APR_ENOMEM;
            }
            (*thefile)->cntxt = cont;
            ap_register_cleanup((*thefile)->cntxt, (void *)(*thefile),
                                file_cleanup, ap_null_cleanup);
            (*thefile)->fname = ap_pstrdup(cont, fname);
            (*thefile)->filehand = NULL;
            (*thefile)->filedes = -1;
        }
        (*thefile)->protection = info.st_mode;
        (*thefile)->user = info.st_uid;
        (*thefile)->group = info.st_gid;
        (*thefile)->size = info.st_size;
        (*thefile)->atime = info.st_atime;
        (*thefile)->mtime = info.st_mtime;
        (*thefile)->ctime = info.st_ctime;
        (*thefile)->stated = 1; 
        return APR_SUCCESS;
    }
    else {
        return APR_ENOSTAT;
    }
}
