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

ap_status_t ap_make_time(struct atime_t **new, ap_context_t *cont)
{
    (*new) = (struct atime_t *)ap_palloc(cont, sizeof(struct atime_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    (*new)->currtime = -1;
    (*new)->explodedtime = NULL;
    return APR_SUCCESS;
}

ap_status_t ap_current_time(struct atime_t *new)
{
    if (!new) {
        return APR_ENOTIME;
    }
    if (new->explodedtime == NULL) {
        new->explodedtime = (SYSTEMTIME *)ap_palloc(new->cntxt, sizeof(SYSTEMTIME));
    }
    GetSystemTime(new->explodedtime);
    return APR_SUCCESS; 
}       

ap_status_t ap_explode_time(struct atime_t *atime, ap_timetype_e type)
{
    if (!atime || !atime->explodedtime) {
        return APR_ENOTIME;
    }   
    return APR_SUCCESS;
}

ap_status_t ap_implode_time(struct atime_t *atime)
{
    FILETIME temp;
    
    if (!atime || !atime->explodedtime) {
        return APR_ENOTIME;
    }

    if (SystemTimeToFileTime(atime->explodedtime, &temp) == 0) {
        return APR_EEXIST;
    }
    atime->currtime = WinTimeToUnixTime(&temp);
    return APR_SUCCESS;
}

ap_status_t ap_get_os_time(ap_os_time_t **atime, struct atime_t *thetime)
{
    if (thetime == NULL) {
        return APR_ENOTIME;
    }
    if (thetime->explodedtime == NULL) {
        ap_explode_time(thetime, APR_LOCALTIME); 
    }
    *atime = thetime->explodedtime;
    return APR_SUCCESS;
}

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
    (*thetime)->explodedtime = atime;
    return APR_SUCCESS;
}

ap_status_t ap_timediff(struct atime_t *a, struct atime_t *b, ap_int32_t *rv)
{
    FILETIME fa, fb;
    LONGLONG ia = 0, ib = 0;
    
    SystemTimeToFileTime(a->explodedtime, &fa);
    SystemTimeToFileTime(b->explodedtime, &fb);
    
	ia = fa.dwHighDateTime;
	ia = ia << 32;
	ia |= fa.dwLowDateTime;

	ib = fb.dwHighDateTime;
	ib = ib << 32;
	ib |= fb.dwLowDateTime;

    *rv = (int)((ia - ib) / 10000);
    return APR_SUCCESS;
}
