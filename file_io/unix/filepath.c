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
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

/* Any OS that requires/refuses trailing slashes should be dealt with here.
 */
APR_DECLARE(apr_status_t) apr_filepath_get(char **defpath, apr_pool_t *p)
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
                                            apr_pool_t *p)
{
    if (**inpath == '/') 
    {
        *rootpath = apr_pstrdup(p, "/");
        ++*inpath;
        return APR_EABSOLUTE;
    }

    return APR_ERELATIVE;
}

APR_DECLARE(apr_status_t) apr_filepath_merge(char **newpath, 
                                             const char *rootpath, 
                                             const char *addpath, 
                                             apr_int32_t flags,
                                             apr_pool_t *p)
{
    char path[APR_PATH_MAX];
    apr_size_t rootlen; /* is the original rootpath len */
    apr_size_t newseg;  /* is the path offset to the added path */
    apr_size_t addseg;  /* is the path offset we are appending at */
    apr_size_t endseg;  /* is the end of the current segment */
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
        newseg = 0;
        while (addpath[0] == '/')
            ++addpath;
        strcpy (path, "/");
        addseg = 1;
    }
    else
    {
        /* If both paths are relative, fail early
         */
        if (rootpath[0] != '/' && (flags & APR_FILEPATH_NOTRELATIVE))
                return APR_ERELATIVE;

        /* Base the result path on the rootpath
         */
        newseg = rootlen;
        if (rootlen >= sizeof(path))
            return APR_ENAMETOOLONG;
        strcpy(path, rootpath);
        
        /* Always '/' terminate the given root path
         */
        if (newseg && path[newseg - 1] != '/') {
            if (newseg + 1 >= sizeof(path))
                return APR_ENAMETOOLONG;
            path[newseg++] = '/';
            path[newseg] = '\0';
        }
        addseg = newseg;
    }

    while (*addpath) 
    {
        /* Parse each segment, find the closing '/' 
         */
        endseg = 0;
        while (addpath[endseg] && addpath[endseg] != '/')
            ++endseg;

        if (endseg == 0 || (endseg == 1 && addpath[0] == '.')) 
        {
            /* noop segment (/ or ./) so skip it 
             */
        }
        else if (endseg == 2 && addpath[0] == '.' && addpath[1] == '.') 
        {
            /* backpath (../) */
            if (addseg == 1 && path[0] == '/') 
            {
                /* Attempt to move above root.  Always die if the 
                 * APR_FILEPATH_SECUREROOTTEST flag is specified.
                 */
                if (flags & APR_FILEPATH_SECUREROOTTEST)
                    return APR_EABOVEROOT;
                
                /* Otherwise this is simply a noop, above root is root.
                 * Flag that rootpath was entirely replaced.
                 */
                newseg = 0;
            }
            else if (addseg == 0 || (addseg >= 3 
                                  && strcmp(path + addseg - 3, "../") == 0)) 
            {
                /* Path is already backpathed or empty, if the
                 * APR_FILEPATH_SECUREROOTTEST.was given die now.
                 */
                if (flags & APR_FILEPATH_SECUREROOTTEST)
                    return APR_EABOVEROOT;

                /* Otherwise append another backpath.
                 */
                if (addseg + 3 >= sizeof(path))
                    return APR_ENAMETOOLONG;
                strcpy(path + addseg, "../");
                addseg += 3;
            }
            else 
            {
                /* otherwise crop the prior segment 
                 */
                do {
                    --addseg;
                } while (addseg && path[addseg - 1] != '/');
                path[addseg] = '\0';
            }

            /* Now test if we are above where we started and back up
             * the newseg offset to reflect the added/altered path.
             */
            if (addseg < newseg) 
            {
                if (flags & APR_FILEPATH_SECUREROOTTEST)
                    return APR_EABOVEROOT;
                newseg = addseg;
            }
        }
        else 
        {
            /* An actual segment, append it to the destination path
             */
            apr_size_t i = (addpath[endseg] != '\0');
            if (addseg + endseg + i >= sizeof(path))
                return APR_ENAMETOOLONG;
            strncpy(path + addseg, addpath, endseg + i);
            addseg += endseg + i;
        }

        /* Skip over trailing slash to the next segment
         */
        if (addpath[endseg])
            ++endseg;

        addpath += endseg;
    }

    /* newseg will be the rootlen unless the addpath contained
     * backpath elements.  If so, and APR_FILEPATH_NOTABOVEROOT
     * is specified (APR_FILEPATH_SECUREROOTTEST was caught above),
     * compare the original root to assure the result path is
     * still within given root path.
     */
    if ((flags & APR_FILEPATH_NOTABOVEROOT) && newseg < rootlen) {
        if (strncmp(rootpath, path, rootlen))
            return APR_EABOVEROOT;
        if (rootpath[rootlen - 1] != '/'
                && path[rootlen] && path[rootlen] != '/')
            return APR_EABOVEROOT;
    }

    *newpath = apr_pstrdup(p, path);
    return (newpath ? APR_SUCCESS : APR_ENOMEM);
}
