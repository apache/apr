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

static ap_status_t pipeblock(ap_file_t *thepipe)
{
#if !BEOS /* this code won't work on BeOS */
      int fd_flags;

      fd_flags = fcntl(thepipe->filedes, F_GETFL, 0);
  #if defined(O_NONBLOCK)
      fd_flags &= ~O_NONBLOCK;
  #elif defined(~O_NDELAY)
      fd_flags &= ~O_NDELAY;
  #elif defined(FNDELAY)
      fd_flags &= ~O_FNDELAY;
  #else 
      /* XXXX: this breaks things, but an alternative isn't obvious...*/
      return APR_ENOTIMPL;
  #endif
    if (fcntl(thepipe->filedes, F_SETFL, fd_flags) == -1) {
        return errno;
    }

#else

#if BEOS_BONE /* This only works on BONE or beyond */
    int on = 0;
    if (ioctl(thepipe->filedes, FIONBIO, &on, sizeof(on)) < 0)
        return errno;
#else /* BEOS_BONE */
    return APR_ENOTIMPL;
#endif /* BEOS_BONE */
 
#endif /* !BEOS_R5 */

    thepipe->blocking = BLK_ON;
    return APR_SUCCESS;
}

static ap_status_t pipenonblock(ap_file_t *thepipe)
{
#if !BEOS /* this code won't work on BeOS */
      int fd_flags = fcntl(thepipe->filedes, F_GETFL, 0);

  #if defined(O_NONBLOCK)
      fd_flags |= O_NONBLOCK;
  #elif defined(O_NDELAY)
      fd_flags |= O_NDELAY;
  #elif defined(FNDELAY)
      fd_flags |= O_FNDELAY;
  #else
      /* XXXX: this breaks things, but an alternative isn't obvious...*/
      return APR_ENOTIMPL;
  #endif
      if (fcntl(thepipe->filedes, F_SETFL, fd_flags) == -1) {
          return errno;
      }
    
#else /* !BEOS */

#if BEOS_BONE /* This only works on BONE and later...*/
    int on = 1;
    if (ioctl(thepipe->filedes, FIONBIO, &on, sizeof(on)) < 0)
        return errno;
#else /* BEOS_BONE */
    return APR_ENOTIMPL;
#endif /* BEOS_BONE */

#endif /* !BEOS */

    thepipe->blocking = BLK_OFF;
    return APR_SUCCESS;
}

ap_status_t ap_set_pipe_timeout(ap_file_t *thepipe, ap_interval_time_t timeout)
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

ap_status_t ap_create_pipe(ap_file_t **in, ap_file_t **out, ap_pool_t *cont)
{
    int filedes[2];

    if (pipe(filedes) == -1) {
        return errno;
    }
    
    (*in) = (ap_file_t *)ap_pcalloc(cont, sizeof(ap_file_t));
    (*in)->cntxt = cont;
    (*in)->filedes = filedes[0];
    (*in)->pipe = 1;
    (*in)->fname = ap_pstrdup(cont, "PIPE");
    (*in)->buffered = 0;
    (*in)->blocking = BLK_ON;
    (*in)->timeout = -1;
    (*in)->ungetchar = -1;
#if APR_HAS_THREADS
    (*in)->thlock = NULL;
#endif

    (*out) = (ap_file_t *)ap_pcalloc(cont, sizeof(ap_file_t));
    (*out)->cntxt = cont;
    (*out)->filedes = filedes[1];
    (*out)->pipe = 1;
    (*out)->fname = ap_pstrdup(cont, "PIPE");
    (*out)->buffered = 0;
    (*out)->blocking = BLK_ON;
    (*out)->timeout = -1;
#if APR_HAS_THREADS
    (*in)->thlock = NULL;
#endif

    return APR_SUCCESS;
}

ap_status_t ap_create_namedpipe(const char *filename, 
                                ap_fileperms_t perm, ap_pool_t *cont)
{
    mode_t mode = ap_unix_get_fileperms(perm);

    if (mkfifo(filename, mode) == -1) {
        return errno;
    }
    return APR_SUCCESS;
} 

    

