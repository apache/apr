/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
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

#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_strings.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_poll.h"
#include "apr_lib.h"
#include "test_apr.h"

#define FILENAME "data/file_datafile.txt"

static const struct view_fileinfo
{
    apr_int32_t bits;
    char *description;
} vfi[] = {
    {APR_FINFO_MTIME,  "MTIME"},
    {APR_FINFO_CTIME,  "CTIME"},
    {APR_FINFO_ATIME,  "ATIME"},
    {APR_FINFO_SIZE,   "SIZE"},
    {APR_FINFO_DEV,    "DEV"},
    {APR_FINFO_INODE,  "INODE"},
    {APR_FINFO_NLINK,  "NLINK"},
    {APR_FINFO_TYPE,   "TYPE"},
    {APR_FINFO_USER,   "USER"}, 
    {APR_FINFO_GROUP,  "GROUP"}, 
    {APR_FINFO_UPROT,  "UPROT"}, 
    {APR_FINFO_GPROT,  "GPROT"},
    {APR_FINFO_WPROT,  "WPROT"},
    {0,                NULL}
}; 

static int finfo_equal(apr_finfo_t f1, apr_finfo_t f2)
{
    return (f1.valid == f2.valid &&
            f1.protection == f2.protection &&
            f1.filetype == f2.filetype &&
            f1.user == f2.user &&
            f1.group == f2.group &&
            f1.inode == f2.inode &&
            f1.device == f2.device &&
            f1.nlink == f2.nlink &&
            f1.size == f2.size &&
/*  Can't check csize, we don't fill it out, which makes me wonder why it
 *  is still there.
 *          f1.csize == f2.csize &&
 */
            f1.atime == f2.atime &&
            f1.mtime == f2.mtime &&
            f1.ctime == f2.ctime &&
            !strcmp(f1.fname, f2.fname));
/*  We also can't check name, because it is only ever set on Unix.  This 
 *  means that we have non-portable fields in a transparant structure.
            !strcmp(f1.name, f2.name));
 */
}

static void test_info_get(CuTest *tc)
{
    apr_file_t *thefile;
    apr_finfo_t finfo;
    apr_status_t rv;

    rv = apr_file_open(&thefile, FILENAME, APR_READ, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);
    if (rv  == APR_INCOMPLETE) {
        char *str;
	int i;
        str = apr_pstrdup(p, "APR_INCOMPLETE:  Missing ");
        for (i = 0; vfi[i].bits; ++i) {
            if (vfi[i].bits & ~finfo.valid) {
                str = apr_pstrcat(p, str, vfi[i].description);
            }
        }
        CuFail(tc, str);
    }
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    apr_file_close(thefile);
}

static void test_stat(CuTest *tc)
{
    apr_finfo_t finfo;
    apr_status_t rv;

    rv = apr_stat(&finfo, FILENAME, APR_FINFO_NORM, p);
    if (rv  == APR_INCOMPLETE) {
        char *str;
	int i;
        str = apr_pstrdup(p, "APR_INCOMPLETE:  Missing ");
        for (i = 0; vfi[i].bits; ++i) {
            if (vfi[i].bits & ~finfo.valid) {
                str = apr_pstrcat(p, str, vfi[i].description);
            }
        }
        CuFail(tc, str);
    }
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
}

static void test_stat_eq_finfo(CuTest *tc)
{
    apr_file_t *thefile;
    apr_finfo_t finfo;
    apr_finfo_t stat_finfo;
    apr_status_t rv;

    rv = apr_stat(&stat_finfo, FILENAME, APR_FINFO_NORM, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);

    rv = apr_file_open(&thefile, FILENAME, APR_READ, APR_OS_DEFAULT, p);
    CuAssertIntEquals(tc, APR_SUCCESS, rv);
    rv = apr_file_info_get(&finfo, APR_FINFO_NORM, thefile);
    apr_file_close(thefile);

    CuAssert(tc, "results from apr_stat are not identical to results "
                 "from apr_finfo", finfo_equal(stat_finfo, finfo));
}

CuSuite *testfileinfo(void)
{
    CuSuite *suite = CuSuiteNew("File Info");

    SUITE_ADD_TEST(suite, test_info_get);
    SUITE_ADD_TEST(suite, test_stat);
    SUITE_ADD_TEST(suite, test_stat_eq_finfo);

    return suite;
}

