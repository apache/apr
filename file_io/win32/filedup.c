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
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <string.h>

ap_status_t ap_dupfile(ap_file_t **new_file, ap_file_t *old_file)
{
    HANDLE hCurrentProcess = GetCurrentProcess();

    if ((*new_file) == NULL) {
        (*new_file) = (ap_file_t *) ap_pcalloc(old_file->cntxt,
                                                   sizeof(ap_file_t));
        if ((*new_file) == NULL) {
            return APR_ENOMEM;
        }
        if (!DuplicateHandle(hCurrentProcess, old_file->filehand, 
                             hCurrentProcess,
                             &(*new_file)->filehand, 0, FALSE, 
                             DUPLICATE_SAME_ACCESS)) {
            return GetLastError();
        }
    } else {
        HANDLE hFile = (*new_file)->filehand;
        /* dup2 is not supported with native Windows handles. We 
         * can, however, emulate dup2 for the standard i/o handles.
         */
        if (hFile == GetStdHandle(STD_ERROR_HANDLE)) {
            if (!SetStdHandle(STD_ERROR_HANDLE, old_file->filehand))
                return GetLastError();
        }
        else if (hFile == GetStdHandle(STD_OUTPUT_HANDLE)) {
            if (!SetStdHandle(STD_OUTPUT_HANDLE, old_file->filehand))
                return GetLastError();
        }
        else if (hFile == GetStdHandle(STD_INPUT_HANDLE)) {
            if (!SetStdHandle(STD_INPUT_HANDLE, old_file->filehand))
                return GetLastError();
        }
        else
            return APR_ENOTIMPL;
    }

    (*new_file)->cntxt = old_file->cntxt;
    (*new_file)->fname = ap_pstrdup(old_file->cntxt, old_file->fname);
    (*new_file)->demonfname = ap_pstrdup(old_file->cntxt, old_file->demonfname);
    (*new_file)->lowerdemonfname = ap_pstrdup(old_file->cntxt, old_file->lowerdemonfname);
    (*new_file)->buffered = old_file->buffered;
	(*new_file)->append = old_file->append;
 /*   (*new_file)->protection = old_file->protection;
    (*new_file)->user = old_file->user;
    (*new_file)->group = old_file->group;*/
    (*new_file)->size = old_file->size;
    (*new_file)->atime = old_file->atime;    
    (*new_file)->mtime = old_file->mtime;
    (*new_file)->ctime = old_file->ctime;
    ap_register_cleanup((*new_file)->cntxt, (void *)(*new_file), file_cleanup,
                        ap_null_cleanup);

    return APR_SUCCESS;
}


