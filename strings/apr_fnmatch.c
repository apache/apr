/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */ 

#ifndef WIN32
#include "apr_private.h"
#endif
#include "apr_file_info.h"
#include "apr_fnmatch.h"
#include "apr_tables.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include <string.h>
#if APR_HAVE_CTYPE_H
# include <ctype.h>
#endif

/* This function is an Apache addition
 * return non-zero if pattern has any glob chars in it
 * @bug Function does not distinguish for FNM_PATHNAME mode, which renders
 * a false positive for test[/]this (which is not a range, but 
 * seperate test[ and ]this segments and no glob.)
 * @bug Function does not distinguish for non-FNM_ESCAPE mode.
 * @bug Function does not parse []] correctly
 * Solution may be to use fnmatch_ch() to walk the patterns?
 */
APR_DECLARE(int) apr_fnmatch_test(const char *pattern)
{
    int nesting;

    nesting = 0;
    while (*pattern) {
        switch (*pattern) {
        case '?':
        case '*':
            return 1;

        case '\\':
            if (*++pattern == '\0') {
                return 0;
            }
            break;

        case '[':         /* '[' is only a glob if it has a matching ']' */
            ++nesting;
            break;

        case ']':
            if (nesting) {
                return 1;
            }
            break;
        }
        ++pattern;    }
    return 0;
}


/* Find all files matching the specified pattern */
APR_DECLARE(apr_status_t) apr_match_glob(const char *pattern, 
                                         apr_array_header_t **result,
                                         apr_pool_t *p)
{
    apr_dir_t *dir;
    apr_finfo_t finfo;
    apr_status_t rv;
    char *path;

    /* XXX So, this is kind of bogus.  Basically, I need to strip any leading
     * directories off the pattern, but there is no portable way to do that.
     * So, for now we just find the last occurance of '/' and if that doesn't
     * return anything, then we look for '\'.  This means that we could
     * screw up on unix if the pattern is something like "foo\.*"  That '\'
     * isn't a directory delimiter, it is a part of the filename.  To fix this,
     * we really need apr_filepath_basename, which will be coming as soon as
     * I get to it.  rbb
     */
    char *idx = strrchr(pattern, '/');
    
    if (idx == NULL) {
        idx = strrchr(pattern, '\\');
    }
    if (idx == NULL) {
        path = ".";
    }
    else {
        path = apr_pstrndup(p, pattern, idx - pattern);
        pattern = idx + 1;
    }

    *result = apr_array_make(p, 0, sizeof(char *));
    rv = apr_dir_open(&dir, path, p);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    while (apr_dir_read(&finfo, APR_FINFO_NAME, dir) == APR_SUCCESS) {
        if (apr_fnmatch(pattern, finfo.name, 0) == APR_SUCCESS) {
            *(const char **)apr_array_push(*result) = apr_pstrdup(p, finfo.name);
        }
    }
    apr_dir_close(dir);
    return APR_SUCCESS;
}
