/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_strings.h"


/* Win32 Exceptions:
 *
 * Note that trailing spaces and trailing periods are never recorded
 * in the file system, except by a very obscure bug where any file
 * that is created with a trailing space or period, followed by the 
 * ':' stream designator on an NTFS volume can never be accessed again.
 * In other words, don't ever accept them if designating a stream!
 *
 * An interesting side effect is that two or three periods are both 
 * treated as the parent directory, although the fourth and on are
 * not [strongly suggest all trailing periods are trimmed off, or
 * down to two if there are no other characters.]
 *
 * Leading spaces and periods are accepted, however.
 */
static int is_fnchar(char ch) 
{
    /* No control code between 0 and 31 is allowed
     * The * ? < > codes all have wildcard effects
     * The " / \ : are exlusively separator tokens 
     * The system doesn't accept | for any purpose.
     * Oddly, \x7f _is_ acceptable.
     */
    if (ch >= 0 && ch < 32)
        return 0;

    if (ch == '\"' || ch ==  '*' || ch == '/' 
     || ch ==  ':' || ch ==  '<' || ch == '>' 
     || ch ==  '?' || ch == '\\' || ch == '|')
        return 0;

    return 1;
}


APR_DECLARE(apr_status_t) apr_filepath_get(char **rootpath,
                                           apr_pool_t *p)
{
    char path[APR_PATH_MAX];
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(p, &os_level) && os_level >= APR_WIN_NT)
    {
        apr_wchar_t wpath[APR_PATH_MAX];
        apr_status_t rv;
        if (!GetCurrentDirectoryW(sizeof(wpath) / sizeof(apr_wchar_t), wpath))
            return apr_get_os_error();
        if ((rv = unicode_to_utf8_path(path, sizeof(path), wpath)))
            return rv;
    }
    else
#endif
    {
        if (!GetCurrentDirectory(sizeof(path), path))
            return apr_get_os_error();
    }
    *rootpath = apr_pstrdup(p, path);
    return APR_SUCCESS;
}


APR_DECLARE(apr_status_t) apr_filepath_set(const char *rootpath,
                                           apr_pool_t *p)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(p, &os_level) && os_level >= APR_WIN_NT)
    {
        apr_wchar_t wpath[APR_PATH_MAX];
        apr_status_t rv;
        if (rv = utf8_to_unicode_path(wpath, sizeof(wpath) 
                                           / sizeof(apr_wchar_t), rootpath))
            return rv;
        if (!SetCurrentDirectoryW(wpath))
            return apr_get_os_error();
    }
    else
#endif
    {
        if (!SetCurrentDirectory(rootpath))
            return apr_get_os_error();
    }
    return APR_SUCCESS;
}


/* WinNT accepts several odd forms of a 'root' path.  Under Unicode
 * calls (ApiFunctionW) the //?/C:/foo or //?/UNC/mach/share/foo forms
 * are accepted.  Ansi and Unicode functions both accept the //./C:/foo 
 * form.  Since these forms are handled in the utf-8 to unicode 
 * translation phase, we don't want the user confused by them, so we 
 * will accept them but always return the canonical C:/ or //mach/share/
 */

