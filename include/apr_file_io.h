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

/**
 * @package APR File handling
 */

typedef enum {APR_NOFILE, APR_REG, APR_DIR, APR_CHR, APR_BLK, APR_PIPE, APR_LNK, 
              APR_SOCK} apr_filetype_e; 

/* Flags for apr_open */
#define APR_READ       1           /* Open the file for reading */
#define APR_WRITE      2           /* Open the file for writing */
#define APR_CREATE     4           /* Create the file if not there */
#define APR_APPEND     8           /* Append to the end of the file */
#define APR_TRUNCATE   16          /* Open the file and truncate to 0 length */
#define APR_BINARY     32          /* Open the file in binary mode */
#define APR_EXCL       64          /* Open should fail if APR_CREATE and file
				    exists. */
#define APR_BUFFERED   128         /* Open the file for buffered I/O */
#define APR_DELONCLOSE 256         /* Delete the file after close */

/* flags for apr_seek */
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
typedef apr_int32_t       apr_seek_where_t;

typedef struct apr_file_t         apr_file_t;
typedef struct apr_finfo_t        apr_finfo_t;
typedef struct apr_dir_t          apr_dir_t;
typedef apr_int32_t               apr_fileperms_t;
typedef uid_t                    apr_uid_t;
typedef gid_t                    apr_gid_t;
typedef ino_t                    apr_ino_t;
typedef dev_t                    apr_dev_t;

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

/*   Function definitions */

/**
 * Open the specified file.
 * @param new_file The opened file descriptor.
 * @param fname The full path to the file (using / on all systems)
 * @param flag Or'ed value of:
 * <PRE>
 *           APR_READ             open for reading
 *           APR_WRITE            open for writing
 *           APR_CREATE           create the file if not there
 *           APR_APPEND           file ptr is set to end prior to all writes
 *           APR_TRUNCATE         set length to zero if file exists
 *           APR_BINARY           not a text file (This flag is ignored on 
 *                                UNIX because it has no meaning)
 *           APR_BUFFERED         buffer the data.  Default is non-buffered
 *           APR_EXCL             return error if APR_CREATE and file exists
 *           APR_DELONCLOSE       delete the file after closing.
 * </PRE>
 * @param perm Access permissions for file.
 * @param cont The pool to use.
 * @tip If perm is APR_OS_DEFAULT and the file is being created, appropriate 
 *      default permissions will be used.  *arg1 must point to a valid file_t, 
 *      or NULL (in which case it will be allocated)
 */
apr_status_t apr_open(apr_file_t **new_file, const char *fname, apr_int32_t flag, 
                    apr_fileperms_t perm, apr_pool_t *cont);

/**
 * Close the specified file.
 * @param file The file descriptor to close.
 */
apr_status_t apr_close(apr_file_t *file);

/**
 * delete the specified file.
 * @param path The full path to the file (using / on all systems)
 * @param cont The pool to use.
 * @tip If the file is open, it won't be removed until all instances are closed.
 */
apr_status_t apr_remove_file(const char *path, apr_pool_t *cont);

/**
 * rename the specified file.
 * @param from_path The full path to the original file (using / on all systems)
 * @param to_path The full path to the new file (using / on all systems)
 * @param pool The pool to use.
 * @tip If a file exists at the new location, then it will be overwritten.  
 *      Moving files or directories across devices may not be possible.
 */
apr_status_t apr_rename_file(const char *from_path, const char *to_path,
                           apr_pool_t *pool);

/**
 * Are we at the end of the file
 * @param fptr The apr file we are testing.
 * @tip Returns APR_EOF if we are at the end of file, APR_SUCCESS otherwise.
 */
apr_status_t apr_eof(apr_file_t *fptr);

/**
 * Is there an error on the stream?
 * @param fptr The apr file we are testing.
 * @tip Returns -1 if the error indicator is set, APR_SUCCESS otherwise.
 */
apr_status_t apr_ferror(apr_file_t *fptr);

/**
 * open standard error as an apr file pointer.
 * @param thefile The apr file to use as stderr.
 * @param cont The pool to allocate the file out of.
 */
