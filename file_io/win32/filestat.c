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

static apr_gid_t worldid = NULL;

static void free_world(void)
{
    if (worldid) {
        FreeSid(worldid);
        worldid = NULL;
    }
}

/* Left bit shifts from World scope to given scope */
typedef enum prot_scope_e {
    prot_scope_world = 0,
    prot_scope_group = 4,
    prot_scope_user =  8
} prot_scope_e;

static apr_fileperms_t convert_prot(ACCESS_MASK acc, prot_scope_e scope)
{
    /* These choices are based on the single filesystem bit that controls
     * the given behavior.  They are -not- recommended for any set protection
     * function, such a function should -set- use GENERIC_READ/WRITE/EXECUTE
     */
    apr_fileperms_t prot;
    if (acc & FILE_EXECUTE)
        prot |= APR_WEXECUTE;
    if (acc & FILE_WRITE_DATA)
        prot |= APR_WWRITE;
    if (acc & FILE_READ_DATA)
        prot |= APR_WREAD;
    return (prot << scope);
}

static void resolve_prot(apr_finfo_t *finfo, apr_int32_t wanted, PACL dacl)
{
    TRUSTEE ident = {NULL, NO_MULTIPLE_TRUSTEE, TRUSTEE_IS_SID};
    ACCESS_MASK acc;
    if ((wanted & APR_FINFO_WPROT) && !worldid) {
        SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_WORLD_SID_AUTHORITY;
        if (AllocateAndInitializeSid(&SIDAuth, 1, SECURITY_WORLD_RID,
                                     0, 0, 0, 0, 0, 0, 0, &worldid))
            atexit(free_world);
        else
            worldid = NULL;
    }
    if ((wanted & APR_FINFO_UPROT) && (finfo->valid & APR_FINFO_USER)) {
        ident.TrusteeType = TRUSTEE_IS_USER;
        ident.ptstrName = finfo->user;
        if (GetEffectiveRightsFromAcl(dacl, &ident, &acc) == ERROR_SUCCESS) {
            finfo->protection |= convert_prot(acc, prot_scope_user);
            finfo->valid |= APR_FINFO_UPROT;
        }
    }
    if ((wanted & APR_FINFO_GPROT) && (finfo->valid & APR_FINFO_GROUP)) {
        ident.TrusteeType = TRUSTEE_IS_GROUP;
        ident.ptstrName = finfo->group;
        if (GetEffectiveRightsFromAcl(dacl, &ident, &acc) == ERROR_SUCCESS) {
            finfo->protection |= convert_prot(acc, prot_scope_group);
            finfo->valid |= APR_FINFO_GPROT;
        }
    }
    if ((wanted & APR_FINFO_WPROT) && (worldid)) {
        ident.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ident.ptstrName = worldid;
        if (GetEffectiveRightsFromAcl(dacl, &ident, &acc) == ERROR_SUCCESS) {
            finfo->protection |= convert_prot(acc, prot_scope_world);
            finfo->valid |= APR_FINFO_WPROT;
        }
    }
}

