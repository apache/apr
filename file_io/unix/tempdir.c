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
#include "apr_private.h"
#include "apr_file_io.h"
#include "apr_strings.h"
#include "apr_env.h"

/*
 * FIXME
 * Currently, this variable is a bit of misnomer.
 * The intention is to have this in APR's global pool so that we don't have 
 * to go through this every time.
 */
static char global_temp_dir[APR_PATH_MAX+1] = { 0 };

/* Try to open a temporary file in the temporary dir, write to it,
   and then close it. */
static int test_tempdir(const char *temp_dir, apr_pool_t *p)
{
    apr_file_t *dummy_file;
    const char *path = apr_pstrcat(p, temp_dir, "/apr-tmp", NULL);

    if (apr_file_mktemp(&dummy_file, (char *)path, 0, p) == APR_SUCCESS) {
        if (apr_file_putc('!', dummy_file) == APR_SUCCESS) {
            if (apr_file_close(dummy_file) == APR_SUCCESS) {
                return 1;
            }
        }
    }
    return 0;
}


APR_DECLARE(apr_status_t) apr_temp_dir_get(const char **temp_dir, 
                                           apr_pool_t *p)
{
    apr_status_t apr_err;
    const char *try_dirs[] = { "/tmp", "/usr/tmp", "/var/tmp" };
    const char *try_envs[] = { "TMP", "TEMP", "TMPDIR" };
    char *cwd;
    int i;

    /* Our goal is to find a temporary directory suitable for writing
       into.  We'll only pay the price once if we're successful -- we
       cache our successful find.  Here's the order in which we'll try
       various paths:

          $TMP
          $TEMP
          $TMPDIR
          "C:\TEMP"     (windows only)
          "SYS:\TMP"    (netware only)
          "/tmp"
          "/var/tmp"
          "/usr/tmp"
          P_tmpdir      (POSIX define)
          `pwd` 

       NOTE: This algorithm is basically the same one used by Python
       2.2's tempfile.py module.  */

    /* Try the environment first. */
    for (i = 0; i < (sizeof(try_envs) / sizeof(const char *)); i++) {
        char *value;
        apr_err = apr_env_get(&value, try_envs[i], p);
        if ((apr_err == APR_SUCCESS) && value) {
            apr_size_t len = strlen(value);
            if (len && (len < APR_PATH_MAX) && test_tempdir(value, p)) {
                memcpy(global_temp_dir, value, len + 1);
                goto end;
            }
        }
    }

#ifdef WIN32
    /* Next, on Win32, try the C:\TEMP directory. */
    if (test_tempdir("C:\\TEMP", p)) {
        memcpy(global_temp_dir, "C:\\TEMP", 7 + 1);
        goto end;
    }
#endif
#ifdef NETWARE
    /* Next, on NetWare, try the SYS:/TMP directory. */
    if (test_tempdir("SYS:/TMP", p)) {
        memcpy(global_temp_dir, "SYS:/TMP", 8 + 1);
        goto end;
    }
#endif

    /* Next, try a set of hard-coded paths. */
    for (i = 0; i < (sizeof(try_dirs) / sizeof(const char *)); i++) {
        if (test_tempdir(try_dirs[i], p)) {
            memcpy(global_temp_dir, try_dirs[i], strlen(try_dirs[i]) + 1);
            goto end;
        }
    }

#ifdef P_tmpdir
    /* 
     * If we have it, use the POSIX definition of where 
     * the tmpdir should be 
     */
    if (test_tempdir(P_tmpdir, p)) {
        memcpy(global_temp_dir, P_tmpdir, strlen(P_tmpdir) +1);
        goto end;
    }
#endif
    
    /* Finally, try the current working directory. */
    if (APR_SUCCESS == apr_filepath_get(&cwd, APR_FILEPATH_NATIVE, p)) {
        if (test_tempdir(cwd, p)) {
            memcpy(global_temp_dir, cwd, strlen(cwd) + 1);
            goto end;
        }
    }

end:
    if (global_temp_dir[0]) {
        *temp_dir = apr_pstrdup(p, global_temp_dir);
        return APR_SUCCESS;
    }
    return APR_EGENERAL;
}