apr_status_t apr_open_stderr(apr_file_t **thefile, apr_pool_t *cont);

/**
 * Read data from the specified file.
 * @param thefile The file descriptor to read from.
 * @param buf The buffer to store the data to.
 * @param nbytes On entry, the number of bytes to read; on exit, the number of bytes read.
 * @tip apr_read will read up to the specified number of bytes, but never 
 *      more.  If there isn't enough data to fill that number of bytes, all 
 *      of the available data is read.  The third argument is modified to 
 *      reflect the number of bytes read.  If a char was put back into the 
 *      stream via ungetc, it will be the first character returned. 
 *
 *      It is possible for both bytes to be read and an APR_EOF or other 
 *      error to be returned.
 *
 *      APR_EINTR is never returned.
 */
apr_status_t apr_read(apr_file_t *thefile, void *buf, apr_ssize_t *nbytes);

/**
 * Write data to the specified file.
 * @param thefile The file descriptor to write to.
 * @param buf The buffer which contains the data.
 * @param nbytes On entry, the number of bytes to write; on exit, the number of bytes written.
 * @tip apr_write will write up to the specified number of bytes, but never 
 *      more.  If the OS cannot write that many bytes, it will write as many 
 *      as it can.  The third argument is modified to reflect the * number 
 *      of bytes written. 
 *
 *      It is possible for both bytes to be written and an error to be returned.
 *
 *      APR_EINTR is never returned.
 */
apr_status_t apr_write(apr_file_t *thefile, const void *buf, apr_ssize_t *nbytes);

/**
 * Write data from iovec array to the specified file.
 * @param thefile The file descriptor to write to.
 * @param vec The array from which to get the data to write to the file.
 * @param nvec The number of elements in the struct iovec array. This must 
 *             be smaller than APR_MAX_IOVEC_SIZE.  If it isn't, the function 
 *             will fail with APR_EINVAL.
 * @param nbytes The number of bytes written.
 * @tip It is possible for both bytes to be written and an error to be returned.
 *      APR_EINTR is never returned.
 *
 *      apr_writev is available even if the underlying operating system 
 *
 *      doesn't provide writev().
 */
apr_status_t apr_writev(apr_file_t *thefile, const struct iovec *vec, 
                      apr_size_t nvec, apr_ssize_t *nbytes);

/**
 * Read data from the specified file.
 * @param thefile The file descriptor to read from.
 * @param buf The buffer to store the data to.
 * @param nbytes The number of bytes to read.
 * @param bytes_read If non-NULL, this will contain the number of bytes read.
 * @tip apr_read will read up to the specified number of bytes, but never 
 *      more.  If there isn't enough data to fill that number of bytes, 
 *      then the process/thread will block until it is available or EOF 
 *      is reached.  If a char was put back into the stream via ungetc, 
 *      it will be the first character returned. 
 *
 *      It is possible for both bytes to be read and an APR_EOF or other 
 *      error to be returned.
 *
 *      APR_EINTR is never returned.
 */
apr_status_t apr_full_read(apr_file_t *thefile, void *buf, apr_size_t nbytes,
                         apr_size_t *bytes_read);

/**
 * Write data to the specified file.
 * @param thefile The file descriptor to write to.
 * @param buf The buffer which contains the data.
 * @param nbytes The number of bytes to write.
 * @param bytes_written If non-NULL, this will contain the number of bytes written.
 * @tip apr_write will write up to the specified number of bytes, but never 
 *      more.  If the OS cannot write that many bytes, the process/thread 
 *      will block until they can be written. Exceptional error such as 
 *      "out of space" or "pipe closed" will terminate with an error.
 *
 *      It is possible for both bytes to be written and an error to be returned.
 *
 *      APR_EINTR is never returned.
 */
apr_status_t apr_full_write(apr_file_t *thefile, const void *buf,
                          apr_size_t nbytes, apr_size_t *bytes_written);

/**
 * put a character into the specified file.
 * @param ch The character to write.
 * @param thefile The file descriptor to write to
 */
apr_status_t apr_putc(char ch, apr_file_t *thefile);

