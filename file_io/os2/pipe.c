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

#define INCL_DOSERRORS
#include "apr_arch_file_io.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include <string.h>
#include <process.h>

APR_DECLARE(apr_status_t) apr_file_pipe_create(apr_file_t **in, apr_file_t **out, apr_pool_t *pool)
{
    ULONG filedes[2];
    ULONG rc, action;
    static int id = 0;
    char pipename[50];

    sprintf(pipename, "/pipe/%d.%d", getpid(), id++);
    rc = DosCreateNPipe(pipename, filedes, NP_ACCESS_INBOUND, NP_NOWAIT|1, 4096, 4096, 0);

    if (rc)
        return APR_FROM_OS_ERROR(rc);

    rc = DosConnectNPipe(filedes[0]);

    if (rc && rc != ERROR_PIPE_NOT_CONNECTED) {
        DosClose(filedes[0]);
        return APR_FROM_OS_ERROR(rc);
    }

    rc = DosOpen (pipename, filedes+1, &action, 0, FILE_NORMAL,
                  OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                  OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYREADWRITE,
                  NULL);

    if (rc) {
        DosClose(filedes[0]);
        return APR_FROM_OS_ERROR(rc);
    }

    (*in) = (apr_file_t *)apr_palloc(pool, sizeof(apr_file_t));
    rc = DosCreateEventSem(NULL, &(*in)->pipeSem, DC_SEM_SHARED, FALSE);

    if (rc) {
        DosClose(filedes[0]);
        DosClose(filedes[1]);
        return APR_FROM_OS_ERROR(rc);
    }

    rc = DosSetNPipeSem(filedes[0], (HSEM)(*in)->pipeSem, 1);

    if (!rc) {
        rc = DosSetNPHState(filedes[0], NP_WAIT);
    }

    if (rc) {
        DosClose(filedes[0]);
        DosClose(filedes[1]);
        DosCloseEventSem((*in)->pipeSem);
        return APR_FROM_OS_ERROR(rc);
    }

    (*in)->pool = pool;
    (*in)->filedes = filedes[0];
    (*in)->fname = apr_pstrdup(pool, pipename);
    (*in)->isopen = TRUE;
    (*in)->buffered = FALSE;
    (*in)->flags = 0;
    (*in)->pipe = 1;
    (*in)->timeout = -1;
    (*in)->blocking = BLK_ON;
    apr_pool_cleanup_register(pool, *in, apr_file_cleanup, apr_pool_cleanup_null);

    (*out) = (apr_file_t *)apr_palloc(pool, sizeof(apr_file_t));
    (*out)->pool = pool;
    (*out)->filedes = filedes[1];
    (*out)->fname = apr_pstrdup(pool, pipename);
    (*out)->isopen = TRUE;
    (*out)->buffered = FALSE;
    (*out)->flags = 0;
    (*out)->pipe = 1;
    (*out)->timeout = -1;
    (*out)->blocking = BLK_ON;
    apr_pool_cleanup_register(pool, *out, apr_file_cleanup, apr_pool_cleanup_null);

    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_file_namedpipe_create(const char *filename, apr_fileperms_t perm, apr_pool_t *pool)
{
    /* Not yet implemented, interface not suitable */
    return APR_ENOTIMPL;
} 

 

APR_DECLARE(apr_status_t) apr_file_pipe_timeout_set(apr_file_t *thepipe, apr_interval_time_t timeout)
{
    if (thepipe->pipe == 1) {
        thepipe->timeout = timeout;

        if (thepipe->timeout >= 0) {
            if (thepipe->blocking != BLK_OFF) {
                thepipe->blocking = BLK_OFF;
                return APR_FROM_OS_ERROR(DosSetNPHState(thepipe->filedes, NP_NOWAIT));
            }
        }
        else if (thepipe->timeout == -1) {
            if (thepipe->blocking != BLK_ON) {
                thepipe->blocking = BLK_ON;
                return APR_FROM_OS_ERROR(DosSetNPHState(thepipe->filedes, NP_WAIT));
            }
        }
    }
    return APR_EINVAL;
}



APR_DECLARE(apr_status_t) apr_file_pipe_timeout_get(apr_file_t *thepipe, apr_interval_time_t *timeout)
{
    if (thepipe->pipe == 1) {
        *timeout = thepipe->timeout;
        return APR_SUCCESS;
    }
    return APR_EINVAL;
}



APR_DECLARE(apr_status_t) apr_os_pipe_put_ex(apr_file_t **file,
                                             apr_os_file_t *thefile,
                                             int register_cleanup,
                                             apr_pool_t *pool)
{
    (*file) = apr_pcalloc(pool, sizeof(apr_file_t));
    (*file)->pool = pool;
    (*file)->isopen = TRUE;
    (*file)->pipe = 1;
    (*file)->blocking = BLK_UNKNOWN; /* app needs to make a timeout call */
    (*file)->timeout = -1;
    (*file)->filedes = *thefile;

    if (register_cleanup) {
        apr_pool_cleanup_register(pool, *file, apr_file_cleanup,
                                  apr_pool_cleanup_null);
    }

    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_os_pipe_put(apr_file_t **file,
                                          apr_os_file_t *thefile,
                                          apr_pool_t *pool)
{
    return apr_os_pipe_put_ex(file, thefile, 0, pool);
}
