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

#ifndef APR_FILE_INFO_H
#define APR_FILE_INFO_H

#include "apr.h"
#include "apr_pools.h"
#include "apr_time.h"
#include "apr_errno.h"

#if APR_HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @package APR File handling
 */

typedef enum {
    APR_NOFILE,         /* the file exists, but APR doesn't know its type */
    APR_REG,            /* a regular file */
    APR_DIR,            /* a directory */
    APR_CHR,            /* a character device */
    APR_BLK,            /* a block device */
    APR_PIPE,           /* a FIFO / pipe */
    APR_LNK,            /* a symbolic link */
    APR_SOCK            /* a [unix domain] socket */
} apr_filetype_e; 

/* Permissions flags */
#define APR_UREAD     0x400 
#define APR_UWRITE    0x200
#define APR_UEXECUTE  0x100

#define APR_GREAD     0x040
#define APR_GWRITE    0x020
#define APR_GEXECUTE  0x010

#define APR_WREAD     0x004
#define APR_WWRITE    0x002
#define APR_WEXECUTE  0x001

#define APR_OS_DEFAULT 0xFFF

/**
 * Structure for referencing directories.
 * @defvar apr_dir_t
 */
typedef struct apr_dir_t          apr_dir_t;
/**
 * Structure for determining file permissions.
 * @defvar apr_fileperms_t
 */
typedef apr_int32_t               apr_fileperms_t;
/**
 * Structure for determining file owner.
 * @defvar apr_uid_t
 */
typedef uid_t                     apr_uid_t;
/**
 * Structure for determining the group that owns the file.
 * @defvar apr_gid_t
 */
typedef gid_t                     apr_gid_t;
#ifdef WIN32
/**
 * Structure for determining the inode of the file.
 * @defvar apr_ino_t
 */
typedef apr_uint64_t              apr_ino_t;
/**
 * Structure for determining the device the file is on.
 * @defvar apr_dev_t
 */
typedef apr_uint32_t              apr_dev_t;
#else
typedef ino_t                     apr_ino_t;
typedef dev_t                     apr_dev_t;
#endif

typedef struct apr_finfo_t        apr_finfo_t;

/**
 * The file information structure.  This is analogous to the POSIX
 * stat structure.
 */
struct apr_finfo_t {
    /** The access permissions of the file.  Currently this mimics Unix
     *  access rights.
     */
    apr_fileperms_t protection;
    /** The type of file.  One of APR_NOFILE, APR_REG, APR_DIR, APR_CHR, 
     *  APR_BLK, APR_PIPE, APR_LNK, APR_SOCK 
     */
    apr_filetype_e filetype;
    /** The user id that owns the file */
    apr_uid_t user;
    /** The group id that owns the file */
    apr_gid_t group;
    /** The inode of the file.  (Not portable?) */
    apr_ino_t inode;
    /** The id of the device the file is on.  (Not portable?) */
    apr_dev_t device;
    /** The size of the file */
    apr_off_t size;
    /** The time the file was last accessed */
    apr_time_t atime;
    /** The time the file was last modified */
    apr_time_t mtime;
    /** The time the file was last changed */
    apr_time_t ctime;
};

/**
 * get the specified file's stats.  The file is specified by filename, 
 * instead of using a pre-opened file.
 * @param finfo Where to store the information about the file, which is
 * never touched if the call fails.
 * @param fname The name of the file to stat.
 * @param cont the pool to use to allocate the new file. 
 * @deffunc apr_status_t apr_stat(apr_finfo_t *finfo, const char *fname, apr_pool_t *cont)
 */ 
APR_DECLARE(apr_status_t) apr_stat(apr_finfo_t *finfo, const char *fname,
                                   apr_pool_t *cont);

/**
 * get the specified file's stats.  The file is specified by filename, 
 * instead of using a pre-opened file.  If the file is a symlink, this function
 * will get the stats for the symlink not the file the symlink refers to.
 * @param finfo Where to store the information about the file, which is
 * never touched if the call fails.
 * @param fname The name of the file to stat.
 * @param cont the pool to use to allocate the new file. 
 * @deffunc apr_status_t apr_lstat(apr_finfo_t *finfo, const char *fname, apr_pool_t *cont)
 */ 
APR_DECLARE(apr_status_t) apr_lstat(apr_finfo_t *finfo, const char *fname,
                                    apr_pool_t *cont);

/**
 * Open the specified directory.
 * @param new_dir The opened directory descriptor.
 * @param dirname The full path to the directory (use / on all systems)
 * @param cont The pool to use.
 * @deffunc apr_status_t apr_dir_open(apr_dir_t **new_dir, const char *dirname, apr_pool_t *cont)
 */                        
APR_DECLARE(apr_status_t) apr_dir_open(apr_dir_t **new_dir, 
                                       const char *dirname, 
                                       apr_pool_t *cont);

/**
 * close the specified directory. 
 * @param thedir the directory descriptor to close.
 * @deffunc apr_status_t apr_closedir(apr_dir_t *thedir)
 */                        
APR_DECLARE(apr_status_t) apr_closedir(apr_dir_t *thedir);

/**
 * Read the next entry from the specified directory. 
 * @param thedir the directory descriptor to read from, and fill out.
 * @tip All systems return . and .. as the first two files.
 * @deffunc apr_status_t apr_readdir(apr_dir_t *thedir)
 */                        
APR_DECLARE(apr_status_t) apr_readdir(apr_dir_t *thedir);

/**
 * Rewind the directory to the first entry.
 * @param thedir the directory descriptor to rewind.
 * @deffunc apr_status_t apr_rewinddir(apr_dir_t *thedir)
 */                        
APR_DECLARE(apr_status_t) apr_rewinddir(apr_dir_t *thedir);

/**
 * Get the file name of the current directory entry.
 * @param new_path the file name of the directory entry. 
 * @param thedir the currently open directory.
 * @deffunc apr_status_t apr_get_dir_filename(const char **new_path, apr_dir_t *thedir)
 */                        
APR_DECLARE(apr_status_t) apr_get_dir_filename(const char **new_path, 
                                               apr_dir_t *thedir);

/**
 * Get the size of the current directory entry.
 * @param size the size of the directory entry. 
 * @param thedir the currently open directory.
 * @deffunc apr_status_t apr_dir_entry_size(apr_size_t *size, apr_dir_t *thedir)
 */                        
APR_DECLARE(apr_status_t) apr_dir_entry_size(apr_size_t *size, 
                                             apr_dir_t *thedir);

/**
 * Get the last modified time of the current directory entry.
 * @param mtime the last modified time of the directory entry. 
 * @param thedir the currently open directory.
 * @deffunc apr_status_t apr_dir_entry_mtime(apr_time_t *mtime, apr_dir_t *thedir)
 */ 
APR_DECLARE(apr_status_t) apr_dir_entry_mtime(apr_time_t *mtime, 
                                              apr_dir_t *thedir);

/**
 * Get the file type of the current directory entry.
 * @param type the file type of the directory entry. 
 * @param thedir the currently open directory.
 * @deffunc apr_status_t apr_dir_entry_ftype(apr_filetype_e *type, apr_dir_t *thedir)
 */
APR_DECLARE(apr_status_t) apr_dir_entry_ftype(apr_filetype_e *type, 
                                              apr_dir_t *thedir);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_FILE_INFO_H */


