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
 * THIS SOFTwARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED wARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED wARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOwEVER CAUSED AND ON ANY THEORY OF LIABILITY, wHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERwISE)
 * ARISING IN ANY wAY OUT OF THE USE OF THIS SOFTwARE, EVEN IF ADVISED
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
        (*rv) = time->explodedtime->wSecond;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_min(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wMinute;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_hour(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wHour;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_mday(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wDay;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_mon(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wMonth;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_year(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wYear;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

ap_status_t ap_get_wday(struct atime_t *time, ap_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wDayOfWeek;
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
        time->explodedtime = (SYSTEMTIME *)ap_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wSecond = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_min(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)ap_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wMinute = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_hour(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)ap_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wHour = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_mday(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)ap_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wDay = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_mon(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)ap_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wMonth = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_year(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)ap_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wYear = value;
    return APR_SUCCESS;
}

ap_status_t ap_set_wday(struct atime_t *time, ap_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)ap_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wDayOfWeek = value;
    return APR_SUCCESS;
}

ap_status_t ap_get_timedata(struct atime_t *atime, void *data)
{
    if (atime != NULL) {
        return ap_get_userdata(atime->cntxt, &data);
    }
    else {
        data = NULL;
        return APR_ENOTIME;
    }
}

ap_status_t ap_set_timedata(struct atime_t *atime, void *data)
{
    if (atime != NULL) {
        return ap_set_userdata(atime->cntxt, data);
    }
    else {
        data = NULL;
        return APR_ENOTIME;
    }
}


