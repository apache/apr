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

#include "win32/fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "misc.h"

apr_status_t apr_set_pipe_timeout(apr_file_t *thepipe, apr_interval_time_t timeout)
{
    if (thepipe->pipe == 1) {
        thepipe->timeout = timeout;
        return APR_SUCCESS;
    }
    return APR_EINVAL;
}

apr_status_t apr_get_pipe_timeout(apr_file_t *thepipe, apr_interval_time_t *timeout)
{
    if (thepipe->pipe == 1) {
        *timeout = thepipe->timeout;
        return APR_SUCCESS;
    }
    return APR_EINVAL;
}

apr_status_t apr_create_pipe(apr_file_t **in, apr_file_t **out, apr_pool_t *p)
{
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    (*in) = (apr_file_t *)apr_pcalloc(p, sizeof(apr_file_t));
    (*in)->cntxt = p;
    (*in)->n.fname = "\0\0"; // What was this??? : apr_pstrdup(p, "PIPE"); */
    (*in)->pipe = 1;
    (*in)->timeout = -1;
    (*in)->ungetchar = -1;
    (*in)->eof_hit = 0;
    (*in)->filePtr = 0;
    (*in)->bufpos = 0;
    (*in)->dataRead = 0;
    (*in)->direction = 0;

    (*out) = (apr_file_t *)apr_pcalloc(p, sizeof(apr_file_t));
    (*out)->cntxt = p;
    (*in)->n.fname = "\0\0"; // What was this??? : apr_pstrdup(p, "PIPE"); */
    (*out)->pipe = 1;
    (*out)->timeout = -1;
    (*out)->ungetchar = -1;
    (*out)->eof_hit = 0;
    (*out)->filePtr = 0;
    (*out)->bufpos = 0;
    (*out)->dataRead = 0;
    (*out)->direction = 0;

    if (!CreatePipe(&(*in)->filehand, &(*out)->filehand, &sa, 0)) {
        return apr_get_os_error();
    }

    apr_register_cleanup((*in)->cntxt, (void *)(*in), file_cleanup,
                        apr_null_cleanup);
    apr_register_cleanup((*out)->cntxt, (void *)(*out), file_cleanup,
                        apr_null_cleanup);
    return APR_SUCCESS;
}

/* apr_create_nt_pipe()
 * An internal (for now) APR function created for use by apr_create_process() 
 * when setting up pipes to communicate with the child process. 
 * apr_create_nt_pipe() allows setting the blocking mode of each end of 
 * the pipe when the pipe is created (rather than after the pipe is created). 
 * A pipe handle must be opened in full async i/o mode in order to 
 * emulate Unix non-blocking pipes with timeouts. 
 *
 * In general, we don't want to enable child side pipe handles for async i/o.
 * This prevents us from enabling both ends of the pipe for async i/o in 
 * apr_create_pipe.
 *
 * Why not use NamedPipes on NT which support setting pipe state to
 * non-blocking? On NT, even though you can set a pipe non-blocking, 
 * there is no clean way to set event driven non-zero timeouts (e.g select(),
 * WaitForSinglelObject, et. al. will not detect pipe i/o). On NT, you 
 * have to poll the pipe to detech i/o on a non-blocking pipe.
 *
 * wgs
 */
apr_status_t apr_create_nt_pipe(apr_file_t **in, apr_file_t **out, 
                                BOOLEAN bAsyncRead, BOOLEAN bAsyncWrite, 
                                apr_pool_t *p)
{
    apr_oslevel_e level;
    SECURITY_ATTRIBUTES sa;
    static unsigned long id = 0;
    DWORD dwPipeMode;
    DWORD dwOpenMode;
    char name[50];

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    (*in) = (apr_file_t *)apr_pcalloc(p, sizeof(apr_file_t));
    (*in)->cntxt = p;
    (*in)->n.fname = "\0\0"; // What was this??? : apr_pstrdup(p, "PIPE"); */
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
    (*out)->cntxt = p;
    (*in)->n.fname = "\0\0"; // What was this??? : apr_pstrdup(p, "PIPE"); */
    (*out)->pipe = 1;
    (*out)->timeout = -1;
    (*out)->ungetchar = -1;
    (*out)->eof_hit = 0;
    (*out)->filePtr = 0;
    (*out)->bufpos = 0;
    (*out)->dataRead = 0;
    (*out)->direction = 0;
    (*out)->pOverlapped = NULL;

    if (apr_get_oslevel(p, &level) == APR_SUCCESS && level >= APR_WIN_NT) {
        /* Create the read end of the pipe */
        dwOpenMode = PIPE_ACCESS_INBOUND;
        if (bAsyncRead) {
            dwOpenMode |= FILE_FLAG_OVERLAPPED;
            (*in)->pOverlapped = (OVERLAPPED*) apr_pcalloc(p, sizeof(OVERLAPPED));
            (*in)->pOverlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            /* register a cleanup for the event handle... */
        }

        dwPipeMode = 0;

        sprintf(name, "\\\\.\\pipe\\%d.%d", getpid(), id++);

        (*in)->filehand = CreateNamedPipe(name,
                                          dwOpenMode,
                                          dwPipeMode,
                                          1,            //nMaxInstances,
                                          8182,         //nOutBufferSize, 
                                          8192,         //nInBufferSize,                   
                                          1,            //nDefaultTimeOut,                
                                          &sa);

        /* Create the write end of the pipe */
        dwOpenMode = FILE_ATTRIBUTE_NORMAL;
        if (bAsyncWrite) {
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
        if (!CreatePipe(&(*in)->filehand, &(*out)->filehand, &sa, 0)) {
            return apr_get_os_error();
        }
    }

    return APR_SUCCESS;
}
