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
#include "apr_buckets.h"
#include "apr_strings.h"

static void test_create(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    ABTS_ASSERT(tc, "new brigade not NULL", bb != NULL);
    ABTS_ASSERT(tc, "new brigade is empty", APR_BRIGADE_EMPTY(bb));

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

static void test_simple(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba;
    apr_bucket_brigade *bb;
    apr_bucket *fb, *tb;

    ba = apr_bucket_alloc_create(p);
    bb = apr_brigade_create(p, ba);

    fb = APR_BRIGADE_FIRST(bb);
    ABTS_ASSERT(tc, "first bucket of empty brigade is sentinel",
                fb == APR_BRIGADE_SENTINEL(bb));

    fb = apr_bucket_flush_create(ba);
    APR_BRIGADE_INSERT_HEAD(bb, fb);

    ABTS_ASSERT(tc, "first bucket of brigade is flush",
                APR_BRIGADE_FIRST(bb) == fb);

    ABTS_ASSERT(tc, "bucket after flush is sentinel",
                APR_BUCKET_NEXT(fb) == APR_BRIGADE_SENTINEL(bb));

    tb = apr_bucket_transient_create("aaa", 3, ba);
    APR_BUCKET_INSERT_BEFORE(fb, tb);

    ABTS_ASSERT(tc, "bucket before flush now transient",
                APR_BUCKET_PREV(fb) == tb);
    ABTS_ASSERT(tc, "bucket after transient is flush",
                APR_BUCKET_NEXT(tb) == fb);
    ABTS_ASSERT(tc, "bucket before transient is sentinel",
                APR_BUCKET_PREV(tb) == APR_BRIGADE_SENTINEL(bb));

    apr_brigade_cleanup(bb);

    ABTS_ASSERT(tc, "cleaned up brigade was empty", APR_BRIGADE_EMPTY(bb));

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

static apr_bucket_brigade *make_simple_brigade(apr_bucket_alloc_t *ba,
                                               const char *first,
                                               const char *second)
{
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_bucket *e;

    e = apr_bucket_transient_create(first, strlen(first), ba);
    APR_BRIGADE_INSERT_TAIL(bb, e);

    if (second) {
        e = apr_bucket_transient_create(second, strlen(second), ba);
        APR_BRIGADE_INSERT_TAIL(bb, e);
    }

    return bb;
}

/* tests that 'bb' flattens to string 'expect'. */
static void flatten_match(abts_case *tc, const char *ctx,
                          apr_bucket_brigade *bb,
                          const char *expect)
{
    apr_size_t elen = strlen(expect);
    char *buf = malloc(elen);
    apr_size_t len = elen;
    char msg[200];

    apr_snprintf(msg, sizeof msg, "%s: flatten brigade", ctx);
    APR_ASSERT_SUCCESS(tc, msg, apr_brigade_flatten(bb, buf, &len));
    apr_snprintf(msg, sizeof msg, "%s: length match (%ld not %ld)", ctx,
                 (long)len, (long)elen);
    ABTS_ASSERT(tc, msg, len == elen);
    apr_snprintf(msg, sizeof msg, "%s: result match", ctx);
    ABTS_STR_NEQUAL(tc, expect, buf, len);
    free(buf);
}

static void test_flatten(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb;
    apr_bucket *e;

    bb = make_simple_brigade(ba, "hello, ", "world");

    /* add an EOS, which should be silently ignored. */
    e = apr_bucket_eos_create(ba);
    APR_BRIGADE_INSERT_HEAD(bb, e);

    flatten_match(tc, "flatten brigade", bb, "hello, world");

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

static int count_buckets(apr_bucket_brigade *bb)
{
    apr_bucket *e;
    int count = 0;

    for (e = APR_BRIGADE_FIRST(bb);
         e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e)) {
        count++;
    }

    return count;
}

static void test_split(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb, *bb2;
    apr_bucket *e;

    bb = make_simple_brigade(ba, "hello, ", "world");

    /* split at the "world" bucket */
    e = APR_BRIGADE_LAST(bb);
    bb2 = apr_brigade_split(bb, e);

    ABTS_ASSERT(tc, "split brigade contains one bucket",
                count_buckets(bb2) == 1);
    ABTS_ASSERT(tc, "original brigade contains one bucket",
                count_buckets(bb) == 1);

    flatten_match(tc, "match original brigade", bb, "hello, ");
    flatten_match(tc, "match split brigade", bb2, "world");

    apr_brigade_destroy(bb2);
    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

#define COUNT 3000
#define THESTR "hello"

static void test_bwrite(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_off_t length;
    int n;

    for (n = 0; n < COUNT; n++) {
        APR_ASSERT_SUCCESS(tc, "brigade_write",
                           apr_brigade_write(bb, NULL, NULL,
                                             THESTR, sizeof THESTR));
    }

    APR_ASSERT_SUCCESS(tc, "determine brigade length",
                       apr_brigade_length(bb, 1, &length));

    ABTS_ASSERT(tc, "brigade has correct length",
                length == (COUNT * sizeof THESTR));

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

static void test_splitline(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bin, *bout;

    bin = make_simple_brigade(ba, "blah blah blah-",
                              "end of line.\nfoo foo foo");
    bout = apr_brigade_create(p, ba);

    APR_ASSERT_SUCCESS(tc, "split line",
                       apr_brigade_split_line(bout, bin,
                                              APR_BLOCK_READ, 100));

    flatten_match(tc, "split line", bout, "blah blah blah-end of line.\n");
    flatten_match(tc, "remainder", bin, "foo foo foo");

    apr_brigade_destroy(bout);
    apr_brigade_destroy(bin);
    apr_bucket_alloc_destroy(ba);
}

static void test_splitline_exactly(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bin, *bout;

    bin = make_simple_brigade(ba, "foo bar.\n", NULL);
    bout = apr_brigade_create(p, ba);

    APR_ASSERT_SUCCESS(tc, "split line",
                       apr_brigade_split_line(bout, bin,
                                              APR_BLOCK_READ, 100));

    ABTS_INT_EQUAL(tc, 1, count_buckets(bout));
    ABTS_INT_EQUAL(tc, 0, count_buckets(bin));

    flatten_match(tc, "output brigade", bout, "foo bar.\n");

    apr_brigade_destroy(bout);
    apr_brigade_destroy(bin);
    apr_bucket_alloc_destroy(ba);
}


static void test_splitline_eos(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bin, *bout;
    apr_bucket *eos = apr_bucket_eos_create(ba);

    bin = make_simple_brigade(ba, "blah blah\n",
                              "foo foo");
    APR_BRIGADE_INSERT_TAIL(bin, eos);

    bout = apr_brigade_create(p, ba);

    APR_ASSERT_SUCCESS(tc, "split line eos #1",
                       apr_brigade_split_line(bout, bin,
                                              APR_BLOCK_READ, 100));
    
    flatten_match(tc, "split line eos", bout, "blah blah\n");

    apr_brigade_cleanup(bout);
    APR_ASSERT_SUCCESS(tc, "split line eos #2",
                       apr_brigade_split_line(bout, bin,
                                              APR_BLOCK_READ, 100));

    flatten_match(tc, "split line eos", bout, "foo foo");

    apr_brigade_destroy(bout);
    apr_brigade_destroy(bin);
    apr_bucket_alloc_destroy(ba);
}

static void test_splitboundary(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bin, *bout;

    /* fast path */
    bin = make_simple_brigade(ba, "quick brown fox",
                              " jumped over the lazy dog");
    bout = apr_brigade_create(p, ba);

    APR_ASSERT_SUCCESS(tc, "split boundary",
                       apr_brigade_split_boundary(bout, bin,
                                              APR_BLOCK_READ, "brown",
                                              APR_BUCKETS_STRING, 100));

    flatten_match(tc, "split boundary", bout, "quick ");
    flatten_match(tc, "remainder", bin, " fox jumped over the lazy dog");

    apr_brigade_destroy(bout);
    apr_brigade_destroy(bin);

    /* slow path */
    bin = make_simple_brigade(ba, "quick brown fox jum",
                              "ped over the lazy dog");
    bout = apr_brigade_create(p, ba);

    APR_ASSERT_SUCCESS(tc, "split boundary",
                       apr_brigade_split_boundary(bout, bin,
                                              APR_BLOCK_READ, "jumped",
                                              APR_BUCKETS_STRING, 100));

    flatten_match(tc, "split boundary", bout, "quick brown fox ");
    flatten_match(tc, "remainder", bin, " over the lazy dog");

    apr_brigade_destroy(bout);
    apr_brigade_destroy(bin);

    /* not found */
    bin = make_simple_brigade(ba, "quick brown fox jum",
                              "ped over the lazy dog");
    bout = apr_brigade_create(p, ba);

    ABTS_ASSERT(tc, "split boundary",
                apr_brigade_split_boundary(bout, bin,
                    APR_BLOCK_READ, "jumping",
                    APR_BUCKETS_STRING, 100) == APR_INCOMPLETE);

    flatten_match(tc, "split boundary", bout, "quick brown fox jumped over the lazy dog");
    flatten_match(tc, "remainder", bin, "");

    apr_brigade_destroy(bout);
    apr_brigade_destroy(bin);

    apr_bucket_alloc_destroy(ba);
}

/* Test that bucket E has content EDATA of length ELEN. */
static void test_bucket_content(abts_case *tc,
                                apr_bucket *e,
                                const char *edata,
                                apr_size_t elen)
{
    const char *adata;
    apr_size_t alen;

    APR_ASSERT_SUCCESS(tc, "read from bucket",
                       apr_bucket_read(e, &adata, &alen,
                                       APR_BLOCK_READ));

    ABTS_ASSERT(tc, "read expected length", alen == elen);
    ABTS_STR_NEQUAL(tc, edata, adata, elen);
}

static void test_splits(abts_case *tc, void *ctx)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb;
    apr_bucket *e;
    char *str = "alphabeta";
    int n;

    bb = apr_brigade_create(p, ba);

    APR_BRIGADE_INSERT_TAIL(bb,
                            apr_bucket_immortal_create(str, 9, ba));
    APR_BRIGADE_INSERT_TAIL(bb,
                            apr_bucket_transient_create(str, 9, ba));
    APR_BRIGADE_INSERT_TAIL(bb,
                            apr_bucket_heap_create(strdup(str), 9, free, ba));
    APR_BRIGADE_INSERT_TAIL(bb,
                            apr_bucket_pool_create(apr_pstrdup(p, str), 9, p,
                                                   ba));

    ABTS_ASSERT(tc, "four buckets inserted", count_buckets(bb) == 4);

    /* now split each of the buckets after byte 5 */
    for (n = 0, e = APR_BRIGADE_FIRST(bb); n < 4; n++) {
        ABTS_ASSERT(tc, "reached end of brigade",
                    e != APR_BRIGADE_SENTINEL(bb));
        ABTS_ASSERT(tc, "split bucket OK",
                    apr_bucket_split(e, 5) == APR_SUCCESS);
        e = APR_BUCKET_NEXT(e);
        ABTS_ASSERT(tc, "split OK", e != APR_BRIGADE_SENTINEL(bb));
        e = APR_BUCKET_NEXT(e);
    }

    ABTS_ASSERT(tc, "four buckets split into eight",
                count_buckets(bb) == 8);

    for (n = 0, e = APR_BRIGADE_FIRST(bb); n < 4; n++) {
        const char *data;
        apr_size_t len;

        APR_ASSERT_SUCCESS(tc, "read alpha from bucket",
                           apr_bucket_read(e, &data, &len, APR_BLOCK_READ));
        ABTS_ASSERT(tc, "read 5 bytes", len == 5);
        ABTS_STR_NEQUAL(tc, "alpha", data, 5);

        e = APR_BUCKET_NEXT(e);

        APR_ASSERT_SUCCESS(tc, "read beta from bucket",
                           apr_bucket_read(e, &data, &len, APR_BLOCK_READ));
        ABTS_ASSERT(tc, "read 4 bytes", len == 4);
        ABTS_STR_NEQUAL(tc, "beta", data, 5);

        e = APR_BUCKET_NEXT(e);
    }

    /* now delete the "alpha" buckets */
    for (n = 0, e = APR_BRIGADE_FIRST(bb); n < 4; n++) {
        apr_bucket *f;

        ABTS_ASSERT(tc, "reached end of brigade",
                    e != APR_BRIGADE_SENTINEL(bb));
        f = APR_BUCKET_NEXT(e);
        apr_bucket_delete(e);
        e = APR_BUCKET_NEXT(f);
    }

    ABTS_ASSERT(tc, "eight buckets reduced to four",
                count_buckets(bb) == 4);

    flatten_match(tc, "flatten beta brigade", bb,
                  "beta" "beta" "beta" "beta");

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

#define TIF_FNAME "testfile.txt"

static void test_insertfile(abts_case *tc, void *ctx)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb;
    const apr_off_t bignum = (APR_INT64_C(2) << 32) + 424242;
    apr_off_t count;
    apr_file_t *f;
    apr_bucket *e;

    ABTS_ASSERT(tc, "open test file",
                apr_file_open(&f, TIF_FNAME,
                              APR_FOPEN_WRITE | APR_FOPEN_TRUNCATE
                            | APR_FOPEN_CREATE | APR_FOPEN_SPARSE,
                              APR_FPROT_OS_DEFAULT, p) == APR_SUCCESS);

    if (apr_file_trunc(f, bignum)) {
        apr_file_close(f);
        apr_file_remove(TIF_FNAME, p);
        ABTS_NOT_IMPL(tc, "Skipped: could not create large file");
        return;
    }

    bb = apr_brigade_create(p, ba);

    e = apr_brigade_insert_file(bb, f, 0, bignum, p);

    ABTS_ASSERT(tc, "inserted file was not at end of brigade",
                e == APR_BRIGADE_LAST(bb));

    /* check that the total size of inserted buckets is equal to the
     * total size of the file. */
    count = 0;

    for (e = APR_BRIGADE_FIRST(bb);
         e != APR_BRIGADE_SENTINEL(bb);
         e = APR_BUCKET_NEXT(e)) {
        ABTS_ASSERT(tc, "bucket size sane", e->length != (apr_size_t)-1);
        count += e->length;
    }

    ABTS_ASSERT(tc, "total size of buckets incorrect", count == bignum);

    apr_brigade_destroy(bb);

    /* Truncate the file to zero size before close() so that we don't
     * actually write out the large file if we are on a non-sparse file
     * system - like Mac OS X's HFS.  Otherwise, pity the poor user who
     * has to wait for the 8GB file to be written to disk.
     */
    apr_file_trunc(f, 0);

    apr_file_close(f);
    apr_bucket_alloc_destroy(ba);
    apr_file_remove(TIF_FNAME, p);
}

/* Make a test file named FNAME, and write CONTENTS to it. */
static apr_file_t *make_test_file(abts_case *tc, const char *fname,
                                  const char *contents)
{
    apr_file_t *f;

    ABTS_ASSERT(tc, "create test file",
                apr_file_open(&f, fname,
                              APR_FOPEN_READ|APR_FOPEN_WRITE|APR_FOPEN_TRUNCATE|APR_FOPEN_CREATE,
                              APR_FPROT_OS_DEFAULT, p) == APR_SUCCESS);

    ABTS_ASSERT(tc, "write test file contents",
                apr_file_puts(contents, f) == APR_SUCCESS);

    return f;
}

static void test_manyfile(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_file_t *f;

    f = make_test_file(tc, "manyfile.bin",
                       "world" "hello" "brave" " ,\n");

    apr_brigade_insert_file(bb, f, 5, 5, p);
    apr_brigade_insert_file(bb, f, 16, 1, p);
    apr_brigade_insert_file(bb, f, 15, 1, p);
    apr_brigade_insert_file(bb, f, 10, 5, p);
    apr_brigade_insert_file(bb, f, 15, 1, p);
    apr_brigade_insert_file(bb, f, 0, 5, p);
    apr_brigade_insert_file(bb, f, 17, 1, p);

    /* can you tell what it is yet? */
    flatten_match(tc, "file seek test", bb,
                  "hello, brave world\n");

    apr_file_close(f);
    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

/* Regression test for PR 34708, where a file bucket will keep
 * duplicating itself on being read() when EOF is reached
 * prematurely. */
static void test_truncfile(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_file_t *f = make_test_file(tc, "testfile.txt", "hello");
    apr_bucket *e;
    const char *buf;
    apr_size_t len;

    apr_brigade_insert_file(bb, f, 0, 5, p);

    apr_file_trunc(f, 0);

    e = APR_BRIGADE_FIRST(bb);

    ABTS_ASSERT(tc, "single bucket in brigade",
                APR_BUCKET_NEXT(e) == APR_BRIGADE_SENTINEL(bb));

    apr_bucket_file_enable_mmap(e, 0);

    ABTS_ASSERT(tc, "read gave APR_EOF",
                apr_bucket_read(e, &buf, &len, APR_BLOCK_READ) == APR_EOF);

    ABTS_ASSERT(tc, "read length 0", len == 0);

    ABTS_ASSERT(tc, "still a single bucket in brigade",
                APR_BUCKET_NEXT(e) == APR_BRIGADE_SENTINEL(bb));

    apr_file_close(f);
    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

#if APR_HAS_MMAP
#include "apr_mmap.h"

#if APR_HAVE_UNISTD_H
#include <unistd.h> /* for sysconf() */
#endif

/* Byte expected at offset OFF of a file created by make_pattern_file(). */
#define PATTERN_BYTE(off) ((char)((off) % 251))

/* Make a test file named FNAME of length LEN, opened with FLAGS. */
static apr_file_t *make_pattern_file(abts_case *tc, const char *fname,
                                     apr_off_t len, apr_int32_t flags)
{
    apr_file_t *f;
    char buf[8192];
    apr_off_t off = 0;
    apr_status_t rv;

    rv = apr_file_open(&f, fname,
                       APR_FOPEN_WRITE|APR_FOPEN_TRUNCATE|APR_FOPEN_CREATE,
                       APR_FPROT_OS_DEFAULT, p);
    APR_ASSERT_SUCCESS(tc, "create test file", rv);
    if (rv != APR_SUCCESS) {
        return NULL;
    }

    while (off < len) {
        apr_size_t n, chunk = sizeof buf;

        if ((apr_off_t)chunk > len - off) {
            chunk = (apr_size_t)(len - off);
        }
        for (n = 0; n < chunk; n++) {
            buf[n] = PATTERN_BYTE(off + n);
        }
        APR_ASSERT_SUCCESS(tc, "write test file contents",
                           apr_file_write_full(f, buf, chunk, NULL));
        off += chunk;
    }
    apr_file_close(f);

    rv = apr_file_open(&f, fname, flags, APR_FPROT_OS_DEFAULT, p);
    APR_ASSERT_SUCCESS(tc, "reopen test file", rv);

    return rv == APR_SUCCESS ? f : NULL;
}

/* Returns non-zero if the LEN bytes at BUF match those expected at
 * offset OFF of a file created by make_pattern_file(). */
static int pattern_match(const char *buf, apr_size_t len, apr_off_t off)
{
    apr_size_t n;

    for (n = 0; n < len; n++) {
        if (buf[n] != PATTERN_BYTE(off + n)) {
            return 0;
        }
    }
    return 1;
}

/* A FILE bucket which starts at an offset which is not a multiple of
 * the page size, and is larger than APR_MMAP_LIMIT, is read up to the
 * next page boundary and then via a sequence of MMAP buckets. */
static void test_mmap_unaligned(abts_case *tc, void *data)
{
    static const char fname[] = "mmapfile.bin";
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    const apr_off_t start = 4321;
    const apr_size_t tail = 100000;
    const apr_size_t flen = APR_MMAP_LIMIT + tail;
    apr_size_t expect[4] = { 0 }; /* expected bucket lengths */
    int first_mmap = 0; /* index of first MMAP bucket */
    apr_off_t off = start;
    apr_file_t *f;
    apr_bucket *e;
    int count = 0;

#ifdef _SC_PAGESIZE
    {
        apr_size_t psize = (apr_size_t)sysconf(_SC_PAGESIZE);

        /* Assumes the page size is small enough that the boundary is
         * reached in a single read. */
        if (psize > APR_BUCKET_BUFF_SIZE) {
            ABTS_NOT_IMPL(tc, "Skipped: page size too large");
            return;
        }
        expect[count++] = psize - (apr_size_t)start % psize;
        first_mmap = 1;
    }
#endif
    expect[count++] = APR_MMAP_LIMIT;
    expect[count] = flen - APR_MMAP_LIMIT - (first_mmap ? expect[0] : 0);

    f = make_pattern_file(tc, fname, start + flen, APR_FOPEN_READ);
    if (!f) {
        return;
    }

    apr_brigade_insert_file(bb, f, start, flen, p);

    count = 0;
    while ((e = APR_BRIGADE_FIRST(bb)) != APR_BRIGADE_SENTINEL(bb)
           && count < 3) {
        const char *buf;
        apr_size_t len;

        ABTS_ASSERT(tc, "bucket is a FILE bucket before read",
                    APR_BUCKET_IS_FILE(e));
        APR_ASSERT_SUCCESS(tc, "read file bucket",
                           apr_bucket_read(e, &buf, &len, APR_BLOCK_READ));
        if (count < first_mmap) {
            ABTS_ASSERT(tc, "FILE bucket read up to page boundary",
                        !APR_BUCKET_IS_MMAP(e));
        }
        else {
            ABTS_ASSERT(tc, "FILE bucket morphed to MMAP bucket",
                        APR_BUCKET_IS_MMAP(e));
        }
        ABTS_SIZE_EQUAL(tc, expect[count], len);
        ABTS_ASSERT(tc, "bucket contents match",
                    pattern_match(buf, len, off));

        off += len;
        count++;
        apr_bucket_delete(e);
    }

    ABTS_ASSERT(tc, "read whole file", off == start + (apr_off_t)flen);
    ABTS_ASSERT(tc, "brigade is empty", APR_BRIGADE_EMPTY(bb));

    apr_file_close(f);
    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
    apr_file_remove(fname, p);
}

/* If the file cannot be mmap()ed, the FILE bucket must fall back to
 * reading the file. */
static void test_mmap_fail(abts_case *tc, void *data)
{
    static const char fname[] = "mmapfile.bin";
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    const apr_size_t flen = 3 * APR_BUCKET_BUFF_SIZE;
    apr_off_t off = 0;
    apr_file_t *f;
    apr_bucket *e;
    const char *buf;
    apr_size_t len;

    /* apr_mmap_create() fails for a buffered file. */
    f = make_pattern_file(tc, fname, flen,
                          APR_FOPEN_READ | APR_FOPEN_BUFFERED);
    if (!f) {
        return;
    }

    e = apr_brigade_insert_file(bb, f, 0, flen, p);

    APR_ASSERT_SUCCESS(tc, "read file bucket",
                       apr_bucket_read(e, &buf, &len, APR_BLOCK_READ));
    ABTS_ASSERT(tc, "FILE bucket did not morph to MMAP bucket",
                !APR_BUCKET_IS_MMAP(e));
    ABTS_ASSERT(tc, "partial read of file", len > 0 && len < flen);

    ABTS_ASSERT(tc, "remainder is a FILE bucket",
                APR_BUCKET_NEXT(e) != APR_BRIGADE_SENTINEL(bb)
                && APR_BUCKET_IS_FILE(APR_BUCKET_NEXT(e)));

    /* The whole file is still readable. */
    while ((e = APR_BRIGADE_FIRST(bb)) != APR_BRIGADE_SENTINEL(bb)) {
        APR_ASSERT_SUCCESS(tc, "read bucket",
                           apr_bucket_read(e, &buf, &len, APR_BLOCK_READ));
        ABTS_ASSERT(tc, "bucket contents match",
                    pattern_match(buf, len, off));
        off += len;
        apr_bucket_delete(e);
    }
    ABTS_ASSERT(tc, "read whole file", off == (apr_off_t)flen);

    apr_file_close(f);
    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
    apr_file_remove(fname, p);
}
#endif /* APR_HAS_MMAP */

static const char hello[] = "hello, world";

static void test_partition(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_bucket *e;

    e = apr_bucket_immortal_create(hello, strlen(hello), ba);
    APR_BRIGADE_INSERT_HEAD(bb, e);

    APR_ASSERT_SUCCESS(tc, "partition brigade",
                       apr_brigade_partition(bb, 5, &e));

    test_bucket_content(tc, APR_BRIGADE_FIRST(bb),
                        "hello", 5);

    test_bucket_content(tc, APR_BRIGADE_LAST(bb),
                        ", world", 7);

    ABTS_ASSERT(tc, "partition returns APR_INCOMPLETE",
                apr_brigade_partition(bb, 8192, &e));

    ABTS_ASSERT(tc, "APR_INCOMPLETE partition returned sentinel",
                e == APR_BRIGADE_SENTINEL(bb));

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

static void test_write_split(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb1 = apr_brigade_create(p, ba);
    apr_bucket_brigade *bb2;
    apr_bucket *e;

    e = apr_bucket_heap_create(hello, strlen(hello), NULL, ba);
    APR_BRIGADE_INSERT_HEAD(bb1, e);
    apr_bucket_split(e, strlen("hello, "));
    bb2 = apr_brigade_split(bb1, APR_BRIGADE_LAST(bb1));
    apr_brigade_write(bb1, NULL, NULL, "foo", strlen("foo"));
    test_bucket_content(tc, APR_BRIGADE_FIRST(bb2), "world", 5);

    apr_brigade_destroy(bb1);
    apr_brigade_destroy(bb2);
    apr_bucket_alloc_destroy(ba);
}

static void test_write_putstrs(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb = apr_brigade_create(p, ba);
    apr_bucket *e;
    char buf[30];
    apr_size_t len = sizeof(buf);
    const char *expect = "123456789abcdefghij";

    e = apr_bucket_heap_create("1", 1, NULL, ba);
    APR_BRIGADE_INSERT_HEAD(bb, e);

    apr_brigade_putstrs(bb, NULL, NULL, "2", "34", "567", "8", "9a", "bcd",
                        "e", "f", "gh", "i", NULL);
    apr_brigade_putstrs(bb, NULL, NULL, "j", NULL);
    e = apr_bucket_eos_create(ba);
    APR_BRIGADE_INSERT_HEAD(bb, e);

    APR_ASSERT_SUCCESS(tc, "apr_brigade_flatten",
                       apr_brigade_flatten(bb, buf, &len));
    ABTS_STR_NEQUAL(tc, expect, buf, strlen(expect));

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

static void test_iovec(abts_case *tc, void *data)
{
    apr_bucket_alloc_t *ba = apr_bucket_alloc_create(p);
    apr_bucket_brigade *bb;
    apr_bucket *e;
    struct iovec vec[3];
    int vecs = 3;

    bb = make_simple_brigade(ba, "foo", "bar");

    /* add an EOS, which should be silently ignored. */
    e = apr_bucket_eos_create(ba);
    APR_BRIGADE_INSERT_HEAD(bb, e);

    APR_ASSERT_SUCCESS(tc, "apr_brigade_to_iovec",
                       apr_brigade_to_iovec(bb, vec, &vecs));

    ABTS_INT_EQUAL(tc, 2, vecs);
    ABTS_STR_EQUAL(tc, "foo", vec[0].iov_base);
    ABTS_SIZE_EQUAL(tc, 3, vec[0].iov_len);
    ABTS_STR_EQUAL(tc, "bar", vec[1].iov_base);
    ABTS_SIZE_EQUAL(tc, 3, vec[1].iov_len);

    apr_brigade_destroy(bb);
    apr_bucket_alloc_destroy(ba);
}

abts_suite *testbuckets(abts_suite *suite)
{
    suite = ADD_SUITE(suite);

    abts_run_test(suite, test_create, NULL);
    abts_run_test(suite, test_simple, NULL);
    abts_run_test(suite, test_flatten, NULL);
    abts_run_test(suite, test_split, NULL);
    abts_run_test(suite, test_bwrite, NULL);
    abts_run_test(suite, test_splitline, NULL);
    abts_run_test(suite, test_splitline_exactly, NULL);
    abts_run_test(suite, test_splitline_eos, NULL);
    abts_run_test(suite, test_splitboundary, NULL);
    abts_run_test(suite, test_splits, NULL);
    abts_run_test(suite, test_insertfile, NULL);
    abts_run_test(suite, test_manyfile, NULL);
    abts_run_test(suite, test_truncfile, NULL);
#if APR_HAS_MMAP
    abts_run_test(suite, test_mmap_unaligned, NULL);
    abts_run_test(suite, test_mmap_fail, NULL);
#endif
    abts_run_test(suite, test_partition, NULL);
    abts_run_test(suite, test_write_split, NULL);
    abts_run_test(suite, test_write_putstrs, NULL);
    abts_run_test(suite, test_iovec, NULL);

    return suite;
}


