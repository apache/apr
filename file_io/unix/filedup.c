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

/* ***APRDOC********************************************************
 * ap_status_t ap_dupfile(ap_file_t **new_file, ap_file_t *old_file)
 *    duplicate the specified file descriptor.
 * arg 1) The structure to duplicate into. 
 * arg 2) The file to duplicate.
 */         
ap_status_t ap_dupfile(struct file_t **new_file, struct file_t *old_file)
{
    char *buf_oflags;
    int have_file = 0;

    if ((*new_file) == NULL) {
        (*new_file) = (struct file_t *)ap_pcalloc(old_file->cntxt,
                                   sizeof(struct file_t));
        if ((*new_file) == NULL) {
            return APR_ENOMEM;
        }
    } else {
        have_file = 1;
    }
    
    (*new_file)->cntxt = old_file->cntxt; 
    if (old_file->buffered) {
        switch (old_file->oflags) {
            case O_RDONLY:
                buf_oflags = "r";
                break;
            case O_WRONLY:
                buf_oflags = "w";
                break;
            case O_RDWR:
                buf_oflags = "r+";
                break;
	    default:
		return APR_BADARG;
        }
        (*new_file)->filehand = freopen(old_file->fname, buf_oflags, 
                                        old_file->filehand); 
    }
    else {
        if (have_file) {
            dup2(old_file->filedes, (*new_file)->filedes);
        }
        else {
            (*new_file)->filedes = dup(old_file->filedes); 
        }
    }
    (*new_file)->fname = ap_pstrdup(old_file->cntxt, old_file->fname);
    (*new_file)->buffered = old_file->buffered;
    ap_register_cleanup((*new_file)->cntxt, (void *)(*new_file), file_cleanup,
                        ap_null_cleanup);
    return APR_SUCCESS;
}

