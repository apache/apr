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
 * In other words, don't ever accept them when designating a stream!
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


static apr_status_t filepath_root_test(char *path,
                                       apr_pool_t *p)
{
    apr_status_t rv;
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(p, &os_level) && os_level >= APR_WIN_NT)
    {
        apr_wchar_t wpath[APR_PATH_MAX];
        if (rv = utf8_to_unicode_path(wpath, sizeof(wpath) 
                                           / sizeof(apr_wchar_t), path))
            return rv;
        rv = GetDriveTypeW(wpath);
    }
    else
#endif
        rv = GetDriveType(path);

    if (rv == DRIVE_UNKNOWN || rv == DRIVE_NO_ROOT_DIR)
        return APR_EBADPATH;
    return APR_SUCCESS;
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
    /* ###: We really should consider adding a flag to allow the user
     * to have the APR_FILEPATH_NATIVE result
     */
    for (*rootpath = path; **rootpath; ++*rootpath) {
        if (**rootpath == '\\')
            **rootpath = '/';
    }
    *rootpath = apr_pstrdup(p, path);
    return APR_SUCCESS;
}


static apr_status_t filepath_drive_get(char **rootpath,
                                       char drive,
                                       apr_pool_t *p)
{
    char path[APR_PATH_MAX];
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(p, &os_level) && os_level >= APR_WIN_NT)
    {
        apr_wchar_t *ignored;
        apr_wchar_t wdrive[8];
        apr_wchar_t wpath[APR_PATH_MAX];
        apr_status_t rv;
        /* ???: This needs review, apparently "\\?\d:." returns "\\?\d:" 
         * as if that is useful for anything.
         */
        wcscpy(wdrive, L"D:.");
        wdrive[0] = (apr_wchar_t)(unsigned char)drive;
        if (!GetFullPathNameW(wdrive, sizeof(wpath) / sizeof(apr_wchar_t), wpath, &ignored))
            return apr_get_os_error();
        if ((rv = unicode_to_utf8_path(path, sizeof(path), wpath)))
            return rv;
    }
    else
#endif
    {
        char *ignored;
        char drivestr[4];
        drivestr[0] = drive;
        drivestr[1] = ':';
        drivestr[2] = '.';;
        drivestr[3] = '\0';
        if (!GetFullPathName(drivestr, sizeof(path), path, &ignored))
            return apr_get_os_error();
    }
    /* ###: We really should consider adding a flag to allow the user
     * to have the APR_FILEPATH_NATIVE result
     */
    for (*rootpath = path; **rootpath; ++*rootpath) {
        if (**rootpath == '\\')
            **rootpath = '/';
    }
    *rootpath = apr_pstrdup(p, path);
    return APR_SUCCESS;
}


static apr_status_t filepath_root_case(char **rootpath,
                                           char *root,
                                           apr_pool_t *p)
{
#if APR_HAS_UNICODE_FS
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(p, &os_level) && os_level >= APR_WIN_NT)
    {
        apr_wchar_t *ignored;
        apr_wchar_t wpath[APR_PATH_MAX];
        apr_status_t rv;
        /* ???: This needs review, apparently "\\?\d:." returns "\\?\d:" 
         * as if that is useful for anything.
         */
        {
            apr_wchar_t wroot[APR_PATH_MAX];
            if (rv = utf8_to_unicode_path(wroot, sizeof(wroot) 
                                               / sizeof(apr_wchar_t), root))
                return rv;
            if (!GetFullPathNameW(wroot, sizeof(wpath) / sizeof(apr_wchar_t), wpath, &ignored))
                return apr_get_os_error();
        }
        {
            char path[APR_PATH_MAX];
            if ((rv = unicode_to_utf8_path(path, sizeof(path), wpath)))
                return rv;
            *rootpath = apr_pstrdup(p, path);
        }
    }
    else
#endif
    {
        char path[APR_PATH_MAX];
        char *ignored;
        if (!GetFullPathName(root, sizeof(path), path, &ignored))
            return apr_get_os_error();
        *rootpath = apr_pstrdup(p, path);
    }
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
 * form under WinNT/2K.  Since these forms are handled in the utf-8 to 
 * unicode translation phase, we don't want the user confused by them, so 
 * we will accept them but always return the canonical C:/ or //mach/share/
 */

