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

#define APR_WANT_STRFUNC
#include "apr_want.h"
#include "apr.h"
#include "apr_arch_misc.h"
#include "apr_arch_utf8.h"
#include "apr_env.h"
#include "apr_errno.h"
#include "apr_pools.h"


#if APR_HAS_UNICODE_FS
static apr_status_t widen_envvar_name (apr_wchar_t *buffer,
                                       apr_size_t bufflen,
                                       const char *envvar)
{
    apr_size_t inchars;
    apr_status_t status;

    inchars = strlen(envvar) + 1;
    status = apr_conv_utf8_to_ucs2(envvar, &inchars, buffer, &bufflen);
    if (status == APR_INCOMPLETE)
        status = APR_ENAMETOOLONG;

    return status;
}
#endif


APR_DECLARE(apr_status_t) apr_env_get(char **value,
                                      const char *envvar,
                                      apr_pool_t *pool)
{
    char *val = NULL;
    DWORD size;

#if APR_HAS_UNICODE_FS
    IF_WIN_OS_IS_UNICODE
    {
        apr_wchar_t wenvvar[APR_PATH_MAX];
        apr_size_t inchars, outchars;
        apr_wchar_t *wvalue, dummy;
        apr_status_t status;

        status = widen_envvar_name(wenvvar, APR_PATH_MAX, envvar);
        if (status)
            return status;

        size = GetEnvironmentVariableW(wenvvar, &dummy, 0);
        if (size == 0)
            /* The environment variable doesn't exist. */
            return APR_ENOENT;

        wvalue = apr_palloc(pool, size * sizeof(*wvalue));
        size = GetEnvironmentVariableW(wenvvar, wvalue, size);
        if (size == 0)
            /* Mid-air collision?. Somebody must've changed the env. var. */
            return APR_INCOMPLETE;

        inchars = wcslen(wvalue) + 1;
        outchars = 3 * inchars; /* Enougn for any UTF-8 representation */
        val = apr_palloc(pool, outchars);
        status = apr_conv_ucs2_to_utf8(wvalue, &inchars, val, &outchars);
        if (status)
            return status;
    }
#endif
#if APR_HAS_ANSI_FS
    ELSE_WIN_OS_IS_ANSI
    {
        char dummy;

        size = GetEnvironmentVariableA(envvar, &dummy, 0);
        if (size == 0)
            /* The environment variable doesn't exist. */
            return APR_ENOENT;

        val = apr_palloc(pool, size);
        size = GetEnvironmentVariableA(envvar, val, size);
        if (size == 0)
            /* Mid-air collision?. Somebody must've changed the env. var. */
            return APR_INCOMPLETE;
    }
#endif

    *value = val;
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_env_set(const char *envvar,
                                      const char *value,
                                      apr_pool_t *pool)
{
#if APR_HAS_UNICODE_FS
    IF_WIN_OS_IS_UNICODE
    {
        apr_wchar_t wenvvar[APR_PATH_MAX];
        apr_wchar_t *wvalue;
        apr_size_t inchars, outchars;
        apr_status_t status;

        status = widen_envvar_name(wenvvar, APR_PATH_MAX, envvar);
        if (status)
            return status;

        outchars = inchars = strlen(value) + 1;
        wvalue = apr_palloc(pool, outchars * sizeof(*wvalue));
        status = apr_conv_utf8_to_ucs2(value, &inchars, wvalue, &outchars);
        if (status)
            return status;

        if (!SetEnvironmentVariableW(wenvvar, wvalue))
            return apr_get_os_error();
    }
#endif
#if APR_HAS_ANSI_FS
    ELSE_WIN_OS_IS_ANSI
    {
        if (!SetEnvironmentVariableA(envvar, value))
            return apr_get_os_error();
    }
#endif

    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_env_delete(const char *envvar, apr_pool_t *pool)
{
#if APR_HAS_UNICODE_FS
    IF_WIN_OS_IS_UNICODE
    {
        apr_wchar_t wenvvar[APR_PATH_MAX];
        apr_status_t status;

        status = widen_envvar_name(wenvvar, APR_PATH_MAX, envvar);
        if (status)
            return status;

        if (!SetEnvironmentVariableW(wenvvar, NULL))
            return apr_get_os_error();
    }
#endif
#if APR_HAS_ANSI_FS
    ELSE_WIN_OS_IS_ANSI
    {
        if (!SetEnvironmentVariableA(envvar, NULL))
            return apr_get_os_error();
    }
#endif

    return APR_SUCCESS;
}
