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

#include "apr_config.h"
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_errno.h"
#include "apr_time.h"
#include <sys/stat.h>
#include "atime.h"

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
BOOLEAN is_exe(const char* fname, ap_context_t *cont) {
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

ap_status_t ap_getfileinfo(ap_finfo_t *finfo, ap_file_t *thefile)
{
    /* TODO: 
     * Windows should call GetFileInformationByHandle(), which is similar 
     * to fstat(), for the best performance. Then we would need to map the 
     * BY_HANDLE_FILE_INFORMATION to ap_finfo_t. 
     */
    struct stat info;
    int rv = stat(thefile->fname, &info);

    if (rv == 0) {
        finfo->protection = info.st_mode;
        finfo->filetype = filetype_from_mode(info.st_mode);
        finfo->user = info.st_uid;
        finfo->group = info.st_gid;
        finfo->size = info.st_size;
        finfo->inode = info.st_ino;
        ap_ansi_time_to_ap_time(&finfo->atime, info.st_atime);
        ap_ansi_time_to_ap_time(&finfo->mtime, info.st_mtime);
        ap_ansi_time_to_ap_time(&finfo->ctime, info.st_ctime);
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
}
ap_status_t ap_stat(ap_finfo_t *finfo, const char *fname, ap_context_t *cont)
{
    /* WIN32_FILE_ATTRIBUTE_DATA is an exact subset of the first 
     * entries of WIN32_FIND_DATA
     */
    WIN32_FIND_DATA FileInformation;
    HANDLE hFind;
    ap_oslevel_e os_level;

    memset(finfo,'\0', sizeof(*finfo));

    if (!ap_get_oslevel(cont, &os_level) && os_level >= APR_WIN_98) {
        if (!GetFileAttributesEx(fname, GetFileExInfoStandard, 
                                 (WIN32_FILE_ATTRIBUTE_DATA*) &FileInformation)) {
            return GetLastError();
        }
    }
    else {
        /*  The question remains, can we assume fname is not a wildcard?
         *  Must we test it?
         */
        hFind = FindFirstFile(fname, &FileInformation);
        if (hFind == INVALID_HANDLE_VALUE) {
            return GetLastError();
    	}
        FindClose(hFind);
    }
    /* Filetype - Directory or file? */
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        finfo->protection |= S_IFDIR;
        finfo->filetype = APR_DIR;
    }
    else {
        finfo->protection |= S_IFREG;
        finfo->filetype = APR_REG;
    }
    /* Read, write execute for owner */
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
        finfo->protection |= S_IREAD;
    }
    else {
        finfo->protection |= S_IREAD;
        finfo->protection |= S_IWRITE;
    }
    /* Is this an executable? Guess based on the file extension. */
    if (is_exe(fname, cont)) {
        finfo->protection |= S_IEXEC;
    }
    /* File times */
    FileTimeToAprTime(&finfo->atime, &FileInformation.ftLastAccessTime);
    FileTimeToAprTime(&finfo->ctime, &FileInformation.ftCreationTime);
    FileTimeToAprTime(&finfo->mtime, &FileInformation.ftLastWriteTime);

    /* File size 
     * Note: This cannot handle files greater than can be held by an int */
    finfo->size = FileInformation.nFileSizeLow;

    return APR_SUCCESS;
#if 0
    /* ap_stat implemented using stat() */
    struct stat info;
    int rv = stat(fname, &info);
    if (rv == 0) {
        finfo->protection = info.st_mode;
        finfo->filetype = filetype_from_mode(info.st_mode);
        finfo->user = info.st_uid;
        finfo->group = info.st_gid;
        finfo->size = info.st_size;
        finfo->inode = info.st_ino;
        ap_ansi_time_to_ap_time(&finfo->atime, info.st_atime);
        ap_ansi_time_to_ap_time(&finfo->mtime, info.st_mtime);
        ap_ansi_time_to_ap_time(&finfo->ctime, info.st_ctime);
        return APR_SUCCESS;
    }
    else {
        return errno;
    }
#endif
}