APR_DECLARE(apr_status_t) apr_filepath_root(const char **rootpath, 
                                            const char **inpath, apr_pool_t *p)
{
    const char *testpath = *inpath;
    const char *delim1;
    const char *delim2;
    char *newpath;

    if (testpath[0] == '/' || testpath[0] == '\\') {
        if (testpath[1] == '/' || testpath[1] == '\\') {
            /* //server/share isn't the only // delimited syntax */
            if ((testpath[2] == '?' || testpath[2] == '.')
                    && (testpath[3] == '/' || testpath[3] == '\\')) {
                if (is_fnchar(testpath[4]) && testpath[5] == ':') 
                {
                    apr_status_t rv;
                    testpath += 4;
                    /* given  '//?/C: or //./C: let us try this
                     * all over again from the drive designator
                     */
                    rv = apr_filepath_root(rootpath, &testpath, p);
                    if (!rv || rv == APR_EINCOMPLETE)
                        *inpath = testpath;
                    return rv;
                }
                else if (strncasecmp(testpath + 4, "UNC", 3) == 0
                      && (testpath[7] == '/' || testpath[7] == '\\') 
                      && (testpath[2] == '?')) {
                    /* given  '//?/UNC/machine/share, a little magic 
                     * at the end makes this all work out by using
                     * 'C/machine' as the starting point and replacing
                     * the UNC delimiters with \'s, including the 'C'
                     */
                    testpath += 6;
                }
                else
                    /* This must not be a path to a file, but rather
                     * a volume or device.  Die for now.
                     */
                    return APR_EBADPATH;
            }

            /* Evaluate path of '//[machine/[share[/]]]' */
            delim1 = testpath + 2;
            do {
                /* Protect against //X/ where X is illegal */
                if (*delim1 && !is_fnchar(*(delim1++)))
                    return APR_EBADPATH;
            } while (*delim1 && *delim1 != '/' && *delim1 != '\\');

            if (*delim1) {
                delim2 = delim1 + 1;
                do {
                    /* Protect against //machine/X/ where X is illegal */
                    if (*delim2 && !is_fnchar(*(delim2++)))
                        return APR_EBADPATH;
                } while (*delim2 && *delim2 != '/' && *delim2 != '\\');

                if (delim2) {
                    /* Have path of '//machine/share/[whatnot]' */
                    newpath = apr_pstrndup(p, testpath, delim2 - testpath);
                    newpath[0] = '\\';
                    newpath[1] = '\\';
                    newpath[delim1 - testpath] = '\\';
                    newpath[delim2 - testpath] = '\\';
                    *rootpath = newpath;
                    *inpath = delim2 + 1;
                    while (**inpath == '/' || **inpath == '\\')
                        ++*inpath;
                    return APR_SUCCESS;
                }

                /* Have path of '//machine/[share]' */
                delim2 = strchr(delim1, '\0');
                newpath = apr_pstrndup(p, testpath, delim2 - testpath);
                newpath[0] = '\\';
                newpath[1] = '\\';
                newpath[delim1 - testpath] = '\\';
                *rootpath = newpath;
                *inpath = delim2;
                return APR_EINCOMPLETE;
            }
            
            /* Have path of '//[machine]' */
            delim1 = strchr(testpath, '\0');
            newpath = apr_pstrndup(p, testpath, delim1 - testpath);
            newpath[0] = '\\';
            newpath[1] = '\\';
            *rootpath = newpath;
            *inpath = delim1;
            return APR_EINCOMPLETE;
        }

        /* Left with a path of '/', what drive are we asking about? */
        *rootpath = apr_pstrdup(p, "/");
        *inpath = ++testpath;
        return APR_EINCOMPLETE;
    }

    /* Evaluate path of 'd:[/]' */
    if (is_fnchar(*testpath) && testpath[1] == ':') {
        if (testpath[2] == '/' || testpath[2] == '\\') {
            *rootpath = apr_pstrndup(p, testpath, 3);
            *inpath = testpath + 3;
            while (**inpath == '/' || **inpath == '\\')
                ++*inpath;
            return APR_SUCCESS;
        }

        /* Left with path of 'd:' from the cwd of this drive letter */
        *rootpath = apr_pstrndup(p, testpath, 2);
        *inpath = testpath + 2;
        return APR_EINCOMPLETE;        
    }

    /* Nothing interesting */
    return APR_ERELATIVE;
}


