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

#include <windows.h>
#include <aclapi.h>
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
    apr_fileperms_t prot = 0;
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

static apr_status_t resolve_ident(apr_finfo_t *finfo, const char *fname,
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
    
    if ((rv = apr_file_open(&thefile, fname, 
                       ((wanted & APR_FINFO_LINK) ? APR_OPENLINK : 0)
                     | ((wanted & (APR_FINFO_PROT | APR_FINFO_OWNER))
                           ? APR_READCONTROL : 0),
                       APR_OS_DEFAULT, cont)) == APR_SUCCESS) {
        rv = apr_file_info_get(finfo, wanted, thefile);
        finfo->filehand = NULL;
        apr_file_close(thefile);
    }
    else if (APR_STATUS_IS_EACCES(rv) && (wanted & (APR_FINFO_PROT 
                                                  | APR_FINFO_OWNER))) {
        /* We have a backup plan.  Perhaps we couldn't grab READ_CONTROL?
         * proceed without asking for that permission...
         */
        if ((rv = apr_file_open(&thefile, fname, 
                           ((wanted & APR_FINFO_LINK) ? APR_OPENLINK : 0),
                           APR_OS_DEFAULT, cont)) == APR_SUCCESS) {
            rv = apr_file_info_get(finfo, wanted & ~(APR_FINFO_PROT 
                                                 | APR_FINFO_OWNER),
                                 thefile);
            finfo->filehand = NULL;
            apr_file_close(thefile);
        }
    }

    if (rv != APR_SUCCESS && rv != APR_INCOMPLETE)
        return (rv);

    /* We picked up this case above and had opened the link's properties */
    if (wanted & APR_FINFO_LINK)
        finfo->valid |= APR_FINFO_LINK;

    return rv;
}

