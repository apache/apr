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


#include "apr_general.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_file_io.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "test_apr.h"

#define ALLOC_BYTES 1024

static apr_pool_t *pmain = NULL;
static apr_pool_t *pchild = NULL;

static void alloc_bytes(CuTest *tc)
{
    int i;
    char *alloc;
    
    alloc = apr_palloc(pmain, ALLOC_BYTES);
    CuAssertPtrNotNull(tc, alloc);

    for (i=0;i<ALLOC_BYTES;i++) {
        char *ptr = alloc + i;
        *ptr = 0xa;
    }
    /* This is just added to get the positive.  If this test fails, the
     * suite will seg fault.
     */
    CuAssertTrue(tc, 1);
}

static void calloc_bytes(CuTest *tc)
{
    int i;
    char *alloc;
    
    alloc = apr_pcalloc(pmain, ALLOC_BYTES);
    CuAssertPtrNotNull(tc, alloc);

    for (i=0;i<ALLOC_BYTES;i++) {
        char *ptr = alloc + i;
        CuAssertTrue(tc, *ptr == '\0');
    }
}

static void parent_pool(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_pool_create(&pmain, NULL);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrNotNull(tc, pmain);
}

static void child_pool(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_pool_create(&pchild, pmain);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrNotNull(tc, pchild);
}

static void test_ancestor(CuTest *tc)
{
    CuAssertIntEquals(tc, 1, apr_pool_is_ancestor(pmain, pchild));
}

static void test_notancestor(CuTest *tc)
{
    CuAssertIntEquals(tc, 0, apr_pool_is_ancestor(pchild, pmain));
}

CuSuite *testpool(void)
{
    CuSuite *suite = CuSuiteNew("Pools");

    SUITE_ADD_TEST(suite, parent_pool);
    SUITE_ADD_TEST(suite, child_pool);
    SUITE_ADD_TEST(suite, test_ancestor);
    SUITE_ADD_TEST(suite, test_notancestor);
    SUITE_ADD_TEST(suite, alloc_bytes);
    SUITE_ADD_TEST(suite, calloc_bytes);

    return suite;
}

