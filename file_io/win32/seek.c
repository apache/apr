/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_arch_file_io.h"
#include "apr_file_io.h"
#include <errno.h>
#include <string.h>

static apr_status_t setptr(apr_file_t *thefile, apr_off_t pos )
{
    apr_off_t newbufpos;
    apr_status_t rv;

    if (thefile->direction == 1) {
        /* XXX: flush here is not mutex protected */
        rv = apr_file_flush(thefile);
        if (rv != APR_SUCCESS)
            return rv;
        thefile->bufpos = thefile->dataRead = 0;
        thefile->direction = 0;
    }

    /* We may be truncating to size here. 
     * XXX: testing an 'unsigned' as >= 0 below indicates a bug
     */
    newbufpos = pos - (thefile->filePtr - thefile->dataRead);

    if (newbufpos >= 0 && newbufpos <= (apr_off_t)thefile->dataRead) {
        thefile->bufpos = (apr_size_t)newbufpos;
        rv = APR_SUCCESS;
    } else {
        LARGE_INTEGER li;
        li.QuadPart = pos;
        
        if (!SetFilePointerEx(thefile->filehand, li, NULL, FILE_BEGIN)) {
            rv = apr_get_os_error();
        }
        else {
            rv = APR_SUCCESS;
        }

        if (rv == APR_SUCCESS) {
            rv = APR_SUCCESS;
            thefile->eof_hit = 0;
            thefile->bufpos = thefile->dataRead = 0;
            thefile->filePtr = pos;
        }
    }

    return rv;
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
                    rc = setptr(thefile, finfo.size + *offset);
                break;

            default:
                return APR_EINVAL;
        }

        *offset = thefile->filePtr - thefile->dataRead + thefile->bufpos;
        return rc;
    }
    /* A file opened with APR_FOPEN_XTHREAD has been opened for overlapped i/o.
     * APR must explicitly track the file pointer in this case.
     */
    else if (thefile->pOverlapped || thefile->flags & APR_FOPEN_XTHREAD) {
        switch(where) {
            case APR_SET:
                thefile->filePtr = *offset;
                break;
        
            case APR_CUR:
                thefile->filePtr += *offset;
                break;
        
            case APR_END:
                rc = apr_file_info_get(&finfo, APR_FINFO_SIZE, thefile);
                if (rc == APR_SUCCESS && finfo.size + *offset >= 0)
                    thefile->filePtr = finfo.size + *offset;
                break;

            default:
                return APR_EINVAL;
        }
        *offset = thefile->filePtr;
        return rc;
    }
    else {
        DWORD howmove;
        LARGE_INTEGER li;
        li.QuadPart = *offset;

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

        if (!SetFilePointerEx(thefile->filehand, li, &li, howmove)) {
            rc = apr_get_os_error();
        }
        else {
            rc = APR_SUCCESS;
        }

        /* Since we can land at 0xffffffff we will measure our APR_SUCCESS */
        if (rc == APR_SUCCESS)
            *offset = li.QuadPart;
        return rc;
    }
}


APR_DECLARE(apr_status_t) apr_file_trunc(apr_file_t *thefile, apr_off_t offset)
{
    apr_status_t rv;
    LARGE_INTEGER li;

    if (thefile->buffered) {
        if (thefile->direction == 1) {
            /* Figure out what needs to be flushed.  Don't flush the part
             * of the write buffer that will get truncated anyway.
             */
            if (offset < thefile->filePtr) {
                thefile->bufpos = 0;
            }
            else if (offset < thefile->filePtr + (apr_off_t)thefile->bufpos) {
                thefile->bufpos = (apr_size_t)(offset - thefile->filePtr);
            }

            if (thefile->bufpos != 0) {
                rv = apr_file_flush(thefile);
                if (rv != APR_SUCCESS)
                    return rv;
            }
        }
        else if (thefile->direction == 0) {
            /* Discard the read buffer, as we are about to reposition
             * ourselves to the end of file.
             */
            thefile->bufpos = 0;
            thefile->dataRead = 0;
        }
    }

    li.QuadPart  = offset;
    if (!SetFilePointerEx(thefile->filehand, li, NULL, FILE_BEGIN))
        return apr_get_os_error();

    thefile->filePtr = offset;
    /* Don't report EOF until the next read. */
    thefile->eof_hit = 0;

    if (!SetEndOfFile(thefile->filehand))
        return apr_get_os_error();

    return APR_SUCCESS;
}
