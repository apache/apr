/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "win32/apr_arch_file_io.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include <string.h>
#include "apr_arch_inherit.h"

APR_DECLARE(apr_status_t) apr_file_dup(apr_file_t **new_file,
                                       apr_file_t *old_file, apr_pool_t *p)
{
#ifdef _WIN32_WCE
    return APR_ENOTIMPL;
#else
    HANDLE hproc = GetCurrentProcess();
    HANDLE newhand = NULL;

    if (!DuplicateHandle(hproc, old_file->filehand, 
                         hproc, &newhand, 0, FALSE, 
                         DUPLICATE_SAME_ACCESS)) {
        return apr_get_os_error();
    }

    (*new_file) = (apr_file_t *) apr_pcalloc(p, sizeof(apr_file_t));
    (*new_file)->filehand = newhand;
    (*new_file)->flags = old_file->flags & ~APR_INHERIT;
    (*new_file)->pool = p;
    (*new_file)->fname = apr_pstrdup(p, old_file->fname);
    (*new_file)->append = old_file->append;
    (*new_file)->buffered = FALSE;

    apr_pool_cleanup_register((*new_file)->pool, (void *)(*new_file), file_cleanup,
                        apr_pool_cleanup_null);

    return APR_SUCCESS;
#endif /* !defined(_WIN32_WCE) */
}

#define stdin_handle 0x01
#define stdout_handle 0x02
#define stderr_handle 0x04

APR_DECLARE(apr_status_t) apr_file_dup2(apr_file_t *new_file,
                                        apr_file_t *old_file, apr_pool_t *p)
{
#ifdef _WIN32_WCE
    return APR_ENOTIMPL;
#else
    DWORD stdhandle = 0;
    HANDLE hproc = GetCurrentProcess();
    HANDLE newhand = NULL;
    apr_int32_t newflags;

    /* dup2 is not supported literaly with native Windows handles.
     * We can, however, emulate dup2 for the standard i/o handles,
     * and close and replace other handles with duped handles.
     * The os_handle will change, however.
     */
    if (new_file->filehand == GetStdHandle(STD_ERROR_HANDLE)) {
        stdhandle |= stderr_handle;
    }
    if (new_file->filehand == GetStdHandle(STD_OUTPUT_HANDLE)) {
        stdhandle |= stdout_handle;
    }
    if (new_file->filehand == GetStdHandle(STD_INPUT_HANDLE)) {
        stdhandle |= stdin_handle;
    }

    if (stdhandle) {
        if (!DuplicateHandle(hproc, old_file->filehand, 
                             hproc, &newhand, 0,
                             TRUE, DUPLICATE_SAME_ACCESS)) {
            return apr_get_os_error();
        }
        if (((stdhandle & stderr_handle) && !SetStdHandle(STD_ERROR_HANDLE, newhand)) ||
            ((stdhandle & stdout_handle) && !SetStdHandle(STD_OUTPUT_HANDLE, newhand)) ||
            ((stdhandle & stdin_handle) && !SetStdHandle(STD_INPUT_HANDLE, newhand))) {
            return apr_get_os_error();
        }
        newflags = old_file->flags | APR_INHERIT;
    }
    else {
        if (!DuplicateHandle(hproc, old_file->filehand, 
                             hproc, &newhand, 0,
                             FALSE, DUPLICATE_SAME_ACCESS)) {
            return apr_get_os_error();
        }
        newflags = old_file->flags & ~APR_INHERIT;
    }

    if (new_file->filehand && (new_file->filehand != INVALID_HANDLE_VALUE)) {
        CloseHandle(new_file->filehand);
    }

    new_file->flags = newflags;
    new_file->filehand = newhand;
    new_file->fname = apr_pstrdup(new_file->pool, old_file->fname);
    new_file->append = old_file->append;
    new_file->buffered = FALSE;

    return APR_SUCCESS;
#endif /* !defined(_WIN32_WCE) */
}

APR_DECLARE(apr_status_t) apr_file_setaside(apr_file_t **new_file,
                                            apr_file_t *old_file,
                                            apr_pool_t *p)
{
    *new_file = (apr_file_t *)apr_palloc(p, sizeof(apr_file_t));
    memcpy(*new_file, old_file, sizeof(apr_file_t));
    (*new_file)->pool = p;
    if (old_file->buffered) {
        (*new_file)->buffer = apr_palloc(p, APR_FILE_BUFSIZE);
        if (old_file->direction == 1) {
            memcpy((*new_file)->buffer, old_file->buffer, old_file->bufpos);
        }
        else {
            memcpy((*new_file)->buffer, old_file->buffer, old_file->dataRead);
        }
    }
    if (old_file->mutex) {
        apr_thread_mutex_create(&((*new_file)->mutex),
                                APR_THREAD_MUTEX_DEFAULT, p);
        apr_thread_mutex_destroy(old_file->mutex);
    }
    if (old_file->fname) {
        (*new_file)->fname = apr_pstrdup(p, old_file->fname);
    }
    if (!(old_file->flags & APR_FILE_NOCLEANUP)) {
        apr_pool_cleanup_register(p, (void *)(*new_file), 
                                  file_cleanup,
                                  file_cleanup);
    }

    old_file->filehand = INVALID_HANDLE_VALUE;
    apr_pool_cleanup_kill(old_file->pool, (void *)old_file,
                          file_cleanup);
    return APR_SUCCESS;
}
