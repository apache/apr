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
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h> /* for _POSIX_THREAD_SAFE_FUNCTIONS */
#endif
#define APR_WANT_MEMFUNC
#include "apr_want.h"

#define PWBUF_SIZE 512

static apr_status_t getpwnam_safe(const char *username,
                                  struct passwd *pw,
                                  char pwbuf[PWBUF_SIZE])
{
    struct passwd *pwptr;
#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && defined(HAVE_GETPWNAM_R)
    /* IRIX getpwnam_r() returns 0 and sets pwptr to NULL on failure */
    if (!getpwnam_r(username, pw, pwbuf, PWBUF_SIZE, &pwptr) && pwptr) {
        /* nothing extra to do on success */
#else
    if ((pwptr = getpwnam(username)) != NULL) {
        memcpy(pw, pwptr, sizeof *pw);
#endif
    }
    else {
        if (errno == 0) {
            /* this can happen with getpwnam() on FreeBSD 4.3 */
            return APR_EGENERAL;
        }
        return errno;
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_uid_homepath_get(char **dirname,
                                               const char *username,
                                               apr_pool_t *p)
{
    struct passwd pw;
    char pwbuf[PWBUF_SIZE];
    apr_status_t rv;

    if ((rv = getpwnam_safe(username, &pw, pwbuf)) != APR_SUCCESS)
        return rv;

#ifdef OS2
    /* Need to manually add user name for OS/2 */
    *dirname = apr_pstrcat(p, pw.pw_dir, pw.pw_name, NULL);
#else
    *dirname = apr_pstrdup(p, pw.pw_dir);
#endif
    return APR_SUCCESS;
}



APR_DECLARE(apr_status_t) apr_uid_current(apr_uid_t *uid,
                                          apr_gid_t *gid,
                                          apr_pool_t *p)
{
    *uid = getuid();
    *gid = getgid();
  
    return APR_SUCCESS;
}




APR_DECLARE(apr_status_t) apr_uid_get(apr_uid_t *uid, apr_gid_t *gid,
                                      const char *username, apr_pool_t *p)
{
    struct passwd pw;
    char pwbuf[PWBUF_SIZE];
    apr_status_t rv;
        
    if ((rv = getpwnam_safe(username, &pw, pwbuf)) != APR_SUCCESS)
        return rv;

    *uid = pw.pw_uid;
    *gid = pw.pw_gid;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_uid_name_get(char **username, apr_uid_t userid,
                                           apr_pool_t *p)
{
    struct passwd *pw;
#if APR_HAS_THREADS && defined(_POSIX_THREAD_SAFE_FUNCTIONS) && defined(HAVE_GETPWUID_R)
    struct passwd pwd;
    char pwbuf[PWBUF_SIZE];

    if (getpwuid_r(userid, &pwd, pwbuf, sizeof(pwbuf), &pw)) {
#else
    if ((pw = getpwuid(userid)) == NULL) {
#endif
        return errno;
    }
    *username = apr_pstrdup(p, pw->pw_name);
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
APR_DECLARE(apr_status_t) apr_get_username(char **username, apr_uid_t userid, 
                                           apr_pool_t *p)
{
    return apr_uid_name_get(username, userid, p);
}

