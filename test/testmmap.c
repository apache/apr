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
#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_file_io.h"
#include "apr_strings.h"

/* hmmm, what is a truly portable define for the max path
 * length on a platform?
 */
#define PATH_LEN 255
#define TEST_STRING "This is the MMAP data file."APR_EOL_STR

#if !APR_HAS_MMAP
static void not_implemented(CuTest *tc)
{
    CuNotImpl(tc, "User functions");
}

#else

static apr_mmap_t *themmap = NULL;
static apr_file_t *thefile = NULL;
static char *file1;
static apr_finfo_t finfo;
static int fsize;

static void create_filename(CuTest *tc)
{
    char *oldfileptr;

    apr_filepath_get(&file1, 0, p);
#ifndef NETWARE
#ifdef WIN32
    CuAssertTrue(tc, file1[1] == ':');
#else
    CuAssertTrue(tc, file1[0] == '/');
#endif
#endif
    CuAssertTrue(tc, file1[strlen(file1) - 1] != '/');

    oldfileptr = file1;
    file1 = apr_pstrcat(p, file1,"/data/mmap_datafile.txt" ,NULL);
    CuAssertTrue(tc, oldfileptr != file1);
}

static void test_file_close(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_file_close(thefile);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
}
   
static void test_file_open(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_file_open(&thefile, file1, APR_READ, APR_UREAD | APR_GREAD, p);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertPtrNotNull(tc, thefile);
}
   
static void test_get_filesize(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
    CuAssertIntEquals(tc, fsize, finfo.size);
}

static void test_mmap_create(CuTest *tc)
{
    apr_status_t rv;

    rv = apr_mmap_create(&themmap, thefile, 0, finfo.size, APR_MMAP_READ, p);
    CuAssertPtrNotNull(tc, themmap);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
}

static void test_mmap_contents(CuTest *tc)
{
    
    CuAssertPtrNotNull(tc, themmap);
    CuAssertPtrNotNull(tc, themmap->mm);
    CuAssertIntEquals(tc, fsize, themmap->size);

    /* Must use nEquals since the string is not guaranteed to be NULL terminated */
    CuAssertStrNEquals(tc, themmap->mm, TEST_STRING, fsize);
}

static void test_mmap_delete(CuTest *tc)
{
    apr_status_t rv;

    CuAssertPtrNotNull(tc, themmap);
    rv = apr_mmap_delete(themmap);
    CuAssertIntEquals(tc, rv, APR_SUCCESS);
}

static void test_mmap_offset(CuTest *tc)
{
    apr_status_t rv;
    void *addr;

    CuAssertPtrNotNull(tc, themmap);
    rv = apr_mmap_offset(&addr, themmap, 5);

    /* Must use nEquals since the string is not guaranteed to be NULL terminated */
    CuAssertStrNEquals(tc, addr, TEST_STRING + 5, fsize-5);
}
#endif

CuSuite *testmmap(void)
{
    CuSuite *suite = CuSuiteNew("MMAP");

#if APR_HAS_MMAP    
    fsize = strlen(TEST_STRING);

    SUITE_ADD_TEST(suite, create_filename);
    SUITE_ADD_TEST(suite, test_file_open);
    SUITE_ADD_TEST(suite, test_get_filesize);
    SUITE_ADD_TEST(suite, test_mmap_create);
    SUITE_ADD_TEST(suite, test_mmap_contents);
    SUITE_ADD_TEST(suite, test_mmap_offset);
    SUITE_ADD_TEST(suite, test_mmap_delete);
    SUITE_ADD_TEST(suite, test_file_close);
#else
    SUITE_ADD_TEST(suite, not_implemented);
#endif

    return suite;
}

