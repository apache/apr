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
#include "win32/fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_errno.h"
#include "apr_time.h"
#include <sys/stat.h>
#include "atime.h"
#include "misc.h"
#include <aclapi.h>

static apr_status_t free_localheap(void *heap) {
    LocalFree(heap);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_getfileinfo(apr_finfo_t *finfo, apr_int32_t wanted,
                                          apr_file_t *thefile)
{
    BY_HANDLE_FILE_INFORMATION FileInformation;
    apr_oslevel_e os_level;
    PSID user = NULL, grp = NULL;
    PACL dacl = NULL;
    apr_status_t rv;

    if (!GetFileInformationByHandle(thefile->filehand, &FileInformation)) {
        return apr_get_os_error();
    }

    memset(finfo, '\0', sizeof(*finfo));
    finfo->cntxt = thefile->cntxt;

    FileTimeToAprTime(&finfo->atime, &FileInformation.ftLastAccessTime);
    FileTimeToAprTime(&finfo->ctime, &FileInformation.ftCreationTime);
    FileTimeToAprTime(&finfo->mtime, &FileInformation.ftLastWriteTime);

    finfo->inode  =  (apr_ino_t)FileInformation.nFileIndexLow
                  | ((apr_ino_t)FileInformation.nFileIndexHigh << 32);
    finfo->device = FileInformation.dwVolumeSerialNumber;
    finfo->nlink  = FileInformation.nNumberOfLinks;

#if APR_HAS_LARGE_FILES
    finfo->size =  (apr_off_t)FileInformation.nFileSizeLow
                | ((apr_off_t)FileInformation.nFileSizeHigh << 32);
#else
    finfo->size = (apr_off_t)FileInformation.nFileSizeLow;
    if (finfo->size < 0 || FileInformation.nFileSizeHigh)
        finfo->size = 0x7fffffff;
#endif
    
    finfo->valid = APR_FINFO_ATIME | APR_FINFO_CTIME | APR_FINFO_MTIME
                 | APR_FINFO_IDENT | APR_FINFO_NLINK | APR_FINFO_SIZE;


    if (wanted & APR_FINFO_TYPE) 
    {
        DWORD FileType;
        if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            finfo->filetype = APR_LNK;
            finfo->valid |= APR_FINFO_TYPE;
        }
        else if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            finfo->filetype = APR_DIR;
            finfo->valid |= APR_FINFO_TYPE;
        }
        else if (FileType = GetFileType(thefile->filehand)) {
            if (FileType == FILE_TYPE_DISK) {
                finfo->filetype = APR_REG;
                finfo->valid |= APR_FINFO_TYPE;
            }
            else if (FileType == FILE_TYPE_CHAR) {
                finfo->filetype = APR_CHR;
                finfo->valid |= APR_FINFO_TYPE;
            }
            else if (FileType == FILE_TYPE_PIPE) {
                finfo->filetype = APR_PIPE;
                finfo->valid |= APR_FINFO_TYPE;
            }
        }
    }
    

    if ((wanted & (APR_FINFO_PROT | APR_FINFO_OWNER))
            && !apr_get_oslevel(thefile->cntxt, &os_level)
            && os_level >= APR_WIN_NT) {
        SECURITY_INFORMATION sinf = 0;
        PSECURITY_DESCRIPTOR pdesc = NULL;
        if (wanted & (APR_FINFO_USER | APR_FINFO_UPROT))
            sinf |= OWNER_SECURITY_INFORMATION;
        if (wanted & (APR_FINFO_GROUP | APR_FINFO_GPROT))
            sinf |= GROUP_SECURITY_INFORMATION;
        if (wanted & APR_FINFO_PROT)
            sinf |= DACL_SECURITY_INFORMATION;
        rv = GetSecurityInfo(thefile->filehand, SE_FILE_OBJECT, sinf,
                             ((wanted & APR_FINFO_USER) ? &user : NULL),
                             ((wanted & APR_FINFO_GROUP) ? &grp : NULL),
                             ((wanted & APR_FINFO_PROT) ? &dacl : NULL),
                             NULL, &pdesc);
        if (rv == ERROR_SUCCESS)
            apr_register_cleanup(thefile->cntxt, pdesc, free_localheap, 
                                 apr_null_cleanup);
    }

    if (user) {
        finfo->user = user;
        finfo->valid |= APR_FINFO_USER;
    }

    if (grp) {
        finfo->group = grp;
        finfo->valid |= APR_FINFO_GROUP;
    }

    if (dacl) {
        /* Retrieved the discresionary access list */
        
    }
    else {
        /* Read, write execute for owner.  In the Win32 environment, 
         * anything readable is executable (well, not entirely 100% true, 
         * but I'm looking for some obvious logic that would help us here.)
         * TODO: The real permissions come from the DACL
         */
        if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
            finfo->protection |= S_IREAD | S_IEXEC;
        }
        else {
            finfo->protection |= S_IREAD | S_IWRITE | S_IEXEC;
        }
        finfo->valid |= APR_FINFO_UPROT;
    }    
    
    if (wanted & ~finfo->valid)
        return APR_INCOMPLETE;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_setfileperms(const char *fname,
                                           apr_fileperms_t perms)
{
    return APR_ENOTIMPL;
}

