/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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
#include "apr_general.h"
#include "apr_lib.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

/* Returns TRUE if the input string is a string
 * of one or more '.' characters.
 */
static BOOL OnlyDots(char *pString)
{
    char *c;

    if (*pString == '\0')
        return FALSE;

    for (c = pString;*c;c++)
        if (*c != '.')
            return FALSE;

    return TRUE;
}

/* Accepts as input a pathname, and tries to match it to an 
 * existing path and return the pathname in the case that
 * is present on the existing path.  This routine also
 * converts alias names to long names.
 */
APR_EXPORT(char *) ap_os_systemcase_filename(ap_pool_t *pCont, 
                                             const char *szFile)
{
    char buf[HUGE_STRING_LEN];
    char *pInputName;
    char *p, *q;
    BOOL bDone = FALSE;
    BOOL bFileExists = TRUE;
    HANDLE hFind;
    WIN32_FIND_DATA wfd;

    if (!szFile || strlen(szFile) == 0 || strlen(szFile) >= sizeof(buf))
        return ap_pstrdup(pCont, "");

    buf[0] = '\0';
    pInputName = ap_pstrdup(pCont, szFile);

    /* First convert all slashes to \ so Win32 calls work OK */
    for (p = pInputName; *p; p++) {
        if (*p == '/')
            *p = '\\';
    }
    
    p = pInputName;
    /* If there is drive information, copy it over. */ 
    if (pInputName[1] == ':') {
        buf[0] = tolower(*p++);
        buf[1] = *p++;
        buf[2] = '\0';

        /* If all we have is a drive letter, then we are done */
        if (strlen(pInputName) == 2)
            bDone = TRUE;
    }
    
    q = p;
    if (*p == '\\') {
        p++;
        if (*p == '\\')  /* Possible UNC name */
        {
            p++;
            /* Get past the machine name.  FindFirstFile */
            /* will not find a machine name only */
            p = strchr(p, '\\'); 
            if (p)
            {
                p++;
                /* Get past the share name.  FindFirstFile */
                /* will not find a \\machine\share name only */
                p = strchr(p, '\\'); 
                if (p) {
                    strncat(buf,q,p-q);
                    q = p;
                    p++;
                }
            }

            if (!p)
                p = q;
        }
    }

    p = strchr(p, '\\');

    while (!bDone) {
        if (p)
            *p = '\0';

        if (strchr(q, '*') || strchr(q, '?'))
            bFileExists = FALSE;

        /* If the path exists so far, call FindFirstFile
         * again.  However, if this portion of the path contains
         * only '.' charaters, skip the call to FindFirstFile
         * since it will convert '.' and '..' to actual names.
         * Note: in the call to OnlyDots, we may have to skip
         *       a leading slash.
         */
        if (bFileExists && !OnlyDots((*q == '.' ? q : q+1))) {            
            hFind = FindFirstFile(pInputName, &wfd);
            
            if (hFind == INVALID_HANDLE_VALUE) {
                bFileExists = FALSE;
            }
            else {
                FindClose(hFind);

                if (*q == '\\')
                    strcat(buf,"\\");
                strcat(buf, wfd.cFileName);
            }
        }
        
        if (!bFileExists || OnlyDots((*q == '.' ? q : q+1))) {
            strcat(buf, q);
        }
        
        if (p) {
            q = p;
            *p++ = '\\';
            p = strchr(p, '\\');
        }
        else {
            bDone = TRUE;
        }
    }
    
    /* First convert all slashes to / so server code handles it ok */
    for (p = buf; *p; p++) {
        if (*p == '\\')
            *p = '/';
    }

    return ap_pstrdup(pCont, buf);
}
 
/*  Perform canonicalization with the exception that the
 *  input case is preserved.
 */
char * canonical_filename(ap_pool_t *pCont, const char *szFile)
{
    char *pNewStr;
    char *s;
    char *p; 
    char *q;

    if (szFile == NULL || strlen(szFile) == 0)
        return ap_pstrdup(pCont, "");

    pNewStr = ap_pstrdup(pCont, szFile);

    /*  Change all '\' characters to '/' characters.
     *  While doing this, remove any trailing '.'.
     *  Also, blow away any directories with 3 or
     *  more '.'
     */
    for (p = pNewStr,s = pNewStr; *s; s++,p++) {
        if (*s == '\\' || *s == '/') {

            q = p;
            while (p > pNewStr && *(p-1) == '.')
                p--;

            if (p == pNewStr && q-p <= 2 && *p == '.')
                p = q;
            else if (p > pNewStr && p < q && *(p-1) == '/') {
                if (q-p > 2)
                    p--;
                else
                    p = q;
            }

            *p = '/';
        }
        else {
            *p = *s;
        }
    }
    *p = '\0';

    /*  Blow away any final trailing '.' since on Win32
     *  foo.bat == foo.bat. == foo.bat... etc.
     *  Also blow away any trailing spaces since
     *  "filename" == "filename "
     */
    q = p;
    while (p > pNewStr && (*(p-1) == '.' || *(p-1) == ' '))
        p--;
    if ((p > pNewStr) ||
        (p == pNewStr && q-p > 2))
        *p = '\0';
        

    /*  One more security issue to deal with.  Win32 allows
     *  you to create long filenames.  However, alias filenames
     *  are always created so that the filename will
     *  conform to 8.3 rules.  According to the Microsoft
     *  Developer's network CD (1/98) 
     *  "Automatically generated aliases are composed of the 
     *   first six characters of the filename plus ~n 
     *   (where n is a number) and the first three characters 
     *   after the last period."
     *  Here, we attempt to detect and decode these names.
     */
    p = strchr(pNewStr, '~');
    if (p != NULL) {
        char *pConvertedName, *pQstr, *pPstr;
        char buf[HUGE_STRING_LEN];
        /* We potentially have a short name.  Call 
         * ap_os_systemcase_filename to examine the filesystem
         * and possibly extract the long name.
         */
        pConvertedName = ap_os_systemcase_filename(pCont, pNewStr);

        /* Since we want to preserve the incoming case as much
         * as we can, compare for differences in the string and
         * only substitute in the path names that changed.
         */
        if (stricmp(pNewStr, pConvertedName)) {
            buf[0] = '\0';

            q = pQstr = pConvertedName;
            p = pPstr = pNewStr;
            do {
                q = strchr(q,'/');
                p = strchr(p,'/');

                if (p != NULL) {
                    *q = '\0';
                    *p = '\0';
                }

                if (stricmp(pQstr, pPstr)) 
                    strcat(buf, pQstr);   /* Converted name */
                else 
                    strcat(buf, pPstr);   /* Original name  */


                if (p != NULL) {
                    pQstr = q;
                    pPstr = p;
                    *q++ = '/';
                    *p++ = '/';
                }

            } while (p != NULL); 
			
            pNewStr = ap_pstrdup(pCont, buf);
        }
    }
    return pNewStr;
}
