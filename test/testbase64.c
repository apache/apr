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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "apr_base64.h"

#include "abts.h"
#include "testutil.h"

static struct {
   const char *orig;
   const char *enc;
} base64_tbl[] =
{
    {"", ""},
    {"H", "SA=="},
    {"He", "SGU="},
    {"Hel", "SGVs"},
    {"Hell", "SGVsbA=="},
    {"Hello", "SGVsbG8="},
    {"Hello World", "SGVsbG8gV29ybGQ="},
    {"\xff\xff\xff\xff", "/////w=="},
};
static int num_base64 = sizeof(base64_tbl) / sizeof(base64_tbl[0]);

static void test_base64(abts_case *tc, void *data)
{
    apr_pool_t *pool;
    int i;

    apr_pool_create(&pool, NULL);

    for (i = 0; i < num_base64; i++) {
        char *enc;
        int orig_len, enc_len, b64_len, b64_enc_len;

        apr_pool_clear(pool);

        orig_len = strlen(base64_tbl[i].orig);
        enc_len = strlen(base64_tbl[i].enc);

        /* includes + 1 for term null */
        b64_enc_len = apr_base64_encode_len(orig_len);
        ABTS_ASSERT(tc, "base 64 exp. length", (enc_len == (b64_enc_len - 1)));

        enc = apr_palloc(pool, b64_enc_len);

        b64_len = apr_base64_encode(enc, base64_tbl[i].orig, orig_len);

        ABTS_ASSERT(tc, "base 64 encoded length", (b64_enc_len == b64_len));
        ABTS_ASSERT(tc, "base 64 encoded matches expected output",
                    (memcmp(enc, base64_tbl[i].enc, b64_enc_len) == 0));

        enc = apr_pbase64_encode(pool, base64_tbl[i].orig);
        ABTS_ASSERT(tc, "base 64 encoded from pool matches expected output",
                (strcmp(enc, base64_tbl[i].enc) == 0));
        ABTS_ASSERT(tc, "base 64 length", strlen(enc) == strlen(base64_tbl[i].enc));
    }
}

abts_suite *testbase64(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_base64, NULL);

    return suite;
}
