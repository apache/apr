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
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "misc.h"

ap_status_t ap_set_pipe_timeout(ap_file_t *thepipe, ap_int32_t timeout)
{
    DWORD dwMode;
    ap_oslevel_e oslevel;

    /* This code relies on the fact that anonymous pipes (which
     * do not support nonblocking I/O) are really named pipes
     * (which support nonblocking I/O) on Windows NT.
     */
    if (thepipe->pipe == 1) {
        thepipe->timeout = timeout;
        if (ap_get_oslevel(thepipe->cntxt, &oslevel) == APR_SUCCESS &&
            oslevel >= APR_WIN_NT) {
            if (timeout == 0) {
                dwMode = PIPE_NOWAIT;
            } else {
                dwMode = PIPE_WAIT;
            }
            SetNamedPipeHandleState(thepipe->filehand, &dwMode, NULL, NULL);
        }
        return APR_SUCCESS;
    }
    return APR_EINVAL;
}

ap_status_t ap_create_pipe(ap_file_t **in, ap_file_t **out, ap_pool_t *cont)
{
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    (*in) = (ap_file_t *)ap_pcalloc(cont, sizeof(ap_file_t));
    (*in)->cntxt = cont;
    (*in)->pipe = 1;
    (*in)->fname = ap_pstrdup(cont, "PIPE");
    (*in)->timeout = -1;
    (*in)->ungetchar = -1;

    (*out) = (ap_file_t *)ap_pcalloc(cont, sizeof(ap_file_t));
    (*out)->cntxt = cont;
    (*out)->pipe = 1;
    (*out)->fname = ap_pstrdup(cont, "PIPE");
    (*out)->timeout = -1;

    if (!CreatePipe(&(*in)->filehand, &(*out)->filehand, &sa, 0)) {
        return GetLastError();
    }

    return APR_SUCCESS;
}
