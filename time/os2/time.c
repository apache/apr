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
#include <time.h>
#include <errno.h>
#include <string.h>

ap_status_t ap_make_time(ap_context_t *cont, struct atime_t **new)
{
    (*new) = (struct atime_t *)ap_palloc(cont, sizeof(struct atime_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    (*new)->explodedtime = NULL;
    return APR_SUCCESS;
}

ap_status_t ap_current_time(struct atime_t *new)
{
    if (time(&new->currtime) == -1) {
        return errno;
    }
    return APR_SUCCESS; 
}       

ap_status_t ap_explode_time(struct atime_t *time, ap_timetype_e type)
{
    switch (type) {
    case APR_LOCALTIME: {
        time->explodedtime = localtime(&time->currtime);
        break;
    }
    case APR_UTCTIME: {
        time->explodedtime = gmtime(&time->currtime);
        break;
    }
    }
    return APR_SUCCESS;
}

ap_status_t ap_implode_time(struct atime_t *time)
{
    int year;
    time_t days;
    static const int dayoffset[12] =
    {306, 337, 0, 31, 61, 92, 122, 153, 184, 214, 245, 275};

    year = time->explodedtime->tm_year;

    if (year < 70 || ((sizeof(time_t) <= 4) && (year >= 138))) {
        return APR_EBADDATE;
    }

    /* shift new year to 1st March in order to make leap year calc easy */

    if (time->explodedtime->tm_mon < 2)
        year--;

    /* Find number of days since 1st March 1900 (in the Gregorian calendar). */

    days = year * 365 + year / 4 - year / 100 + (year / 100 + 3) / 4;
    days += dayoffset[time->explodedtime->tm_mon] + 
            time->explodedtime->tm_mday - 1;
    days -= 25508;              /* 1 jan 1970 is 25508 days since 1 mar 1900 */

    days = ((days * 24 + time->explodedtime->tm_hour) * 60 + 
             time->explodedtime->tm_min) * 60 + time->explodedtime->tm_sec;

    if (days < 0) {
        return APR_EBADDATE;
    }
    time->currtime = days;            /* must be a valid time */
    return APR_SUCCESS;
}

