/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#ifndef APR_FILE_IO_H
#define APR_FILE_IO_H

#include "apr_general.h"
#include "apr_errno.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {APR_REG, APR_DIR, APR_CHR, APR_BLK, APR_PIPE, APR_LNK, 
              APR_SOCK} ap_filetype_e; 

/* Flags for ap_open */
#define APR_READ     1           /* Open the file for reading */
#define APR_WRITE    2           /* Open the file for writing */
#define APR_CREATE   4           /* Create the file if not there */
#define APR_APPEND   8           /* Append to the end of the file */
#define APR_TRUNCATE 16          /* Open the file and truncate to 0 length */
#define APR_BINARY   32          /* Open the file in binary mode */
#define APR_BUFFERED 64          /* Buffer the data when reading or writing */
#define APR_EXCL     128         /* Open should fail if APR_CREATE and file
				    exists. */

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

/* should be same as whence type in lseek, POSIZ defines this as int */
typedef ap_int32_t       ap_seek_where_t;

typedef struct file_t            ap_file_t;
typedef struct dir_t             ap_dir_t;
typedef struct iovec_t           ap_iovec_t;
typedef ap_int32_t               ap_fileperms_t;

/*   Function definitions */
ap_status_t ap_open(ap_context_t *, char *, ap_int32_t, ap_fileperms_t, ap_file_t **);
ap_status_t ap_close(ap_file_t *);
ap_status_t ap_remove_file(ap_context_t *, char *);
ap_status_t ap_eof(ap_file_t *);

ap_status_t ap_read(ap_file_t *, void *, ap_ssize_t *);
ap_status_t ap_write(ap_file_t *, void *, ap_ssize_t *);
ap_status_t ap_writev(ap_file_t *, const ap_iovec_t *, ap_ssize_t *);
ap_status_t ap_putc(ap_file_t *, char);
ap_status_t ap_getc(ap_file_t *, char *);
API_EXPORT(int) ap_fprintf(ap_file_t *fptr, const char *format, ...)
        __attribute__((format(printf,2,3)));

ap_status_t ap_dupfile(ap_file_t *, ap_file_t **);
ap_status_t ap_getfileinfo(ap_file_t *);
ap_status_t ap_seek(ap_file_t *, ap_seek_where_t, ap_off_t *);

ap_status_t ap_opendir(ap_context_t *, const char *, ap_dir_t **);
ap_status_t ap_closedir(ap_dir_t *);
ap_status_t ap_readdir(ap_dir_t *);
ap_status_t ap_rewinddir(ap_dir_t *);
ap_status_t ap_make_dir(ap_context_t *, const char *, ap_fileperms_t);
ap_status_t ap_remove_dir(ap_context_t *, const char *);

ap_status_t ap_create_pipe(ap_context_t *, ap_file_t **, ap_file_t **);
ap_status_t ap_create_namedpipe(ap_context_t *, char *, ap_fileperms_t, char **);

/*accessor and general file_io functions. */
ap_status_t ap_get_filename(ap_file_t *, char **);
ap_status_t ap_get_dir_filename(ap_dir_t *, char **);
ap_status_t ap_get_filedata(ap_file_t *, void *);
ap_status_t ap_set_filedata(ap_file_t *, void *);

ap_status_t ap_dir_entry_size(ap_dir_t *, ap_ssize_t *);
ap_status_t ap_dir_entry_mtime(ap_dir_t *, time_t *);
ap_status_t ap_dir_entry_ftype(ap_dir_t *, ap_filetype_e *);

ap_status_t ap_get_filesize(ap_file_t *, ap_ssize_t *);
ap_status_t ap_get_fileperms(ap_file_t *, ap_fileperms_t *);
ap_status_t ap_get_fileatime(ap_file_t *,time_t *);
ap_status_t ap_get_filectime(ap_file_t *,time_t *);
ap_status_t ap_get_filemtime(ap_file_t *,time_t *);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_FILE_IO_H */


