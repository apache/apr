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
#include "apr_portable.h"

API_VAR_EXPORT const char ap_month_snames[12][4] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
API_VAR_EXPORT const char ap_day_snames[7][4] =
{
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

ap_status_t ap_timestr(char **date_str, struct atime_t *t, ap_timetype_e type, ap_context_t *p)
{
    struct tm *tms;
    char *date_str_ptr;
    int real_year;

    (*date_str) = ap_palloc(p, 48 * sizeof(char));
    date_str_ptr = (*date_str);

    ap_explode_time(t, type);

    /* Assumption: this is always 3 */
    /* i = strlen(ap_day_snames[tms->tm_wday]); */
    memcpy(date_str_ptr, ap_day_snames[t->explodedtime->tm_wday], 3);
    date_str_ptr += 3;
    *date_str_ptr++ = ',';
    *date_str_ptr++ = ' ';
    *date_str_ptr++ = t->explodedtime->tm_mday / 10 + '0';
    *date_str_ptr++ = t->explodedtime->tm_mday % 10 + '0';
    *date_str_ptr++ = ' ';
    /* Assumption: this is also always 3 */
    /* i = strlen(ap_month_snames[tms->tm_mon]); */
    memcpy(date_str_ptr, ap_month_snames[t->explodedtime->tm_mon], 3);
    date_str_ptr += 3;
    *date_str_ptr++ = ' ';
    real_year = 1900 + t->explodedtime->tm_year;
    /* This routine isn't y10k ready. */
    *date_str_ptr++ = real_year / 1000 + '0';
    *date_str_ptr++ = real_year % 1000 / 100 + '0';
    *date_str_ptr++ = real_year % 100 / 10 + '0';
    *date_str_ptr++ = real_year % 10 + '0';
    *date_str_ptr++ = ' ';
    *date_str_ptr++ = t->explodedtime->tm_hour / 10 + '0';
    *date_str_ptr++ = t->explodedtime->tm_hour % 10 + '0';
    *date_str_ptr++ = ':';
    *date_str_ptr++ = t->explodedtime->tm_min / 10 + '0';
    *date_str_ptr++ = t->explodedtime->tm_min % 10 + '0';
    *date_str_ptr++ = ':';
    *date_str_ptr++ = t->explodedtime->tm_sec / 10 + '0';
    *date_str_ptr++ = t->explodedtime->tm_sec % 10 + '0';
    if (type == APR_UTCTIME) {
        *date_str_ptr++ = ' ';
        *date_str_ptr++ = 'G';
        *date_str_ptr++ = 'M';
        *date_str_ptr++ = 'T';
    }
    *date_str_ptr = '\0';
                                                                                    return APR_SUCCESS;
    /* RFC date format; as strftime '%a, %d %b %Y %T GMT' */

    /* The equivalent using sprintf. Use this for more legible but slower code
    return ap_psprintf(p,
                "%s, %.2d %s %d %.2d:%.2d:%.2d GMT", 
                ap_day_snames[t->explodedtime->tm_wday], 
                t->explodedtime->tm_mday, 
                ap_month_snames[t->explodedtime->tm_mon], 
                t->explodedtime->tm_year + 1900, t->explodedtime->tm_hour, 
                t->explodedtime->tm_min, t->explodedtime->tm_sec);
    */
}

ap_status_t ap_strftime(char *s, ap_size_t *retsize, ap_size_t max, 
                        const char *format, struct atime_t *tm)
{
    (*retsize) = strftime(s, max, format, tm->explodedtime);
    return APR_SUCCESS;
}

