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

apr_status_t filepath_root_case(char **rootpath, char *root, apr_pool_t *p)
{
/* See the Windows code to figure out what to do here.
    It probably checks to make sure that the root exists 
    and case it correctly according to the file system.
*/
    *rootpath = apr_pstrdup(p, root);
    return APR_SUCCESS;
}

apr_status_t filepath_has_drive(const char *rootpath, int only, apr_pool_t *p)
{
    char *s;

    if (rootpath) {
        s = strchr (rootpath, ':');
        if (only)
            /* Test if the path only has a drive/volume and nothing else
            */
            return (s && (s != rootpath) && !s[1]);
        else
            /* Test if the path includes a drive/volume
            */
            return (s && (s != rootpath));
    }
    return 0;
}

apr_status_t filepath_compare_drive(const char *path1, const char *path2, apr_pool_t *p)
{
    char *s1, *s2;

    if (path1 && path2) {
        s1 = strchr (path1, ':');
        s2 = strchr (path2, ':');

        /* Make sure that they both have a drive/volume delimiter
            and are the same size.  Then see if they match.
        */
        if (s1 && s2 && ((s1-path1) == (s2-path2))) {
            return strnicmp (s1, s2, s1-path1);
        }
    }
    return -1;
}

APR_DECLARE(apr_status_t) apr_filepath_get(char **rootpath, apr_int32_t flags,
                                           apr_pool_t *p)
{
    char path[APR_PATH_MAX];
    char *ptr;

    /* use getcwdpath to make sure that we get the volume name*/
    if (!getcwdpath(path, NULL, 0)) {
        if (errno == ERANGE)
            return APR_ENAMETOOLONG;
        else
            return errno;
    }
    /* Strip off the server name if there is one*/
    ptr = strpbrk(path, "\\/:");
    if (!ptr) {
        return APR_ENOENT;
    }
    if (*ptr == ':') {
        ptr = path;
    }
    *rootpath = apr_pstrdup(p, ptr);
    if (!(flags & APR_FILEPATH_NATIVE)) {
        for (ptr = *rootpath; *ptr; ++ptr) {
            if (*ptr == '\\')
                *ptr = '/';
        }
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_filepath_set(const char *rootpath,
                                           apr_pool_t *p)
{
    if (chdir2(rootpath) != 0)
        return errno;
    return APR_SUCCESS;
}


