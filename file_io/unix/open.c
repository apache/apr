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

#ifdef NETWARE
#include "nks/dirio.h"
#include "apr_hash.h"
#include "fsio.h"
#endif

apr_status_t apr_unix_file_cleanup(void *thefile)
{
    apr_file_t *file = thefile;
    apr_status_t flush_rv = APR_SUCCESS, rv = APR_SUCCESS;

    if (file->buffered) {
        flush_rv = apr_file_flush(file);
    }
    if (close(file->filedes) == 0) {
        file->filedes = -1;
        if (file->flags & APR_DELONCLOSE) {
            unlink(file->fname);
        }
#if APR_HAS_THREADS
        if (file->thlock) {
            rv = apr_thread_mutex_destroy(file->thlock);
        }
#endif
    }
    else {
        /* Are there any error conditions other than EINTR or EBADF? */
        rv = errno;
    }
    return rv != APR_SUCCESS ? rv : flush_rv;
}

APR_DECLARE(apr_status_t) apr_file_open(apr_file_t **new, 
                                        const char *fname, 
                                        apr_int32_t flag, 
                                        apr_fileperms_t perm, 
                                        apr_pool_t *pool)
{
    apr_os_file_t fd;
    int oflags = 0;
#if APR_HAS_THREADS
    apr_thread_mutex_t *thlock;
    apr_status_t rv;
#endif

    if ((flag & APR_READ) && (flag & APR_WRITE)) {
        oflags = O_RDWR;
    }
    else if (flag & APR_READ) {
        oflags = O_RDONLY;
    }
    else if (flag & APR_WRITE) {
        oflags = O_WRONLY;
    }
    else {
        return APR_EACCES; 
    }

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
        oflags |= O_APPEND;
    }
    if (flag & APR_TRUNCATE) {
        oflags |= O_TRUNC;
    }
#ifdef O_BINARY
    if (flag & APR_BINARY) {
        oflags |= O_BINARY;
    }
#endif
    
#if APR_HAS_THREADS
    if ((flag & APR_BUFFERED) && (flag & APR_XTHREAD)) {
        rv = apr_thread_mutex_create(&thlock,
                                     APR_THREAD_MUTEX_DEFAULT, pool);
        if (rv) {
            return rv;
        }
    }
#endif

    if (perm == APR_OS_DEFAULT) {
        fd = open(fname, oflags, 0666);
    }
    else {
        fd = open(fname, oflags, apr_unix_perms2mode(perm));
    } 
    if (fd < 0) {
       return errno;
    }

    (*new) = (apr_file_t *)apr_pcalloc(pool, sizeof(apr_file_t));
    (*new)->pool = pool;
    (*new)->flags = flag;
    (*new)->filedes = fd;

    (*new)->fname = apr_pstrdup(pool, fname);

    (*new)->blocking = BLK_ON;
    (*new)->buffered = (flag & APR_BUFFERED) > 0;

    if ((*new)->buffered) {
        (*new)->buffer = apr_palloc(pool, APR_FILE_BUFSIZE);
#if APR_HAS_THREADS
        if ((*new)->flags & APR_XTHREAD) {
            (*new)->thlock = thlock;
        }
#endif
    }
    else {
        (*new)->buffer = NULL;
    }

    (*new)->is_pipe = 0;
    (*new)->timeout = -1;
    (*new)->ungetchar = -1;
    (*new)->eof_hit = 0;
    (*new)->filePtr = 0;
    (*new)->bufpos = 0;
    (*new)->dataRead = 0;
    (*new)->direction = 0;

    if (!(flag & APR_FILE_NOCLEANUP)) {
        apr_pool_cleanup_register((*new)->pool, (void *)(*new), 
                                  apr_unix_file_cleanup, 
                                  apr_unix_file_cleanup);
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_close(apr_file_t *file)
{
    return apr_pool_cleanup_run(file->pool, file, apr_unix_file_cleanup);
}

APR_DECLARE(apr_status_t) apr_file_remove(const char *path, apr_pool_t *pool)
{
    if (unlink(path) == 0) {
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}

APR_DECLARE(apr_status_t) apr_file_rename(const char *from_path, 
                                          const char *to_path,
                                          apr_pool_t *p)
{
    if (rename(from_path, to_path) != 0) {
        return errno;
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_file_get(apr_os_file_t *thefile, 
                                          apr_file_t *file)
{
    *thefile = file->filedes;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_file_put(apr_file_t **file, 
                                          apr_os_file_t *thefile,
                                          apr_int32_t flags, apr_pool_t *pool)
{
    int *dafile = thefile;
    
    (*file) = apr_pcalloc(pool, sizeof(apr_file_t));
    (*file)->pool = pool;
    (*file)->eof_hit = 0;
    (*file)->blocking = BLK_UNKNOWN; /* in case it is a pipe */
    (*file)->timeout = -1;
    (*file)->ungetchar = -1; /* no char avail */
    (*file)->filedes = *dafile;
    (*file)->flags = flags | APR_FILE_NOCLEANUP;
    (*file)->buffered = (flags & APR_BUFFERED) > 0;

    if ((*file)->buffered) {
        (*file)->buffer = apr_palloc(pool, APR_FILE_BUFSIZE);
#if APR_HAS_THREADS
        if ((*file)->flags & APR_XTHREAD) {
            apr_status_t rv;
            rv = apr_thread_mutex_create(&((*file)->thlock),
                                         APR_THREAD_MUTEX_DEFAULT, pool);
            if (rv) {
                return rv;
            }
        }
#endif
    }
    return APR_SUCCESS;
}    

APR_DECLARE(apr_status_t) apr_file_eof(apr_file_t *fptr)
{
    if (fptr->eof_hit == 1) {
        return APR_EOF;
    }
    return APR_SUCCESS;
}   

APR_DECLARE(apr_status_t) apr_file_open_stderr(apr_file_t **thefile, 
                                               apr_pool_t *pool)
{
    int fd = STDERR_FILENO;

    return apr_os_file_put(thefile, &fd, 0, pool);
}

APR_DECLARE(apr_status_t) apr_file_open_stdout(apr_file_t **thefile, 
                                               apr_pool_t *pool)
{
    int fd = STDOUT_FILENO;

    return apr_os_file_put(thefile, &fd, 0, pool);
}

APR_DECLARE(apr_status_t) apr_file_open_stdin(apr_file_t **thefile, 
                                              apr_pool_t *pool)
{
    int fd = STDIN_FILENO;

    return apr_os_file_put(thefile, &fd, 0, pool);
}

APR_IMPLEMENT_INHERIT_SET(file, flags, pool, apr_unix_file_cleanup)

APR_IMPLEMENT_INHERIT_UNSET(file, flags, pool, apr_unix_file_cleanup)

APR_POOL_IMPLEMENT_ACCESSOR(file)
