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

#ifndef APR_TIME_H
#define APR_TIME_H

#include "apr_general.h"
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @package APR Time library
 */

APR_VAR_IMPORT const char ap_month_snames[12][4];
APR_VAR_IMPORT const char ap_day_snames[7][4];

/* number of microseconds since 00:00:00 january 1, 1970 UTC */
typedef ap_int64_t ap_time_t;

/* intervals for I/O timeouts, in microseconds */
typedef ap_int32_t ap_interval_time_t;

#ifdef WIN32
#define AP_USEC_PER_SEC ((LONGLONG) 1000000)
#else
/* XXX: this is wrong -- the LL is only required if int64 is implemented as
 * a long long, it could be just a long on some platforms.  the C99
 * correct way of doing this is to use INT64_C(1000000) which comes
 * from stdint.h.  we'd probably be doing a Good Thing to check for
 * INT64_C in autoconf... or otherwise define an AP_INT64_C(). -dean
 */
#define AP_USEC_PER_SEC (1000000LL)
#endif

/**
 * return the current time
 */
ap_time_t ap_now(void);

/* a structure similar to ANSI struct tm with the following differences:
   - tm_usec isn't an ANSI field
   - tm_gmtoff isn't an ANSI field (it's a bsdism)
*/
typedef struct {
    ap_int32_t tm_usec;	/* microseconds past tm_sec */
    ap_int32_t tm_sec;	/* (0-61) seconds past tm_min */
    ap_int32_t tm_min;  /* (0-59) minutes past tm_hour */
    ap_int32_t tm_hour; /* (0-23) hours past midnight */
    ap_int32_t tm_mday; /* (1-31) day of the month */
    ap_int32_t tm_mon;  /* (0-11) month of the year */
    ap_int32_t tm_year; /* year since 1900 */
    ap_int32_t tm_wday; /* (0-6) days since sunday */
    ap_int32_t tm_yday; /* (0-365) days since jan 1 */
    ap_int32_t tm_isdst; /* daylight saving time */
    ap_int32_t tm_gmtoff; /* seconds east of UTC */
} ap_exploded_time_t;

/**
 * convert an ansi time_t to an ap_time_t
 * @param result the resulting ap_time_t
 * @param input the time_t to convert
 */
ap_status_t ap_ansi_time_to_ap_time(ap_time_t *result, time_t input);

/**
 * convert a time to its human readable components in GMT timezone
 * @param result the exploded time
 * @param input the time to explode
 */
ap_status_t ap_explode_gmt(ap_exploded_time_t *result, ap_time_t input);

/**
 * convert a time to its human readable components in local timezone
 * @param result the exploded time
 * @param input the time to explode
 */
ap_status_t ap_explode_localtime(ap_exploded_time_t *result, ap_time_t input);

/**
 * Convert time value from human readable format to number of seconds 
 * since epoch
 * @param result the resulting imploded time
 * @param input the input exploded time
 */
ap_status_t ap_implode_time(ap_time_t *result, ap_exploded_time_t *input);

/**
 * Sleep for the specified number of micro-seconds.
 * @param t desired amount of time to sleep.
 * @tip May sleep for longer than the specified time. 
 */
void ap_sleep(ap_interval_time_t t);

#define AP_RFC822_DATE_LEN (30)
/**
 * ap_rfc822_date formats dates in the RFC822
 * format in an efficient manner.  it is a fixed length
 * format and requires the indicated amount of storage
 * including trailing \0
 * @param date_str String to write to.
 * @param t the time to convert 
 */
ap_status_t ap_rfc822_date(char *date_str, ap_time_t t);

#define AP_CTIME_LEN (25)
/**
 * ap_ctime formats dates in the ctime() format
 * in an efficient manner.  it is a fixed length format
 * and requires the indicated amount of storage
 * including trailing \0 
 * @param date_str String to write to.
 * @param t the time to convert 
 */
ap_status_t ap_ctime(char *date_str, ap_time_t t);

/**
 * formats the exploded time according to the format specified
 * @param s string to write to
 * @param retsize The length of the returned string
 * @param max The maximum length of the string
 * @param format The format for the time string
 * @param tm The time to convert
 */
ap_status_t ap_strftime(char *s, ap_size_t *retsize, ap_size_t max, const char *format, ap_exploded_time_t *tm);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_TIME_H */