APR_DECLARE(apr_status_t) apr_filepath_root(const char **rootpath, 
                                            const char **inpath, 
                                            apr_int32_t flags,
                                            apr_pool_t *p)
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
                    rv = apr_filepath_root(rootpath, &testpath, flags, p);
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
                apr_status_t rv;
                delim2 = delim1 + 1;
                while (*delim2 && *delim2 != '/' && *delim2 != '\\') {
                    /* Protect against //machine/X/ where X is illegal */
                    if (!is_fnchar(*(delim2++)))
                        return APR_EBADPATH;
                } 

                if (!*delim2) {
                /* Have path of '//machine/[share]' so we must have
                 * an extra byte for the trailing slash
                 */
                    newpath = apr_pstrndup(p, testpath, delim2 - testpath + 1);
                    newpath[delim2 - testpath + 1] = '\0';
                }
                else
                    newpath = apr_pstrndup(p, testpath, delim2 - testpath);

                /* Win32 will argue about slashed in UNC paths, so use 
                 * backslashes till we finish testing
                 */
                newpath[0] = '\\';
                newpath[1] = '\\';
                newpath[delim1 - testpath] = '\\';

                if (delim2 == delim1 + 1) {
                    /* We simply \\machine\, so give up already
                     */
                    *rootpath = newpath;
                    *inpath = delim2;
                    return APR_EINCOMPLETE;
                }

                /* Validate the \\Machine\Share\ designation, must
                 * root this designation!
                 */
                newpath[delim2 - testpath] = '\\';
                if (flags & APR_FILEPATH_TRUENAME) {
                    rv = filepath_root_test(newpath, p);
                    if (rv)
                        return rv;
                    rv = filepath_root_case(&newpath, newpath, p);
                    if (rv)
                        return rv;
                }
                /* If this root included the trailing / or \ designation 
                 * then lop off multiple trailing slashes
                 */
                if (*delim2) {
                    *inpath = delim2 + 1;
                    while (**inpath == '/' || **inpath == '\\')
                        ++*inpath;
                    /* Give back the caller's own trailing delimiter
                     */
                    newpath[delim2 - testpath] = *delim2;
                }
                else
                    *inpath = delim2;
                
                *rootpath = newpath;
                return APR_SUCCESS;
            }
            
            /* Have path of '\\[machine]', if the machine is given,
             * append the trailing \
             */
            delim1 = strchr(testpath, '\0');
            if (delim1 > testpath + 2) {
                newpath = apr_pstrndup(p, testpath, delim1 - testpath + 1);
                newpath[delim1 - testpath] = '\\';
                newpath[delim1 - testpath + 1] = '\0';
            }
            else
                newpath = apr_pstrndup(p, testpath, delim1 - testpath);
            newpath[0] = '\\';
            newpath[1] = '\\';
            *rootpath = newpath;
            *inpath = delim1;
            return APR_EINCOMPLETE;
        }

        /* Left with a path of '/', what drive are we asking about? 
         */
        // ?? if (flags & APR_FILEPATH_TRUENAME) 
        *inpath = ++testpath;
        newpath = apr_palloc(p, 2);
        newpath[0] = ((flags & APR_FILEPATH_NATIVE) ? '\\' : '/');
        newpath[1] = '\0';
        *rootpath = newpath;
        return APR_EINCOMPLETE;
    }

    /* Evaluate path of 'd:[/]' */
    if (is_fnchar(*testpath) && testpath[1] == ':') 
    {
        apr_status_t rv;
        /* Validate that D:\ drive exists, test must be rooted
         * Note that posix/win32 insists a drive letter is upper case,
         * so who are we to argue with a 'feature'.
         * It is a safe fold, since only A-Z is legal, and has no
         * side effects of legal mis-mapped non-us-ascii codes.
         */
        newpath = apr_palloc(p, 4);
        newpath[0] = testpath[0];
        newpath[1] = ':';
        newpath[2] = ((flags & APR_FILEPATH_NATIVE) ? '\\' : '/');
        newpath[3] = '\0';
        if (flags & APR_FILEPATH_TRUENAME) {
            newpath[0] = toupper(newpath[0]);
            rv = filepath_root_test(newpath, p);
            if (rv)
                return rv;
        }
        /* Just give back the root the user handed to us.
         */
        if (testpath[2] != '/' && testpath[2] != '\\') {
            newpath[2] = '\0';
            *rootpath = newpath;
            *inpath = testpath + 2;
            return APR_EINCOMPLETE;
        }

        /* strip off remaining slashes that designate the root.
         */
        *inpath = testpath + 3;
        while (**inpath == '/' || **inpath == '\\')
            ++*inpath;
        *rootpath = newpath;
        return APR_SUCCESS;
    }

    /* Nothing interesting */
    return APR_ERELATIVE;
}


