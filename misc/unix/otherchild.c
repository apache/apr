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

#ifdef APR_HAS_OTHER_CHILD

static other_child_rec *other_children = NULL;

API_EXPORT(void) ap_register_other_child(ap_proc_t *pid,
                     void (*maintenance) (int reason, void *),
                     void *data, int write_fd)
{
    other_child_rec *ocr;

    ocr = ap_palloc(pconf, sizeof(*ocr));
    ocr->pid = pid->pid;
    ocr->maintenance = maintenance;
    ocr->data = data;
    ocr->write_fd = write_fd;
    ocr->next = other_children;
    other_children = ocr;
}

/* note that since this can be called by a maintenance function while we're
 * scanning the other_children list, all scanners should protect themself
 * by loading ocr->next before calling any maintenance function.
 */
API_EXPORT(void) ap_unregister_other_child(void *data)
{
    other_child_rec **pocr, *nocr;

    for (pocr = &other_children; *pocr; pocr = &(*pocr)->next) {
        if ((*pocr)->data == data) {
            nocr = (*pocr)->next;
            (*(*pocr)->maintenance) (OC_REASON_UNREGISTER, (*pocr)->data);
            *pocr = nocr;
            /* XXX: um, well we've just wasted some space in pconf ? */
            return;
        }
    }
}

/* test to ensure that the write_fds are all still writable, otherwise
 * invoke the maintenance functions as appropriate */
static void probe_writable_fds(void)
{
    fd_set writable_fds;
    int fd_max;
    other_child_rec *ocr, *nocr;                                                    struct timeval tv;
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
        rc = ap_select(fd_max + 1, NULL, &writable_fds, NULL, &tv);
    } while (rc == -1 && errno == EINTR);

    if (rc == -1) {
        /* XXX: uhh this could be really bad, we could have a bad file
         * descriptor due to a bug in one of the maintenance routines */
        ap_log_unixerr("probe_writable_fds", "select",
                    "could not probe writable fds", server_conf);
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
        (*ocr->maintenance) (OC_REASON_UNWRITABLE, ocr->data, -1);
    }
}

/* possibly reap an other_child, return 0 if yes, -1 if not */
API_EXPORT(int) reap_other_child(int pid)
{
    other_child_rec *ocr, *nocr;

    for (ocr = other_children; ocr; ocr = nocr) {
        nocr = ocr->next;
        if (ocr->pid != pid)
            continue;
        ocr->pid = -1;
        (*ocr->maintenance) (OC_REASON_DEATH, ocr->data);
        return 0;
    }
    return -1;
}
#endif

