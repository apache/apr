/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
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
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "atime.h"
#include "apr_portable.h"

/* ***APRDOC********************************************************
 * ap_status_t ap_ansi_time_to_ap_time(ap_time_t *result, time_t input)
 *    convert an ansi time_t to an ap_time_t
 * arg 1) the resulting ap_time_t
 * arg 2) the time_t to convert
 */
ap_status_t ap_ansi_time_to_ap_time(ap_time_t *result, time_t input)
{
    *result = (ap_time_t)input * AP_USEC_PER_SEC;
    return APR_SUCCESS;
}


/* ***APRDOC********************************************************
 * ap_time_t ap_now(void)
 *    return the current time
 */
ap_time_t ap_now(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * AP_USEC_PER_SEC + tv.tv_usec;
}


static void tm_to_exp(ap_exploded_time_t *xt, struct tm *tm)
{
    xt->tm_sec  = tm->tm_sec;
    xt->tm_min  = tm->tm_min;
    xt->tm_hour = tm->tm_hour;
    xt->tm_mday = tm->tm_mday;
    xt->tm_mon  = tm->tm_mon;
    xt->tm_year = tm->tm_year;
    xt->tm_wday = tm->tm_wday;
    xt->tm_yday = tm->tm_yday;
    xt->tm_isdst = tm->tm_isdst;
}


/* ***APRDOC********************************************************
 * ap_status_t ap_explode_gmt(ap_exploded_time_t *result, ap_time_t input)
 *   convert a time to its human readable components in GMT timezone
 * arg 1) the exploded time
 * arg 2) the time to explode
 */
ap_status_t ap_explode_gmt(ap_exploded_time_t *result, ap_time_t input)
{
    time_t t = input / AP_USEC_PER_SEC;
#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    struct tm banana;
#endif

    result->tm_usec = input % AP_USEC_PER_SEC;

#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    gmtime_r(&t, &banana);
    tm_to_exp(result, &banana);
#else
    tm_to_exp(result, gmtime(&t));
#endif
    result->tm_gmtoff = 0;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_explode_localtime(ap_exploded_time_t *result, ap_time_t input)
 *   convert a time to its human readable components in local timezone
 * arg 1) the exploded time
 * arg 2) the time to explode
 */
ap_status_t ap_explode_localtime(ap_exploded_time_t *result, ap_time_t input)
{
#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
    time_t t = input / AP_USEC_PER_SEC;
    struct tm apricot;

    result->tm_usec = input % AP_USEC_PER_SEC;

    localtime_r(&t, &apricot);
    tm_to_exp(result, &apricot);
#if defined(HAVE_GMTOFF)
    result->tm_gmtoff = apricot.tm_gmtoff;
#elif defined(HAVE___GMTOFF)
    result->tm_gmtoff = apricot.__tm_gmtoff;
#else
    /* solaris is backwards enough to have pthreads but no tm_gmtoff, feh */
    {
	int days, hours, minutes;

	gmtime_r(&t, &apricot);
	days = result->tm_yday - apricot.tm_yday;
	hours = ((days < -1 ? 24 : 1 < days ? -24 : days * 24)
		+ result->tm_hour - apricot.tm_hour);
	minutes = hours * 60 + result->tm_min - apricot.tm_min;
	result->tm_gmtoff = minutes * 60;
    }
#endif
#else
    time_t t = input / AP_USEC_PER_SEC;
    struct tm *tmx;

    result->tm_usec = input % AP_USEC_PER_SEC;

    tmx = localtime(&t);
    tm_to_exp(result, tmx);
#if defined(HAVE_GMTOFF)
    result->tm_gmtoff = tmx->tm_gmtoff;
#elif defined(HAVE___GMTOFF)
    result->tm_gmtoff = tmx->__tm_gmtoff;
#else
    /* need to create tm_gmtoff... assume we are never more than 24 hours away */
    {
	int days, hours, minutes;

	tmx = gmtime(&t);
	days = result->tm_yday - tmx->tm_yday;
	hours = ((days < -1 ? 24 : 1 < days ? -24 : days * 24)
		+ result->tm_hour - tmx->tm_hour);
	minutes = hours * 60 + result->tm_min - tmx->tm_min;
	result->tm_gmtoff = minutes * 60;
    }
#endif
#endif
    return APR_SUCCESS;
}


