/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002 The Apache Software Foundation.  All rights
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

#include "apr.h"
#include "apr_strings.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_tables.h"
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif

int main(int argc, const char *const argv[])
{
    apr_pool_t *p;
    apr_table_t *t1, *t2;

    const char *val;

    apr_initialize();
    atexit(apr_terminate);

    apr_pool_create(&p, NULL);

    t1 = apr_table_make(p, 5);
    t2 = apr_table_make(p, 5);

    fprintf(stderr, "Test 1: apr_table_set...");
    apr_table_set(t1, "foo", "bar");
    if (!(val = apr_table_get(t1, "foo")) || strcmp(val, "bar")) {
        fprintf(stderr, "ERROR\n");
        exit(-1);
    }
    fprintf(stderr, "OK\n");

    fprintf(stderr, "Test 2: apr_table_add...");
    apr_table_add(t1, "foo", "12345");
    if (!(val = apr_table_get(t1, "foo")) || strcmp(val, "bar")) {
        fprintf(stderr, "ERROR\n");
        exit(-1);
    }
    fprintf(stderr, "OK\n");

    fprintf(stderr, "Test 3: apr_table_set...");
    apr_table_set(t1, "abc", "def");
    apr_table_addn(t1, "foo", "dummy1");
    apr_table_addn(t1, "foo", "dummy2");
    apr_table_set(t1, "def", "abc");
    apr_table_set(t1, "foo", "zzz");
    if (!(val = apr_table_get(t1, "foo")) || strcmp(val, "zzz") ||
        (apr_table_elts(t1)->nelts != 3) ||
        !(val = apr_table_get(t1, "abc")) || strcmp(val, "def") ||
        !(val = apr_table_get(t1, "def")) || strcmp(val, "abc")) {
        fprintf(stderr, "ERROR\n");
        exit(-1);
    }
    fprintf(stderr, "OK\n");

    fprintf(stderr, "Test 4: apr_table_unset...");
    apr_table_clear(t1);
    apr_table_addn(t1, "a", "1");
    apr_table_addn(t1, "b", "2");
    apr_table_addn(t1, "c", "3");
    apr_table_addn(t1, "b", "2");
    apr_table_addn(t1, "d", "4");
    apr_table_addn(t1, "e", "5");
    apr_table_addn(t1, "b", "2");
    apr_table_addn(t1, "f", "6");
    apr_table_unset(t1, "b");
    if ((apr_table_elts(t1)->nelts != 5) ||
        !(val = apr_table_get(t1, "a")) || strcmp(val, "1") ||
        !(val = apr_table_get(t1, "c")) || strcmp(val, "3") ||
        !(val = apr_table_get(t1, "d")) || strcmp(val, "4") ||
        !(val = apr_table_get(t1, "e")) || strcmp(val, "5") ||
        !(val = apr_table_get(t1, "f")) || strcmp(val, "6") ||
        (apr_table_get(t1, "b") != NULL)) {
        fprintf(stderr, "ERROR\n");
        exit(-1);
    }
    fprintf(stderr, "OK\n");

    fprintf(stderr, "Test 5: apr_table_overlap...");
    apr_table_clear(t1);
    apr_table_addn(t1, "a", "0");
    apr_table_addn(t1, "g", "7");
    apr_table_clear(t2);
    apr_table_addn(t2, "a", "1");
    apr_table_addn(t2, "b", "2");
    apr_table_addn(t2, "c", "3");
    apr_table_addn(t2, "b", "2.0");
    apr_table_addn(t2, "d", "4");
    apr_table_addn(t2, "e", "5");
    apr_table_addn(t2, "b", "2.");
    apr_table_addn(t2, "f", "6");
    apr_table_overlap(t1, t2, APR_OVERLAP_TABLES_SET);
    if ((apr_table_elts(t1)->nelts != 7) ||
        !(val = apr_table_get(t1, "a")) || strcmp(val, "1") ||
        !(val = apr_table_get(t1, "b")) || strcmp(val, "2") ||
        !(val = apr_table_get(t1, "c")) || strcmp(val, "3") ||
        !(val = apr_table_get(t1, "d")) || strcmp(val, "4") ||
        !(val = apr_table_get(t1, "e")) || strcmp(val, "5") ||
        !(val = apr_table_get(t1, "f")) || strcmp(val, "6") ||
        !(val = apr_table_get(t1, "g")) || strcmp(val, "7") ||
        (apr_table_get(t1, "h") != NULL)) {
        fprintf(stderr, "ERROR\n");
        exit(-1);
    }
    fprintf(stderr, "OK\n");

    return 0;
}
