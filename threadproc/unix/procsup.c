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

#include "apr_arch_threadproc.h"

APR_DECLARE(apr_status_t) apr_proc_detach(int daemonize)
{
    int x;

    chdir("/");
#if !defined(MPE) && !defined(OS2) && !defined(TPF) && !defined(BEOS)
    /* Don't detach for MPE because child processes can't survive the death of
     * the parent. */
    if (daemonize) {
	    if ((x = fork()) > 0) {
	        exit(0);
        }
	    else if (x == -1) {
	        perror("fork");
	        fprintf(stderr, "unable to fork new process\n");
	        exit(1);  /* we can't do anything here, so just exit. */
	    }
	    /* RAISE_SIGSTOP(DETACH); */
    }
#endif

#ifdef HAVE_SETSID
    if (setsid() == -1) {
        return errno;
    }
#elif defined(NEXT) || defined(NEWSOS)
    if (setpgrp(0, getpid()) == -1) {
        return errno;
    }
#elif defined(OS2) || defined(TPF) || defined(MPE)
    /* do nothing */
#else
    if (setpgid(0, 0) == -1) {
        return errno;
    }
#endif

    /* close out the standard file descriptors */
    if (freopen("/dev/null", "r", stdin) == NULL) {
        return errno;
        /* continue anyhow -- note we can't close out descriptor 0 because we
         * have nothing to replace it with, and if we didn't have a descriptor
         * 0 the next file would be created with that value ... leading to
         * havoc.
         */
    }
    if (freopen("/dev/null", "w", stdout) == NULL) {
        return errno;
    }
     /* We are going to reopen this again in a little while to the error
      * log file, but better to do it twice and suffer a small performance
      * hit for consistancy than not reopen it here.
      */
    if (freopen("/dev/null", "w", stderr) == NULL) {
        return errno;
    }
    return APR_SUCCESS;
}

#if (!HAVE_WAITPID)
/* From ikluft@amdahl.com
 * this is not ideal but it works for SVR3 variants
 * Modified by dwd@bell-labs.com to call wait3 instead of wait because
 *   apache started to use the WNOHANG option.
 */
int waitpid(pid_t pid, int *statusp, int options)
{
    int tmp_pid;
    if (kill(pid, 0) == -1) {
        errno = ECHILD;
        return -1;
    }
    while (((tmp_pid = wait3(statusp, options, 0)) != pid) &&
                (tmp_pid != -1) && (tmp_pid != 0) && (pid != -1))
        ;
    return tmp_pid;
}
#endif

