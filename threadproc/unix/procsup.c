/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
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
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "threadproc.h"
#include "fileio.h"

#include "apr_config.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"

/* ***APRDOC********************************************************
 * ap_status_t ap_detach(ap_context_t *, ap_proc_t *)
 *    Detach the process from the controlling terminal.
 * arg 1) The context to use if it is needed.
 * arg 2) The new process handler
 */
ap_status_t ap_detach(struct proc_t **new, ap_context_t *cont)
{
    int x;

    (*new) = (struct proc_t *)ap_palloc(cont, sizeof(struct proc_t));
    (*new)->cntxt = cont;
    (*new)->attr = NULL;

    chdir("/");
#if !defined(MPE) && !defined(OS2) && !defined(TPF)
/* Don't detach for MPE because child processes can't survive the death of
   the parent. */
    if ((x = fork()) > 0)
        exit(0);
    else if (x == -1) {
        perror("fork");
        fprintf(stderr, "unable to fork new process\n");
        exit(1);  /* we can't do anything here, so just exit. */
    }
/*    RAISE_SIGSTOP(DETACH);*/
#endif
#if HAVE_SETSID
    if (((*new)->pid = setsid()) == -1) {
        return errno;
    }
#elif defined(NEXT) || defined(NEWSOS)
    if (setpgrp(0, getpid()) == -1 || ((*new)->pid = getpgrp(0)) == -1) {
        return errno;
    }
#elif defined(OS2) || defined(TPF)
    /* OS/2 don't support process group IDs */
    (*new)->pid = getpid();
#elif defined(MPE)
    /* MPE uses negative pid for process group */
     (*new)->pid = -getpid();
#else
    if (((*new)->pid = setpgrp(getpid(), 0)) == -1) {
        return errno;
    }
#endif

    /* close out the standard file descriptors */
    if (freopen("/dev/null", "r", stdin) == NULL) {
        return APR_ALLSTD;
        /* continue anyhow -- note we can't close out descriptor 0 because we
         * have nothing to replace it with, and if we didn't have a descriptor
         * 0 the next file would be created with that value ... leading to
         * havoc.
         */
    }
    if (freopen("/dev/null", "w", stdout) == NULL) {
        return APR_STDOUT;
    }
     /* We are going to reopen this again in a little while to the error
      * log file, but better to do it twice and suffer a small performance
      * hit for consistancy than not reopen it here.
      */
    if (freopen("/dev/null", "w", stderr) == NULL) {
        return APR_STDERR;
    }

    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_procdata(ap_proc_t *, char *, void *)
 *    Return the context associated with the current proc.
 * arg 1) The currently open proc.
 * arg 2) The user data associated with the proc.
 */
ap_status_t ap_get_procdata(struct proc_t *proc, char *key, void *data)
{
    if (proc != NULL) {
        return ap_get_userdata(&data, proc->cntxt, key);
    }
    else {
        data = NULL;
        return APR_ENOPROC;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_procdata(ap_proc_t *, void *, char *key,
                               ap_status_t (*cleanup) (void *))
 *    Return the context associated with the current proc.
 * arg 1) The currently open proc.
 * arg 2) The user data to associate with the proc.
*/
ap_status_t ap_set_procdata(struct proc_t *proc, void *data, char *key, 
                            ap_status_t (*cleanup) (void *))
{
    if (proc != NULL) {
        return ap_set_userdata(proc->cntxt, data, key, cleanup);
    }
    else {
        data = NULL;
        return APR_ENOPROC;
    }
}

