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
#include "apr_portable.h"
#include <time.h>
#include <errno.h>
#include <string.h>

/* ***APRDOC********************************************************
 * ap_status_t ap_make_time(ap_context_t *, ap_time_t *)
 *    Create a time entity.
 * arg 1) The context to operate on.
 * arg 2) The new time entity to create.
 */
ap_status_t ap_make_time(struct atime_t **new, ap_context_t *cont)
{
    (*new) = (struct atime_t *)ap_palloc(cont, sizeof(struct atime_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    (*new)->explodedtime = ap_palloc(cont, sizeof(struct tm));
    (*new)->currtime = NULL;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_current_time(ap_time_t *)
 *    Return the number of seconds since January 1, 1970. 
 * arg 1) The time entity to reference.
 */
ap_status_t ap_current_time(struct atime_t *new)
{
    new->currtime = ap_palloc(new->cntxt, sizeof(struct timeval));
    gettimeofday(new->currtime, NULL);
    return APR_SUCCESS; 
}       

/* ***APRDOC********************************************************
 * ap_status_t ap_explode_time(ap_time_t *, ap_timetype_e)
 *    Convert time value from number of seconds since epoch to a set
 *    of integers representing the time in a human readable form.
 * arg 1) The time entity to reference.
 * arg 2) How to explode the time.  One of:
 *            APR_LOCALTIME  -- Use local time
 *            APR_UTCTIME    -- Use UTC time
 */
ap_status_t ap_explode_time(struct atime_t *atime, ap_timetype_e type)
{
    switch (type) {
    case APR_LOCALTIME: {
#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
        localtime_r(&atime->currtime->tv_sec, atime->explodedtime);
#else
        atime->explodedtime = localtime(&atime->currtime->tv_sec);
#endif
        break;
    }
    case APR_UTCTIME: {
#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS)
        gmtime_r(&atime->currtime->tv_sec, atime->explodedtime);
#else
        atime->explodedtime = gmtime(&atime->currtime->tv_sec);
#endif
        break;
    }
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_implode_time(ap_time_t *)
 *    Convert time value from human readable format to number of seconds 
 *    since epoch
 * arg 1) The time entity to reference.
 */
ap_status_t ap_implode_time(struct atime_t *atime)
{
    int year;
    time_t days;
    static const int dayoffset[12] =
    {306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275};

    year = atime->explodedtime->tm_year;

    if (year < 70 || ((sizeof(time_t) <= 4) && (year >= 138))) {
        return APR_EBADDATE;
    }

    /* shift new year to 1st March in order to make leap year calc easy */

    if (atime->explodedtime->tm_mon < 2)
        year--;

    /* Find number of days since 1st March 1900 (in the Gregorian calendar). */

    days = year * 365 + year / 4 - year / 100 + (year / 100 + 3) / 4;
    days += dayoffset[atime->explodedtime->tm_mon] + 
            atime->explodedtime->tm_mday - 1;
    days -= 25508;              /* 1 jan 1970 is 25508 days since 1 mar 1900 */

    days = ((days * 24 + atime->explodedtime->tm_hour) * 60 + 
             atime->explodedtime->tm_min) * 60 + atime->explodedtime->tm_sec;

    if (days < 0) {
        return APR_EBADDATE;
    }
    atime->currtime = ap_palloc(atime->cntxt, sizeof(struct timeval));
    atime->currtime->tv_sec = days;            /* must be a valid time */
    atime->currtime->tv_usec = 0;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_time(ap_os_time_t **, ap_time_t *)
 *    Convert from apr time type to OS specific time value
 * arg 1) The time value to convert.
 * arg 2) The OS specific value to convert to.
 */
ap_status_t ap_get_os_time(ap_os_time_t **atime, struct atime_t *thetime)
{
    if (thetime == NULL) {
        return APR_ENOTIME;
    }
    if (thetime->currtime == NULL) {
        ap_implode_time(thetime); 
    }
    atime = &(thetime->currtime);
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_time(ap_time_t **, ap_os_time_t *, ap_context_t *)
 *    Convert to apr time type from OS specific time value
 * arg 1) The context to use.
 * arg 2) The time value to convert to.
 * arg 3) The OS specific value to convert.
 */
ap_status_t ap_put_os_time(struct atime_t **thetime, ap_os_time_t *atime, 
                           ap_context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if (thetime == NULL) {
        (*thetime) = (struct atime_t *)ap_palloc(cont, sizeof(struct atime_t));
        (*thetime)->cntxt = cont;
    }
    (*thetime)->currtime = atime;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_timediff(ap_time_t *, ap_time_t *, ap_int32_t *)
 *    Retrieve the difference between two time structures in milliseconds.
 * arg 1) The first time value
 * arg 2) The second timevalue
 * arg 3) The difference to return.
 */
ap_status_t ap_timediff(struct atime_t *a, struct atime_t *b, ap_int32_t *rv)
{
    register int us, s;

    us = a->currtime->tv_usec - b->currtime->tv_usec;
    us /= 1000;
    s = a->currtime->tv_sec - b->currtime->tv_sec;
    s *= 1000;
    *rv = s + us;
    return APR_SUCCESS;
} 
 