static int stat_with_handle(apr_finfo_t *finfo, const char *fname,
                            apr_int32_t wanted, apr_pool_t *cont)
{
    apr_file_t *thefile = NULL;
    apr_status_t rv;
    /* 
     * NT5 (W2K) only supports symlinks in the same manner as mount points.
     * This code should eventually take that into account, for now treat
     * every reparse point as a symlink...
     *
     * We must open the file with READ_CONTROL if we plan to retrieve the
     * user, group or permissions.
     */
    
    if ((rv = apr_open(&thefile, fname, 
                       ((wanted & APR_FINFO_LINK) ? APR_OPENLINK : 0)
                     | ((wanted & (APR_FINFO_PROT | APR_FINFO_OWNER))
                           ? APR_READCONTROL : 0),
                       APR_OS_DEFAULT, cont)) == APR_SUCCESS) {
        rv = apr_getfileinfo(finfo, wanted, thefile);
        finfo->filehand = NULL;
        apr_close(thefile);
    }
    else if (APR_STATUS_IS_EACCES(rv) && (wanted & (APR_FINFO_PROT 
                                                  | APR_FINFO_OWNER))) {
        /* We have a backup plan.  Perhaps we couldn't grab READ_CONTROL?
         * proceed without asking for that permission...
         */
        if ((rv = apr_open(&thefile, fname, 
                           ((wanted & APR_FINFO_LINK) ? APR_OPENLINK : 0),
                           APR_OS_DEFAULT, cont)) == APR_SUCCESS) {
            rv = apr_getfileinfo(finfo, wanted & ~(APR_FINFO_PROT 
                                                 | APR_FINFO_OWNER),
                                 thefile);
            finfo->filehand = NULL;
            apr_close(thefile);
        }
    }
    if (rv != APR_SUCCESS && rv != APR_INCOMPLETE)
        return (rv);
    /* We picked up this case above and had opened the link's properties */
    if (wanted & APR_FINFO_LINK)
        finfo->valid |= APR_FINFO_LINK;
    finfo->fname = thefile->fname;

    return rv;
}

