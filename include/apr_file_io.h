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

#ifndef APR_FILE_IO_H
#define APR_FILE_IO_H

#include "apr_general.h"
#include "apr_time.h"
#include "apr_errno.h"
#if APR_HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {APR_NOFILE, APR_REG, APR_DIR, APR_CHR, APR_BLK, APR_PIPE, APR_LNK, 
              APR_SOCK} ap_filetype_e; 

/* Flags for ap_open */
#define APR_READ       1           /* Open the file for reading */
#define APR_WRITE      2           /* Open the file for writing */
#define APR_CREATE     4           /* Create the file if not there */
#define APR_APPEND     8           /* Append to the end of the file */
#define APR_TRUNCATE   16          /* Open the file and truncate to 0 length */
#define APR_BINARY     32          /* Open the file in binary mode */
#define APR_EXCL       64          /* Open should fail if APR_CREATE and file
				    exists. */
#define APR_DELONCLOSE 256         /* Delete the file after close */

/* flags for ap_seek */
#define APR_SET SEEK_SET
#define APR_CUR SEEK_CUR
#define APR_END SEEK_END

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

/* should be same as whence type in lseek, POSIX defines this as int */
typedef ap_int32_t       ap_seek_where_t;

typedef struct ap_file_t            ap_file_t;
typedef struct ap_finfo_t        ap_finfo_t;
typedef struct ap_dir_t             ap_dir_t;
typedef ap_int32_t               ap_fileperms_t;
typedef uid_t                    ap_uid_t;
typedef gid_t                    ap_gid_t;
typedef ino_t                    ap_ino_t;

struct ap_finfo_t {
    ap_fileperms_t protection;
    ap_filetype_e filetype;
    ap_uid_t user;
    ap_gid_t group;
    ap_ino_t inode;
    ap_off_t size;
    ap_time_t atime;
    ap_time_t mtime;
    ap_time_t ctime;
};

/*   Function definitions */
ap_status_t ap_open(ap_file_t **new, const char *fname, ap_int32_t flag, 
                    ap_fileperms_t perm, ap_context_t *cont);
ap_status_t ap_close(ap_file_t *file);
ap_status_t ap_remove_file(char *path, ap_context_t *cont);
ap_status_t ap_eof(ap_file_t *fptr);
ap_status_t ap_ferror(ap_file_t *fptr);
ap_status_t ap_open_stderr(ap_file_t **thefile, ap_context_t *cont);

/* ***APRDOC********************************************************
 * ap_status_t ap_read(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
 *    Read data from the specified file.
 * arg 1) The file descriptor to read from.
 * arg 2) The buffer to store the data to.
 * arg 3) On entry, the number of bytes to read; on exit, the number
 *        of bytes read.
 * NOTE:  ap_read will read up to the specified number of bytes, but never
 * more.  If there isn't enough data to fill that number of bytes, all of
 * the available data is read.  The third argument is modified to reflect the
 * number of bytes read.  If a char was put back into the stream via
 * ungetc, it will be the first character returned. 
 *
 * It is possible for both bytes to be read and an APR_EOF or other error
 * to be returned.
 *
 * APR_EINTR is never returned.
 */
ap_status_t ap_read(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes);

/* ***APRDOC********************************************************
 * ap_status_t ap_write(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
 *    Write data to the specified file.
 * arg 1) The file descriptor to write to.
 * arg 2) The buffer which contains the data.
 * arg 3) On entry, the number of bytes to write; on exit, the number
 *        of bytes write.
 * NOTE:  ap_write will write up to the specified number of bytes, but never
 * more.  If the OS cannot write that many bytes, it will write as many as it
 * can.  The third argument is modified to reflect the * number of bytes 
 * written. 
 *
 * It is possible for both bytes to be written and an error to be
 * returned.
 *
 * APR_EINTR is never returned.
 */
ap_status_t ap_write(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes);

/* ***APRDOC********************************************************
 * ap_status_t ap_writev(ap_file_t *thefile, struct iovec *vec, ap_size_t nvec,
 *                       ap_ssize_t *nbytes)
 *    Write data from iovec array to the specified file.
 * arg 1) The file descriptor to write to.
 * arg 2) The array from which to get the data to write to the file.
 * arg 3) The number of elements in the struct iovec array. This must be
 *        smaller than AP_MAX_IOVEC_SIZE.  If it isn't, the function will
 *        fail with APR_EINVAL.
 * arg 4) The number of bytes written.
 *
 * It is possible for both bytes to be written and an error to be
 * returned.
 *
 * APR_EINTR is never returned.
 *
 * ap_writev is available even if the underlying operating system
 * doesn't provide writev().
 */
