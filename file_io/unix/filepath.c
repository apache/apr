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
#include "apr_private.h"
#include "apr_arch_file_io.h"
#include "apr_file_io.h"
#include "apr_strings.h"
#define APR_WANT_STRFUNC
#include "apr_want.h"
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Win32 malpropism that can go away once everyone believes this
 * code is golden, and I'm not testing it anymore :-)
 */
#if APR_HAVE_DIRENT_H
#include <dirent.h>
#endif

/* Any OS that requires/refuses trailing slashes should be dealt with here.
 */
APR_DECLARE(apr_status_t) apr_filepath_get(char **defpath, apr_int32_t flags,
                                           apr_pool_t *p)
{
    char path[APR_PATH_MAX];

    if (!getcwd(path, sizeof(path))) {
        if (errno == ERANGE)
            return APR_ENAMETOOLONG;
        else
            return errno;
    }
    *defpath = apr_pstrdup(p, path);

    return APR_SUCCESS;
}


/* Any OS that requires/refuses trailing slashes should be dealt with here
 */
APR_DECLARE(apr_status_t) apr_filepath_set(const char *path, apr_pool_t *p)
{
    if (chdir(path) != 0)
        return errno;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_filepath_root(const char **rootpath,
                                            const char **inpath,
                                            apr_int32_t flags,
                                            apr_pool_t *p)
{
    if (**inpath == '/') {
        *rootpath = apr_pstrdup(p, "/");
        do {
            ++(*inpath);
        } while (**inpath == '/');

        return APR_SUCCESS;
    }

    return APR_ERELATIVE;
}

APR_DECLARE(apr_status_t) apr_filepath_merge(char **newpath,
                                             const char *rootpath,
                                             const char *addpath,
                                             apr_int32_t flags,
                                             apr_pool_t *p)
{
    char *path;
    apr_size_t rootlen; /* is the length of the src rootpath */
    apr_size_t maxlen;  /* maximum total path length */
    apr_size_t keptlen; /* is the length of the retained rootpath */
    apr_size_t pathlen; /* is the length of the result path */
    apr_size_t seglen;  /* is the end of the current segment */
    apr_status_t rv;

    /* Treat null as an empty path.
     */
    if (!addpath)
        addpath = "";

    if (addpath[0] == '/') {
        /* If addpath is rooted, then rootpath is unused.
         * Ths violates any APR_FILEPATH_SECUREROOTTEST and
         * APR_FILEPATH_NOTABSOLUTE flags specified.
         */
        if (flags & APR_FILEPATH_SECUREROOTTEST)
            return APR_EABOVEROOT;
        if (flags & APR_FILEPATH_NOTABSOLUTE)
            return APR_EABSOLUTE;

        /* If APR_FILEPATH_NOTABOVEROOT wasn't specified,
         * we won't test the root again, it's ignored.
         * Waste no CPU retrieving the working path.
         */
        if (!rootpath && !(flags & APR_FILEPATH_NOTABOVEROOT))
            rootpath = "";
    }
    else {
        /* If APR_FILEPATH_NOTABSOLUTE is specified, the caller
         * requires a relative result.  If the rootpath is
         * ommitted, we do not retrieve the working path,
         * if rootpath was supplied as absolute then fail.
         */
        if (flags & APR_FILEPATH_NOTABSOLUTE) {
            if (!rootpath)
                rootpath = "";
            else if (rootpath[0] == '/')
                return APR_EABSOLUTE;
        }
    }

    if (!rootpath) {
        /* Start with the current working path.  This is bass akwards,
         * but required since the compiler (at least vc) doesn't like
         * passing the address of a char const* for a char** arg.
         */
        char *getpath;
        rv = apr_filepath_get(&getpath, flags, p);
        rootpath = getpath;
        if (rv != APR_SUCCESS)
            return errno;

        /* XXX: Any kernel subject to goofy, uncanonical results
         * must run the rootpath against the user's given flags.
         * Simplest would be a recursive call to apr_filepath_merge
         * with an empty (not null) rootpath and addpath of the cwd.
         */
    }

    rootlen = strlen(rootpath);
    maxlen = rootlen + strlen(addpath) + 4; /* 4 for slashes at start, after
                                             * root, and at end, plus trailing
                                             * null */
    if (maxlen > APR_PATH_MAX) {
        return APR_ENAMETOOLONG;
    }
    path = (char *)apr_palloc(p, maxlen);

    if (addpath[0] == '/') {
        /* Ignore the given root path, strip off leading
         * '/'s to a single leading '/' from the addpath,
         * and leave addpath at the first non-'/' character.
         */
        keptlen = 0;
        while (addpath[0] == '/')
            ++addpath;
        path[0] = '/';
        pathlen = 1;
    }
    else {
        /* If both paths are relative, fail early
         */
        if (rootpath[0] != '/' && (flags & APR_FILEPATH_NOTRELATIVE))
            return APR_ERELATIVE;

        /* Base the result path on the rootpath
         */
        keptlen = rootlen;
        memcpy(path, rootpath, rootlen);

        /* Always '/' terminate the given root path
         */
        if (keptlen && path[keptlen - 1] != '/') {
            path[keptlen++] = '/';
        }
        pathlen = keptlen;
    }

    while (*addpath) {
        /* Parse each segment, find the closing '/'
         */
        const char *next = addpath;
        while (*next && (*next != '/')) {
            ++next;
        }
        seglen = next - addpath;

        if (seglen == 0 || (seglen == 1 && addpath[0] == '.')) {
            /* noop segment (/ or ./) so skip it
             */
        }
        else if (seglen == 2 && addpath[0] == '.' && addpath[1] == '.') {
            /* backpath (../) */
            if (pathlen == 1 && path[0] == '/') {
                /* Attempt to move above root.  Always die if the
                 * APR_FILEPATH_SECUREROOTTEST flag is specified.
                 */
                if (flags & APR_FILEPATH_SECUREROOTTEST) {
                    return APR_EABOVEROOT;
                }

                /* Otherwise this is simply a noop, above root is root.
                 * Flag that rootpath was entirely replaced.
                 */
                keptlen = 0;
            }
            else if (pathlen == 0
                     || (pathlen == 3
                         && !memcmp(path + pathlen - 3, "../", 3))
                     || (pathlen  > 3
                         && !memcmp(path + pathlen - 4, "/../", 4))) {
                /* Path is already backpathed or empty, if the
                 * APR_FILEPATH_SECUREROOTTEST.was given die now.
                 */
                if (flags & APR_FILEPATH_SECUREROOTTEST) {
                    return APR_EABOVEROOT;
                }

                /* Otherwise append another backpath.
                 */
                memcpy(path + pathlen, "../", 3);
                pathlen += 3;
            }
            else {
                /* otherwise crop the prior segment
                 */
                do {
                    --pathlen;
                } while (pathlen && path[pathlen - 1] != '/');
            }

            /* Now test if we are above where we started and back up
             * the keptlen offset to reflect the added/altered path.
             */
            if (pathlen < keptlen) {
                if (flags & APR_FILEPATH_SECUREROOTTEST) {
                    return APR_EABOVEROOT;
                }
                keptlen = pathlen;
            }
        }
        else {
            /* An actual segment, append it to the destination path
             */
            if (*next) {
                seglen++;
            }
            memcpy(path + pathlen, addpath, seglen);
            pathlen += seglen;
        }

        /* Skip over trailing slash to the next segment
         */
        if (*next) {
            ++next;
        }

        addpath = next;
    }
    path[pathlen] = '\0';

    /* keptlen will be the rootlen unless the addpath contained
     * backpath elements.  If so, and APR_FILEPATH_NOTABOVEROOT
     * is specified (APR_FILEPATH_SECUREROOTTEST was caught above),
     * compare the original root to assure the result path is
     * still within given root path.
     */
    if ((flags & APR_FILEPATH_NOTABOVEROOT) && keptlen < rootlen) {
        if (strncmp(rootpath, path, rootlen)) {
            return APR_EABOVEROOT;
        }
        if (rootpath[rootlen - 1] != '/'
            && path[rootlen] && path[rootlen] != '/') {
            return APR_EABOVEROOT;
        }
    }

    *newpath = path;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_filepath_list_split(apr_array_header_t **pathelts,
                                                  const char *liststr,
                                                  apr_pool_t *p)
{
    return apr_filepath_list_split_impl(pathelts, liststr, ':', p);
}

APR_DECLARE(apr_status_t) apr_filepath_list_merge(char **liststr,
                                                  apr_array_header_t *pathelts,
                                                  apr_pool_t *p)
{
    return apr_filepath_list_merge_impl(liststr, pathelts, ':', p);
}

APR_DECLARE(apr_status_t) apr_filepath_encoding(int *style, apr_pool_t *p)
{
    *style = APR_FILEPATH_ENCODING_LOCALE;
    return APR_SUCCESS;
}
