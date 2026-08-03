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

#include "apr.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_xlate.h"
#include "abts.h"
#include "testutil.h"

#if APR_HAS_XLATE

static const char cs_utf7[] = "UTF-7";
static const char cs_utf8[] = "UTF-8";
static const char cs_latin1[] = "ISO-8859-1";
static const char cs_latin2[] = "ISO-8859-2";

static const char test_utf8[] = "Edelwei\xc3\x9f";
static const char test_utf7[] = "Edelwei+AN8-";
static const char test_latin1[] = "Edelwei\xdf";
static const char test_latin2[] = "Edelwei\xdf";

struct test_params
{
    const char *cs1;            /* source encoding */
    const char *cs2;            /* target encoding */
    const char *source;
    const char *expected;
    int check_xlate_supported;
};

#define DECLARE_TEST_PARAMS(src, dst, chk)              \
static struct test_params test_params_##src##_##dst = { \
    cs_##src, cs_##dst, test_##src, test_##dst, (chk)   \
}
DECLARE_TEST_PARAMS(utf8, utf8, 0);
DECLARE_TEST_PARAMS(utf8, latin1, 0);
DECLARE_TEST_PARAMS(latin1, utf8, 0);
DECLARE_TEST_PARAMS(latin1, latin2, 1);
DECLARE_TEST_PARAMS(latin2, latin1, 1);
DECLARE_TEST_PARAMS(utf8, utf7, 1);
/* NOTE: The system libiconv on macOS has a bug in the UTF-7 to UTF-8
 *       conversion that leaves the trailing '-' in the translated
 *       string, causing this test to fail. */
DECLARE_TEST_PARAMS(utf7, utf8, 1);
#undef DECLARE_TEST_PARAMS


static void test_conversion(const char *ctx, abts_case *tc, apr_xlate_t *convset,
                            const char *inbuf, const char *expected)
{
    static char buf[1024];
    apr_size_t inbytes_left = strlen(inbuf);
    apr_size_t outbytes_left = sizeof(buf) - 1;
    apr_status_t rv;

    rv = apr_xlate_conv_buffer(convset, inbuf, &inbytes_left, buf, &outbytes_left);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    if (rv != APR_SUCCESS)
        return;

    rv = apr_xlate_conv_buffer(convset, NULL, NULL,
                               buf + sizeof(buf) - outbytes_left - 1,
                               &outbytes_left);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    buf[sizeof(buf) - outbytes_left - 1] = '\0';

    ABTS_CTX_STR_EQUAL(ctx, tc, expected, buf);
}

/* some iconv implementations don't support all tested transforms;
 * example: 8859-1 <-> 8859-2 using native Solaris iconv
 */
static int is_transform_supported(abts_case *tc,
                                  const char *cs1, const char *cs2,
                                  apr_pool_t *pool) {
    apr_status_t rv;
    apr_xlate_t *convset;

    rv = apr_xlate_open(&convset, cs2, cs1, pool);
    if (rv != APR_SUCCESS) {
        return 0;
    }

    rv = apr_xlate_close(convset);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    return 1;
}

static void test_transformation(abts_case *tc, void *data)
{
    const struct test_params *const params = data;

    apr_status_t rv;
    apr_xlate_t *convset;

    if (params->check_xlate_supported
        && !is_transform_supported(tc, params->cs1, params->cs2, p)) {
        ABTS_SKIP(tc, data,
                  apr_psprintf(p, "xlate not supported: %s to %s",
                  params->cs1, params->cs2));
        return;
    }

    rv = apr_xlate_open(&convset, params->cs2, params->cs1, p);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);

    if (rv != APR_SUCCESS)
        return;

    test_conversion(apr_psprintf(p, "%s to %s", params->cs1, params->cs2),
                    tc, convset, params->source, params->expected);

    rv = apr_xlate_close(convset);
    ABTS_INT_EQUAL(tc, APR_SUCCESS, rv);
}

#endif /* APR_HAS_XLATE */

abts_suite *testxlate(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

#if APR_HAS_XLATE
    /* 1. Identity transformation: UTF-8 -> UTF-8 */
    abts_run_test(suite, test_transformation, &test_params_utf8_utf8);

    /* 2. UTF-8 <-> ISO-8859-1 */
    abts_run_test(suite, test_transformation, &test_params_latin1_utf8);
    abts_run_test(suite, test_transformation, &test_params_utf8_latin1);

    /* 3. Identity transformation: ISO-8859-1 <-> ISO-8859-2 */
    abts_run_test(suite, test_transformation, &test_params_latin1_latin2);
    abts_run_test(suite, test_transformation, &test_params_latin2_latin1);

    /* 4. Transformation using charset aliases */
    abts_run_test(suite, test_transformation, &test_params_utf8_utf7);
    abts_run_test(suite, test_transformation, &test_params_utf7_utf8);
#endif

    return suite;
}
