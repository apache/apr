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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifndef ABTS_H
#define ABTS_H

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE  1
#endif

struct sub_suite {
    char *name;
    int num_test;
    int failed;
    int skipped;
    int not_run;
    int not_impl;
    struct sub_suite *next;
};
typedef struct sub_suite sub_suite;

struct abts_suite {
    sub_suite *head;
    sub_suite *tail;
};
typedef struct abts_suite abts_suite;

struct abts_case {
    int failed;
    int skipped;
    sub_suite *suite;
};
typedef struct abts_case abts_case;

typedef void (*test_func)(abts_case *tc, void *data);

#define ADD_SUITE(suite) abts_add_suite(suite, __FILE__);

abts_suite *abts_add_suite(abts_suite *suite, const char *suite_name);
void abts_run_test(abts_suite *ts, test_func f, void *value);
void abts_log_message(const char *fmt, ...);

void abts_int_equal(const char *ctx, abts_case *tc, const int expected, const int actual, int lineno);
void abts_int_nequal(const char *ctx, abts_case *tc, const int expected, const int actual, int lineno);
void abts_uint_equal(const char *ctx, abts_case *tc, const unsigned int expected,
                     const unsigned int actual, int lineno);
void abts_uint_nequal(const char *ctx, abts_case *tc, const unsigned int expected,
                      const unsigned int actual, int lineno);
void abts_long_equal(const char *ctx, abts_case *tc, const long expected,
                     const long actual, int lineno);
void abts_long_nequal(const char *ctx, abts_case *tc, const long expected,
                      const long actual, int lineno);
void abts_ulong_equal(const char *ctx, abts_case *tc, const unsigned long expected,
                      const unsigned long actual, int lineno);
void abts_ulong_nequal(const char *ctx, abts_case *tc, const unsigned long expected,
                       const unsigned long actual, int lineno);
void abts_llong_equal(const char *ctx, abts_case *tc, const long long expected,
                      const long long actual, int lineno);
void abts_llong_nequal(const char *ctx, abts_case *tc, const long long expected,
                       const long long actual, int lineno);
void abts_ullong_equal(const char *ctx, abts_case *tc, const unsigned long long expected,
                       const unsigned long long actual, int lineno);
void abts_ullong_nequal(const char *ctx, abts_case *tc, const unsigned long long expected,
                        const unsigned long long actual, int lineno);
void abts_size_equal(const char *ctx, abts_case *tc, size_t expected, size_t actual, int lineno);
void abts_size_nequal(const char *ctx, abts_case *tc, size_t expected, size_t actual, int lineno);
void abts_str_equal(const char *ctx, abts_case *tc, const char *expected, const char *actual, int lineno);
void abts_str_nequal(const char *ctx, abts_case *tc, const char *expected, const char *actual,
                       size_t n, int lineno);
void abts_ptr_notnull(const char *ctx, abts_case *tc, const void *ptr, int lineno);
void abts_ptr_equal(const char *ctx, abts_case *tc, const void *expected, const void *actual, int lineno);
void abts_true(const char *ctx, abts_case *tc, int condition, int lineno);
void abts_fail(abts_case *tc, const char *message, int lineno);
void abts_skip(abts_case *tc, const char *message, int lineno);
void abts_not_impl(abts_case *tc, const char *message, int lineno);
void abts_assert(abts_case *tc, const char *message, int condition, int lineno);