/**
 * get a character from the specified file.
 * @param ch The character to write.
 * @param thefile The file descriptor to write to
 */
apr_status_t apr_getc(char *ch, apr_file_t *thefile);

/**
 * put a character back onto a specified stream.
 * @param ch The character to write.
 * @param thefile The file descriptor to write to
 */
apr_status_t apr_ungetc(char ch, apr_file_t *thefile);

/**
 * Get a string from a specified file.
 * @param str The buffer to store the string in. 
 * @param len The length of the string
 * @param thefile The file descriptor to read from
 */
apr_status_t apr_fgets(char *str, int len, apr_file_t *thefile);

/**
 * Put the string into a specified file.
 * @param str The string to write. 
 * @param thefile The file descriptor to write to
 */
apr_status_t apr_puts(const char *str, apr_file_t *thefile);

/**
 * Flush the file's buffer.
 * @param thefile The file descriptor to flush
 */
apr_status_t apr_flush(apr_file_t *thefile);

/**
 * duplicate the specified file descriptor.
 * @param new_file The structure to duplicate into. 
 * @param old_file The file to duplicate.
 * @param p The pool to use for the new file.
 * @tip *arg1 must point to a valid apr_file_t, or point to NULL
 */         
apr_status_t apr_dupfile(apr_file_t **new_file, apr_file_t *old_file, apr_pool_t *p);

/**
 * get the specified file's stats.
 * @param finfo Where to store the information about the file.
 * @param thefile The file to get information about.
 */ 
apr_status_t apr_getfileinfo(apr_finfo_t *finfo, apr_file_t *thefile);

/**
 * set the specified file's permission bits.
 * @param fname The file (name) to apply the permissions to.
 * @param perms The permission bits to apply to the file.
 * @tip Some platforms may not be able to apply all of the available 
 *      permission bits; APR_INCOMPLETE will be returned if some permissions 
 *      are specified which could not be set.
 *
 *      Platforms which do not implement this feature will return APR_ENOTIMPL.
 */
apr_status_t apr_setfileperms(const char *fname, apr_fileperms_t perms);

/**
 * get the specified file's stats.  The file is specified by filename, 
 * instead of using a pre-opened file.
 * @param finfo Where to store the information about the file.
 * @param fname The name of the file to stat.
 * @param cont the pool to use to allocate the new file. 
 */ 
apr_status_t apr_stat(apr_finfo_t *finfo, const char *fname, apr_pool_t *cont);

/**
 * get the specified file's stats.  The file is specified by filename, 
 * instead of using a pre-opened file.  If the file is a symlink, this function
 * will get the stats for the symlink not the file the symlink refers to.
 * @param finfo Where to store the information about the file.
 * @param fname The name of the file to stat.
 * @param cont the pool to use to allocate the new file. 
 */ 
apr_status_t apr_lstat(apr_finfo_t *finfo, const char *fname, apr_pool_t *cont);

/**
 * Move the read/write file offset to a specified byte within a file.
 * @param thefile The file descriptor
 * @param where How to move the pointer, one of:
 * <PRE>
 *            APR_SET  --  set the offset to offset
 *            APR_CUR  --  add the offset to the current position 
 *            APR_END  --  add the offset to the current file size 
 * @param offset The offset to move the pointer to.
 * @tip The third argument is modified to be the offset the pointer
          was actually moved to.
 */
apr_status_t apr_seek(apr_file_t *thefile, apr_seek_where_t where,apr_off_t *offset);

/**
 * Open the specified directory.
 * @param new_dir The opened directory descriptor.
 * @param dirname The full path to the directory (use / on all systems)
 * @param cont The pool to use.
 */                        
apr_status_t apr_opendir(apr_dir_t **new_dir, const char *dirname, apr_pool_t *cont);

/**
 * close the specified directory. 
 * @param thedir the directory descriptor to close.
 */                        
apr_status_t apr_closedir(apr_dir_t *thedir);

/**
 * Read the next entry from the specified directory. 
 * @param thedir the directory descriptor to read from, and fill out.
 * @tip All systems return . and .. as the first two files.
 */                        
