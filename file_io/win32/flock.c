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

APR_DECLARE(apr_status_t) apr_file_lock(apr_file_t *thefile, int type)
{
#ifdef _WIN32_WCE
    /* The File locking is unsuported on WCE */
    return APR_ENOTIMPL;
#else
    const DWORD len = 0xffffffff;
    DWORD flags; 

    flags = ((type & APR_FLOCK_NONBLOCK) ? LOCKFILE_FAIL_IMMEDIATELY : 0)
          + (((type & APR_FLOCK_TYPEMASK) == APR_FLOCK_SHARED) 
                                       ? 0 : LOCKFILE_EXCLUSIVE_LOCK);
    if (apr_os_level >= APR_WIN_NT) {
        /* Syntax is correct, len is passed for LengthLow and LengthHigh*/
        OVERLAPPED offset;
        memset (&offset, 0, sizeof(offset));
        if (!LockFileEx(thefile->filehand, flags, 0, len, len, &offset))
            return apr_get_os_error();
    }
    else {
        if (!LockFile(thefile->filehand, 0, 0, len, 0))
            return apr_get_os_error();
    }

    return APR_SUCCESS;
#endif /* !defined(_WIN32_WCE) */
}

APR_DECLARE(apr_status_t) apr_file_unlock(apr_file_t *thefile)
{
#ifdef _WIN32_WCE
    return APR_ENOTIMPL;
#else
    DWORD len = 0xffffffff;

    if (apr_os_level >= APR_WIN_NT) {
        /* Syntax is correct, len is passed for LengthLow and LengthHigh*/
        OVERLAPPED offset;
        memset (&offset, 0, sizeof(offset));
        if (!UnlockFileEx(thefile->filehand, 0, len, len, &offset))
            return apr_get_os_error();
    }
    else {
        if (!UnlockFile(thefile->filehand, 0, 0, len, 0))
            return apr_get_os_error();
    }

    return APR_SUCCESS;
#endif /* !defined(_WIN32_WCE) */
}