ap_status_t ap_writev(ap_file_t *thefile, const struct iovec *vec, 
                      ap_size_t nvec, ap_ssize_t *nbytes);

/* ***APRDOC********************************************************
 * ap_status_t ap_putc(char ch, ap_file_t *thefile)
 *    put a character into the specified file.
 * arg 1) The character to write.
 * arg 2) The file descriptor to write to
 */
ap_status_t ap_putc(char ch, ap_file_t *thefile);

/* ***APRDOC********************************************************
 * ap_status_t ap_getc(char *ch, ap_file_t *thefil)
 *    get a character from the specified file.
 * arg 1) The character to write.
 * arg 2) The file descriptor to write to
 */
ap_status_t ap_getc(char *ch, ap_file_t *thefile);

/* ***APRDOC********************************************************
 * ap_status_t ap_ungetc(char ch, ap_file_t *thefile)
 *    put a character back onto a specified stream.
 * arg 1) The character to write.
 * arg 2) The file descriptor to write to
 */
ap_status_t ap_ungetc(char ch, ap_file_t *thefile);

/* ***APRDOC********************************************************
 * ap_status_t ap_fgets(char *str, int len, ap_file_t *thefile)
 *    Get a string from a specified file.
 * arg 1) The buffer to store the string in. 
 * arg 2) The length of the string
 * arg 3) The file descriptor to read from
 */
ap_status_t ap_fgets(char *str, int len, ap_file_t *thefile);

/* ***APRDOC********************************************************
 * ap_status_t ap_puts(char *str, ap_file_t *thefile)
 *    Put the string into a specified file.
 * arg 1) The string to write. 
 * arg 2) The file descriptor to write to from
 */
ap_status_t ap_puts(char *str, ap_file_t *thefile);

/* ***APRDOC********************************************************
 * ap_status_t ap_flush(ap_file_t *thefile)
 *    Flush the file's buffer.
 * arg 1) The file descriptor to flush
 */
ap_status_t ap_flush(ap_file_t *thefile);
API_EXPORT(int) ap_fprintf(ap_file_t *fptr, const char *format, ...)
        __attribute__((format(printf,2,3)));

ap_status_t ap_dupfile(ap_file_t **new_file, ap_file_t *old_file);
ap_status_t ap_getfileinfo(ap_finfo_t *finfo, ap_file_t *thefile);
ap_status_t ap_stat(ap_finfo_t *finfo, const char *fname, ap_context_t *cont);
ap_status_t ap_seek(ap_file_t *thefile, ap_seek_where_t where,ap_off_t *offset);

ap_status_t ap_opendir(ap_dir_t **new, const char *dirname, ap_context_t *cont);
ap_status_t ap_closedir(ap_dir_t *thedir);
ap_status_t ap_readdir(ap_dir_t *thedir);
ap_status_t ap_rewinddir(ap_dir_t *thedir);
ap_status_t ap_make_dir(const char *path, ap_fileperms_t perm, 
                        ap_context_t *cont);
ap_status_t ap_remove_dir(const char *path, ap_context_t *cont);

ap_status_t ap_create_pipe(ap_file_t **in, ap_file_t **out, ap_context_t *cont);
ap_status_t ap_create_namedpipe(char *filename, ap_fileperms_t perm, 
                                ap_context_t *cont);
ap_status_t ap_set_pipe_timeout(ap_file_t *thepipe, ap_int32_t timeout);
ap_status_t ap_block_pipe(ap_file_t *thepipe);

/*accessor and general file_io functions. */
ap_status_t ap_get_filename(char **new, ap_file_t *thefile);
ap_status_t ap_get_dir_filename(char **new, ap_dir_t *thedir);
ap_status_t ap_get_filedata(void **data, char *key, ap_file_t *file);
ap_status_t ap_set_filedata(ap_file_t *file, void *data, char *key,
                            ap_status_t (*cleanup) (void *));

ap_status_t ap_dir_entry_size(ap_ssize_t *size, ap_dir_t *thedir);
ap_status_t ap_dir_entry_mtime(ap_time_t *mtime, ap_dir_t *thedir);
ap_status_t ap_dir_entry_ftype(ap_filetype_e *type, ap_dir_t *thedir);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_FILE_IO_H */


