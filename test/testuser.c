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

#include "test_apr.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_user.h"

#if APR_HAS_USER
static void uid_current(CuTest *tc)
{
    apr_uid_t uid;
    apr_gid_t gid;

    apr_assert_success(tc, "apr_uid_current failed",
                       apr_uid_current(&uid, &gid, p));
}

static void username(CuTest *tc)
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
    CuAssertPtrNotNull(tc, uname);

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
            CuNotImpl(tc, "Groups from apr_uid_current");
        }
        else {
            CuNotImpl(tc, "Groups from apr_uid_get");
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

static void groupname(CuTest *tc)
{
    apr_uid_t uid;
    apr_gid_t gid;
    apr_gid_t retreived_gid;
    char *gname = NULL;

    apr_assert_success(tc, "apr_uid_current failed",
                       apr_uid_current(&uid, &gid, p));

    apr_assert_success(tc, "apr_gid_name_get failed",
                       apr_gid_name_get(&gname, gid, p));
    CuAssertPtrNotNull(tc, gname);

    apr_assert_success(tc, "apr_gid_get failed",
                       apr_gid_get(&retreived_gid, gname, p));

    apr_assert_success(tc, "apr_gid_compare failed",
                       apr_gid_compare(gid, retreived_gid));
}
#else
static void users_not_impl(CuTest *tc)
{
    CuNotImpl(tc, "Users not implemented on this platform");
}
#endif

CuSuite *testuser(void)
{
    CuSuite *suite = CuSuiteNew("Users");

#if !APR_HAS_USER
    SUITE_ADD_TEST(suite, users_not_impl);
#else
    SUITE_ADD_TEST(suite, uid_current);
    SUITE_ADD_TEST(suite, username);
    SUITE_ADD_TEST(suite, groupname);
#endif

    return suite;
}
