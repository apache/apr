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

#ifndef APR_USER_H
#define APR_USER_H

/**
 * @file apr_user.h
 * @brief APR User ID Services 
 */

#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup apr_user User and Group ID Services
 * @ingroup APR 
 * @{
 */

/**
 * Structure for determining user ownership.
 */
#ifdef WIN32
typedef PSID                      apr_uid_t;
#else
typedef uid_t                     apr_uid_t;
#endif

/**
 * Structure for determining group ownership.
 */
#ifdef WIN32
typedef PSID                      apr_gid_t;
#else
typedef gid_t                     apr_gid_t;
#endif

#if APR_HAS_USER 

/**
 * Get the userid (and groupid) of the calling process
 * @param userid   Returns the user id
 * @param groupid  Returns the user's group id
 * @param p The pool from which to allocate working space
 * @remark This function is available only if APR_HAS_USER is defined.
 */
APR_DECLARE(apr_status_t) apr_uid_current(apr_uid_t *userid,
                                          apr_gid_t *groupid,
                                          apr_pool_t *p);

/** @deprecated @see apr_uid_current */
APR_DECLARE(apr_status_t) apr_current_userid(apr_uid_t *userid,
                                             apr_gid_t *groupid,
                                             apr_pool_t *p);
/**
 * Get the user name for a specified userid
 * @param username Pointer to new string containing user name (on output)
 * @param userid The userid
 * @param p The pool from which to allocate the string
 * @remark This function is available only if APR_HAS_USER is defined.
 */
APR_DECLARE(apr_status_t) apr_uid_name_get(char **username, apr_uid_t userid,
                                           apr_pool_t *p);

/** @deprecated @see apr_uid_name_get */
APR_DECLARE(apr_status_t) apr_get_username(char **username, apr_uid_t userid,
                                           apr_pool_t *p);
/**
 * Get the userid (and groupid) for the specified username
 * @param userid   Returns the user id
 * @param groupid  Returns the user's group id
 * @param username The username to lookup
 * @param p The pool from which to allocate working space
 * @remark This function is available only if APR_HAS_USER is defined.
 */
APR_DECLARE(apr_status_t) apr_uid_get(apr_uid_t *userid, apr_gid_t *groupid,
                                      const char *username, apr_pool_t *p);

/** @deprecated @see apr_uid_get */
APR_DECLARE(apr_status_t) apr_get_userid(apr_uid_t *userid, apr_gid_t *groupid,
                                         const char *username, apr_pool_t *p);

/**
 * Get the home directory for the named user
 * @param dirname Pointer to new string containing directory name (on output)
 * @param username The named user
 * @param p The pool from which to allocate the string
 * @remark This function is available only if APR_HAS_USER is defined.
 */
APR_DECLARE(apr_status_t) apr_uid_homepath_get(char **dirname, 
                                               const char *username, 
                                               apr_pool_t *p);

/** @deprecated @see apr_uid_homepath_get */
APR_DECLARE(apr_status_t) apr_get_home_directory(char **dirname, 
                                                 const char *username, 
                                                 apr_pool_t *p);

/**
 * Compare two user identifiers for equality.
 * @param left One uid to test
 * @param right Another uid to test
 * @return APR_SUCCESS if the apr_uid_t strutures identify the same user,
 * APR_EMISMATCH if not, APR_BADARG if an apr_uid_t is invalid.
 * @remark This function is available only if APR_HAS_USER is defined.
 */
#if defined(WIN32)
APR_DECLARE(apr_status_t) apr_uid_compare(apr_uid_t left, apr_uid_t right);

/** @deprecated @see apr_uid_compare */
APR_DECLARE(apr_status_t) apr_compare_users(apr_uid_t left, apr_uid_t right);
#else
#define apr_uid_compare(left,right) (((left) == (right)) ? APR_SUCCESS : APR_EMISMATCH)
/** @deprecated @see apr_uid_compare */
#define apr_compare_users(left,right) (((left) == (right)) ? APR_SUCCESS : APR_EMISMATCH)
#endif

/**
 * Get the group name for a specified groupid
 * @param groupname Pointer to new string containing group name (on output)
 * @param groupid The groupid
 * @param p The pool from which to allocate the string
 * @remark This function is available only if APR_HAS_USER is defined.
 */
APR_DECLARE(apr_status_t) apr_gid_name_get(char **groupname, 
                                             apr_gid_t groupid, apr_pool_t *p);

/** @deprecated @see apr_gid_name_get */
APR_DECLARE(apr_status_t) apr_group_name_get(char **groupname, 
                                             apr_gid_t groupid, apr_pool_t *p);

/** @deprecated @see apr_gid_name_get */
APR_DECLARE(apr_status_t) apr_get_groupname(char **groupname, 
                                            apr_gid_t groupid, apr_pool_t *p);

/**
 * Get the groupid for a specified group name
 * @param groupid Pointer to the group id (on output)
 * @param groupname The group name to look up
 * @param p The pool from which to allocate the string
 * @remark This function is available only if APR_HAS_USER is defined.
 */
APR_DECLARE(apr_status_t) apr_gid_get(apr_gid_t *groupid, 
                                      const char *groupname, apr_pool_t *p);

/** @deprecated @see apr_gid_get */
APR_DECLARE(apr_status_t) apr_get_groupid(apr_gid_t *groupid, 
                                          const char *groupname, apr_pool_t *p);

/**
 * Compare two group identifiers for equality.
 * @param left One gid to test
 * @param right Another gid to test
 * @return APR_SUCCESS if the apr_gid_t strutures identify the same group,
 * APR_EMISMATCH if not, APR_BADARG if an apr_gid_t is invalid.
 * @remark This function is available only if APR_HAS_USER is defined.
 */
#if defined(WIN32)
APR_DECLARE(apr_status_t) apr_gid_compare(apr_gid_t left, apr_gid_t right);
/** @deprecated @see apr_gid_compare */
APR_DECLARE(apr_status_t) apr_compare_groups(apr_gid_t left, apr_gid_t right);
#else
#define apr_gid_compare(left,right) (((left) == (right)) ? APR_SUCCESS : APR_EMISMATCH)
/** @deprecated @see apr_gid_compare */
#define apr_compare_groups(left,right) (((left) == (right)) ? APR_SUCCESS : APR_EMISMATCH)
#endif

#endif  /* ! APR_HAS_USER */

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_USER_H */
