/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "win32/apr_arch_atime.h"
#include "apr_time.h"
#include "apr_general.h"
#include "apr_lib.h"

apr_status_t apr_get_curtime(struct atime_t *time, apr_time_t *rv)
{
    if (time) {
        (*rv) = time->currtime;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;    
}

apr_status_t apr_get_sec(struct atime_t *time, apr_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wSecond;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

apr_status_t apr_get_min(struct atime_t *time, apr_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wMinute;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

apr_status_t apr_get_hour(struct atime_t *time, apr_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wHour;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

apr_status_t apr_get_mday(struct atime_t *time, apr_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wDay;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

apr_status_t apr_get_mon(struct atime_t *time, apr_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wMonth;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

apr_status_t apr_get_year(struct atime_t *time, apr_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wYear;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

apr_status_t apr_get_wday(struct atime_t *time, apr_int32_t *rv)
{
    if (time) {
        (*rv) = time->explodedtime->wDayOfWeek;
        return APR_SUCCESS;
    }
    return APR_ENOTIME;
}

apr_status_t apr_set_sec(struct atime_t *time, apr_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)apr_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wSecond = value;
    return APR_SUCCESS;
}

apr_status_t apr_set_min(struct atime_t *time, apr_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)apr_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wMinute = value;
    return APR_SUCCESS;
}

apr_status_t apr_set_hour(struct atime_t *time, apr_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)apr_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wHour = value;
    return APR_SUCCESS;
}

apr_status_t apr_set_mday(struct atime_t *time, apr_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)apr_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wDay = value;
    return APR_SUCCESS;
}

apr_status_t apr_set_mon(struct atime_t *time, apr_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)apr_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wMonth = value;
    return APR_SUCCESS;
}

apr_status_t apr_set_year(struct atime_t *time, apr_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)apr_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wYear = value;
    return APR_SUCCESS;
}

apr_status_t apr_set_wday(struct atime_t *time, apr_int32_t value)
{
    if (!time) {
        return APR_ENOTIME;
    }
    if (time->explodedtime == NULL) {
        time->explodedtime = (SYSTEMTIME *)apr_pcalloc(time->cntxt, 
                              sizeof(SYSTEMTIME));
    }
    if (time->explodedtime == NULL) {
        return APR_ENOMEM;
    }
    time->explodedtime->wDayOfWeek = value;
    return APR_SUCCESS;
}
