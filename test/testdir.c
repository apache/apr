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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "test_apr.h"


/* Test for a (fixed) bug in apr_dir_read().  This bug only happened
   in threadless cases. */

int main(void)
{
    apr_pool_t *pool;
    apr_file_t *thefile = NULL;
    apr_finfo_t finfo;
    apr_int32_t finfo_flags = APR_FINFO_TYPE | APR_FINFO_NAME;
    apr_dir_t *this_dir;
 
    printf("APR Directory Read Test\n===========================\n\n");
    
    STD_TEST_NEQ("Initializing APR", apr_initialize())
    atexit(apr_terminate);
    STD_TEST_NEQ("Creating the main pool we'll use", 
                 apr_pool_create(&pool, NULL))    

    fprintf(stdout, "Testing for readdir() bug.\n");

    /* Make two empty directories, and put a file in one of them. */
    STD_TEST_NEQ("    Creating empty dir1",
                 apr_dir_make("dir1", APR_OS_DEFAULT, pool))
    STD_TEST_NEQ("    Creating empty dir2",
                 apr_dir_make("dir2", APR_OS_DEFAULT, pool))
    STD_TEST_NEQ("    Creating dir1/file1",
                 apr_file_open(&thefile, "dir1/file1",
                               APR_READ | APR_WRITE | APR_CREATE,
                               APR_OS_DEFAULT, pool))
    STD_TEST_NEQ("    Closing dir1/file1",
                 apr_file_close(thefile))

    /* Try to remove dir1.  This should fail because it's not empty.
       However, on a platform with threads disabled (such as FreeBSD),
       `errno' will be set as a result. */
    TEST_EQ("    Failing to remove dir1", apr_dir_remove("dir1", pool), 
            APR_SUCCESS, "OK", "Failed") 
    
    /* Read `.' and `..' out of dir2. */
    STD_TEST_NEQ("    Opening dir2",
                 apr_dir_open(&this_dir, "dir2", pool))
    STD_TEST_NEQ("       reading `.' entry",
                 apr_dir_read(&finfo, finfo_flags, this_dir))
    STD_TEST_NEQ("       reading `..' entry",
                 apr_dir_read(&finfo, finfo_flags, this_dir))

    /* Now, when we attempt to do a third read of empty dir2, and the
       underlying system readdir() returns NULL, the old value of
       errno shouldn't cause a false alarm.  We should get an ENOENT
       back from apr_dir_read, and *not* the old errno. */
    TEST_STATUS("       get ENOENT on 3rd read", 
                apr_dir_read(&finfo, finfo_flags, this_dir),
                APR_STATUS_IS_ENOENT, "OK", "Failed")

    STD_TEST_NEQ("       closing dir",
		 apr_dir_close(this_dir));
		 
    /* Cleanup */
    STD_TEST_NEQ("    Cleanup file1",
                 apr_file_remove("dir1/file1", pool))
    STD_TEST_NEQ("    Cleanup dir1",
                 apr_dir_remove("dir1", pool))
    STD_TEST_NEQ("    Cleanup dir2",
                 apr_dir_remove("dir2", pool))

    apr_pool_destroy(pool);

    printf("\nAll tests passed OK\n");
    return 0;
}
