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

#include "apr_private.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include "apr_user.h"
#include "apr_arch_file_io.h"
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef _WIN32_WCE
/* Internal sid binary to string translation, see MSKB Q131320.
 * Several user related operations require our SID to access
 * the registry, but in a string format.  All error handling
 * depends on IsValidSid(), which internally we better test long
 * before we get here!
 */
void get_sid_string(char *buf, int blen, apr_uid_t id)
{
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD nsa;
    DWORD sa;
    int slen;

    /* Determine authority values (these is a big-endian value, 
     * and NT records the value as hex if the value is > 2^32.)
     */
    psia = GetSidIdentifierAuthority(id);
    nsa =  (DWORD)(psia->Value[5])        + ((DWORD)(psia->Value[4]) <<  8)
        + ((DWORD)(psia->Value[3]) << 16) + ((DWORD)(psia->Value[2]) << 24);
    sa  =  (DWORD)(psia->Value[1])        + ((DWORD)(psia->Value[0]) <<  8);
    if (sa) {
        slen = apr_snprintf(buf, blen, "S-%lu-0x%04x%08x",
                            SID_REVISION, sa, nsa);
    } else {
        slen = apr_snprintf(buf, blen, "S-%lu-%lu",
                            SID_REVISION, nsa);
    }

    /* Now append all the subauthority strings.
     */
    nsa = *GetSidSubAuthorityCount(id);
    for (sa = 0; sa < nsa; ++sa) {
        slen += apr_snprintf(buf + slen, blen - slen, "-%lu",
                             *GetSidSubAuthority(id, sa));
    }
} 
#endif
/* Query the ProfileImagePath from the version-specific branch, where the
 * regkey uses the user's name on 9x, and user's sid string on NT.
 */
APR_DECLARE(apr_status_t) apr_uid_homepath_get(char **dirname, 
                                               const char *username, 
                                               apr_pool_t *p)
{
#ifdef _WIN32_WCE
    *dirname = apr_pstrdup(p, "/My Documents");
    return APR_SUCCESS;
#else
    apr_status_t rv;
    char regkey[MAX_PATH * 2];
    char *fixch;
    DWORD keylen;
    DWORD type;
    HKEY key;

    if (apr_os_level >= APR_WIN_NT) {
        apr_uid_t uid;
        apr_gid_t gid;
    
        if ((rv = apr_uid_get(&uid, &gid, username, p)) != APR_SUCCESS)
            return rv;

        strcpy(regkey, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\"
                       "ProfileList\\");
        keylen = strlen(regkey);
        get_sid_string(regkey + keylen, sizeof(regkey) - keylen, uid);
    }
    else {
        strcpy(regkey, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\"
                       "ProfileList\\");
        keylen = strlen(regkey);
        apr_cpystrn(regkey + keylen, username, sizeof(regkey) - keylen);

    }

    if ((rv = RegOpenKeyEx(HKEY_LOCAL_MACHINE, regkey, 0, 
                           KEY_QUERY_VALUE, &key)) != ERROR_SUCCESS)
        return APR_FROM_OS_ERROR(rv);

#if APR_HAS_UNICODE_FS
    IF_WIN_OS_IS_UNICODE
    {

        keylen = sizeof(regkey);
        rv = RegQueryValueExW(key, L"ProfileImagePath", NULL, &type,
                                   (void*)regkey, &keylen);
        RegCloseKey(key);
        if (rv != ERROR_SUCCESS)
            return APR_FROM_OS_ERROR(rv);
        if (type == REG_SZ) {
            char retdir[MAX_PATH];
            if ((rv = unicode_to_utf8_path(retdir, sizeof(retdir), 
                                           (apr_wchar_t*)regkey)) != APR_SUCCESS)
                return rv;
            *dirname = apr_pstrdup(p, retdir);
        }
        else if (type == REG_EXPAND_SZ) {
            apr_wchar_t path[MAX_PATH];
            char retdir[MAX_PATH];
            ExpandEnvironmentStringsW((apr_wchar_t*)regkey, path, sizeof(path));
            if ((rv = unicode_to_utf8_path(retdir, sizeof(retdir), path))
                    != APR_SUCCESS)
                return rv;
            *dirname = apr_pstrdup(p, retdir);
        }
        else
            return APR_ENOENT;
    }
#endif
#if APR_HAS_ANSI_FS
    ELSE_WIN_OS_IS_ANSI
    {
        keylen = sizeof(regkey);
        rv = RegQueryValueEx(key, "ProfileImagePath", NULL, &type,
                                  (void*)regkey, &keylen);
        RegCloseKey(key);
        if (rv != ERROR_SUCCESS)
            return APR_FROM_OS_ERROR(rv);
        if (type == REG_SZ) {
            *dirname = apr_pstrdup(p, regkey);
        }
        else if (type == REG_EXPAND_SZ) {
            char path[MAX_PATH];
            ExpandEnvironmentStrings(regkey, path, sizeof(path));
            *dirname = apr_pstrdup(p, path);
        }
        else
            return APR_ENOENT;
    }
#endif /* APR_HAS_ANSI_FS */
    for (fixch = *dirname; *fixch; ++fixch)
        if (*fixch == '\\')
            *fixch = '/';
    return APR_SUCCESS;
#endif /* _WIN32_WCE */
}

