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

#include "apr_arch_file_io.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include "apr_thread_mutex.h"
#include "apr_arch_inherit.h"

static apr_status_t _file_dup(apr_file_t **new_file, 
                              apr_file_t *old_file, apr_pool_t *p,
                              int which_dup)
{
    int rv;
    
    if (which_dup == 2) {
        if ((*new_file) == NULL) {
            /* We can't dup2 unless we have a valid new_file */
            return APR_EINVAL;
        }
        rv = dup2(old_file->filedes, (*new_file)->filedes);
    } else {
        rv = dup(old_file->filedes);
    }

    if (rv == -1)
        return errno;
    
    if (which_dup == 1) {
        (*new_file) = (apr_file_t *)apr_pcalloc(p, sizeof(apr_file_t));
        (*new_file)->pool = p;
        (*new_file)->filedes = rv;
    }

    (*new_file)->fname = apr_pstrdup(p, old_file->fname);
    (*new_file)->buffered = old_file->buffered;

    /* If the existing socket in a dup2 is already buffered, we
     * have an existing and valid (hopefully) mutex, so we don't
     * want to create it again as we could leak!
     */
#if APR_HAS_THREADS
    if ((*new_file)->buffered && !(*new_file)->thlock && old_file->thlock) {
        apr_thread_mutex_create(&((*new_file)->thlock),
                                APR_THREAD_MUTEX_DEFAULT, p);
    }
#endif
    /* As above, only create the buffer if we haven't already
     * got one.
     */
    if ((*new_file)->buffered && !(*new_file)->buffer) {
        (*new_file)->buffer = apr_palloc(p, APR_FILE_BUFSIZE);
    }

    /* this is the way dup() works */
    (*new_file)->blocking = old_file->blocking; 

    /* make sure unget behavior is consistent */
    (*new_file)->ungetchar = old_file->ungetchar;

    /* apr_file_dup2() retains the original cleanup, reflecting 
     * the existing inherit and nocleanup flags.  This means, 
     * that apr_file_dup2() cannot be called against an apr_file_t
     * already closed with apr_file_close, because the expected
     * cleanup was already killed.
     */
    if (which_dup == 2) {
        return APR_SUCCESS;
    }

    /* apr_file_dup() retains all old_file flags with the exceptions
     * of APR_INHERIT and APR_FILE_NOCLEANUP.
     * The user must call apr_file_inherit_set() on the dupped 
     * apr_file_t when desired.
     */
    (*new_file)->flags = old_file->flags
                       & ~(APR_INHERIT | APR_FILE_NOCLEANUP);

    apr_pool_cleanup_register((*new_file)->pool, (void *)(*new_file),
                              apr_unix_file_cleanup, 
                              apr_unix_file_cleanup);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_dup(apr_file_t **new_file,
                                       apr_file_t *old_file, apr_pool_t *p)
{
    return _file_dup(new_file, old_file, p, 1);
}

APR_DECLARE(apr_status_t) apr_file_dup2(apr_file_t *new_file,
                                        apr_file_t *old_file, apr_pool_t *p)
{
    return _file_dup(&new_file, old_file, p, 2);
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
#if APR_HAS_THREADS
        if (old_file->thlock) {
            apr_thread_mutex_create(&((*new_file)->thlock),
                                    APR_THREAD_MUTEX_DEFAULT, p);
            apr_thread_mutex_destroy(old_file->thlock);
        }
#endif /* APR_HAS_THREADS */
    }
    if (old_file->fname) {
        (*new_file)->fname = apr_pstrdup(p, old_file->fname);
    }
    if (!(old_file->flags & APR_FILE_NOCLEANUP)) {
        apr_pool_cleanup_register(p, (void *)(*new_file), 
                                  apr_unix_file_cleanup,
                                  ((*new_file)->flags & APR_INHERIT)
                                     ? apr_pool_cleanup_null
                                     : apr_unix_file_cleanup);
    }

    old_file->filedes = -1;
    apr_pool_cleanup_kill(old_file->pool, (void *)old_file,
                          apr_unix_file_cleanup);
    return APR_SUCCESS;
}
