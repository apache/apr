/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include <stdio.h>
#include <nks/fsio.h>

#include "fileio.h"
#include "apr_strings.h"

static int convert_error (int err)
{
	switch (err)
	{
		default :			break;
		case NX_EINVAL:	    return APR_EINVAL;
		case NX_EBADF:		return APR_EBADF;
		case NX_ENOENT:		return APR_ENOENT;
		case NX_ENAMETOOLONG:return APR_ENAMETOOLONG;
	}

    return err;
}

static apr_status_t pipeblock(apr_file_t *thepipe)
{
	int				err;
	unsigned long	flags;

	if (!(err = NXGetCtlInfo(thepipe->filedes, NX_CTL_FLAGS, &flags)))
	{
		flags &= ~NX_O_NONBLOCK;
		err    = NXSetCtlInfo(thepipe->filedes, NX_CTL_FLAGS, flags);
	}

    if (err)
        return convert_error (err);

    thepipe->blocking = BLK_ON;
    return APR_SUCCESS;
}

static apr_status_t pipenonblock(apr_file_t *thepipe)
{
	int				err;
	unsigned long	flags;

	if (!(err = NXGetCtlInfo(thepipe->filedes, NX_CTL_FLAGS, &flags)))
	{
		flags |= NX_O_NONBLOCK;
		err    = NXSetCtlInfo(thepipe->filedes, NX_CTL_FLAGS, flags);
	}

    if (err)
        return convert_error (err);

    thepipe->blocking = BLK_OFF;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_pipe_timeout_set(apr_file_t *thepipe, apr_interval_time_t timeout)
{
    if (thepipe->pipe == 1) {
        thepipe->timeout = timeout;
        if (timeout >= 0) {
            if (thepipe->blocking != BLK_OFF) { /* blocking or unknown state */
                return pipenonblock(thepipe);
            }
        }
        else {
            if (thepipe->blocking != BLK_ON) { /* non-blocking or unknown state */
                return pipeblock(thepipe);
            }
        }
        return APR_SUCCESS;
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

APR_DECLARE(apr_status_t) apr_file_pipe_create(apr_file_t **in, apr_file_t **out, apr_pool_t *cont)
{
	char        tname[L_tmpnam+1];
	NXHandle_t	filedes[2];
	int 		err;

	if (!tmpnam(tname))
		return errno;

	if (  !(err = NXFifoOpen(0, tname, NX_O_RDONLY, 0, &filedes[0]))
		&& !(err = NXFifoOpen(0, tname, NX_O_WRONLY, 0, &filedes[1])))
	{
		(*in)->cntxt     =
		(*out)->cntxt    = cont;
		(*in)->filedes   = filedes[0];
		(*out)->filedes  = filedes[1];
		(*in)->pipe      =
		(*out)->pipe     = 1;
		(*out)->fname    =
		(*in)->fname     = NULL;
		(*in)->buffered  =
		(*out)->buffered = 0;
		(*in)->blocking  =
		(*out)->blocking = BLK_ON;
		(*in)->timeout   =
		(*out)->timeout  = -1;
		(*in)->ungetchar = -1;
		(*in)->thlock    =
		(*out)->thlock   = NULL;
	}
	else
	{
		if (filedes[0] != (NXHandle_t) -1)
			NXClose(filedes[0]);

        if (err)
            return convert_error (err);

	}
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_namedpipe_create(const char *filename, 
                                                    apr_fileperms_t perm, apr_pool_t *cont)
{
    mode_t mode = apr_unix_perms2mode(perm);
	NXHandle_t	filedes;
	int err;

	err = NXFifoOpen(0, filename, mode, 0, &filedes);

    if (err)
        return convert_error (err);

    return APR_SUCCESS;
} 

    

