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

APR_DECLARE(apr_status_t) apr_file_lock(apr_file_t *thefile, int type)
{
    const DWORD len = 0xffffffff;
    DWORD flags;

    flags = ((type & APR_FLOCK_NONBLOCK) ? LOCKFILE_FAIL_IMMEDIATELY : 0)
          + (((type & APR_FLOCK_TYPEMASK) == APR_FLOCK_SHARED)
                                       ? 0 : LOCKFILE_EXCLUSIVE_LOCK);
    /* Syntax is correct, len is passed for LengthLow and LengthHigh*/
    OVERLAPPED offset;
    memset (&offset, 0, sizeof(offset));
    if (!LockFileEx(thefile->filehand, flags, 0, len, len, &offset))
        return apr_get_os_error();

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_unlock(apr_file_t *thefile)
{
    DWORD len = 0xffffffff;

    /* Syntax is correct, len is passed for LengthLow and LengthHigh*/
    OVERLAPPED offset;
    memset (&offset, 0, sizeof(offset));
    if (!UnlockFileEx(thefile->filehand, 0, len, len, &offset))
        return apr_get_os_error();

    return APR_SUCCESS;
}
