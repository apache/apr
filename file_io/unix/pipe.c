/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "fileio.h"

static ap_status_t pipenonblock(struct file_t *thefile)
{
#ifndef BEOS /* this code won't work on BeOS */
    int fd_flags = fcntl(thefile->filedes, F_GETFL, 0);

#if defined(O_NONBLOCK)
    fd_flags |= O_NONBLOCK;
#elif defined(O_NDELAY)
    fd_flags |= O_NDELAY;
#elif defined(FNDELAY)
    fd_flags |= O_FNDELAY;
#else */
    /* XXXX: this breaks things, but an alternative isn't obvious...*/
    return -1;
#endif
    if (fcntl(thefile->filedes, F_SETFL, fd_flags) == -1) {
        return errno;
    }
#endif /* !BeOS */
    return APR_SUCCESS;
}


/* ***APRDOC********************************************************
 * ap_status_t ap_set_pipe_timeout(ap_file_t *, ap_int32_t)
 *    Set the timeout value for a pipe.
 * arg 1) The pipe we are setting a timeout on.
 * arg 3) The timeout value in seconds.  Values < 0 mean wait forever, 0
 *        means do not wait at all.
 */
ap_status_t ap_set_pipe_timeout(struct file_t *thepipe, ap_int32_t timeout)
{
    if (thepipe->pipe == 1) {
        thepipe->timeout = timeout;
        return APR_SUCCESS;
    }
    return APR_EINVAL;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_create_pipe(ap_file_t **, ap_context_t *, ap_file_t **)
 *    Create an anonymous pipe.
 * arg 1) The context to operate on.
 * arg 2) The file descriptor to use as input to the pipe.
 * arg 3) The file descriptor to use as output from the pipe.
 */
ap_status_t ap_create_pipe(struct file_t **in, struct file_t **out, ap_context_t *cont)
{
    int filedes[2];

    if (pipe(filedes) == -1) {
        return errno;
    }
    
    (*in) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));
    (*in)->cntxt = cont;
    (*in)->filedes = filedes[0];
    (*in)->buffered = 0;
    (*in)->pipe = 1;
    (*in)->fname = ap_pstrdup(cont, "PIPE");
    (*in)->timeout = -1;

    (*out) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));
    (*out)->cntxt = cont;
    (*out)->filedes = filedes[1];
    (*out)->buffered = 0;
    (*out)->pipe = 1;
    (*out)->fname = ap_pstrdup(cont, "PIPE");
    (*out)->timeout = -1;

    pipenonblock(*in);
    pipenonblock(*out);

    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_create_namedpipe(ap_context_t *, char *, ap_fileperms_t, 
 *                                 char **)
 *    Create a named pipe.
 * arg 1) The context to operate on.
 * arg 2) The directory to create the pipe in.
 * arg 3) The permissions for the newly created pipe.
 * arg 4) The name of the new pipe. 
 */
ap_status_t ap_create_namedpipe(char **new, char *dirpath, 
                                ap_fileperms_t perm, ap_context_t *cont)
{
    mode_t mode = get_fileperms(perm);

    *new = tempnam(dirpath, NULL);
    if (mkfifo((*new), mode) == -1) {
        return errno;
    }
    return APR_SUCCESS;
} 

ap_status_t ap_block_pipe(ap_file_t *thefile)
{
#ifndef BEOS /* this code won't work on BeOS */
    int fd_flags;

    fd_flags = fcntl(thefile->filedes, F_GETFL, 0);
#if defined(O_NONBLOCK)
    fd_flags &= ~O_NONBLOCK;
#elif defined(~O_NDELAY)
    fd_flags &= ~O_NDELAY;
#elif defined(FNDELAY)
    fd_flags &= ~O_FNDELAY;
#else 
    /* XXXX: this breaks things, but an alternative isn't obvious...*/
    return -1;
#endif
    if (fcntl(thefile->filedes, F_SETFL, fd_flags) == -1) {
        return errno;
    }
#endif /* !BeOS */
    return APR_SUCCESS;
}
    