/* Convenience macros. Ryan hates these! */
#define ABTS_INT_EQUAL(a, b, c)     abts_int_equal(NULL, (a), (b), (c), __LINE__)
#define ABTS_INT_NEQUAL(a, b, c)    abts_int_nequal(NULL, (a), (b), (c), __LINE__)
#define ABTS_UINT_EQUAL(a, b, c)    abts_uint_equal(NULL, (a), (b), (c), __LINE__)
#define ABTS_UINT_NEQUAL(a, b, c)   abts_uint_nequal(NULL, (a), (b), (c), __LINE__)
#define ABTS_LONG_EQUAL(a, b, c)    abts_long_equal(NULL, (a), (b), (c), __LINE__)
#define ABTS_LONG_NEQUAL(a, b, c)   abts_long_nequal(NULL, (a), (b), (c), __LINE__)
#define ABTS_ULONG_EQUAL(a, b, c)   abts_ulong_equal(NULL, (a), (b), (c), __LINE__)
#define ABTS_ULONG_NEQUAL(a, b, c)  abts_ulong_nequal(NULL, (a), (b), (c), __LINE__)
#define ABTS_LLONG_EQUAL(a, b, c)   abts_llong_equal(NULL, (a), (b), (c), __LINE__)
#define ABTS_LLONG_NEQUAL(a, b, c)  abts_llong_nequal(NULL, (a), (b), (c), __LINE__)
#define ABTS_ULLONG_EQUAL(a, b, c)  abts_ullong_equal(NULL, (a), (b), (c), __LINE__)
#define ABTS_ULLONG_NEQUAL(a, b, c) abts_ullong_nequal(NULL, (a), (b), (c), __LINE__)
#define ABTS_SIZE_EQUAL(a, b, c)    abts_size_equal(NULL, (a), (b), (c), __LINE__)
#define ABTS_SIZE_NEQUAL(a, b, c)   abts_size_nequal(NULL, (a), (b), (c), __LINE__)
#define ABTS_STR_EQUAL(a, b, c)     abts_str_equal(NULL, (a), (b), (c), __LINE__)
#define ABTS_STR_NEQUAL(a, b, c, d) abts_str_nequal(NULL, (a), (b), (c), (d), __LINE__)
#define ABTS_PTR_NOTNULL(a, b)      abts_ptr_notnull(NULL, (a), (b), __LINE__)
#define ABTS_PTR_EQUAL(a, b, c)     abts_ptr_equal(NULL, (a), (b), (c), __LINE__)
#define ABTS_TRUE(a, b)             abts_true(NULL, (a), (b), __LINE__);

#define ABTS_CTX_INT_EQUAL(x, a, b, c)     abts_int_equal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_INT_NEQUAL(x, a, b, c)    abts_int_nequal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_UINT_EQUAL(x, a, b, c)    abts_uint_equal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_UINT_NEQUAL(x, a, b, c)   abts_uint_nequal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_LONG_EQUAL(x, a, b, c)    abts_long_equal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_LONG_NEQUAL(x, a, b, c)   abts_long_nequal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_ULONG_EQUAL(x, a, b, c)   abts_ulong_equal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_ULONG_NEQUAL(x, a, b, c)  abts_ulong_nequal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_LLONG_EQUAL(x, a, b, c)   abts_llong_equal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_LLONG_NEQUAL(x, a, b, c)  abts_llong_nequal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_ULLONG_EQUAL(x, a, b, c)  abts_ullong_equal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_ULLONG_NEQUAL(x, a, b, c) abts_ullong_nequal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_SIZE_EQUAL(x, a, b, c)    abts_size_equal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_SIZE_NEQUAL(x, a, b, c)   abts_size_nequal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_STR_EQUAL(x, a, b, c)     abts_str_equal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_STR_NEQUAL(x, a, b, c, d) abts_str_nequal((x), (a), (b), (c), (d), __LINE__)
#define ABTS_CTX_PTR_NOTNULL(x, a, b)      abts_ptr_notnull((x), (a), (b), __LINE__)
#define ABTS_CTX_PTR_EQUAL(x, a, b, c)     abts_ptr_equal((x), (a), (b), (c), __LINE__)
#define ABTS_CTX_TRUE(x, a, b)             abts_true((x), (a), (b), __LINE__);

#define ABTS_FAIL(a, b)             abts_fail((a), (b), __LINE__);
#define ABTS_NOT_IMPL(a, b)         abts_not_impl((a), (b), __LINE__);
#define ABTS_ASSERT(a, b, c)        abts_assert((a), (b), (c), __LINE__);

/* When skipping tests, make a reference to the test data parameter
   to avoid unused variable warnings. */
#define ABTS_SKIP(tc, data, msg) do {     \
        ((void)(data));                   \
        abts_skip((tc), (msg), __LINE__); \
    } while (0)

abts_suite *run_tests(abts_suite *suite);
abts_suite *run_tests1(abts_suite *suite);


#endif

#ifdef __cplusplus
}
#endif