APR_DECLARE(apr_status_t) apr_filepath_merge(char **newpath, 
                                             const char *basepath, 
                                             const char *addpath, 
                                             apr_int32_t flags,
                                             apr_pool_t *p)
{
    char path[APR_PATH_MAX]; /* isn't null term */
    char *baseroot = NULL;
    char *addroot;
    apr_size_t rootlen; /* the length of the root portion of path, d:/ is 3 */
    apr_size_t baselen; /* the length of basepath (excluding baseroot) */
    apr_size_t keptlen; /* the length of the retained basepath (incl root) */
    apr_size_t pathlen; /* the length of the result path */
    apr_size_t segend;  /* the end of the current segment */
    apr_size_t seglen;  /* the length of the segment (excl trailing chars) */
    apr_status_t basetype = 0; /* from parsing the basepath's baseroot */
    apr_status_t addtype;      /* from parsing the addpath's addroot */
    apr_status_t rv;
    int fixunc = 0;  /* flag to complete an incomplete UNC basepath */
    
    /* Treat null as an empty path, otherwise split addroot from the addpath
     */
    if (!addpath) {
        addpath = addroot = "";
        addtype = APR_ERELATIVE;
    }
    else {
        /* This call _should_ test the path
         */
        addtype = apr_filepath_root(&addroot, &addpath, 
                                    APR_FILEPATH_TRUENAME, p);
        if (addtype == APR_SUCCESS) {
            addtype = APR_EABSOLUTE;
        }
        else if (addtype == APR_ERELATIVE) {
            addroot = "";
        }
        else if (addtype != APR_EINCOMPLETE) {
            /* apr_filepath_root was incomprehensible so fail already
             */
            return addtype;
        }
    }

    /* If addpath is (even partially) rooted, then basepath is
     * unused.  Ths violates any APR_FILEPATH_SECUREROOTTEST 
     * and APR_FILEPATH_NOTABSOLUTE flags specified.
     */
    if (addtype == APR_EABSOLUTE || addtype == APR_EINCOMPLETE)
    {
        if (flags & APR_FILEPATH_SECUREROOTTEST)
            return APR_EABOVEROOT;
        if (flags & APR_FILEPATH_NOTABSOLUTE)
            return addtype;
    }

    /* Optimized tests before we query the current working path
     */
    if (!basepath) {

        /* If APR_FILEPATH_NOTABOVEROOT wasn't specified,
         * we won't test the root again, it's ignored.
         * Waste no CPU retrieving the working path.
         */
        if (addtype == APR_EABSOLUTE && !(flags & APR_FILEPATH_NOTABOVEROOT)) {
            basepath = baseroot = "";
            basetype = APR_ERELATIVE;
        }

        /* If APR_FILEPATH_NOTABSOLUTE is specified, the caller 
         * requires an absolutely relative result, So do not retrieve 
         * the working path.
         */
        if (addtype == APR_ERELATIVE && (flags & APR_FILEPATH_NOTABSOLUTE)) {
            basepath = baseroot = "";
            basetype = APR_ERELATIVE;
        }
    }

    if (!basepath) 
    {
        /* Start with the current working path.  This is bass akwards,
         * but required since the compiler (at least vc) doesn't like
         * passing the address of a char const* for a char** arg.
         * We must grab the current path of the designated drive 
         * if addroot is given in drive-relative form (e.g. d:foo)
         */
        char *getpath;
        if (addtype == APR_EINCOMPLETE && addroot[1] == ':')
            rv = filepath_drive_get(&getpath, addroot[0], p);
        else
            rv = apr_filepath_get(&getpath, p);
        if (rv != APR_SUCCESS)
            return rv;
        basepath = getpath;
    }

    if (!baseroot) {
        /* This call should _not_ test the path
         */
        basetype = apr_filepath_root(&baseroot, &basepath, 0, p);
        if (basetype == APR_SUCCESS) {
            basetype = APR_EABSOLUTE;
        }
        else if (basetype == APR_ERELATIVE) {
            baseroot = "";
        }
        else if (basetype != APR_EINCOMPLETE) {
            /* apr_filepath_root was incomprehensible so fail already
             */
            return basetype;
        }
    }
    baselen = strlen(basepath);

    /* If APR_FILEPATH_NOTABSOLUTE is specified, the caller 
     * requires an absolutely relative result.  If the given 
     * basepath is not relative then fail.
     */
    if ((flags & APR_FILEPATH_NOTABSOLUTE) && basetype != APR_ERELATIVE)
        return basetype;

    /* The Win32 nightmare on unc street... start combining for
     * many possible root combinations.
     */
    if (addtype == APR_EABSOLUTE)
    {
        /* Ignore the given root path, and start with the addroot
         */
        if ((flags & APR_FILEPATH_NOTABOVEROOT) 
                && strncmp(baseroot, addroot, strlen(baseroot)))
            return APR_EABOVEROOT;
        keptlen = 0;
        rootlen = pathlen = strlen(addroot);
        memcpy(path, addroot, pathlen);
    }
    else if (addtype == APR_EINCOMPLETE)
    {
        /* There are several types of incomplete paths, 
         *     incomplete UNC paths         (//foo/ or //),
         *     drives without rooted paths  (d: as in d:foo), 
         * and simple roots                 (/ as in /foo).
         * Deal with these in significantly different manners...
         */
        if ((addroot[0] == '/' || addroot[0] == '\\') &&
            (addroot[1] == '/' || addroot[1] == '\\')) 
        {
            /* Ignore the given root path if the incomplete addpath is UNC,
             * (note that the final result will be incomplete).
             */
            if (flags & APR_FILEPATH_NOTRELATIVE)
                return addtype;
            if ((flags & APR_FILEPATH_NOTABOVEROOT) 
                    && strncmp(baseroot, addroot, strlen(baseroot)))
                return APR_EABOVEROOT;
            fixunc = 1;
            keptlen = 0;
            rootlen = pathlen = strlen(addroot);
            memcpy(path, addroot, pathlen);
        }
        else if ((addroot[0] == '/' || addroot[0] == '\\') && !addroot[1]) 
        {
            /* Bring together the drive or UNC root from the baseroot
             * if the addpath is a simple root and basepath is rooted,
             * otherwise disregard the basepath entirely.
             */
            if (basetype != APR_EABSOLUTE && (flags & APR_FILEPATH_NOTRELATIVE))
                return basetype;
            if (basetype != APR_ERELATIVE) {
                if (basetype == APR_INCOMPLETE 
                        && (baseroot[0] == '/' || baseroot[0] == '\\')
                        && (baseroot[1] == '/' || baseroot[1] == '\\'))
                    fixunc = 1;
                keptlen = rootlen = pathlen = strlen(baseroot);
                memcpy(path, baseroot, pathlen);
            }
            else {
                if (flags & APR_FILEPATH_NOTABOVEROOT)
                    return APR_EABOVEROOT;
                keptlen = 0;
                rootlen = pathlen = strlen(addroot);
                memcpy(path, addroot, pathlen);
            }
        }
        else if (addroot[0] && addroot[1] == ':' && !addroot[2]) 
        {
            /* If the addroot is a drive (without a volume root)
             * use the basepath _if_ it matches this drive letter!
             * Otherwise we must discard the basepath.
             */
            if (addroot[0] == baseroot[0] && baseroot[1] == ':') {
                /* Base the result path on the basepath
                 */
                if (basetype != APR_EABSOLUTE && (flags & APR_FILEPATH_NOTRELATIVE))
                    return basetype;
                rootlen = strlen(baseroot);
                keptlen = pathlen = rootlen + baselen;
                if (keptlen >= sizeof(path))
                    return APR_ENAMETOOLONG;
                memcpy(path, baseroot, rootlen);
                memcpy(path + rootlen, basepath, baselen);
            } 
            else {
                if (flags & APR_FILEPATH_NOTRELATIVE)
                    return addtype;
                if (flags & APR_FILEPATH_NOTABOVEROOT)
                    return APR_EABOVEROOT;
                keptlen = 0;
                rootlen = pathlen = strlen(addroot);
                memcpy(path, addroot, pathlen);
            }
        }
        else {
            /* Now this is unexpected, we aren't aware of any other
             * incomplete path forms!  Fail now.
             */
            return APR_EBADPATH;
        }
    }
    else { /* addtype == APR_ERELATIVE */
        /* If both paths are relative, fail early
         */
        if (basetype != APR_EABSOLUTE && (flags & APR_FILEPATH_NOTRELATIVE))
            return basetype;

        /* An incomplete UNC path must be completed
         */
        if (basetype == APR_INCOMPLETE 
                && (baseroot[0] == '/' || baseroot[0] == '\\')
                && (baseroot[1] == '/' || baseroot[1] == '\\'))
            fixunc = 1;

        /* Base the result path on the basepath
         */
        rootlen = strlen(baseroot);
        keptlen = pathlen = rootlen + baselen;
        if (keptlen >= sizeof(path))
            return APR_ENAMETOOLONG;
        memcpy(path, baseroot, rootlen);
        memcpy(path + rootlen, basepath, baselen);
    }

    /* '/' terminate the given root path unless it's already terminated
     * or is an incomplete drive root.  Correct the trailing slash unless
     * we have an incomplete UNC path still to fix.
     */
    if (pathlen && path[pathlen - 1] != ':') {
        if (path[pathlen - 1] != '/' && path[pathlen - 1] != '\\') {
            if (pathlen + 1 >= sizeof(path))
                return APR_ENAMETOOLONG;
        
            path[pathlen++] = ((flags & APR_FILEPATH_NATIVE) ? '\\' : '/');
        }
    /*  XXX: wrong, but gotta figure out what I intended;
     *  else if (!fixunc)
     *      path[pathlen++] = ((flags & APR_FILEPATH_NATIVE) ? '\\' : '/');
     */
    }

    while (*addpath) 
    {
        /* Parse each segment, find the closing '/' 
         */
        seglen = 0;
        while (addpath[seglen] && addpath[seglen] != '/'
                               && addpath[seglen] != '\\')
            ++seglen;

        /* Truncate all trailing spaces and all but the first two dots */
        segend = seglen;
        while (seglen && (addpath[seglen - 1] == ' ' 
                       || addpath[seglen - 1] == '.')) {
            if (seglen > 2 || addpath[seglen - 1] != '.' || addpath[0] != '.')
                --seglen;
            else
                break;
        }

        if (seglen == 0 || (seglen == 1 && addpath[0] == '.')) 
        {
            /* NOTE: win32 _hates_ '/ /' and '/. /' (yes, with spaces in there)
             * so eliminate all preconceptions that it is valid.
             */
            if (seglen < segend)
                return APR_EBADPATH;

            /* This isn't legal unless the unc path is completed
             */
            if (fixunc)
                return APR_EBADPATH;

            /* Otherwise, this is a noop segment (/ or ./) so ignore it 
             */
        }
        else if (seglen == 2 && addpath[0] == '.' && addpath[1] == '.') 
        {
            /* NOTE: win32 _hates_ '/.. /' (yes, with a space in there) and
             * '/..../' so eliminate all preconceptions that they are valid.
             */
            if (seglen < segend && (seglen != 3 || addpath[2] != '.'))
                return APR_EBADPATH;

            /* This isn't legal unless the unc path is completed
             */
            if (fixunc)
                return APR_EBADPATH;

            /* backpath (../) */
            if (pathlen <= rootlen) 
            {
                /* Attempt to move above root.  Always die if the 
                 * APR_FILEPATH_SECUREROOTTEST flag is specified.
                 */
                if (flags & APR_FILEPATH_SECUREROOTTEST)
                    return APR_EABOVEROOT;
                
                /* Otherwise this is simply a noop, above root is root.
                 */
            }
            else if (pathlen == 0 ||
                     (pathlen >= 3 && (pathlen == 3
                                    || path[pathlen - 4] == ':')
                                   &&  path[pathlen - 3] == '.' 
                                   &&  path[pathlen - 2] == '.' 
                                   && (path[pathlen - 1] == '/' 
                                    || path[pathlen - 1] == '\\')))
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
                memcpy(path + pathlen, ((flags && APR_FILEPATH_NATIVE) 
                                          ? "..\\" : "../"), 3);
                pathlen += 3;
            }
            else 
            {
                /* otherwise crop the prior segment 
                 */
                do {
                    --pathlen;
                } while (pathlen && path[pathlen - 1] != '/'
                                 && path[pathlen - 1] != '\\');
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
        else /* not empty or dots */
        {
            if (fixunc) {
                char *testpath = path;
                char *testroot;
                apr_status_t testtype;
                apr_size_t i = (addpath[segend] != '\0');
                

                /* This isn't legal unless the unc path is complete!
                 */
                if (seglen < segend)
                    return APR_EBADPATH;
                if (pathlen + seglen + 1 >= sizeof(path))
                    return APR_ENAMETOOLONG;
                memcpy(path + pathlen, addpath, seglen + i);
                
                /* Always add the trailing slash to a UNC segment
                 */
                path[pathlen + seglen] = ((flags & APR_FILEPATH_NATIVE) 
                                             ? '\\' : '/');
                pathlen += seglen + 1;

                /* Recanonicalize the UNC root with the new UNC segment,
                 * and if we succeed, reset this test and the rootlen,
                 * and replace our path with the canonical UNC root path
                 */
                path[pathlen] = '\0';
                /* This call _should_ test the path
                 */
                testtype = apr_filepath_root(&testroot, &testpath, 
                                             APR_FILEPATH_TRUENAME, p);
                if (testtype == APR_SUCCESS) {
                    rootlen = pathlen = (testpath - path);
                    memcpy(path, testroot, pathlen);
                    fixunc = 0;
                }
                else if (testtype != APR_EINCOMPLETE) {
                    /* apr_filepath_root was very unexpected so fail already
                     */
                    return testtype;
                }
            }
            else {
                /* An actual segment, append it to the destination path
                 */
                apr_size_t i = (addpath[segend] != '\0');
                if (pathlen + seglen + i >= sizeof(path))
                    return APR_ENAMETOOLONG;
                memcpy(path + pathlen, addpath, seglen + i);
                if (i)
                    path[pathlen + seglen] = ((flags & APR_FILEPATH_NATIVE) 
                                                 ? '\\' : '/');
                pathlen += seglen + i;
            }
        }

        /* Skip over trailing slash to the next segment
         */
        if (addpath[segend])
            ++segend;

        addpath += segend;
    }
    path[pathlen] = '\0';
    
    /* keptlen will be the baselen unless the addpath contained
     * backpath elements.  If so, and APR_FILEPATH_NOTABOVEROOT
     * is specified (APR_FILEPATH_SECUREROOTTEST was caught above),
     * compare the string beyond the root to assure the result path 
     * is still within given basepath.  Note that the root path 
     * segment is thoroughly tested prior to path parsing.
     */
    if (flags & APR_FILEPATH_NOTABOVEROOT && (keptlen - rootlen) < baselen) {
        if (strncmp(basepath, path + rootlen, baselen))
            return APR_EABOVEROOT;

        /* Ahem... if we weren't given a trailing slash on the basepath,
         * we better be sure that /foo wasn't replaced with /foobar!
         */
        if (basepath[baselen - 1] != '/' && basepath[baselen - 1] != '\\'
                && path[rootlen + baselen] && path[rootlen + baselen] != '/' 
                                           && path[rootlen + baselen] != '\\')
            return APR_EABOVEROOT;
    }

#if 0
    /* Just an idea - still don't know where it's headed */
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
