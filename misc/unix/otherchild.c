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

#include "misc.h"
#include "threadproc.h"
#include "../../file_io/unix/fileio.h"
#ifdef HAVE_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

static ap_other_child_rec_t *other_children = NULL;

API_EXPORT(void) ap_register_other_child(ap_proc_t *pid,
                     void (*maintenance) (int reason, void *, int status),
                     void *data, ap_file_t *write_fd, ap_pool_t *p)
{
    ap_other_child_rec_t *ocr;

    ocr = ap_palloc(p, sizeof(*ocr));
    ocr->id = pid->pid;
    ocr->maintenance = maintenance;
    ocr->data = data;
    if (write_fd == NULL) {
        ocr->write_fd = -1;
    }
    else {
        ocr->write_fd = write_fd->filedes;
    }
    ocr->next = other_children;
    other_children = ocr;
}

API_EXPORT(void) ap_unregister_other_child(void *data)
{
    ap_other_child_rec_t **pocr, *nocr;

    for (pocr = &other_children; *pocr; pocr = &(*pocr)->next) {
        if ((*pocr)->data == data) {
            nocr = (*pocr)->next;
            (*(*pocr)->maintenance) (APR_OC_REASON_UNREGISTER, (*pocr)->data, -1);
            *pocr = nocr;
            /* XXX: um, well we've just wasted some space in pconf ? */
            return;
        }
    }
}

/* test to ensure that the write_fds are all still writable, otherwise
 * invoke the maintenance functions as appropriate */
void ap_probe_writable_fds(void)
{
    fd_set writable_fds;
    int fd_max;
    ap_other_child_rec_t *ocr, *nocr; 
    struct timeval tv; 
    int rc;

    if (other_children == NULL)
        return;

    fd_max = 0;
    FD_ZERO(&writable_fds);
    do {
        for (ocr = other_children; ocr; ocr = ocr->next) {
            if (ocr->write_fd == -1)
                continue;
            FD_SET(ocr->write_fd, &writable_fds);
            if (ocr->write_fd > fd_max) {
                fd_max = ocr->write_fd;
            }
        }
        if (fd_max == 0)
            return;

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        rc = select(fd_max + 1, NULL, &writable_fds, NULL, &tv);
    } while (rc == -1 && errno == EINTR);

    if (rc == -1) {
        /* XXX: uhh this could be really bad, we could have a bad file
         * descriptor due to a bug in one of the maintenance routines */
        return;
    }
    if (rc == 0)
        return;

    for (ocr = other_children; ocr; ocr = nocr) {
        nocr = ocr->next;
        if (ocr->write_fd == -1)
            continue;
        if (FD_ISSET(ocr->write_fd, &writable_fds))
            continue;
        (*ocr->maintenance) (APR_OC_REASON_UNWRITABLE, ocr->data, -1);
    }
}

API_EXPORT(ap_status_t) ap_reap_other_child(ap_proc_t *pid, int status)
{
    ap_other_child_rec_t *ocr, *nocr;

    for (ocr = other_children; ocr; ocr = nocr) {
        nocr = ocr->next;
        if (ocr->id != pid->pid)
            continue;

        ocr->id = -1;
        (*ocr->maintenance) (APR_OC_REASON_DEATH, ocr->data, status);
        return 0;
    }
    return APR_CHILD_NOTDONE;
}

API_EXPORT(void) ap_check_other_child(void)
{
    ap_other_child_rec_t *ocr, *nocr;
    pid_t waitret; 
    int status;

    for (ocr = other_children; ocr; ocr = nocr) {
        nocr = ocr->next;
        if (ocr->id == -1)
            continue;

        waitret = waitpid(ocr->id, &status, WNOHANG);
        if (waitret == ocr->id) {
            ocr->id = -1;
            (*ocr->maintenance) (APR_OC_REASON_DEATH, ocr->data, status);
        }
        else if (waitret == 0) {
            (*ocr->maintenance) (APR_OC_REASON_RESTART, ocr->data, -1);
        }
        else if (waitret == -1) {
            /* uh what the heck? they didn't call unregister? */
            ocr->id = -1;
            (*ocr->maintenance) (APR_OC_REASON_LOST, ocr->data, -1);
        }
    }
}
