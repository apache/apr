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


APR_DECLARE(apr_status_t) apr_getfileinfo(apr_finfo_t *finfo, apr_int32_t wanted,
                                          apr_file_t *thefile)
{
    BY_HANDLE_FILE_INFORMATION FileInformation;
    DWORD FileType;
    apr_oslevel_e os_level;

    if (!GetFileInformationByHandle(thefile->filehand, &FileInformation)) {
        return apr_get_os_error();
    }

    FileType = GetFileType(thefile->filehand);
    if (!FileType) {
        return apr_get_os_error();
    }

    /* If my rudimentary knowledge of posix serves... inode is the absolute
     * id of the file (uniquifier) that is returned by NT (not 9x) as follows:
     */
    if (apr_get_oslevel(thefile->cntxt, &os_level) || os_level >= APR_WIN_NT) 
    {
        finfo->inode = (apr_ino_t) FileInformation.nFileIndexHigh << 16
                                 | FileInformation.nFileIndexLow;
        finfo->device = FileInformation.dwVolumeSerialNumber;
    }
    else 
    {
        /* Since the apr_stat is not implemented with CreateFile(),
         * (directories can't be opened with CreateFile() at all)
         * the apr_stat always returns 0 - so must apr_getfileinfo().
         */
        finfo->inode = 0;
        finfo->device = 0;
    }
    /* user and group could be returned as SID's, although this creates
     * it's own unique set of issues.  All three fields are significantly
     * longer than the posix compatible kernals would ever require.
     * TODO: Someday solve this, and fix the executable flag below the
     * right way with a security permission test (as well as r/w flags.)
     */
    finfo->user = 0;
    finfo->group = 0;
    
    /* Filetype - Directory or file: this case _will_ never happen */
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        finfo->protection = S_IFLNK;
        finfo->filetype = APR_LNK;
    }
    else if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        finfo->protection = S_IFDIR;
        finfo->filetype = APR_DIR;
    }
    else if (FileType == FILE_TYPE_DISK) {
        finfo->protection = S_IFREG;
        finfo->filetype = APR_REG;
    }
    else if (FileType == FILE_TYPE_CHAR) {
        finfo->protection = S_IFCHR;
        finfo->filetype = APR_CHR;
    }
    else if (FileType == FILE_TYPE_PIPE) {
        finfo->protection = S_IFIFO;
        finfo->filetype = APR_PIPE;
    }
    else {
        finfo->protection = 0;
        finfo->filetype = APR_NOFILE;
    }

    /* Read, write execute for owner
     * In the Win32 environment, anything readable is executable
     * (well, not entirely 100% true, but I'm looking for a way 
     * to get at the acl permissions in simplified fashion.)
     */
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
        finfo->protection |= S_IREAD | S_IEXEC;
    }
    else {
        finfo->protection |= S_IREAD | S_IWRITE | S_IEXEC;
    }
    
    /* File times */
    FileTimeToAprTime(&finfo->atime, &FileInformation.ftLastAccessTime);
    FileTimeToAprTime(&finfo->ctime, &FileInformation.ftCreationTime);
    FileTimeToAprTime(&finfo->mtime, &FileInformation.ftLastWriteTime);

    /* File size 
     * Note: This cannot handle files greater than can be held by an int */
    finfo->size = FileInformation.nFileSizeLow;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_setfileperms(const char *fname,
                                           apr_fileperms_t perms)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_stat(apr_finfo_t *finfo, const char *fname,
                                   apr_int32_t wanted, apr_pool_t *cont)
{
    apr_oslevel_e os_level;
    /*
     * We need to catch the case where fname length == MAX_PATH since for
     * some strange reason GetFileAttributesEx fails with PATH_NOT_FOUND.
     * We would rather indicate length error than 'not found'
     * since in many cases the apr user is testing for 'not found' 
     * and this is not such a case.
     */        
    if (!apr_get_oslevel(cont, &os_level) && os_level >= APR_WIN_NT)
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
        
        if (rv = apr_open(&thefile, fname, 
                          ((wanted & APR_FINFO_LINK) ? APR_OPENLINK : 0)
                        | ((wanted & (APR_FINFO_PROT | APR_FINFO_OWNER))
                             ? APR_READCONTROL : 0), APR_OS_DEFAULT, cont))
        {
            /* We have a backup plan.  Perhaps we couldn't grab READ_CONTROL?
             * proceed with the alternate...
             */
            if (wanted & (APR_FINFO_PROT | APR_FINFO_OWNER)) {
                rv = apr_open(&thefile, fname, 
                              ((wanted & APR_FINFO_LINK) ? APR_OPENLINK : 0),
                              APR_OS_DEFAULT, cont);
                wanted &= ~(APR_FINFO_PROT | APR_FINFO_OWNER);
            }
            if (rv)
                return rv;
        }

        /* 
         * NT5 (W2K) only supports symlinks in the same manner as mount points.
         * This code should eventually take that into account, for now treat
         * every reparse point as a symlink...
         *
         * We must open the file with READ_CONTROL if we plan to retrieve the
         * user, group or permissions.
         */
        rv = apr_getfileinfo(finfo, wanted, thefile);
        finfo->cntxt = thefile->cntxt;
        finfo->fname = thefile->fname;
        finfo->filehand = thefile;
        return (rv);
    }
    else
    {
        WIN32_FIND_DATA FileInformation;
        /*
         * Big enough for the Win95 FindFile, but actually the same
         * initial fields as the GetFileAttributesEx return structure
         * used for Win98
         */
        if (strlen(fname) >= MAX_PATH) {
            return APR_ENAMETOOLONG;
        }
        else if (os_level >= APR_WIN_98) 
        {
            if (!GetFileAttributesEx(fname, GetFileExInfoStandard, 
                                     &FileInformation)) {
                return apr_get_os_error();
            }
        }
        else 
        {
            /* What a waste of cpu cycles... but we don't have a choice
             */
            HANDLE hFind;
            if (strchr(fname, '*') || strchr(fname, '?'))
                return APR_ENOENT;
            hFind = FindFirstFile(fname, &FileInformation);
            if (hFind == INVALID_HANDLE_VALUE) {
                return apr_get_os_error();
    	    } else {
                FindClose(hFind);
            }
        }

        memset(finfo, '\0', sizeof(*finfo));
        /* Filetype - Directory or file?
         * Short of opening the handle to the file, the 'FileType' appears
         * to be unknowable (in any trustworthy or consistent sense), that
         * is, as far as PIPE, CHR, etc.
         */
        if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            finfo->protection = S_IFDIR;
            finfo->filetype = APR_DIR;
        }
        else {
            finfo->protection = S_IFREG;
            finfo->filetype = APR_REG;
        }
    
        /* Read, write execute for owner
         * In the Win32 environment, anything readable is executable
         * (well, not entirely 100% true, but I'm looking for a way 
         * to get at the acl permissions in simplified fashion.)
         */
        if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
            finfo->protection |= S_IREAD | S_IEXEC;
        }
        else {
            finfo->protection |= S_IREAD | S_IWRITE | S_IEXEC;
        }

        /* Is this an executable? Guess based on the file extension?
         * This is a rather silly test, IMHO... we are looking to test
         * if the user 'may' execute a file (permissions), 
         * not if the filetype is executable.
         * XXX: We need to find a solution, for real.  Or provide some
         * new mechanism for control.  Note that the PATHEXT env var,
         * in the format .ext[;.ext]... actually lists the 'executable'
         * types (invoking without an extension.)  Perhaps a registry
         * key test is even appropriate here.
         */
    
        /* File times */
        FileTimeToAprTime(&finfo->atime, &FileInformation.ftLastAccessTime);
        FileTimeToAprTime(&finfo->ctime, &FileInformation.ftCreationTime);
        FileTimeToAprTime(&finfo->mtime, &FileInformation.ftLastWriteTime);

        /* Note: This cannot handle files greater than can be held by an int 
         */
        finfo->size = FileInformation.nFileSizeLow;
        if (finfo->size < 0 || FileInformation.nFileSizeHigh)
            finfo->size = 0x7fffffff;
    }
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