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

#include "apr.h"
#include "apr_arch_file_io.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include <ctype.h>

/* OS/2 Exceptions:
 *
 * Note that trailing spaces and trailing periods are never recorded
 * in the file system.
 *
 * Leading spaces and periods are accepted, however.
 * The * ? < > codes all have wildcard side effects
 * The " / \ : are exclusively component separator tokens 
 * The system doesn't accept | for any (known) purpose 
 * Oddly, \x7f _is_ acceptable ;)
 */

const char c_is_fnchar[256] =
{/* Reject all ctrl codes...                                         */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 /*     "               *         /                      :   <   > ? */
    1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,0, 1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,0,
 /*                                                          \       */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,
 /*                                                          |       */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,
 /* High bit codes are accepted                                      */
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};


#define IS_SLASH(c) (c == '/' || c == '\\')


apr_status_t filepath_root_test(char *path, apr_pool_t *p)
{
    char drive = apr_toupper(path[0]);

    if (drive >= 'A' && drive <= 'Z' && path[1] == ':' && IS_SLASH(path[2]))
        return APR_SUCCESS;

    return APR_EBADPATH;
}


apr_status_t filepath_drive_get(char **rootpath, char drive, 
                                apr_int32_t flags, apr_pool_t *p)
{
    char path[APR_PATH_MAX];
    char *pos;
    ULONG rc;
    ULONG bufsize = sizeof(path) - 3;

    path[0] = drive;
    path[1] = ':';
    path[2] = '/';

    rc = DosQueryCurrentDir(apr_toupper(drive) - 'A', path+3, &bufsize);

    if (rc) {
        return APR_FROM_OS_ERROR(rc);
    }

    if (!(flags & APR_FILEPATH_NATIVE)) {
        for (pos=path; *pos; pos++) {
            if (*pos == '\\')
                *pos = '/';
        }
    }

    *rootpath = apr_pstrdup(p, path);
    return APR_SUCCESS;
}


apr_status_t filepath_root_case(char **rootpath, char *root, apr_pool_t *p)
{
    char path[APR_PATH_MAX];

    strcpy(path, root);
    if (path[1] == ':')
        path[0] = apr_toupper(path[0]);
    *rootpath = apr_pstrdup(p, path);
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_filepath_get(char **defpath, apr_int32_t flags,
                                           apr_pool_t *p)
{
    char path[APR_PATH_MAX];
    ULONG drive;
    ULONG drivemap;
    ULONG rv, pathlen = sizeof(path) - 3;
    char *pos;

    DosQueryCurrentDisk(&drive, &drivemap);
    path[0] = '@' + drive;
    strcpy(path+1, ":\\");
    rv = DosQueryCurrentDir(drive, path+3, &pathlen);

    *defpath = apr_pstrdup(p, path);

    if (!(flags & APR_FILEPATH_NATIVE)) {
        for (pos=*defpath; *pos; pos++) {
            if (*pos == '\\')
                *pos = '/';
        }
    }

    return APR_SUCCESS;
}    



APR_DECLARE(apr_status_t) apr_filepath_set(const char *path, apr_pool_t *p)
{
    ULONG rv = 0;

    if (path[1] == ':')
        rv = DosSetDefaultDisk(apr_toupper(path[0]) - '@');

    if (rv == 0)
        rv = DosSetCurrentDir(path);

    return APR_FROM_OS_ERROR(rv);
}
