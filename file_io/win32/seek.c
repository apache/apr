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

#include "win32/apr_arch_file_io.h"
#include "apr_file_io.h"
#include <errno.h>
#include <string.h>

static apr_status_t setptr(apr_file_t *thefile, apr_off_t pos )
{
    apr_off_t newbufpos;
    DWORD rc;

    if (thefile->direction == 1) {
        apr_file_flush(thefile);
        thefile->bufpos = thefile->direction = thefile->dataRead = 0;
    }

    newbufpos = pos - (thefile->filePtr - thefile->dataRead);

    if (newbufpos >= 0 && newbufpos <= thefile->dataRead) {
        thefile->bufpos = (apr_size_t)newbufpos;
        rc = 0;
    } else {
        DWORD offlo = (DWORD)pos;
        DWORD offhi = (DWORD)(pos >> 32);
        rc = SetFilePointer(thefile->filehand, offlo, &offhi, FILE_BEGIN);

        if (rc == 0xFFFFFFFF)
            rc = apr_get_os_error();
        else
            rc = APR_SUCCESS;
        if (rc == APR_SUCCESS) {
            thefile->eof_hit = thefile->bufpos = thefile->dataRead = 0;
            thefile->filePtr = pos;
        }
    }

    return rc;
}


APR_DECLARE(apr_status_t) apr_file_seek(apr_file_t *thefile, apr_seek_where_t where, apr_off_t *offset)
{
    apr_finfo_t finfo;
    apr_status_t rc = APR_SUCCESS;

    thefile->eof_hit = 0;

    if (thefile->buffered) {
        switch (where) {
            case APR_SET:
                rc = setptr(thefile, *offset);
                break;

            case APR_CUR:
                rc = setptr(thefile, thefile->filePtr - thefile->dataRead 
                                      + thefile->bufpos + *offset);
                break;

            case APR_END:
                rc = apr_file_info_get(&finfo, APR_FINFO_SIZE, thefile);
                if (rc == APR_SUCCESS)
                    rc = setptr(thefile, finfo.size - *offset);
                break;

            default:
                return APR_EINVAL;
        }

        *offset = thefile->filePtr - thefile->dataRead + thefile->bufpos;
        return rc;
    } 
    else if (thefile->pOverlapped) {
        switch(where) {
            case APR_SET:
                thefile->filePtr = *offset;
                break;
        
            case APR_CUR:
                thefile->filePtr += *offset;
                break;
        
            case APR_END:
                rc = apr_file_info_get(&finfo, APR_FINFO_SIZE, thefile);
                if (rc == APR_SUCCESS && finfo.size - *offset < 0)
                    thefile->filePtr = finfo.size - *offset;
                break;

            default:
                return APR_EINVAL;
        }
        *offset = thefile->filePtr;
        return rc;
    }
    else {
        DWORD howmove;
        DWORD offlo = (DWORD)*offset;
        DWORD offhi = (DWORD)(*offset >> 32);

        switch(where) {
            case APR_SET:
                howmove = FILE_BEGIN;   break;
            case APR_CUR:
                howmove = FILE_CURRENT; break;
            case APR_END:
                howmove = FILE_END;     break;
            default:
                return APR_EINVAL;
        }
        offlo = SetFilePointer(thefile->filehand, (LONG)offlo, 
                               (LONG*)&offhi, howmove);
        if (offlo == 0xFFFFFFFF)
            rc = apr_get_os_error();
        else
            rc = APR_SUCCESS;
        /* Since we can land at 0xffffffff we will measure our APR_SUCCESS */
        if (rc == APR_SUCCESS)
            *offset = ((apr_off_t)offhi << 32) | offlo;
        return rc;
    }
}


APR_DECLARE(apr_status_t) apr_file_trunc(apr_file_t *thefile, apr_off_t offset)
{
    apr_status_t rv;
    DWORD offlo = (DWORD)offset;
    DWORD offhi = (DWORD)(offset >> 32);
    DWORD rc;

    rc = SetFilePointer(thefile->filehand, offlo, &offhi, FILE_BEGIN);
    if (rc == 0xFFFFFFFF)
        if ((rv = apr_get_os_error()) != APR_SUCCESS)
            return rv;

    if (!SetEndOfFile(thefile->filehand))
        return apr_get_os_error();

    if (thefile->buffered) {
        return setptr(thefile, offset);
    }

    return APR_SUCCESS;
}
