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
#include "apr_thread_proc.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"

#if APR_HAS_OTHER_CHILD

/* XXX I'm sure there has to be a better way to do this ... */
#ifdef WIN32
#define EXTENSION ".exe"
#elif NETWARE
#define EXTENSION ".nlm"
#else
#define EXTENSION
#endif

static char reasonstr[256];

static void ocmaint(int reason, void *data, int status)
{
    switch (reason) {
    case APR_OC_REASON_DEATH:
        apr_cpystrn(reasonstr, "APR_OC_REASON_DEATH", 
                    strlen("APR_OC_REASON_DEATH") + 1);
        break;
    case APR_OC_REASON_LOST:
        apr_cpystrn(reasonstr, "APR_OC_REASON_LOST", 
                    strlen("APR_OC_REASON_LOST") + 1);
        break;
    case APR_OC_REASON_UNWRITABLE:
        apr_cpystrn(reasonstr, "APR_OC_REASON_UNWRITEABLE", 
                    strlen("APR_OC_REASON_UNWRITEABLE") + 1);
        break;
    case APR_OC_REASON_RESTART:
        apr_cpystrn(reasonstr, "APR_OC_REASON_RESTART", 
                    strlen("APR_OC_REASON_RESTART") + 1);
        break;
    }
}

#ifndef SIGKILL
#define SIGKILL 1
#endif

/* It would be great if we could stress this stuff more, and make the test
 * more granular.
 */
static void test_child_kill(CuTest *tc)
{
    apr_file_t *std = NULL;
    apr_proc_t newproc;
    apr_procattr_t *procattr = NULL;
    const char *args[3];
    apr_status_t rv;

    args[0] = apr_pstrdup(p, "occhild" EXTENSION);
    args[1] = apr_pstrdup(p, "-X");
    args[2] = NULL;

    rv = apr_procattr_create(&procattr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_procattr_io_set(procattr, APR_FULL_BLOCK, APR_NO_PIPE, 
                             APR_NO_PIPE);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_proc_create(&newproc, "./occhild" EXTENSION, args, NULL, procattr, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    CuAssertPtrNotNull(tc, newproc.in);
    CuAssertPtrEquals(tc, NULL, newproc.out);
    CuAssertPtrEquals(tc, NULL, newproc.err);

    std = newproc.in;

    apr_proc_other_child_register(&newproc, ocmaint, NULL, std, p);

    apr_sleep(apr_time_from_sec(1));
    rv = apr_proc_kill(&newproc, SIGKILL);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    
    /* allow time for things to settle... */
    apr_sleep(apr_time_from_sec(3));
    
    apr_proc_other_child_check();
    CuAssertStrEquals(tc, "APR_OC_REASON_DEATH", reasonstr);
}    
#else

static void oc_not_impl(CuTest *tc)
{
    CuNotImpl(tc, "Other child logic not implemented on this platform");
}
#endif

CuSuite *testoc(void)
{
    CuSuite *suite = CuSuiteNew("Other Child");

#if !APR_HAS_OTHER_CHILD
    SUITE_ADD_TEST(suite, oc_not_impl);
#else

    SUITE_ADD_TEST(suite, test_child_kill); 

#endif
    return suite;
}

