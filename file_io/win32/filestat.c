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
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_errno.h"
#include "apr_time.h"
#include <sys/stat.h>
#include "atime.h"
#include "misc.h"

#define S_ISLNK(m)  (0)
#define S_ISREG(m)  (((m) & (S_IFMT))  == S_IFREG)
#define S_ISDIR(m)  (((m) & (S_IFDIR)) == S_IFDIR)

static ap_filetype_e filetype_from_mode(int mode)
{
    ap_filetype_e type = APR_NOFILE;

    if (S_ISREG(mode))
        type = APR_REG;
    if (S_ISDIR(mode))
        type = APR_DIR;
    if (S_ISLNK(mode))
        type = APR_LNK;

    return type;
}
BOOLEAN is_exe(const char* fname, apr_pool_t *cont) {
    const char* exename;
    const char* ext;
    exename = strrchr(fname, '/');
    if (!exename) {
        exename = strrchr(fname, '\\');
    }
    if (!exename) {
        exename = fname;
    }
    else {
        exename++;
    }
    ext = strrchr(exename, '.');

    if (ext && (!strcasecmp(ext,".exe") || !strcasecmp(ext,".com") || 
                !strcasecmp(ext,".bat") || !strcasecmp(ext,".cmd"))) {
        return TRUE;
    }
    return FALSE;
}

apr_status_t apr_getfileinfo(apr_finfo_t *finfo, apr_file_t *thefile)
{
    BY_HANDLE_FILE_INFORMATION FileInformation;
    DWORD FileType;

    if (!GetFileInformationByHandle(thefile->filehand, &FileInformation)) {
        return GetLastError();
    }

    FileType = GetFileType(thefile->filehand);
    if (!FileType) {
        return GetLastError();
    }

    /* If my rudimentary knowledge of posix serves... inode is the absolute
     * id of the file (uniquifier) that is returned by NT as follows:
     * user and group could be related as SID's, although this would ensure
     * it's own unique set of issues.  All three fields are significantly
     * longer than the posix compatible kernals would ever require.
     * TODO: Someday solve this, and fix the executable flag below the
     * right way with a security permission test (as well as r/w flags.)
     *
     *     dwVolumeSerialNumber
     *     nFileIndexHigh
     *     nFileIndexLow
     */
    finfo->user = 0;
    finfo->group = 0;
    finfo->inode = 0;
    finfo->device = 0;  /* ### use drive letter - 'A' ? */

    /* Filetype - Directory or file: this case _will_ never happen */
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
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
        /* obscure ommission in msvc... missing declaration sans underscore */
#ifdef _MSC_VER
        finfo->protection = _S_IFIFO;
#else
        finfo->protection = S_IFIFO;
#endif
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

apr_status_t apr_setfileperms(const char *fname, apr_fileperms_t perms)
{
    return APR_ENOTIMPL;
}

apr_status_t apr_stat(apr_finfo_t *finfo, const char *fname, apr_pool_t *cont)
{
    /* WIN32_FILE_ATTRIBUTE_DATA is an exact subset of the first 
     * entries of WIN32_FIND_DATA
     */
    WIN32_FIND_DATA FileInformation;
    HANDLE hFind;
    ap_oslevel_e os_level;
    apr_status_t rv = APR_SUCCESS;

    memset(finfo,'\0', sizeof(*finfo));

	/* We need to catch the case where fname length == MAX_PATH since for
	 * some strange reason GetFileAttributesEx fails with PATH_NOT_FOUND.
	 * We would rather indicate length error than 'not found'
	 * since in many cases the apr user is testing for 'not found' 
	 * and this is not such a case.
	 */
    if (strlen(fname) >= MAX_PATH) {
        rv = ERROR_FILENAME_EXCED_RANGE;
    }
    else if (!ap_get_oslevel(cont, &os_level) && os_level >= APR_WIN_98) {
        if (!GetFileAttributesEx(fname, GetFileExInfoStandard, 
                                 (WIN32_FILE_ATTRIBUTE_DATA*) &FileInformation)) {
            rv = GetLastError();
        }
    }
    else {
        /*  The question remains, can we assume fname is not a wildcard?
         *  Must we test it?
         */
        hFind = FindFirstFile(fname, &FileInformation);
        if (hFind == INVALID_HANDLE_VALUE) {
            rv = GetLastError();
    	} else {
            FindClose(hFind);
        }
    }

    if (rv != APR_SUCCESS) {
        /* a little ad-hoc canonicalization to the most common
         * error conditions
         */
        if (rv == ERROR_FILE_NOT_FOUND || rv == ERROR_PATH_NOT_FOUND)
            return APR_ENOENT;
       return rv;
    }

    /* Filetype - Directory or file?
     * Short of opening the handle to the file, the 'FileType' appears
     * to be unknowable (in any trustworthy or consistent sense.)
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

    /* Is this an executable? Guess based on the file extension. 
     * This is a rather silly test, IMHO... we are looking to test
     * if the user 'may' execute a file (permissions), 
     * not if the filetype is executable.
     */
/*  if (is_exe(fname, cont)) {
 *       finfo->protection |= S_IEXEC;
 *  }
 */
    
    /* File times */
    FileTimeToAprTime(&finfo->atime, &FileInformation.ftLastAccessTime);
    FileTimeToAprTime(&finfo->ctime, &FileInformation.ftCreationTime);
    FileTimeToAprTime(&finfo->mtime, &FileInformation.ftLastWriteTime);

    /* File size 
     * Note: This cannot handle files greater than can be held by an int */
    finfo->size = FileInformation.nFileSizeLow;

    return APR_SUCCESS;
}

