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

#include "apr.h"
#include "apr_arch_misc.h"
#include "apr_arch_threadproc.h"
#include "apr_arch_file_io.h"

#if APR_HAS_OTHER_CHILD

#ifdef HAVE_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#if APR_HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef BEOS
#include <sys/socket.h> /* for fd_set definition! */
#endif

static apr_other_child_rec_t *other_children = NULL;

static apr_status_t other_child_cleanup(void *data)
{
    apr_other_child_rec_t **pocr, *nocr;

    for (pocr = &other_children; *pocr; pocr = &(*pocr)->next) {
        if ((*pocr)->data == data) {
            nocr = (*pocr)->next;
            (*(*pocr)->maintenance) (APR_OC_REASON_UNREGISTER, (*pocr)->data, -1);
            *pocr = nocr;
            /* XXX: um, well we've just wasted some space in pconf ? */
            return APR_SUCCESS;
        }
    }
    return APR_SUCCESS;
}

APR_DECLARE(void) apr_proc_other_child_register(apr_proc_t *proc,
                     void (*maintenance) (int reason, void *, int status),
                     void *data, apr_file_t *write_fd, apr_pool_t *p)
{
    apr_other_child_rec_t *ocr;

    ocr = apr_palloc(p, sizeof(*ocr));
    ocr->p = p;
    ocr->proc = proc;
    ocr->maintenance = maintenance;
    ocr->data = data;
    if (write_fd == NULL) {
        ocr->write_fd = (apr_os_file_t) -1;
    }
    else {
#ifdef WIN32
        /* This should either go away as part of eliminating apr_proc_probe_writable_fds
         * or write_fd should point to an apr_file_t
         */
        ocr->write_fd = write_fd->filehand; 
#else
        ocr->write_fd = write_fd->filedes;
#endif

    }
    ocr->next = other_children;
    other_children = ocr;
    apr_pool_cleanup_register(p, ocr->data, other_child_cleanup, 
                              apr_pool_cleanup_null);
}

APR_DECLARE(void) apr_proc_other_child_unregister(void *data)
{
    apr_other_child_rec_t *cur;

    cur = other_children;
    while (cur) {
        if (cur->data == data) {
            break;
        }
        cur = cur->next;
    }

    /* segfault if this function called with invalid parm */
    apr_pool_cleanup_kill(cur->p, cur->data, other_child_cleanup);
    other_child_cleanup(data);
}

APR_DECLARE(apr_status_t) apr_proc_other_child_alert(apr_proc_t *proc,
                                                     int reason,
                                                     int status)
{
    apr_other_child_rec_t *ocr, *nocr;

    for (ocr = other_children; ocr; ocr = nocr) {
        nocr = ocr->next;
        if (ocr->proc->pid != proc->pid)
            continue;

        ocr->proc = NULL;
        (*ocr->maintenance) (reason, ocr->data, status);
        return APR_SUCCESS;
    }
    return APR_EPROC_UNKNOWN;
}

APR_DECLARE(void) apr_proc_other_child_refresh(apr_other_child_rec_t *ocr,
                                               int reason)
{
    /* Todo: 
     * Implement code to detect if pipes are still alive.
     */
#ifdef WIN32
    DWORD status;

    if (ocr->proc == NULL)
        return;

    if (!ocr->proc->hproc) {
        /* Already mopped up, perhaps we apr_proc_kill'ed it,
         * they should have already unregistered!
         */
        ocr->proc = NULL;
        (*ocr->maintenance) (APR_OC_REASON_LOST, ocr->data, -1);
    }
    else if (!GetExitCodeProcess(ocr->proc->hproc, &status)) {
        CloseHandle(ocr->proc->hproc);
        ocr->proc->hproc = NULL;
        ocr->proc = NULL;
        (*ocr->maintenance) (APR_OC_REASON_LOST, ocr->data, -1);
    }
    else if (status == STILL_ACTIVE) {
        (*ocr->maintenance) (reason, ocr->data, -1);
    }
    else {
        CloseHandle(ocr->proc->hproc);
        ocr->proc->hproc = NULL;
        ocr->proc = NULL;
        (*ocr->maintenance) (APR_OC_REASON_DEATH, ocr->data, status);
    }

#else /* ndef Win32 */
    pid_t waitret; 
    int status;

    if (ocr->proc == NULL)
        return;

    waitret = waitpid(ocr->proc->pid, &status, WNOHANG);
    if (waitret == ocr->proc->pid) {
        ocr->proc = NULL;
        (*ocr->maintenance) (APR_OC_REASON_DEATH, ocr->data, status);
    }
    else if (waitret == 0) {
        (*ocr->maintenance) (reason, ocr->data, -1);
    }
    else if (waitret == -1) {
        /* uh what the heck? they didn't call unregister? */
        ocr->proc = NULL;
        (*ocr->maintenance) (APR_OC_REASON_LOST, ocr->data, -1);
    }
#endif
}

APR_DECLARE(void) apr_proc_other_child_refresh_all(int reason)
{
    apr_other_child_rec_t *ocr, *next_ocr;

    for (ocr = other_children; ocr; ocr = next_ocr) {
        next_ocr = ocr->next;
        apr_proc_other_child_refresh(ocr, reason);
    }
}

#else /* !APR_HAS_OTHER_CHILD */

APR_DECLARE(void) apr_proc_other_child_register(apr_proc_t *proc,
                     void (*maintenance) (int reason, void *, int status),
                     void *data, apr_file_t *write_fd, apr_pool_t *p)
{
    return;
}

APR_DECLARE(void) apr_proc_other_child_unregister(void *data)
{
    return;
}

APR_DECLARE(apr_status_t) apr_proc_other_child_alert(apr_proc_t *proc,
                                                     int reason,
                                                     int status)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(void) apr_proc_other_child_refresh(apr_other_child_rec_t *ocr,
                                               int reason)
{
    return;
}

APR_DECLARE(void) apr_proc_other_child_refresh_all(int reason)
{
    return;
}

#endif /* APR_HAS_OTHER_CHILD */


/* XXX: deprecated for removal in 1.0
 * The checks behaved differently between win32 and unix, while
 * the old win32 code did a health check, the unix code called
 * other_child_check only at restart.
 */
APR_DECLARE(void) apr_proc_other_child_check(void)
{
#ifdef WIN32
    apr_proc_other_child_refresh_all(APR_OC_REASON_RUNNING);
#else
    apr_proc_other_child_refresh_all(APR_OC_REASON_RESTART);
#endif
}

/* XXX: deprecated for removal in 1.0
 * This really didn't test any sort of read - it simply notified
 * the maintenance function that the process had died.
 */
APR_DECLARE(apr_status_t) apr_proc_other_child_read(apr_proc_t *proc, int status)
{
    return apr_proc_other_child_alert(proc, APR_OC_REASON_DEATH, status);
}

