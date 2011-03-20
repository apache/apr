/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "abts.h"
#include "testutil.h"

#define APR_HOOK_PROBES_ENABLED

#define APR_HOOK_PROBE_ENTRY(ud,ns,name,args) \
  ud = toy_hook_probe_entry(#name)

#define APR_HOOK_PROBE_RETURN(ud,ns,name,rv,args) \
  toy_hook_probe_return(ud, #name, rv)

#define APR_HOOK_PROBE_INVOKE(ud,ns,name,src,args) \
  toy_hook_probe_invoke(ud, #name, src)

#define APR_HOOK_PROBE_COMPLETE(ud,ns,name,src,rv,args) \
  toy_hook_probe_complete(ud, #name, src, rv)

#include "apr_hooks.h"

#define TEST_DECLARE(type) type

APR_DECLARE_EXTERNAL_HOOK(test,TEST,int, toyhook, (char *x, apr_size_t s))

APR_HOOK_STRUCT(
    APR_HOOK_LINK(toyhook)
)

static void *toy_hook_probe_entry(const char *name);
static void toy_hook_probe_return(void *ud, const char *name, int rv);
static void toy_hook_probe_invoke(void *ud, const char *name, const char *src);
static void toy_hook_probe_complete(void *ud, const char *name,
                                    const char *src, int rv);

APR_IMPLEMENT_EXTERNAL_HOOK_RUN_ALL(test,TEST,int, toyhook,
                                    (char *x, apr_size_t s), (x, s), 0, -1)

typedef struct {
    char *buf;
    apr_size_t buf_size;
} hook_probe_data_t;

static apr_pool_t *probe_buf_pool;
static char *probe_buf;

static void safe_concat(char *buf, apr_size_t buf_size, const char *append)
{
    if (strlen(buf) + strlen(append) + 1 <= buf_size) {
        strcat(buf, append);
    }
}

static void *toy_hook_probe_entry(const char *name)
{
    hook_probe_data_t *ud = apr_palloc(probe_buf_pool, sizeof *ud);
    ud->buf_size = 18;
    ud->buf = (char *)apr_palloc(probe_buf_pool, ud->buf_size);
    ud->buf[0] = '\0';
    safe_concat(ud->buf, ud->buf_size, "E");
    return (void *)ud;
}

static void toy_hook_probe_return(void *vud, const char *name, int rv)
{
    hook_probe_data_t *ud = vud;

    safe_concat(ud->buf, ud->buf_size, "R");
    probe_buf = ud->buf;
}

static void toy_hook_probe_invoke(void *vud, const char *name,
                                  const char *src)
{
    hook_probe_data_t *ud = vud;

    safe_concat(ud->buf, ud->buf_size, "I");
    safe_concat(ud->buf, ud->buf_size, src);
}

static void toy_hook_probe_complete(void *vud, const char *name,
                                    const char *src, int rv)
{
    hook_probe_data_t *ud = vud;

    safe_concat(ud->buf, ud->buf_size, "C");
}

static int toyhook_1(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "1");
    return 0;
}

static int toyhook_2(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "2");
    return 0;
}

static int toyhook_3(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "3");
    return 0;
}

static int toyhook_4(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "4");
    return 0;
}

static int toyhook_5(char *x, apr_size_t buf_size)
{
    safe_concat(x, buf_size, "5");
    return 0;
}

static void test_basic_ordering(abts_case *tc, void *data)
{
    char buf[6] = {0};

    apr_hook_global_pool = p;
    apr_hook_deregister_all();

    apr_hook_debug_current = "5";
    test_hook_toyhook(toyhook_5, NULL, NULL, APR_HOOK_MIDDLE + 2);
    apr_hook_debug_current = "1";
    test_hook_toyhook(toyhook_1, NULL, NULL, APR_HOOK_MIDDLE - 2);
    apr_hook_debug_current = "3";
    test_hook_toyhook(toyhook_3, NULL, NULL, APR_HOOK_MIDDLE);
    apr_hook_debug_current = "2";
    test_hook_toyhook(toyhook_2, NULL, NULL, APR_HOOK_MIDDLE - 1);
    apr_hook_debug_current = "4";
    test_hook_toyhook(toyhook_4, NULL, NULL, APR_HOOK_MIDDLE + 1);

    apr_hook_sort_all();

    probe_buf_pool = p;
    test_run_toyhook(buf, sizeof buf);
    
    ABTS_STR_EQUAL(tc, "12345", buf);
    ABTS_STR_EQUAL(tc, "EI1CI2CI3CI4CI5CR", probe_buf);
}

static void test_pred_ordering(abts_case *tc, void *data)
{
    char buf[10] = {0};
    static const char *hook2_predecessors[] = {"1", NULL};
    static const char *hook3_predecessors[] = {"2", NULL};

    apr_hook_global_pool = p;
    apr_hook_deregister_all();

    apr_hook_debug_current = "1";
    test_hook_toyhook(toyhook_1, NULL, NULL, APR_HOOK_MIDDLE);
    apr_hook_debug_current = "3";
    test_hook_toyhook(toyhook_3, hook3_predecessors, NULL, APR_HOOK_MIDDLE);
    apr_hook_debug_current = "2";
    test_hook_toyhook(toyhook_2, hook2_predecessors, NULL, APR_HOOK_MIDDLE);
    apr_hook_debug_current = "2";
    test_hook_toyhook(toyhook_2, hook2_predecessors, NULL, APR_HOOK_MIDDLE);

    apr_hook_sort_all();

    probe_buf_pool = p;
    test_run_toyhook(buf, sizeof buf);
    
    /* FAILS ABTS_STR_EQUAL(tc, "1223", buf); */
}

abts_suite *testhooks(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_basic_ordering, NULL);
    abts_run_test(suite, test_pred_ordering, NULL);

    return suite;
}
