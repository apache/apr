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

#ifndef APR_TIME_H
#define APR_TIME_H

#include "apr_general.h"
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {APR_LOCALTIME, APR_UTCTIME} ap_timetype_e;

typedef struct atime_t       ap_time_t;

API_VAR_IMPORT const char ap_month_snames[12][4];
API_VAR_IMPORT const char ap_day_snames[7][4];

/* Function Definitions */
ap_status_t ap_make_time(ap_time_t **, ap_context_t *);
ap_status_t ap_current_time(ap_time_t *);
ap_status_t ap_explode_time(ap_time_t *, ap_timetype_e);
ap_status_t ap_implode_time(ap_time_t *);

ap_status_t ap_timestr(char **date_str, struct atime_t *t, ap_timetype_e type, ap_context_t *p);
ap_status_t ap_strftime(char *s, ap_int32_t *retsize, ap_size_t max, const char *format, ap_time_t *tm);

/* accessor functions */
ap_status_t ap_get_curtime(ap_time_t *, ap_int64_t *);
ap_status_t ap_timediff(ap_time_t *, ap_time_t *, ap_int32_t *);

ap_status_t ap_get_sec(ap_time_t *, ap_int32_t *);
ap_status_t ap_get_min(ap_time_t *, ap_int32_t *);
ap_status_t ap_get_hour(ap_time_t *, ap_int32_t *);
ap_status_t ap_get_mday(ap_time_t *, ap_int32_t *);
ap_status_t ap_get_mon(ap_time_t *, ap_int32_t *);
ap_status_t ap_get_year(ap_time_t *, ap_int32_t *);
ap_status_t ap_get_wday(ap_time_t *, ap_int32_t *);

ap_status_t ap_set_curtime(ap_time_t *, ap_int64_t);
ap_status_t ap_set_sec(ap_time_t *, ap_int32_t);
ap_status_t ap_set_min(ap_time_t *, ap_int32_t);
ap_status_t ap_set_hour(ap_time_t *, ap_int32_t);
ap_status_t ap_set_mday(ap_time_t *, ap_int32_t);
ap_status_t ap_set_mon(ap_time_t *, ap_int32_t);
ap_status_t ap_set_year(ap_time_t *, ap_int32_t);
ap_status_t ap_set_wday(ap_time_t *, ap_int32_t);
ap_status_t ap_timecmp(ap_time_t *a, ap_time_t *b);

ap_status_t ap_get_gmtoff(int *tz, ap_time_t *tt, ap_context_t *cont);

ap_status_t ap_get_timedata(ap_time_t *, char *, void *);
ap_status_t ap_set_timedata(ap_time_t *, void *, char *,
                            ap_status_t (*cleanup) (void *));
 
#ifdef __cplusplus
}
#endif

#endif  /* ! APR_TIME_H */
