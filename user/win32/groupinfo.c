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

#include "apr_strings.h"
#include "apr_portable.h"
#include "apr_user.h"
#include "apr_private.h"
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

APR_DECLARE(apr_status_t) apr_gid_get(apr_gid_t *gid, 
                                      const char *groupname, apr_pool_t *p)
{
#ifdef _WIN32_WCE
    return APR_ENOTIMPL;
#else
    SID_NAME_USE sidtype;
    char anydomain[256];
    char *domain;
    DWORD sidlen = 0;
    DWORD domlen = sizeof(anydomain);
    DWORD rv;
    char *pos;

    if (pos = strchr(groupname, '/')) {
        domain = apr_pstrndup(p, groupname, pos - groupname);
        groupname = pos + 1;
    }
    else if (pos = strchr(groupname, '\\')) {
        domain = apr_pstrndup(p, groupname, pos - groupname);
        groupname = pos + 1;
    }
    else {
        domain = NULL;
    }
    /* Get nothing on the first pass ... need to size the sid buffer 
     */
    rv = LookupAccountName(domain, groupname, domain, &sidlen, 
                           anydomain, &domlen, &sidtype);
    if (sidlen) {
        /* Give it back on the second pass
         */
        *gid = apr_palloc(p, sidlen);
        domlen = sizeof(anydomain);
        rv = LookupAccountName(domain, groupname, *gid, &sidlen, 
                               anydomain, &domlen, &sidtype);
    }
    if (!sidlen || !rv) {
        return apr_get_os_error();
    }
    return APR_SUCCESS;
#endif
}

APR_DECLARE(apr_status_t) apr_gid_name_get(char **groupname, apr_gid_t groupid, apr_pool_t *p)
{
#ifdef _WIN32_WCE
    *groupname = apr_pstrdup(p, "Administrators");
#else
    SID_NAME_USE type;
    char name[MAX_PATH], domain[MAX_PATH];
    DWORD cbname = sizeof(name), cbdomain = sizeof(domain);
    if (!groupid)
        return APR_EINVAL;
    if (!LookupAccountSid(NULL, groupid, name, &cbname, domain, &cbdomain, &type))
        return apr_get_os_error();
    if (type != SidTypeGroup && type != SidTypeWellKnownGroup 
                             && type != SidTypeAlias)
        return APR_EINVAL;
    *groupname = apr_pstrdup(p, name);
#endif
    return APR_SUCCESS;
}
  
APR_DECLARE(apr_status_t) apr_gid_compare(apr_gid_t left, apr_gid_t right)
{
    if (!left || !right)
        return APR_EINVAL;
#ifndef _WIN32_WCE
    if (!IsValidSid(left) || !IsValidSid(right))
        return APR_EINVAL;
    if (!EqualSid(left, right))
        return APR_EMISMATCH;
#endif
    return APR_SUCCESS;
}

/* Deprecated */
APR_DECLARE(apr_status_t) apr_get_groupname(char **groupname, 
                                            apr_gid_t groupid, apr_pool_t *p)
{
    return apr_gid_name_get(groupname, groupid, p);
}

APR_DECLARE(apr_status_t) apr_group_name_get(char **groupname, 
                                             apr_gid_t groupid, apr_pool_t *p)
{
    return apr_gid_name_get(groupname, groupid, p);
}

APR_DECLARE(apr_status_t) apr_get_groupid(apr_gid_t *gid, 
                                         const char *groupname, apr_pool_t *p)
{
    return apr_gid_get(gid, groupname, p);
}

APR_DECLARE(apr_status_t) apr_compare_groups(apr_gid_t left, apr_gid_t right)
{
    return apr_gid_compare(left, right);
}
