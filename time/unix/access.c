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

/* ***APRDOC********************************************************
 * ap_status_t ap_get_curtime(ap_time_t *, ap_int64_t *)
 *    Get the current time in seconds since Jan 1, 1970.
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_get_curtime(struct atime_t *atime, ap_int64_t *rv)
{
    if (atime) {
        (*rv) = atime->currtime->tv_sec;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;    
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_sec(ap_time_t *, ap_int64_t *)
 *    Get the number of seconds since the top of the minute
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_get_sec(struct atime_t *atime, ap_int32_t *rv)
{
    if (atime) {
        (*rv) = atime->explodedtime->tm_sec;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_min(ap_time_t *, ap_int64_t *)
 *    Get the number of minutes since the top of the hour 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_get_min(struct atime_t *atime, ap_int32_t *rv)
{
    if (atime) {
        (*rv) = atime->explodedtime->tm_min;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_min(ap_time_t *, ap_int64_t *)
 *    Get the number of minutes since the top of the hour 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_get_hour(struct atime_t *atime, ap_int32_t *rv)
{
    if (atime) {
        (*rv) = atime->explodedtime->tm_hour;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_mday(ap_time_t *, ap_int64_t *)
 *    Get the number of days since the beginning of the month 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_get_mday(struct atime_t *atime, ap_int32_t *rv)
{
    if (atime) {
        (*rv) = atime->explodedtime->tm_mday;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_mon(ap_time_t *, ap_int64_t *)
 *    Get the number of months since the beginning of the year 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_get_mon(struct atime_t *atime, ap_int32_t *rv)
{
    if (atime) {
        (*rv) = atime->explodedtime->tm_mon;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_year(ap_time_t *, ap_int64_t *)
 *    Get the number of years since 1900 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_get_year(struct atime_t *atime, ap_int32_t *rv)
{
    if (atime) {
        (*rv) = atime->explodedtime->tm_year;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_wday(ap_time_t *, ap_int64_t *)
 *    Get the number of days since the beginning of the week. 0 == Sunday 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_get_wday(struct atime_t *atime, ap_int32_t *rv)
{
    if (atime) {
        (*rv) = atime->explodedtime->tm_wday;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_sec(ap_time_t *, ap_int64_t)
 *    Set the number of sec since the top of the minute 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_set_sec(struct atime_t *atime, ap_int32_t value)
{
    if (!atime) {
        return APR_ENOTIME;
    }
    if (atime->explodedtime == NULL) {
        atime->explodedtime = (struct tm *)ap_palloc(atime->cntxt, 
                              sizeof(struct tm));
    }
    if (atime->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    atime->explodedtime->tm_sec = value;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_min(ap_time_t *, ap_int64_t)
 *    Set the number of minutes since the top of the hour 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_set_min(struct atime_t *atime, ap_int32_t value)
{
    if (!atime) {
        return APR_ENOTIME;
    }
    if (atime->explodedtime == NULL) {
        atime->explodedtime = (struct tm *)ap_palloc(atime->cntxt, 
                              sizeof(struct tm));
    }
    if (atime->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    atime->explodedtime->tm_min = value;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_min(ap_time_t *, ap_int64_t)
 *    Set the number of hours since the beginning of the day 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_set_hour(struct atime_t *atime, ap_int32_t value)
{
    if (!atime) {
        return APR_ENOTIME;
    }
    if (atime->explodedtime == NULL) {
        atime->explodedtime = (struct tm *)ap_palloc(atime->cntxt, 
                              sizeof(struct tm));
    }
    if (atime->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    atime->explodedtime->tm_hour = value;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_mday(ap_time_t *, ap_int64_t)
 *    Set the number of days since the beginning of the month 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_set_mday(struct atime_t *atime, ap_int32_t value)
{
    if (!atime) {
        return APR_ENOTIME;
    }
    if (atime->explodedtime == NULL) {
        atime->explodedtime = (struct tm *)ap_palloc(atime->cntxt, 
                              sizeof(struct tm));
    }
    if (atime->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    atime->explodedtime->tm_mday = value;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_mon(ap_time_t *, ap_int64_t)
 *    Set the number of months since the beginning of the year 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_set_mon(struct atime_t *atime, ap_int32_t value)
{
    if (!atime) {
        return APR_ENOTIME;
    }
    if (atime->explodedtime == NULL) {
        atime->explodedtime = (struct tm *)ap_palloc(atime->cntxt, 
                              sizeof(struct tm));
    }
    if (atime->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    atime->explodedtime->tm_mon = value;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_min(ap_time_t *, ap_int64_t)
 *    Set the number of years since the 1900 
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_set_year(struct atime_t *atime, ap_int32_t value)
{
    if (!atime) {
        return APR_ENOTIME;
    }
    if (atime->explodedtime == NULL) {
        atime->explodedtime = (struct tm *)ap_palloc(atime->cntxt, 
                              sizeof(struct tm));
    }
    if (atime->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    atime->explodedtime->tm_year = value;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_wday(ap_time_t *, ap_int64_t)
 *    Get the number of days since the beginning of the week.  0 == Sunday
 * arg 1) The time value we care about.
 * arg 2) Integer to store time value in
 */
ap_status_t ap_set_wday(struct atime_t *atime, ap_int32_t value)
{
    if (!atime) {
        return APR_ENOTIME;
    }
    if (atime->explodedtime == NULL) {
        atime->explodedtime = (struct tm *)ap_palloc(atime->cntxt, 
                              sizeof(struct tm));
    }
    if (atime->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    atime->explodedtime->tm_wday = value;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_timedata(ap_time_t *, char *, void *)
 *    Return the context associated with the current atime.
 * arg 1) The currently open atime.
 * arg 2) The user data associated with the atime.
 */
ap_status_t ap_get_timedata(struct atime_t *atime, char *key, void *data)
{
    if (atime != NULL) {
        return ap_get_userdata(atime->cntxt, key, &data);
    }
    else {
        data = NULL;
        return APR_ENOTIME;
    }
}

/* ***APRDOC********************************************************
 * ap_status_t ap_set_timedata(ap_time_t *, void *, char *,
                               ap_status_t (*cleanup) (void *))
 *    Set the context associated with the current atime.
 * arg 1) The currently open atime.
 * arg 2) The user data to associate with the atime.
 */
ap_status_t ap_set_timedata(struct atime_t *atime, void *data, char *key,
                            ap_status_t (*cleanup) (void *))
{
    if (atime != NULL) {
        return ap_set_userdata(atime->cntxt, data, key, cleanup);
    }
    else {
        data = NULL;
        return APR_ENOTIME;
    }
}

