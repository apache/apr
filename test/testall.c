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

#include <stdio.h>
#include <stdlib.h>

#include "test_apr.h"

/* Top-level pool which can be used by tests. */
apr_pool_t *p;

void apr_assert_success(CuTest* tc, const char* context, apr_status_t rv)
{
    if (rv == APR_ENOTIMPL) {
        CuNotImpl(tc, context);
    }

    if (rv != APR_SUCCESS) {
        char buf[STRING_MAX], ebuf[128];
        sprintf(buf, "%s (%d): %s\n", context, rv,
                apr_strerror(rv, ebuf, sizeof ebuf));
        CuFail(tc, buf);
    }
}

static const struct testlist {
    const char *testname;
    CuSuite *(*func)(void);
} tests[] = {
    {"teststr", teststr},
    {"testtime", testtime},
    {"testvsn", testvsn},
    {"testipsub", testipsub},
    {"testmmap", testmmap},
    {"testud", testud},
    {"testtable", testtable},
    {"testhash", testhash},
    {"testsleep", testsleep},
    {"testpool", testpool},
    {"testfmt", testfmt},
    {"testfile", testfile},
    {"testfileinfo", testfileinfo},
    {"testpipe", testpipe},
    {"testdup", testdup},
    {"testdir", testdir},
    {"testrand", testrand},
    {"testdso", testdso},
    {"testoc", testoc},
    {"testsockets", testsockets},
    {"testsockopt", testsockopt},
    {"testproc", testproc},
    {"testprocmutex", testprocmutex},
    {"testpoll", testpoll},
    {"testlock", testlock},
    {"testthread", testthread},
    {"testargs", testgetopt},
    {"testnames", testnames},
    {"testuser", testuser},
    {"testpath", testpath},
    {"testenv", testenv},
    {"LastTest", NULL}
};

int main(int argc, char *argv[])
{
    CuSuiteList *alltests = NULL;
    CuString *output = CuStringNew();
    int i;
    int exclude = 0;
    int list_provided = 0;

    apr_initialize();
    atexit(apr_terminate);

    CuInit(argc, argv);

    apr_pool_create(&p, NULL);

    /* see if we're in exclude mode, see if list of testcases provided */
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-v")) {
            continue;
        }
        if (!strcmp(argv[i], "-x")) {
            exclude = 1;
            continue;
        }
        if (!strcmp(argv[i], "-l")) {
            for (i = 0; tests[i].func != NULL; i++) {
                printf("%s\n", tests[i].testname);
            }
            exit(0);
        }
        if (argv[i][0] == '-') {
            fprintf(stderr, "invalid option: `%s'\n", argv[i]);
            exit(1);
        }
        list_provided = 1;
    }

    if (!list_provided) {
        /* add everything */
        alltests = CuSuiteListNew("All APR Tests");
        for (i = 0; tests[i].func != NULL; i++) {
            CuSuiteListAdd(alltests, tests[i].func());
        }
    }
    else if (exclude) {
        /* add everything but the tests listed */
        alltests = CuSuiteListNew("Partial APR Tests");
        for (i = 0; tests[i].func != NULL; i++) {
            int this_test_excluded = 0;
            int j;

            for (j = 1; j < argc && !this_test_excluded; j++) {
                if (!strcmp(argv[j], tests[i].testname)) {
                    this_test_excluded = 1;
                }
            }
            if (!this_test_excluded) {
                CuSuiteListAdd(alltests, tests[i].func());
            }
        }
    }
    else {
        /* add only the tests listed */
        alltests = CuSuiteListNew("Partial APR Tests");
        for (i = 1; i < argc; i++) {
            int j;
            int found = 0;

            if (argv[i][0] == '-') {
                continue;
            }
            for (j = 0; tests[j].func != NULL; j++) {
                if (!strcmp(argv[i], tests[j].testname)) {
                    CuSuiteListAdd(alltests, tests[j].func());
                    found = 1;
                }
            }
            if (!found) {
                fprintf(stderr, "invalid test name: `%s'\n", argv[i]);
                exit(1);
            }
        }
    }
    
    CuSuiteListRunWithSummary(alltests);
    i = CuSuiteListDetails(alltests, output);
    printf("%s\n", output->buffer);

    return i > 0 ? 1 : 0;
}

