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

#ifndef APR_TEST_INCLUDES
#define APR_TEST_INCLUDES

#include "CuTest.h"
#include "apr_pools.h"

/* XXX FIXME */
#ifdef WIN32
#define EXTENSION ".exe"
#elif NETWARE
#define EXTENSION ".nlm"
#else
#define EXTENSION
#endif

/* Some simple functions to make the test apps easier to write and
 * a bit more consistent...
 */

extern apr_pool_t *p;

CuSuite *getsuite(void);

CuSuite *testatomic(void);
CuSuite *testdir(void);
CuSuite *testdso(void);
CuSuite *testdup(void);
CuSuite *testenv(void);
CuSuite *testfile(void);
CuSuite *testfilecopy(void);
CuSuite *testfileinfo(void);
CuSuite *testflock(void);
CuSuite *testfmt(void);
CuSuite *testfnmatch(void);
CuSuite *testgetopt(void);
CuSuite *testglobalmutex(void);
CuSuite *testhash(void);
CuSuite *testipsub(void);
CuSuite *testlock(void);
CuSuite *testmmap(void);
CuSuite *testnames(void);
CuSuite *testoc(void);
CuSuite *testpath(void);
CuSuite *testpipe(void);
CuSuite *testpoll(void);
CuSuite *testpool(void);
CuSuite *testproc(void);
CuSuite *testprocmutex(void);
CuSuite *testrand(void);
CuSuite *testrand2(void);
CuSuite *testsleep(void);
CuSuite *testshm(void);
CuSuite *testsock(void);
CuSuite *testsockets(void);
CuSuite *testsockopt(void);
CuSuite *teststr(void);
CuSuite *teststrnatcmp(void);
CuSuite *testtable(void);
CuSuite *testthread(void);
CuSuite *testtime(void);
CuSuite *testud(void);
CuSuite *testuser(void);
CuSuite *testvsn(void);


/* Assert that RV is an APR_SUCCESS value; else fail giving strerror
 * for RV and CONTEXT message. */
void apr_assert_success(CuTest* tc, const char *context, apr_status_t rv);


#endif /* APR_TEST_INCLUDES */
