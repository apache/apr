/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "atime.h"
#include "apr_time.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <errno.h>
#include <string.h>

ap_status_t ap_get_curtime(struct atime_t *time, ap_int64_t *rv)
{
    if (time) {
        (*rv) = time->currtime;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;    
}

ap_status_t ap_get_sec(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->tm_sec;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_min(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->tm_min;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_hour(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->tm_hour;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_mday(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->tm_mday;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_mon(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->tm_mon;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_year(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->tm_year;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_wday(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->tm_wday;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_set_sec(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (struct tm *)ap_palloc(time->cntxt, 
                              sizeof(struct tm));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->tm_sec = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_min(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (struct tm *)ap_palloc(time->cntxt, 
                              sizeof(struct tm));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->tm_min = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_hour(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (struct tm *)ap_palloc(time->cntxt, 
                              sizeof(struct tm));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->tm_hour = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_mday(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (struct tm *)ap_palloc(time->cntxt, 
                              sizeof(struct tm));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->tm_mday = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_mon(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (struct tm *)ap_palloc(time->cntxt, 
                              sizeof(struct tm));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->tm_mon = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_year(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (struct tm *)ap_palloc(time->cntxt, 
                              sizeof(struct tm));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->tm_year = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_wday(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (struct tm *)ap_palloc(time->cntxt, 
                              sizeof(struct tm));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->tm_wday = value;
    return APR_SUCCESS;
}