APR_DECLARE(apr_status_t) apr_filepath_merge(char **newpath, 
                                             const char *rootpath, 
                                             const char *addpath, 
                                             apr_int32_t flags,
                                             apr_pool_t *p)
{
    char path[APR_PATH_MAX]; /* isn't null term */
    apr_size_t rootlen; /* is the length of the src rootpath */
    apr_size_t keptlen; /* is the length of the retained rootpath */
    apr_size_t pathlen; /* is the length of the result path */
    apr_size_t seglen;  /* is the end of the current segment */
    apr_status_t rv;

    /* Treat null as an empty path.
     */
    if (!addpath)
        addpath = "";

    if (addpath[0] == '/') 
    {
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
    else 
    {
        /* If APR_FILEPATH_NOTABSOLUTE is specified, the caller 
         * requires a relative result.  If the rootpath is
         * ommitted, we do not retrieve the working path,
         * if rootpath was supplied as absolute then fail.
         */
        if (flags & APR_FILEPATH_NOTABSOLUTE) 
        {
            if (!rootpath)
                rootpath = "";
            else if (rootpath[0] == '/')
                return APR_EABSOLUTE;
        }
    }

    if (!rootpath) 
    {
        /* Start with the current working path.  This is bass akwards,
         * but required since the compiler (at least vc) doesn't like
         * passing the address of a char const* for a char** arg.
         */
        char *getpath;
        rv = apr_filepath_get(&getpath, p);
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

    if (addpath[0] == '/') 
    {
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
    else
    {
        /* If both paths are relative, fail early
         */
        if (rootpath[0] != '/' && (flags & APR_FILEPATH_NOTRELATIVE))
                return APR_ERELATIVE;

        /* Base the result path on the rootpath
         */
        keptlen = rootlen;
        if (rootlen >= sizeof(path))
            return APR_ENAMETOOLONG;
        memcpy(path, rootpath, rootlen);
        
        /* Always '/' terminate the given root path
         */
        if (keptlen && path[keptlen - 1] != '/') {
            if (keptlen + 1 >= sizeof(path))
                return APR_ENAMETOOLONG;
            path[keptlen++] = '/';
        }
        pathlen = keptlen;
    }

    while (*addpath) 
    {
        /* Parse each segment, find the closing '/' 
         */
        seglen = 0;
        while (addpath[seglen] && addpath[seglen] != '/')
            ++seglen;

        if (seglen == 0 || (seglen == 1 && addpath[0] == '.')) 
        {
            /* noop segment (/ or ./) so skip it 
             */
        }
        else if (seglen == 2 && addpath[0] == '.' && addpath[1] == '.') 
        {
            /* backpath (../) */
            if (pathlen == 1 && path[0] == '/') 
            {
                /* Attempt to move above root.  Always die if the 
                 * APR_FILEPATH_SECUREROOTTEST flag is specified.
                 */
                if (flags & APR_FILEPATH_SECUREROOTTEST)
                    return APR_EABOVEROOT;
                
                /* Otherwise this is simply a noop, above root is root.
                 * Flag that rootpath was entirely replaced.
                 */
                keptlen = 0;
            }
            else if (pathlen == 0 
                  || (pathlen == 3 && !memcmp(path + pathlen - 3, "../", 3))
                  || (pathlen  > 3 && !memcmp(path + pathlen - 4, "/../", 4)))
            {
                /* Path is already backpathed or empty, if the
                 * APR_FILEPATH_SECUREROOTTEST.was given die now.
                 */
                if (flags & APR_FILEPATH_SECUREROOTTEST)
                    return APR_EABOVEROOT;

                /* Otherwise append another backpath.
                 */
                if (pathlen + 3 >= sizeof(path))
                    return APR_ENAMETOOLONG;
                memcpy(path + pathlen, "../", 3);
                pathlen += 3;
            }
            else 
            {
                /* otherwise crop the prior segment 
                 */
                do {
                    --pathlen;
                } while (pathlen && path[pathlen - 1] != '/');
            }

            /* Now test if we are above where we started and back up
             * the keptlen offset to reflect the added/altered path.
             */
            if (pathlen < keptlen) 
            {
                if (flags & APR_FILEPATH_SECUREROOTTEST)
                    return APR_EABOVEROOT;
                keptlen = pathlen;
            }
        }
        else 
        {
            /* An actual segment, append it to the destination path
             */
            apr_size_t i = (addpath[seglen] != '\0');
            if (pathlen + seglen + i >= sizeof(path))
                return APR_ENAMETOOLONG;
            memcpy(path + pathlen, addpath, seglen + i);
            pathlen += seglen + i;
        }

        /* Skip over trailing slash to the next segment
         */
        if (addpath[seglen])
            ++seglen;

        addpath += seglen;
    }
    path[pathlen] = '\0';
    
    /* keptlen will be the rootlen unless the addpath contained
     * backpath elements.  If so, and APR_FILEPATH_NOTABOVEROOT
     * is specified (APR_FILEPATH_SECUREROOTTEST was caught above),
     * compare the original root to assure the result path is
     * still within given root path.
     */
    if ((flags & APR_FILEPATH_NOTABOVEROOT) && keptlen < rootlen) {
        if (strncmp(rootpath, path, rootlen))
            return APR_EABOVEROOT;
        if (rootpath[rootlen - 1] != '/'
                && path[rootlen] && path[rootlen] != '/')
            return APR_EABOVEROOT;
    }

#if 0
    /* Just an idea - don't know where it's headed yet */
    if (addpath && addpath[endseg - 1] != '/' 
                && (flags & APR_FILEPATH_TRUECASE)) {
        apr_finfo_t finfo;
        if (apr_stat(&finfo, path, APR_FINFO_TYPE, p) == APR_SUCCESS) {
            if (addpath[endseg - 1] != finfo.filetype == APR_DIR) {
                if (endseg + 1 >= sizeof(path))
                    return APR_ENAMETOOLONG;
                path[endseg++] = '/';
                path[endseg] = '\0';
            }
        }
    }
#endif

    *newpath = apr_pstrdup(p, path);
    return APR_SUCCESS;
}