/* ***APRDOC********************************************************
 * ap_status_t ap_implode_time(ap_time_t *t, ap_exploded_time_t *xt)
 *    Convert time value from human readable format to number of seconds 
 *    since epoch
 * arg 1) the resulting imploded time
 * arg 2) the input exploded time
 */
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

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_imp_time(ap_os_imp_time_t **ostime, ap_time_t *aprtime)
 *    Get the imploded time in the platforms native format.
 * arg 1) the native time format
 * arg 2) the time to convert
 */
ap_status_t ap_get_os_imp_time(ap_os_imp_time_t **ostime, ap_time_t *aprtime)
{
    (*ostime)->tv_usec = *aprtime % AP_USEC_PER_SEC;
    (*ostime)->tv_sec = *aprtime / AP_USEC_PER_SEC;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_exp_time(ap_os_exp_time_t **ostime, ap_exploded_time_t *aprtime)
 *    Get the exploded time in the platforms native format.
 * arg 1) the native time format
 * arg 2) the time to convert
 */
ap_status_t ap_get_os_exp_time(ap_os_exp_time_t **ostime, ap_exploded_time_t *aprtime)
{
    (*ostime)->tm_sec  = aprtime->tm_sec;
    (*ostime)->tm_min  = aprtime->tm_min;
    (*ostime)->tm_hour = aprtime->tm_hour;
    (*ostime)->tm_mday = aprtime->tm_mday;
    (*ostime)->tm_mon  = aprtime->tm_mon;
    (*ostime)->tm_year = aprtime->tm_year;
    (*ostime)->tm_wday = aprtime->tm_wday;
    (*ostime)->tm_yday = aprtime->tm_yday;
    (*ostime)->tm_isdst = aprtime->tm_isdst;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_imp_time(ap_time_t *aprtime, ap_os_imp_time_t **ostime, ap_context_t, *cont)
 *    Put the imploded time in the APR format.
 * arg 1) the APR time format
 * arg 2) the time to convert
 * arg 3) the context to use if necessary
 */
ap_status_t ap_put_os_imp_time(ap_time_t *aprtime, ap_os_imp_time_t **ostime,
                               ap_context_t *cont)
{
    *aprtime = (*ostime)->tv_sec * AP_USEC_PER_SEC + (*ostime)->tv_usec;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_exp_time(ap_exploded_time_t *aprtime, ap_os_exp_time_t **ostime, ap_context_t, *cont)
 *    Put the exploded time in the APR format.
 * arg 1) the APR time format
 * arg 2) the time to convert
 * arg 3) the context to use if necessary
 */
ap_status_t ap_put_os_exp_time(ap_exploded_time_t *aprtime,
                               ap_os_exp_time_t **ostime, ap_context_t *cont)
{
    aprtime->tm_sec = (*ostime)->tm_sec;
    aprtime->tm_min = (*ostime)->tm_min;
    aprtime->tm_hour = (*ostime)->tm_hour;
    aprtime->tm_mday = (*ostime)->tm_mday;
    aprtime->tm_mon = (*ostime)->tm_mon;
    aprtime->tm_year = (*ostime)->tm_year;
    aprtime->tm_wday = (*ostime)->tm_wday;
    aprtime->tm_yday = (*ostime)->tm_yday;
    aprtime->tm_isdst = (*ostime)->tm_isdst;
    return APR_SUCCESS;
}



#ifdef OS2
#define INCL_DOS
#include <os2.h>

ap_status_t ap_os2_time_to_ap_time(ap_time_t *result, FDATE os2date, FTIME os2time)
{
  struct tm tmpdate;

  memset(&tmpdate, 0, sizeof(tmpdate));
  tmpdate.tm_hour  = os2time.hours;
  tmpdate.tm_min   = os2time.minutes;
  tmpdate.tm_sec   = os2time.twosecs * 2;

  tmpdate.tm_mday  = os2date.day;
  tmpdate.tm_mon   = os2date.month - 1;
  tmpdate.tm_year  = os2date.year + 80;
  tmpdate.tm_isdst = -1;

  *result = mktime(&tmpdate) * AP_USEC_PER_SEC;
  return APR_SUCCESS;
}
#endif
