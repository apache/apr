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

#define INCL_DOSERRORS
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <string.h>
#include <process.h>

apr_status_t apr_create_pipe(apr_file_t **in, apr_file_t **out, apr_pool_t *cont)
{
    ULONG filedes[2];
    ULONG rc, action;
    static int id = 0;
    char pipename[50];

    sprintf(pipename, "/pipe/%d.%d", getpid(), id++);
    rc = DosCreateNPipe(pipename, filedes, NP_ACCESS_INBOUND, NP_NOWAIT|1, 4096, 4096, 0);

    if (rc)
        return APR_OS2_STATUS(rc);

    rc = DosConnectNPipe(filedes[0]);

    if (rc && rc != ERROR_PIPE_NOT_CONNECTED) {
        DosClose(filedes[0]);
        return APR_OS2_STATUS(rc);
    }

    rc = DosOpen (pipename, filedes+1, &action, 0, FILE_NORMAL,
                  OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                  OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYREADWRITE,
                  NULL);

    if (rc) {
        DosClose(filedes[0]);
        return APR_OS2_STATUS(rc);
    }

    (*in) = (apr_file_t *)apr_palloc(cont, sizeof(apr_file_t));
    rc = DosCreateEventSem(NULL, &(*in)->pipeSem, DC_SEM_SHARED, FALSE);

    if (rc) {
        DosClose(filedes[0]);
        DosClose(filedes[1]);
        return APR_OS2_STATUS(rc);
    }

    rc = DosSetNPipeSem(filedes[0], (HSEM)(*in)->pipeSem, 1);

    if (!rc) {
        rc = DosSetNPHState(filedes[0], NP_WAIT);
    }

    if (rc) {
        DosClose(filedes[0]);
        DosClose(filedes[1]);
        DosCloseEventSem((*in)->pipeSem);
        return APR_OS2_STATUS(rc);
    }

    (*in)->cntxt = cont;
    (*in)->filedes = filedes[0];
    (*in)->fname = apr_pstrdup(cont, pipename);
    (*in)->isopen = TRUE;
    (*in)->buffered = FALSE;
    (*in)->flags = 0;
    (*in)->pipe = 1;
    (*in)->timeout = -1;
    apr_register_cleanup(cont, *in, apr_file_cleanup, apr_null_cleanup);

    (*out) = (apr_file_t *)apr_palloc(cont, sizeof(apr_file_t));
    (*out)->cntxt = cont;
    (*out)->filedes = filedes[1];
    (*out)->fname = apr_pstrdup(cont, pipename);
    (*out)->isopen = TRUE;
    (*out)->buffered = FALSE;
    (*out)->flags = 0;
    (*out)->pipe = 1;
    (*out)->timeout = -1;
    apr_register_cleanup(cont, *out, apr_file_cleanup, apr_null_cleanup);

    return APR_SUCCESS;
}



apr_status_t apr_create_namedpipe(const char *filename, apr_fileperms_t perm, apr_pool_t *cont)
{
    /* Not yet implemented, interface not suitable */
    return APR_ENOTIMPL;
} 

 

apr_status_t apr_set_pipe_timeout(apr_file_t *thepipe, apr_interval_time_t timeout)
{
    if (thepipe->pipe == 1) {
        thepipe->timeout = timeout;
        if (thepipe->timeout >= 0) {
            return APR_OS2_STATUS(DosSetNPHState (thepipe->filedes, NP_NOWAIT));
        }
        else if (thepipe->timeout == -1) {
            return APR_OS2_STATUS(DosSetNPHState (thepipe->filedes, NP_WAIT));
        }
    }
    return APR_EINVAL;
}