static int resolve_ident(apr_finfo_t *finfo, const char *fname,
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

apr_status_t more_finfo(apr_finfo_t *finfo, const void *ufile, apr_int32_t wanted, 
                        int whatfile, apr_oslevel_e os_level)
{
    PSID user = NULL, grp = NULL;
    PACL dacl = NULL;
    apr_status_t rv;

    if (whatfile == MORE_OF_WFSPEC)
        (apr_wchar_t*)ufile;
    else if (whatfile == MORE_OF_FSPEC)
        (char*)ufile;
    else if (whatfile == MORE_OF_HANDLE)
        (HANDLE)ufile;

    if ((wanted & (APR_FINFO_PROT | APR_FINFO_OWNER))
            && os_level >= APR_WIN_NT)
    {
        SECURITY_INFORMATION sinf = 0;
        PSECURITY_DESCRIPTOR pdesc = NULL;
        if (wanted & (APR_FINFO_USER | APR_FINFO_UPROT))
            sinf |= OWNER_SECURITY_INFORMATION;
        if (wanted & (APR_FINFO_GROUP | APR_FINFO_GPROT))
            sinf |= GROUP_SECURITY_INFORMATION;
        if (wanted & APR_FINFO_PROT)
            sinf |= DACL_SECURITY_INFORMATION;
        if (whatfile == MORE_OF_WFSPEC)
            rv = GetNamedSecurityInfoW((apr_wchar_t*)ufile, 
                                 SE_FILE_OBJECT, sinf,
                                 ((wanted & APR_FINFO_USER) ? &user : NULL),
                                 ((wanted & APR_FINFO_GROUP) ? &grp : NULL),
                                 ((wanted & APR_FINFO_PROT) ? &dacl : NULL),
                                 NULL, &pdesc);
        else if (whatfile == MORE_OF_FSPEC)
            rv = GetNamedSecurityInfoA((char*)ufile, 
                                 SE_FILE_OBJECT, sinf,
                                 ((wanted & APR_FINFO_USER) ? &user : NULL),
                                 ((wanted & APR_FINFO_GROUP) ? &grp : NULL),
                                 ((wanted & APR_FINFO_PROT) ? &dacl : NULL),
                                 NULL, &pdesc);
        else if (whatfile == MORE_OF_HANDLE)
            rv = GetSecurityInfo((HANDLE)ufile, 
                                 SE_FILE_OBJECT, sinf,
                                 ((wanted & APR_FINFO_USER) ? &user : NULL),
                                 ((wanted & APR_FINFO_GROUP) ? &grp : NULL),
                                 ((wanted & APR_FINFO_PROT) ? &dacl : NULL),
                                 NULL, &pdesc);
        if (rv == ERROR_SUCCESS)
            apr_register_cleanup(finfo->cntxt, pdesc, free_localheap, 
                                 apr_null_cleanup);
        else
            user = grp = dacl = NULL;

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
            resolve_prot(finfo, wanted, dacl);
        }
    }

    if (!(finfo->valid & APR_FINFO_UPROT)) {
        /* Read, write execute for owner.  In the Win32 environment, 
         * anything readable is executable (well, not entirely 100% true, 
         * but I'm looking for some obvious logic that would help us here.)
         */
        if (finfo->protection & APR_FREADONLY) {
            finfo->protection |= S_IREAD | S_IEXEC;
        }
        else {
            finfo->protection |= S_IREAD | S_IWRITE | S_IEXEC;
        }
        finfo->valid |= APR_FINFO_UPROT;
    }    

    return ((wanted & ~finfo->valid) ? APR_INCOMPLETE : APR_SUCCESS);
}


APR_DECLARE(apr_status_t) apr_getfileinfo(apr_finfo_t *finfo, apr_int32_t wanted,
                                          apr_file_t *thefile)
{
    BY_HANDLE_FILE_INFORMATION FileInformation;
    apr_oslevel_e os_level;

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
    
    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        finfo->protection = APR_FREADONLY;

    if (wanted &= ~finfo->valid)
        return more_finfo(finfo, thefile->filehand, wanted, MORE_OF_HANDLE, os_level);

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
#ifdef APR_HAS_UNICODE_FS
    apr_wchar_t wfname[APR_PATH_MAX];
#endif
    /*
     * The WIN32_FILE_ATTRIBUTE_DATA is a subset of this structure
     */
    WIN32_FIND_DATA FileInformation;
    apr_oslevel_e os_level;

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
    if (os_level >= APR_WIN_NT) {
        apr_status_t rv;
        if (rv = utf8_to_unicode_path(wfname, sizeof(wfname) 
                                            / sizeof(apr_wchar_t), fname))
            return rv;
        if (!GetFileAttributesExW(wfname, GetFileExInfoStandard, 
                                  &FileInformation)) {
            return apr_get_os_error();
        }
    }
    else 
#endif
      if (os_level >= APR_WIN_98) {
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

    if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        finfo->protection = APR_FREADONLY;

    if (wanted &= ~finfo->valid) {
#ifdef APR_HAS_UNICODE_FS
        if (os_level >= APR_WIN_NT)
            return more_finfo(finfo, wfname, wanted, MORE_OF_WFSPEC, os_level);
#endif
        return more_finfo(finfo, fname, wanted, MORE_OF_FSPEC, os_level);
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_lstat(apr_finfo_t *finfo, const char *fname,
                                    apr_int32_t wanted, apr_pool_t *cont)
{
    return apr_stat(finfo, fname, wanted & APR_FINFO_LINK, cont);
}
