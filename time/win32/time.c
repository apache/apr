/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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

#include "atime.h"
#include "apr_time.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include <time.h>
#include <errno.h>
#include <string.h>
#include <winbase.h>

/* Number of micro-seconds between the beginning of the Windows epoch
 * (Jan. 1, 1601) and the Unix epoch (Jan. 1, 1970) 
 */
#define AP_DELTA_EPOCH_IN_USEC   11644473600000000;

void FileTimeToAprTime(ap_time_t *result, FILETIME *input)
{
    /* Convert FILETIME one 64 bit number so we can work with it. */
    *result = input->dwHighDateTime;
    *result = (*result) << 32;
    *result |= input->dwLowDateTime;
    *result /= 10;    /* Convert from 100 nano-sec periods to micro-seconds. */
    *result -= AP_DELTA_EPOCH_IN_USEC;  /* Convert from Windows epoch to Unix epoch */
    return;
}
void AprTimeToFileTime(LPFILETIME pft, ap_time_t t)
{
    LONGLONG ll;
    t += AP_DELTA_EPOCH_IN_USEC;
    ll = t * 10;
    pft->dwLowDateTime = (DWORD)ll;
    pft->dwHighDateTime = (DWORD) (ll >> 32);
    return;
}

void SystemTimeToAprExpTime(ap_exploded_time_t *xt, SYSTEMTIME *tm)
{
    TIME_ZONE_INFORMATION tz;
    DWORD rc;

    xt->tm_usec = tm->wMilliseconds * 1000;
    xt->tm_sec  = tm->wSecond;
    xt->tm_min  = tm->wMinute;
    xt->tm_hour = tm->wHour;
    xt->tm_mday = tm->wDay;
    xt->tm_mon  = tm->wMonth - 1;
    xt->tm_year = tm->wYear - 1900;
    xt->tm_wday = tm->wDayOfWeek;
    xt->tm_yday = 0; /* ToDo: need to compute this */

    rc = GetTimeZoneInformation(&tz);
    switch (rc) {
    case TIME_ZONE_ID_UNKNOWN:
    case TIME_ZONE_ID_STANDARD:
        xt->tm_isdst = 0;
        /* Bias = UTC - local time in minutes 
         * tm_gmtoff is seconds east of UTC
         */
        xt->tm_gmtoff = tz.Bias * 60;
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        xt->tm_isdst = 1;
        xt->tm_gmtoff = tz.Bias * 60;
        break;
    default:
        xt->tm_isdst = 0;
        xt->tm_gmtoff = 0;
    }
    return;
}

ap_status_t ap_ansi_time_to_ap_time(ap_time_t *result, time_t input)
{
    *result = (ap_time_t) input * AP_USEC_PER_SEC;
    return APR_SUCCESS;
}

/* Return micro-seconds since the Unix epoch (jan. 1, 1970) UTC */
ap_time_t ap_now(void)
{
    LONGLONG aprtime = 0;
    FILETIME time;
    GetSystemTimeAsFileTime(&time);
    FileTimeToAprTime(&aprtime, &time);
    return aprtime; 
}

ap_status_t ap_explode_gmt(ap_exploded_time_t *result, ap_time_t input)
{
    FILETIME ft;
    SYSTEMTIME st;
    AprTimeToFileTime(&ft, input);
    FileTimeToSystemTime(&ft, &st);
    SystemTimeToAprExpTime(result, &st);
    return APR_SUCCESS;
}

ap_status_t ap_explode_localtime(ap_exploded_time_t *result, ap_time_t input)
{
    SYSTEMTIME st;
    FILETIME ft, localft;

    AprTimeToFileTime(&ft, input);
    FileTimeToLocalFileTime(&ft, &localft);
    FileTimeToSystemTime(&localft, &st);
    SystemTimeToAprExpTime(result, &st);
    return APR_SUCCESS;
}

ap_status_t ap_implode_time(ap_time_t *t, ap_exploded_time_t *xt)
{
    int year;
    time_t days;
    static const int dayoffset[12] =
    {306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275};

    year = xt->tm_year;

    if (year < 70 || ((sizeof(time_t) <= 4) && (year >= 138))) {
        return APR_EBADDATE;
    }

    /* shift new year to 1st March in order to make leap year calc easy */

    if (xt->tm_mon < 2)
        year--;

    /* Find number of days since 1st March 1900 (in the Gregorian calendar). */

    days = year * 365 + year / 4 - year / 100 + (year / 100 + 3) / 4;
    days += dayoffset[xt->tm_mon] + xt->tm_mday - 1;
    days -= 25508;              /* 1 jan 1970 is 25508 days since 1 mar 1900 */

    days = ((days * 24 + xt->tm_hour) * 60 + xt->tm_min) * 60 + xt->tm_sec;

    if (days < 0) {
        return APR_EBADDATE;
    }
    days -= xt->tm_gmtoff;
    *t = days * AP_USEC_PER_SEC + xt->tm_usec;
    return APR_SUCCESS;
}

ap_status_t ap_get_os_imp_time(ap_os_imp_time_t **ostime, ap_time_t *aprtime)
{
    /* TODO: Consider not passing in pointer to ap_time_t (e.g., call by value) */
    AprTimeToFileTime(*ostime, *aprtime);
    return APR_SUCCESS;
}

ap_status_t ap_get_os_exp_time(ap_os_exp_time_t **ostime, ap_exploded_time_t *aprexptime)
{
    (*ostime)->wYear = aprexptime->tm_year + 1900;
    (*ostime)->wMonth = aprexptime->tm_mon + 1;
    (*ostime)->wDayOfWeek = aprexptime->tm_wday;
    (*ostime)->wDay = aprexptime->tm_mday;
    (*ostime)->wHour = aprexptime->tm_hour;
    (*ostime)->wMinute = aprexptime->tm_min;
    (*ostime)->wSecond = aprexptime->tm_sec;
    (*ostime)->wMilliseconds = aprexptime->tm_usec / 1000;
    return APR_SUCCESS;
}

ap_status_t ap_put_os_imp_time(ap_time_t *aprtime, ap_os_imp_time_t **ostime,
                               ap_pool_t *cont)
{
    FileTimeToAprTime(aprtime, *ostime);
    return APR_SUCCESS;
}

ap_status_t ap_put_os_exp_time(ap_exploded_time_t *aprtime,
                               ap_os_exp_time_t **ostime, ap_pool_t *cont)
{
    SystemTimeToAprExpTime(aprtime, *ostime);
    return APR_SUCCESS;
}