apr_status_t apr_readdir(apr_dir_t *thedir);

/**
 * Rewind the directory to the first entry.
 * @param thedir the directory descriptor to rewind.
 */                        
apr_status_t apr_rewinddir(apr_dir_t *thedir);

/**
 * Create a new directory on the file system.
 * @param path the path for the directory to be created.  (use / on all systems)
 * @param perm Permissions for the new direcoty.
 * @param cont the pool to use.
 */                        
apr_status_t apr_make_dir(const char *path, apr_fileperms_t perm, 
                        apr_pool_t *cont);

/**
 * Remove directory from the file system.
 * @param path the path for the directory to be removed.  (use / on all systems)
 * @param cont the pool to use.
 */                        
apr_status_t apr_remove_dir(const char *path, apr_pool_t *cont);

/**
 * Create an anonymous pipe.
 * @param in The file descriptor to use as input to the pipe.
 * @param out The file descriptor to use as output from the pipe.
 * @param cont The pool to operate on.
 */
apr_status_t apr_create_pipe(apr_file_t **in, apr_file_t **out, apr_pool_t *cont);

/**
 * Create a named pipe.
 * @param filename The filename of the named pipe
 * @param perm The permissions for the newly created pipe.
 * @param cont The pool to operate on.
 */
apr_status_t apr_create_namedpipe(const char *filename, apr_fileperms_t perm, 
                                apr_pool_t *cont);

/**
 * Set the timeout value for a pipe or manipulate the blocking state.
 * @param thepipe The pipe we are setting a timeout on.
 * @param timeoutThe timeout value in microseconds.  Values < 0 mean wait 
 *        forever, 0 means do not wait at all.
 */
apr_status_t apr_set_pipe_timeout(apr_file_t *thepipe, apr_interval_time_t timeout);

/**accessor and general file_io functions. */

/**
 * return the file name of the current file.
 * @param new_path The path of the file.  
 * @param thefile The currently open file.
 */                     
apr_status_t apr_get_filename(char **new_path, apr_file_t *thefile);

/**
 * Get the file name of the current directory entry.
 * @param new_path the file name of the directory entry. 
 * @param thedir the currently open directory.
 */                        
apr_status_t apr_get_dir_filename(char **new_path, apr_dir_t *thedir);

/**
 * Return the data associated with the current file.
 * @param data The user data associated with the file.  
 * @param key The key to use for retreiving data associated with this file.
 * @param file The currently open file.
 */                     
apr_status_t apr_get_filedata(void **data, const char *key, apr_file_t *file);

/**
 * Set the data associated with the current file.
 * @param file The currently open file.
 * @param data The user data to associate with the file.  
 * @param key The key to use for assocaiteing data with the file.
 * @param cleanup The cleanup routine to use when the file is destroyed.
 */                     
apr_status_t apr_set_filedata(apr_file_t *file, void *data, const char *key,
                            apr_status_t (*cleanup) (void *));

/**
 * Get the size of the current directory entry.
 * @param size the size of the directory entry. 
 * @param thedir the currently open directory.
 */                        
apr_status_t apr_dir_entry_size(apr_ssize_t *size, apr_dir_t *thedir);

/**
 * Get the last modified time of the current directory entry.
 * @param mtime the last modified time of the directory entry. 
 * @param thedir the currently open directory.
 */ 
apr_status_t apr_dir_entry_mtime(apr_time_t *mtime, apr_dir_t *thedir);

/**
 * Get the file type of the current directory entry.
 * @param type the file type of the directory entry. 
 * @param thedir the currently open directory.
 */
apr_status_t apr_dir_entry_ftype(apr_filetype_e *type, apr_dir_t *thedir);

/**
 * Write a string to a file using a printf format.
 * @param fptr The file to write to.
 * @param format The format string
 * @param ... The values to substitute in the format string
 * @return The number of bytes written
 * @deffunc int apr_fprintf(apr_file_t *fptr, const char *format, ...)
 */ 
APR_DECLARE(int) apr_fprintf(apr_file_t *fptr, const char *format, ...)
        __attribute__((format(printf,2,3)));

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_FILE_IO_H */


