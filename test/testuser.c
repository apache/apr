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

#include "test_apr.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_user.h"

#if APR_HAS_USER
static void uid_current(CuTest *tc)
{
    apr_uid_t uid;
    apr_gid_t gid;
    apr_status_t rv;

    rv = apr_uid_current(&uid, &gid, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void username(CuTest *tc)
{
    apr_uid_t uid;
    apr_gid_t gid;
    apr_uid_t retreived_uid;
    apr_gid_t retreived_gid;
    apr_status_t rv;
    char *uname = NULL;

    rv = apr_uid_current(&uid, &gid, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_uid_name_get(&uname, uid, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, uname);

    rv = apr_uid_get(&retreived_uid, &retreived_gid, uname, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    CuAssertIntEquals(tc, APR_SUCCESS, apr_uid_compare(uid, retreived_uid));
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
        CuAssertIntEquals(tc, APR_SUCCESS, apr_gid_compare(gid, retreived_gid));
#ifdef WIN32
    }
#endif
}

static void groupname(CuTest *tc)
{
    apr_uid_t uid;
    apr_gid_t gid;
    apr_gid_t retreived_gid;
    apr_status_t rv;
    char *gname = NULL;

    rv = apr_uid_current(&uid, &gid, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_gid_name_get(&gname, gid, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, gname);

    rv = apr_gid_get(&retreived_gid, gname, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    CuAssertIntEquals(tc, APR_SUCCESS, apr_gid_compare(gid, retreived_gid));
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
