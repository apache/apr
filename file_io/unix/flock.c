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

#include "apr_arch_file_io.h"

#if APR_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

APR_DECLARE(apr_status_t) apr_file_lock(apr_file_t *thefile, int type)
{
    int rc;

#if defined(HAVE_FCNTL_H)
    {
        struct flock l = { 0 };
        int fc;

        l.l_whence = SEEK_SET;  /* lock from current point */
        l.l_start = 0;          /* begin lock at this offset */
        l.l_len = 0;            /* lock to end of file */
        if ((type & APR_FLOCK_TYPEMASK) == APR_FLOCK_SHARED)
            l.l_type = F_RDLCK;
        else
            l.l_type = F_WRLCK;

        fc = (type & APR_FLOCK_NONBLOCK) ? F_SETLK : F_SETLKW;

        /* keep trying if fcntl() gets interrupted (by a signal) */
        while ((rc = fcntl(thefile->filedes, fc, &l)) < 0 && errno == EINTR)
            continue;

        if (rc == -1) {
            /* on some Unix boxes (e.g., Tru64), we get EACCES instead
             * of EAGAIN; we don't want APR_STATUS_IS_EAGAIN() matching EACCES
             * since that breaks other things, so fix up the retcode here
             */
            if (errno == EACCES) {
                return EAGAIN;
            }
            return errno;
        }
    }
#elif defined(HAVE_SYS_FILE_H)
    {
        int ltype;

        if ((type & APR_FLOCK_TYPEMASK) == APR_FLOCK_SHARED)
            ltype = LOCK_SH;
        else
            ltype = LOCK_EX;
        if ((type & APR_FLOCK_NONBLOCK) != 0)
            ltype |= LOCK_NB;

        /* keep trying if flock() gets interrupted (by a signal) */
        while ((rc = flock(thefile->filedes, ltype)) < 0 && errno == EINTR)
            continue;

        if (rc == -1)
            return errno;
    }
#else
#error No file locking mechanism is available.
#endif

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_unlock(apr_file_t *thefile)
{
    int rc;

#if defined(HAVE_FCNTL_H)
    {
        struct flock l = { 0 };

        l.l_whence = SEEK_SET;  /* lock from current point */
        l.l_start = 0;          /* begin lock at this offset */
        l.l_len = 0;            /* lock to end of file */
        l.l_type = F_UNLCK;

        /* keep trying if fcntl() gets interrupted (by a signal) */
        while ((rc = fcntl(thefile->filedes, F_SETLKW, &l)) < 0
               && errno == EINTR)
            continue;

        if (rc == -1)
            return errno;
    }
#elif defined(HAVE_SYS_FILE_H)
    {
        /* keep trying if flock() gets interrupted (by a signal) */
        while ((rc = flock(thefile->filedes, LOCK_UN)) < 0 && errno == EINTR)
            continue;

        if (rc == -1)
            return errno;
    }
#else
#error No file locking mechanism is available.
#endif

    return APR_SUCCESS;
}