apr_status_t more_finfo(apr_finfo_t *finfo, const void *ufile, apr_int32_t wanted, 
                        int whatfile, apr_oslevel_e os_level)
{
    PSID user = NULL, grp = NULL;
    PACL dacl = NULL;
    apr_status_t rv;

    if (os_level < APR_WIN_NT) 
    {
        /* Read, write execute for owner.  In the Win9x environment, any
         * readable file is executable (well, not entirely 100% true, but
         * still looking for some cheap logic that would help us here.)
         */
        if (finfo->protection & APR_FREADONLY) {
            finfo->protection |= APR_WREAD | APR_WEXECUTE;
        }
        else {
            finfo->protection |= APR_WREAD | APR_WEXECUTE | APR_WWRITE;
        }
        finfo->protection |= (finfo->protection << prot_scope_group) 
                           | (finfo->protection << prot_scope_user);

        finfo->valid |= APR_FINFO_UPROT | APR_FINFO_GPROT | APR_FINFO_WPROT;
    }    
    else if (wanted & (APR_FINFO_PROT | APR_FINFO_OWNER))
    {
        /* On NT this request is incredibly expensive, but accurate.
         */
        SECURITY_INFORMATION sinf = 0;
        PSECURITY_DESCRIPTOR pdesc = NULL;
        if (wanted & (APR_FINFO_USER | APR_FINFO_UPROT))
            sinf |= OWNER_SECURITY_INFORMATION;
        if (wanted & (APR_FINFO_GROUP | APR_FINFO_GPROT))
            sinf |= GROUP_SECURITY_INFORMATION;
        if (wanted & APR_FINFO_PROT)
            sinf |= DACL_SECURITY_INFORMATION;
        if (whatfile == MORE_OF_WFSPEC) {
            apr_wchar_t *wfile = (apr_wchar_t*) ufile;
            int fix = 0;
            if (wcsncmp(wfile, L"\\\\?\\", 4)) {
                fix = 4;
                if (wcsncmp(wfile + fix, L"UNC\\", 4))
                    wfile[6] = L'\\', fix = 6;
            }
            rv = GetNamedSecurityInfoW(wfile + fix, 
                                 SE_FILE_OBJECT, sinf,
                                 ((wanted & APR_FINFO_USER) ? &user : NULL),
                                 ((wanted & APR_FINFO_GROUP) ? &grp : NULL),
                                 ((wanted & APR_FINFO_PROT) ? &dacl : NULL),
                                 NULL, &pdesc);
            if (fix == 6)
                wfile[6] = L'C';
        }
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
        else
            return APR_INCOMPLETE;
        if (rv == ERROR_SUCCESS)
            apr_pool_cleanup_register(finfo->cntxt, pdesc, free_localheap, 
                                 apr_pool_cleanup_null);
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

    return ((wanted & ~finfo->valid) ? APR_INCOMPLETE : APR_SUCCESS);
}


/* This generic fillin depends upon byhandle to be passed as 0 when
 * a WIN32_FILE_ATTRIBUTE_DATA or either WIN32_FIND_DATA [A or W] is
 * passed for wininfo.  When the BY_HANDLE_FILE_INFORMATION structure
 * is passed for wininfo, byhandle is passed as 1 to offset the one
 * dword discrepancy in offset of the High/Low size structure members.
 *
 * The generic fillin returns 1 if the caller should further inquire
 * if this is a CHR filetype.  If it's reasonably certain it can't be,
 * then the function returns 0.
 */
int fillin_fileinfo(apr_finfo_t *finfo, 
                    WIN32_FILE_ATTRIBUTE_DATA *wininfo, 
                    int byhandle, apr_int32_t wanted) 
{
    DWORD *sizes = &wininfo->nFileSizeHigh + byhandle;
    int warn = 0;

    memset(finfo, '\0', sizeof(*finfo));

    FileTimeToAprTime(&finfo->atime, &wininfo->ftLastAccessTime);
    FileTimeToAprTime(&finfo->ctime, &wininfo->ftCreationTime);
    FileTimeToAprTime(&finfo->mtime, &wininfo->ftLastWriteTime);

#if APR_HAS_LARGE_FILES
    finfo->size =  (apr_off_t)sizes[1]
                | ((apr_off_t)sizes[0] << 32);
#else
    finfo->size = (apr_off_t)sizes[1];
    if (finfo->size < 0 || sizes[0])
        finfo->size = 0x7fffffff;
#endif

    if (wininfo->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        finfo->filetype = APR_LNK;
    }
    else if (wininfo->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        finfo->filetype = APR_DIR;
    }
    else if (wininfo->dwFileAttributes & FILE_ATTRIBUTE_DEVICE) {
        /* Warning: This test only succeeds on Win9x, on NT these files
         * (con, aux, nul, lpt#, com# etc) escape early detection!
         */
        finfo->filetype = APR_CHR;
    }
    else {
        /* Warning: Short of opening the handle to the file, the 'FileType'
         * appears to be unknowable (in any trustworthy or consistent sense)
         * on WinNT/2K as far as PIPE, CHR, etc are concerned.
         */
        if (!wininfo->ftLastWriteTime.dwLowDateTime 
                && !wininfo->ftLastWriteTime.dwHighDateTime 
                && !finfo->size)
            warn = 1;
        finfo->filetype = APR_REG;
    }

    /* The following flags are [for this moment] private to Win32.
     * That's the only excuse for not toggling valid bits to reflect them.
     */
    if (wininfo->dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        finfo->protection = APR_FREADONLY;
    
    finfo->valid = APR_FINFO_ATIME | APR_FINFO_CTIME | APR_FINFO_MTIME
                 | APR_FINFO_SIZE  | APR_FINFO_TYPE;   /* == APR_FINFO_MIN */

    /* Only byhandle optionally tests link targets, so tell that caller
     * what it wants to hear, otherwise the byattributes is never
     * reporting anything but the link.
     */
    if (!byhandle || (wanted & APR_FINFO_LINK))
        finfo->valid |= APR_FINFO_LINK;
    return warn;
}


APR_DECLARE(apr_status_t) apr_file_info_get(apr_finfo_t *finfo, apr_int32_t wanted,
                                            apr_file_t *thefile)
{
    BY_HANDLE_FILE_INFORMATION FileInfo;

    if (!GetFileInformationByHandle(thefile->filehand, &FileInfo)) {
        return apr_get_os_error();
    }

    fillin_fileinfo(finfo, (WIN32_FILE_ATTRIBUTE_DATA *) &FileInfo, 1, wanted);

    if (finfo->filetype == APR_REG)
    {
        /* Go the extra mile to be -certain- that we have a real, regular
         * file, since the attribute bits aren't a certain thing.  Even
         * though fillin should have hinted if we *must* do this, we
         * don't need to take chances while the handle is already open.
         */
        DWORD FileType;
        if (FileType = GetFileType(thefile->filehand)) {
            if (FileType == FILE_TYPE_CHAR) {
                finfo->filetype = APR_CHR;
            }
            else if (FileType == FILE_TYPE_PIPE) {
                finfo->filetype = APR_PIPE;
            }
            /* Otherwise leave the original conclusion alone 
             */
        }
    }

    finfo->cntxt = thefile->cntxt;
 
    /* Extra goodies known only by GetFileInformationByHandle() */
    finfo->inode  =  (apr_ino_t)FileInfo.nFileIndexLow
                  | ((apr_ino_t)FileInfo.nFileIndexHigh << 32);
    finfo->device = FileInfo.dwVolumeSerialNumber;
    finfo->nlink  = FileInfo.nNumberOfLinks;

    finfo->valid |= APR_FINFO_IDENT | APR_FINFO_NLINK;

    /* If we still want something more (besides the name) go get it! 
     */
    if ((wanted &= ~finfo->valid) & ~APR_FINFO_NAME) {
        apr_oslevel_e os_level;
        if (apr_get_oslevel(thefile->cntxt, &os_level))
            os_level = APR_WIN_95;
        return more_finfo(finfo, thefile->filehand, wanted, MORE_OF_HANDLE, os_level);
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_perms_set(const char *fname,
                                           apr_fileperms_t perms)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_stat(apr_finfo_t *finfo, const char *fname,
                                   apr_int32_t wanted, apr_pool_t *cont)
{
    /* XXX: is constant - needs testing - which requires a lighter-weight root test fn */
    int isroot = 0;
    apr_status_t ident_rv = 0;
    apr_status_t rv;
#if APR_HAS_UNICODE_FS
    apr_wchar_t wfname[APR_PATH_MAX];

#endif
    apr_oslevel_e os_level;
    char *filename = NULL;
    /* These all share a common subset of this structure */
    union {
        WIN32_FIND_DATAW w;
        WIN32_FIND_DATAA n;
        WIN32_FILE_ATTRIBUTE_DATA i;
    } FileInfo;
    
    if (apr_get_oslevel(cont, &os_level))
        os_level = APR_WIN_95;
    
    /* Catch fname length == MAX_PATH since GetFileAttributesEx fails 
     * with PATH_NOT_FOUND.  We would rather indicate length error than 
     * 'not found'
     */        
    if (strlen(fname) >= APR_PATH_MAX) {
        return APR_ENAMETOOLONG;
    }

    if ((os_level >= APR_WIN_NT) 
            && (wanted & (APR_FINFO_IDENT | APR_FINFO_NLINK))) {
        /* FindFirstFile and GetFileAttributesEx can't figure the inode,
         * device or number of links, so we need to resolve with an open 
         * file handle.  If the user has asked for these fields, fall over 
         * to the get file info by handle method.  If we fail, or the user
         * also asks for the file name, continue by our usual means.
         */
        if ((ident_rv = resolve_ident(finfo, fname, wanted, cont)) 
                == APR_SUCCESS)
            return ident_rv;
        else if (ident_rv == APR_INCOMPLETE)
            wanted &= ~finfo->valid;
    }

#if APR_HAS_UNICODE_FS
    if (os_level >= APR_WIN_NT) {
        if (rv = utf8_to_unicode_path(wfname, sizeof(wfname) 
                                            / sizeof(apr_wchar_t), fname))
            return rv;
        if (!(wanted & APR_FINFO_NAME)) {
            if (!GetFileAttributesExW(wfname, GetFileExInfoStandard, 
                                      &FileInfo.i))
                return apr_get_os_error();
        }
        else {
            /* Guard against bogus wildcards and retrieve by name
             * since we want the true name, and set aside a long
             * enough string to handle the longest file name.
             */
            char tmpname[APR_FILE_MAX * 3 + 1];
            HANDLE hFind;
            if (strchr(fname, '*') || strchr(fname, '?'))
                return APR_ENOENT;
            hFind = FindFirstFileW(wfname, &FileInfo.w);
            if (hFind == INVALID_HANDLE_VALUE)
                return apr_get_os_error();
            FindClose(hFind);
            if (unicode_to_utf8_path(tmpname, sizeof(tmpname), 
                                     FileInfo.w.cFileName)) {
                return APR_ENAMETOOLONG;
            }
            filename = apr_pstrdup(cont, tmpname);
        }
    }
    else
#endif
      if ((os_level >= APR_WIN_98) && (!(wanted & APR_FINFO_NAME) || isroot))
    {
        /* cannot use FindFile on a Win98 root, it returns \*
         */
        if (!GetFileAttributesExA(fname, GetFileExInfoStandard, 
                                 &FileInfo.i)) {
            return apr_get_os_error();
        }
    }
    else if (isroot) {
        /* This is Win95 and we are trying to stat a root.  Lie.
         */
        if (GetDriveType(fname) != DRIVE_UNKNOWN) 
        {
            finfo->cntxt = cont;
            finfo->filetype = 0;
            finfo->mtime = apr_time_now();
            finfo->protection |= APR_WREAD | APR_WEXECUTE | APR_WWRITE;
            finfo->protection |= (finfo->protection << prot_scope_group) 
                               | (finfo->protection << prot_scope_user);
            finfo->valid |= APR_FINFO_TYPE | APR_FINFO_PROT | APR_FINFO_MTIME
                         | (wanted & APR_FINFO_LINK);
            return (wanted &= ~finfo->valid) ? APR_INCOMPLETE : APR_SUCCESS;            
        }
        else
            return APR_FROM_OS_ERROR(ERROR_PATH_NOT_FOUND);
    }
    else  {
        /* Guard against bogus wildcards and retrieve by name
         * since we want the true name, or are stuck in Win95,
         * or are looking for the root of a Win98 drive.
         */
        HANDLE hFind;
        if (strchr(fname, '*') || strchr(fname, '?'))
            return APR_ENOENT;
        hFind = FindFirstFileA(fname, &FileInfo.n);
        if (hFind == INVALID_HANDLE_VALUE) {
            return apr_get_os_error();
    	} 
        FindClose(hFind);
        filename = apr_pstrdup(cont, FileInfo.n.cFileName);
    }

    if (ident_rv != APR_INCOMPLETE) {
        if (fillin_fileinfo(finfo, (WIN32_FILE_ATTRIBUTE_DATA *) &FileInfo, 
                            0, wanted))
        {
            /* Go the extra mile to assure we have a file.  WinNT/2000 seems
             * to reliably translate char devices to the path '\\.\device'
             * so go ask for the full path.
             */
            if (os_level >= APR_WIN_NT) {
#if APR_HAS_UNICODE_FS
                apr_wchar_t tmpname[APR_FILE_MAX];
                apr_wchar_t *tmpoff;
                if (GetFullPathNameW(wfname, sizeof(tmpname) / sizeof(apr_wchar_t),
                                     tmpname, &tmpoff))
                {
                    if ((tmpoff == tmpname + 4) 
                        && !wcsncmp(tmpname, L"\\\\.\\", 4))
                        finfo->filetype = APR_CHR;
                }
#else
                char tmpname[APR_FILE_MAX];
                char *tmpoff;
                if (GetFullPathName(fname, sizeof(tmpname), tmpname, &tmpoff))
                {
                    if ((tmpoff == tmpname + 4) 
                        && !strncmp(tmpname, "\\\\.\\", 4))
                        finfo->filetype = APR_CHR;
                }
#endif
            }
        }
        finfo->cntxt = cont;
    }

    if (filename && !isroot) {
        finfo->name = filename;
        finfo->valid |= APR_FINFO_NAME;
    }

    if (wanted &= ~finfo->valid) {
        /* Caller wants more than APR_FINFO_MIN | APR_FINFO_NAME */
#if APR_HAS_UNICODE_FS
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
    return apr_stat(finfo, fname, wanted | APR_FINFO_LINK, cont);
}