APR_DECLARE(apr_status_t) apr_uid_current(apr_uid_t *uid,
                                          apr_gid_t *gid,
                                          apr_pool_t *p)
{
#ifdef _WIN32_WCE
    return APR_ENOTIMPL;
#else
    HANDLE threadtok;
    DWORD needed;
    TOKEN_USER *usr;
    TOKEN_PRIMARY_GROUP *grp;
    
    if(!OpenProcessToken(GetCurrentProcess(), STANDARD_RIGHTS_READ | READ_CONTROL | TOKEN_QUERY, &threadtok)) {
        return apr_get_os_error();
    }

    *uid = NULL;
    if (!GetTokenInformation(threadtok, TokenUser, NULL, 0, &needed)
        && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
        && (usr = apr_palloc(p, needed))
        && GetTokenInformation(threadtok, TokenUser, usr, needed, &needed))
        *uid = usr->User.Sid;
    else
        return apr_get_os_error();

    if (!GetTokenInformation(threadtok, TokenPrimaryGroup, NULL, 0, &needed)
        && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
        && (grp = apr_palloc(p, needed))
        && GetTokenInformation(threadtok, TokenPrimaryGroup, grp, needed, &needed))
        *gid = grp->PrimaryGroup;
    else
        return apr_get_os_error();

    return APR_SUCCESS;
#endif 
}

APR_DECLARE(apr_status_t) apr_uid_get(apr_uid_t *uid, apr_gid_t *gid,
                                      const char *username, apr_pool_t *p)
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

    if (pos = strchr(username, '/')) {
        domain = apr_pstrndup(p, username, pos - username);
        username = pos + 1;
    }
    else if (pos = strchr(username, '\\')) {
        domain = apr_pstrndup(p, username, pos - username);
        username = pos + 1;
    }
    else {
        domain = NULL;
    }
    /* Get nothing on the first pass ... need to size the sid buffer 
     */
    rv = LookupAccountName(domain, username, domain, &sidlen, 
                           anydomain, &domlen, &sidtype);
    if (sidlen) {
        /* Give it back on the second pass
         */
        *uid = apr_palloc(p, sidlen);
        domlen = sizeof(anydomain);
        rv = LookupAccountName(domain, username, *uid, &sidlen, 
                               anydomain, &domlen, &sidtype);
    }
    if (!sidlen || !rv) {
        return apr_get_os_error();
    }
    /* There doesn't seem to be a simple way to retrieve the primary group sid
     */
    *gid = NULL;
    return APR_SUCCESS;
#endif
}

APR_DECLARE(apr_status_t) apr_uid_name_get(char **username, apr_uid_t userid,
                                           apr_pool_t *p)
{
#ifdef _WIN32_WCE
    *username = apr_pstrdup(p, "Administrator");
    return APR_SUCCESS;
#else
    SID_NAME_USE type;
    char name[MAX_PATH], domain[MAX_PATH];
    DWORD cbname = sizeof(name), cbdomain = sizeof(domain);
    if (!userid)
        return APR_EINVAL;
    if (!LookupAccountSid(NULL, userid, name, &cbname, domain, &cbdomain, &type))
        return apr_get_os_error();
    if (type != SidTypeUser && type != SidTypeAlias && type != SidTypeWellKnownGroup)
        return APR_EINVAL;
    *username = apr_pstrdup(p, name);
    return APR_SUCCESS;
#endif
}
  
APR_DECLARE(apr_status_t) apr_uid_compare(apr_uid_t left, apr_uid_t right)
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

/* deprecated */
APR_DECLARE(apr_status_t) apr_get_home_directory(char **dirname, 
                                                 const char *username, 
                                                 apr_pool_t *p)
{
    return apr_uid_homepath_get(dirname, username, p);
}

/* deprecated */
APR_DECLARE(apr_status_t) apr_get_userid(apr_uid_t *uid, apr_gid_t *gid,
                                         const char *username, apr_pool_t *p)
{
    return apr_uid_get(uid, gid, username, p);
}

/* deprecated */
APR_DECLARE(apr_status_t) apr_current_userid(apr_uid_t *uid,
                                             apr_gid_t *gid,
                                             apr_pool_t *p)
{
    return apr_uid_current(uid, gid, p);
}

/* deprecated */
APR_DECLARE(apr_status_t) apr_compare_users(apr_uid_t left, apr_uid_t right)
{
    return apr_uid_compare(left, right);
}

/* deprecated */
APR_DECLARE(apr_status_t) apr_get_username(char **username, apr_uid_t userid,
                                           apr_pool_t *p)
{
    return apr_uid_name_get(username, userid, p);
}
