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
#if APR_HAVE_ERRNO_H
#include <errno.h>
#endif
#include <string.h>
#include <stdio.h>
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if APR_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include "apr_arch_misc.h"

APR_DECLARE(apr_status_t) apr_file_pipe_timeout_set(apr_file_t *thepipe, apr_interval_time_t timeout)
{
    /* Always OK to unset timeouts */
    if (timeout == -1) {
        thepipe->timeout = timeout;
        return APR_SUCCESS;
    }
    if (!thepipe->pipe) {
        return APR_ENOTIMPL;
    }
    if (timeout && !(thepipe->pOverlapped)) {
        /* Cannot be nonzero if a pipe was opened blocking
         */
        return APR_EINVAL;
    }
    thepipe->timeout = timeout;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_pipe_timeout_get(apr_file_t *thepipe, apr_interval_time_t *timeout)
{
    /* Always OK to get the timeout (even if it's unset ... -1) */
    *timeout = thepipe->timeout;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_pipe_create(apr_file_t **in, apr_file_t **out, apr_pool_t *p)
{
    /* Unix creates full blocking pipes. */
    return apr_create_nt_pipe(in, out, APR_FULL_BLOCK, p);
}

/* apr_create_nt_pipe()
 * An internal (for now) APR function used by apr_proc_create() 
 * when setting up pipes to communicate with the child process. 
 * apr_create_nt_pipe() allows setting the blocking mode of each end of 
 * the pipe when the pipe is created (rather than after the pipe is created). 
 * A pipe handle must be opened in full async i/o mode in order to 
 * emulate Unix non-blocking pipes with timeouts. 
 *
 * In general, we don't want to enable child side pipe handles for async i/o.
 * This prevents us from enabling both ends of the pipe for async i/o in 
 * apr_file_pipe_create.
 *
 * Why not use NamedPipes on NT which support setting pipe state to
 * non-blocking? On NT, even though you can set a pipe non-blocking, 
 * there is no clean way to set event driven non-zero timeouts (e.g select(),
 * WaitForSinglelObject, et. al. will not detect pipe i/o). On NT, you 
 * have to poll the pipe to detect i/o on a non-blocking pipe.
 */
apr_status_t apr_create_nt_pipe(apr_file_t **in, apr_file_t **out, 
                                apr_int32_t blocking_mode, 
                                apr_pool_t *p)
{
#ifdef _WIN32_WCE
    return APR_ENOTIMPL;
#else
    SECURITY_ATTRIBUTES sa;
    static unsigned long id = 0;
    DWORD dwPipeMode;
    DWORD dwOpenMode;
    char name[50];

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    (*in) = (apr_file_t *)apr_pcalloc(p, sizeof(apr_file_t));
    (*in)->pool = p;
    (*in)->fname = NULL;
    (*in)->pipe = 1;
    (*in)->timeout = -1;
    (*in)->ungetchar = -1;
    (*in)->eof_hit = 0;
    (*in)->filePtr = 0;
    (*in)->bufpos = 0;
    (*in)->dataRead = 0;
    (*in)->direction = 0;
    (*in)->pOverlapped = NULL;

    (*out) = (apr_file_t *)apr_pcalloc(p, sizeof(apr_file_t));
    (*out)->pool = p;
    (*out)->fname = NULL;
    (*out)->pipe = 1;
    (*out)->timeout = -1;
    (*out)->ungetchar = -1;
    (*out)->eof_hit = 0;
    (*out)->filePtr = 0;
    (*out)->bufpos = 0;
    (*out)->dataRead = 0;
    (*out)->direction = 0;
    (*out)->pOverlapped = NULL;

    if (apr_os_level >= APR_WIN_NT) {
        /* Create the read end of the pipe */
        dwOpenMode = PIPE_ACCESS_INBOUND;
        if (blocking_mode == APR_WRITE_BLOCK /* READ_NONBLOCK */
               || blocking_mode == APR_FULL_NONBLOCK) {
            dwOpenMode |= FILE_FLAG_OVERLAPPED;
            (*in)->pOverlapped = (OVERLAPPED*) apr_pcalloc(p, sizeof(OVERLAPPED));
            (*in)->pOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        }

        dwPipeMode = 0;

        sprintf(name, "\\\\.\\pipe\\apr-pipe-%u.%lu", getpid(), id++);

        (*in)->filehand = CreateNamedPipe(name,
                                          dwOpenMode,
                                          dwPipeMode,
                                          1,            //nMaxInstances,
                                          0,            //nOutBufferSize, 
                                          65536,        //nInBufferSize,                   
                                          1,            //nDefaultTimeOut,                
                                          &sa);

        /* Create the write end of the pipe */
        dwOpenMode = FILE_ATTRIBUTE_NORMAL;
        if (blocking_mode == APR_READ_BLOCK /* WRITE_NONBLOCK */
                || blocking_mode == APR_FULL_NONBLOCK) {
            dwOpenMode |= FILE_FLAG_OVERLAPPED;
            (*out)->pOverlapped = (OVERLAPPED*) apr_pcalloc(p, sizeof(OVERLAPPED));
            (*out)->pOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        }
        
        (*out)->filehand = CreateFile(name,
                                      GENERIC_WRITE,   // access mode
                                      0,               // share mode
                                      &sa,             // Security attributes
                                      OPEN_EXISTING,   // dwCreationDisposition
                                      dwOpenMode,      // Pipe attributes
                                      NULL);           // handle to template file
    }
    else {
        /* Pipes on Win9* are blocking. Live with it. */
        if (!CreatePipe(&(*in)->filehand, &(*out)->filehand, &sa, 65536)) {
            return apr_get_os_error();
        }
    }

    apr_pool_cleanup_register((*in)->pool, (void *)(*in), file_cleanup,
                        apr_pool_cleanup_null);
    apr_pool_cleanup_register((*out)->pool, (void *)(*out), file_cleanup,
                        apr_pool_cleanup_null);
    return APR_SUCCESS;
#endif /* _WIN32_WCE */
}
