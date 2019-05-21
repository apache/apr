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

#include "apr.h"
#include <rpc.h>
#include <winnt.h>
#include <bcrypt.h>
#include "apr_private.h"
#include "apr_general.h"
#include "apr_portable.h"
#include "apr_arch_misc.h"


APR_DECLARE(apr_status_t) apr_generate_random_bytes(unsigned char * buf,
                                                    apr_size_t length)
{
    NTSTATUS ntstatus;

    ntstatus = BCryptGenRandom(NULL, buf, (DWORD) length,
                               BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    if (BCRYPT_SUCCESS(ntstatus)) {
        return APR_SUCCESS;
    }
    else if (ntstatus == STATUS_INVALID_PARAMETER) {
        return APR_FROM_OS_ERROR(ERROR_INVALID_PARAMETER);
    }
    else {
        return APR_EGENERAL;
    }
}


APR_DECLARE(apr_status_t) apr_os_uuid_get(unsigned char *uuid_data)
{
    /* Note: this call doesn't actually require CoInitialize() first 
     *
     * XXX: we should scramble the bytes or some such to eliminate the
     * possible misuse/abuse since uuid is based on the NIC address, and
     * is therefore not only a uniqifier, but an identity (which might not
     * be appropriate in all cases.
     *
     * Note that Win2000, XP and later no longer suffer from this problem,
     * a scrambling fix is only needed for (apr_os_level < APR_WIN_2000)
     */
    if (FAILED(UuidCreate((UUID *)uuid_data))) {
        return APR_EGENERAL;
    }
    return APR_SUCCESS;
}