APR_DECLARE(apr_status_t) apr_stat(apr_finfo_t *finfo, const char *fname,
                                   apr_int32_t wanted, apr_pool_t *cont)
{
    /* The WIN32_FILE_ATTRIBUTE_DATA is a subset of this structure
     */
    WIN32_FIND_DATA FileInformation;
    apr_oslevel_e os_level;
    PSID user = NULL, grp = NULL;
    PACL dacl = NULL;

    if (apr_get_oslevel(cont, &os_level))
        os_level = APR_WIN_95;
    
    /* Catch fname length == MAX_PATH since GetFileAttributesEx fails 
     * with PATH_NOT_FOUND.  We would rather indicate length error than 
     * 'not found'
     */        
    if (strlen(fname) >= APR_PATH_MAX) {
        return APR_ENAMETOOLONG;
    }
#ifdef APR_HAS_UNICODE_FS
    else if (os_level >= APR_WIN_NT) {
        apr_wchar_t wfname[APR_PATH_MAX];
        apr_status_t rv;
        if (rv = utf8_to_unicode_path(wfname, sizeof(wfname) 
                                            / sizeof(apr_wchar_t), fname))
            return rv;
        if (!GetFileAttributesExW(wfname, GetFileExInfoStandard, 
                                  &FileInformation)) {
            return apr_get_os_error();
        }
        if (wanted & (APR_FINFO_PROT | APR_FINFO_OWNER)) {
            SECURITY_INFORMATION sinf = 0;
            PSECURITY_DESCRIPTOR pdesc = NULL;
            if (wanted & (APR_FINFO_USER | APR_FINFO_UPROT))
                sinf |= OWNER_SECURITY_INFORMATION;
            if (wanted & (APR_FINFO_GROUP | APR_FINFO_GPROT))
                sinf |= GROUP_SECURITY_INFORMATION;
            if (wanted & APR_FINFO_PROT)
                sinf |= DACL_SECURITY_INFORMATION;
            rv = GetNamedSecurityInfoW(wfname, SE_FILE_OBJECT, sinf,
                                 ((wanted & APR_FINFO_USER) ? &user : NULL),
                                 ((wanted & APR_FINFO_GROUP) ? &grp : NULL),
                                 ((wanted & APR_FINFO_PROT) ? &dacl : NULL),
                                 NULL, &pdesc);
            if (rv == ERROR_SUCCESS)
                apr_register_cleanup(cont, pdesc, free_localheap, 
                                     apr_null_cleanup);
        }
    }
#endif
    else if (os_level >= APR_WIN_98) {
        if (!GetFileAttributesExA(fname, GetFileExInfoStandard, 
                                 &FileInformation)) {
            return apr_get_os_error();
        }
    }
    else  {
        /* What a waste of cpu cycles... but we don't have a choice
         * Be sure we insulate ourselves against bogus wildcards
         */
        HANDLE hFind;
        if (strchr(fname, '*') || strchr(fname, '?'))
            return APR_ENOENT;
        hFind = FindFirstFile(fname, &FileInformation);
        if (hFind == INVALID_HANDLE_VALUE) {
            return apr_get_os_error();
    	} 
        else {
            FindClose(hFind);
        }
    }

    memset(finfo, '\0', sizeof(*finfo));
    finfo->cntxt = cont;
    finfo->valid = APR_FINFO_ATIME | APR_FINFO_CTIME | APR_FINFO_MTIME
                 | APR_FINFO_SIZE  | APR_FINFO_TYPE;

    /* File times */
    FileTimeToAprTime(&finfo->atime, &FileInformation.ftLastAccessTime);
    FileTimeToAprTime(&finfo->ctime, &FileInformation.ftCreationTime);
    FileTimeToAprTime(&finfo->mtime, &FileInformation.ftLastWriteTime);

#if APR_HAS_LARGE_FILES
    finfo->size =  (apr_off_t)FileInformation.nFileSizeLow
                | ((apr_off_t)FileInformation.nFileSizeHigh << 32);
#else
    finfo->size = (apr_off_t)FileInformation.nFileSizeLow;
    if (finfo->size < 0 || FileInformation.nFileSizeHigh)
        finfo->size = 0x7fffffff;
#endif

    /* Filetype - Directory or file?
     */
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        finfo->filetype = APR_LNK;
    }
    else if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        finfo->filetype = APR_DIR;
    }
    else {
        /* XXX: Solve this
         * Short of opening the handle to the file, the 'FileType' appears
         * to be unknowable (in any trustworthy or consistent sense), that
         * is, as far as PIPE, CHR, etc.
         */
        finfo->filetype = APR_REG;
    }

    if (user) {
        finfo->user = user;
        finfo->valid |= APR_FINFO_USER;
    }

    if (grp) {
        finfo->group = grp;
        finfo->valid |= APR_FINFO_GROUP;
    }

    if (dacl) {
        /* Retrieved the discresionary access list, provide some real answers
         */
        
    }
    else {
        /* Read, write execute for owner
         * In the Win32 environment, anything readable is executable
         * (not entirely 100% true, but the dacl is -expensive-!)
         */
        if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
            finfo->protection |= S_IREAD | S_IEXEC;
        }
        else {
            finfo->protection |= S_IREAD | S_IWRITE | S_IEXEC;
        }
        /* Lying through our teeth */
        finfo->valid |= APR_FINFO_UPROT;
    }

    if (wanted & ~finfo->valid)
        return APR_INCOMPLETE;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lstat(apr_finfo_t *finfo, const char *fname,
                                    apr_int32_t wanted, apr_pool_t *cont)
{
    return apr_stat(finfo, fname, wanted & APR_FINFO_LINK, cont);
}

#if 0
    apr_oslevel_e os_level;
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT)
    {
        WIN32_FIND_DATAW FileInformation;
        HANDLE hFind;
        apr_wchar_t *wname;
        if (strchr(fspec, '*') || strchr(fspec, '?'))
            return APR_ENOENT;
        wname = utf8_to_unicode_path(fspec, cont);
        if (!wname)
            return APR_ENAMETOOLONG;
        hFind = FindFirstFileW(wname, &FileInformation);
        if (hFind == INVALID_HANDLE_VALUE)
            return apr_get_os_error();
    	else
            FindClose(hFind);
        *fname = unicode_to_utf8_path(FileInformation.cFileName, cont);
#endif