/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "testutil.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_user.h"

#if APR_HAS_USER
static void uid_current(abts_case *tc, void *data)
{
    apr_uid_t uid;
    apr_gid_t gid;

    apr_assert_success(tc, "apr_uid_current failed",
                       apr_uid_current(&uid, &gid, p));
}

static void username(abts_case *tc, void *data)
{
    apr_uid_t uid;
    apr_gid_t gid;
    apr_uid_t retreived_uid;
    apr_gid_t retreived_gid;
    char *uname = NULL;

    apr_assert_success(tc, "apr_uid_current failed",
                       apr_uid_current(&uid, &gid, p));
   
    apr_assert_success(tc, "apr_uid_name_get failed",
                       apr_uid_name_get(&uname, uid, p));
    abts_ptr_notnull(tc, uname);

    apr_assert_success(tc, "apr_uid_get failed",
                       apr_uid_get(&retreived_uid, &retreived_gid, uname, p));

    apr_assert_success(tc, "apr_uid_compare failed",
                       apr_uid_compare(uid, retreived_uid));
#ifdef WIN32
    /* ### this fudge was added for Win32 but makes the test return NotImpl
     * on Unix if run as root, when !gid is also true. */
    if (!gid || !retreived_gid) {
        /* The function had no way to recover the gid (this would have been
         * an ENOTIMPL if apr_uid_ functions didn't try to double-up and
         * also return apr_gid_t values, which was bogus.
         */
        if (!gid) {
            abts_not_impl(tc, "Groups from apr_uid_current");
        }
        else {
            abts_not_impl(tc, "Groups from apr_uid_get");
        }        
    }
    else {
#endif
        apr_assert_success(tc, "apr_gid_compare failed",
                           apr_gid_compare(gid, retreived_gid));
#ifdef WIN32
    }
#endif
}

static void groupname(abts_case *tc, void *data)
{
    apr_uid_t uid;
    apr_gid_t gid;
    apr_gid_t retreived_gid;
    char *gname = NULL;

    apr_assert_success(tc, "apr_uid_current failed",
                       apr_uid_current(&uid, &gid, p));

    apr_assert_success(tc, "apr_gid_name_get failed",
                       apr_gid_name_get(&gname, gid, p));
    abts_ptr_notnull(tc, gname);

    apr_assert_success(tc, "apr_gid_get failed",
                       apr_gid_get(&retreived_gid, gname, p));

    apr_assert_success(tc, "apr_gid_compare failed",
                       apr_gid_compare(gid, retreived_gid));
}
#else
static void users_not_impl(abts_case *tc, void *data)
{
    abts_not_impl(tc, "Users not implemented on this platform");
}
#endif

abts_suite *testuser(abts_suite *suite)
{
    suite = ADD_SUITE(suite)

#if !APR_HAS_USER
    abts_run_test(suite, users_not_impl, NULL);
#else
    abts_run_test(suite, uid_current, NULL);
    abts_run_test(suite, username, NULL);
    abts_run_test(suite, groupname, NULL);
#endif

    return suite;
}
