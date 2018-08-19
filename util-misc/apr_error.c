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
 * levelations under the License.
 */

#include "apu.h"
#include "apr_strings.h"
#include "apr_pools.h"
#include "apu_errno.h"

APR_DECLARE_NONSTD(apr_status_t) apr_errprintf(apu_err_t **result,
        apr_pool_t *p, const char *reason, int rc, const char *fmt, ...)
{
    va_list ap;
    apu_err_t *res;

    res = *result;
    if (!res) {
        res = *result = apr_pcalloc(p, sizeof(apu_err_t));
        if (!res) {
            return APR_ENOMEM;
        }
    }

    va_start(ap, fmt);
    res->msg = apr_pvsprintf(p, fmt, ap);
    va_end(ap);

    res->reason = reason;
    res->rc = rc;

    return APR_SUCCESS;
}

