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
#define APR_BUFFERED   128         /* Open the file for buffered I/O */
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
typedef dev_t                    ap_dev_t;

struct ap_finfo_t {
    ap_fileperms_t protection;
    ap_filetype_e filetype;
    ap_uid_t user;
    ap_gid_t group;
    ap_ino_t inode;
    ap_dev_t device;
    ap_off_t size;
    ap_time_t atime;
    ap_time_t mtime;
    ap_time_t ctime;
};

/*   Function definitions */
/*

=head1 ap_status_t ap_open(ap_file_t **new_file, const char *fname, ap_int32_t flag, ap_fileperms_t perm, ap_pool_t *cont)

B<Open the specified file.>

    arg 1) The opened file descriptor.
    arg 2) The full path to the file (using / on all systems)
    arg 3) Or'ed value of:
             APR_READ             open for reading
             APR_WRITE            open for writing
             APR_CREATE           create the file if not there
             APR_APPEND           file ptr is set to end prior to all writes
             APR_TRUNCATE         set length to zero if file exists
             APR_BINARY           not a text file (This flag is ignored on 
                                  UNIX because it has no meaning)
             APR_BUFFERED         buffer the data.  Default is non-buffered
             APR_EXCL             return error if APR_CREATE and file exists
             APR_DELONCLOSE       delete the file after closing.
    arg 4) Access permissions for file.
    arg 5) The pool to use.

B<NOTE>:  If perm is APR_OS_DEFAULT and the file is being created, appropriate
          default permissions will be used.  *arg1 must point to a valid file_t, 
          or NULL (in which case it will be allocated)

=cut
 */
ap_status_t ap_open(ap_file_t **new_file, const char *fname, ap_int32_t flag, 
                    ap_fileperms_t perm, ap_pool_t *cont);

/*

=head1 ap_status_t ap_close(ap_file_t *file)

B<Close the specified file.>

    arg 1) The file descriptor to close.

=cut
 */
ap_status_t ap_close(ap_file_t *file);

/*

=head1 ap_status_t ap_remove_file(const char *path, ap_pool_t *cont) 

B<delete the specified file.>

    arg 1) The full path to the file (using / on all systems)
    arg 2) The pool to use.

B<NOTE>: If the file is open, it won't be removed until all instances are
       closed.

=cut
 */
ap_status_t ap_remove_file(const char *path, ap_pool_t *cont);

/*

=head1 ap_status_t ap_rename_file(const char *from_path, const char *to_path,
                                  ap_pool_t *cont) 

B<rename the specified file.>

    arg 1) The full path to the original file (using / on all systems)
    arg 2) The full path to the new file (using / on all systems)
    arg 3) The pool to use.

B<NOTE>: If a file exists at the new location, then it will be overwritten.
    Moving files or directories across devices may not be possible.

=cut
 */
ap_status_t ap_rename_file(const char *from_path, const char *to_path,
                           ap_pool_t *pool);

/*

=head1 ap_status_t ap_eof(ap_file_t *fptr) 

B<Are we at the end of the file>

    arg 1) The apr file we are testing.

B<NOTE>:  Returns APR_EOF if we are at the end of file, APR_SUCCESS otherwise.

=cut
 */
ap_status_t ap_eof(ap_file_t *fptr);

/*

=head1 ap_status_t ap_ferror(ap_file_t *fptr) 

B<Is there an error on the stream?>

    arg 1) The apr file we are testing.

B<NOTE>:  Returns -1 if the error indicator is set, APR_SUCCESS otherwise.

=cut
 */
ap_status_t ap_ferror(ap_file_t *fptr);

/*

=head1 ap_status_t ap_open_stderr(ap_file_t **thefile, ap_pool_t *cont) 

B<open standard error as an apr file pointer.>

    arg 1) The apr file to use as stderr.
    arg 2) The pool to allocate the file out of.

=cut
 */
ap_status_t ap_open_stderr(ap_file_t **thefile, ap_pool_t *cont);

/*

=head1 ap_status_t ap_read(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)

B<Read data from the specified file.>

    arg 1) The file descriptor to read from.
    arg 2) The buffer to store the data to.
    arg 3) On entry, the number of bytes to read; on exit, the number
           of bytes read.

B<NOTE>:  ap_read will read up to the specified number of bytes, but never
   more.  If there isn't enough data to fill that number of bytes, all of
   the available data is read.  The third argument is modified to reflect the
   number of bytes read.  If a char was put back into the stream via
   ungetc, it will be the first character returned. 
  
   It is possible for both bytes to be read and an APR_EOF or other error
   to be returned.
  
   APR_EINTR is never returned.

=cut
 */
ap_status_t ap_read(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes);

/*

=head1 ap_status_t ap_write(ap_file_t *thefile, const void *buf, ap_ssize_t *nbytes)

B<Write data to the specified file.>

    arg 1) The file descriptor to write to.
    arg 2) The buffer which contains the data.
    arg 3) On entry, the number of bytes to write; on exit, the number
           of bytes written.

B<NOTE>:  ap_write will write up to the specified number of bytes, but never
   more.  If the OS cannot write that many bytes, it will write as many as it
   can.  The third argument is modified to reflect the * number of bytes 
   written. 
  
   It is possible for both bytes to be written and an error to be
   returned.
  
   APR_EINTR is never returned.

=cut
 */
ap_status_t ap_write(ap_file_t *thefile, const void *buf, ap_ssize_t *nbytes);

/*

=head1 ap_status_t ap_writev(ap_file_t *thefile, const struct iovec *vec, ap_size_t nvec, ap_ssize_t *nbytes)

B<Write data from iovec array to the specified file.>

    arg 1) The file descriptor to write to.
    arg 2) The array from which to get the data to write to the file.
    arg 3) The number of elements in the struct iovec array. This must be
           smaller than AP_MAX_IOVEC_SIZE.  If it isn't, the function will
           fail with APR_EINVAL.
    arg 4) The number of bytes written.
  
   It is possible for both bytes to be written and an error to be
   returned.
  
   APR_EINTR is never returned.
  
   ap_writev is available even if the underlying operating system
   doesn't provide writev().

=cut
 */
ap_status_t ap_writev(ap_file_t *thefile, const struct iovec *vec, 
                      ap_size_t nvec, ap_ssize_t *nbytes);

/*

=head1 ap_status_t ap_full_read(ap_file_t *thefile, void *buf, ap_size_t nbytes, ap_size_t *bytes_read)

B<Read data from the specified file.>

    arg 1) The file descriptor to read from.
    arg 2) The buffer to store the data to.
    arg 3) The number of bytes to read.
    arg 4) If non-NULL, this will contain the number of bytes read.

B<NOTE>:  ap_read will read up to the specified number of bytes, but never
   more.  If there isn't enough data to fill that number of bytes, then the
   process/thread will block until it is available or EOF is reached.  If a
   char was put back into the stream via ungetc, it will be the first
   character returned. 

   It is possible for both bytes to be read and an APR_EOF or other error
   to be returned.
   
   APR_EINTR is never returned.

=cut
 */
ap_status_t ap_full_read(ap_file_t *thefile, void *buf, ap_size_t nbytes,
                         ap_size_t *bytes_read);

/*

=head1 ap_status_t ap_full_write(ap_file_t *thefile, const void *buf, ap_size_t nbytes, ap_size_t *bytes_written)

B<Write data to the specified file.>

    arg 1) The file descriptor to write to.
    arg 2) The buffer which contains the data.
    arg 3) The number of bytes to write.
    arg 4) If non-NULL, this will contain the number of bytes written.

B<NOTE>:  ap_write will write up to the specified number of bytes, but never
   more.  If the OS cannot write that many bytes, the process/thread will
   block until they can be written. Exceptional error such as "out of space"
   or "pipe closed" will terminate with an error.
  
   It is possible for both bytes to be written and an error to be returned.
  
   APR_EINTR is never returned.

=cut
 */
ap_status_t ap_full_write(ap_file_t *thefile, const void *buf,
                          ap_size_t nbytes, ap_size_t *bytes_written);

/*

=head1 ap_status_t ap_putc(char ch, ap_file_t *thefile)

B<put a character into the specified file.>

    arg 1) The character to write.
    arg 2) The file descriptor to write to

=cut
 */
ap_status_t ap_putc(char ch, ap_file_t *thefile);

/*

=head1 ap_status_t ap_getc(char *ch, ap_file_t *thefil)

B<get a character from the specified file.>

    arg 1) The character to write.
    arg 2) The file descriptor to write to

=cut
 */
ap_status_t ap_getc(char *ch, ap_file_t *thefile);

/*

=head1 ap_status_t ap_ungetc(char ch, ap_file_t *thefile)

B<put a character back onto a specified stream.>

    arg 1) The character to write.
    arg 2) The file descriptor to write to

=cut
 */
ap_status_t ap_ungetc(char ch, ap_file_t *thefile);

/*

=head1 ap_status_t ap_fgets(char *str, int len, ap_file_t *thefile)

B<Get a string from a specified file.>

    arg 1) The buffer to store the string in. 
    arg 2) The length of the string
    arg 3) The file descriptor to read from

=cut
 */
ap_status_t ap_fgets(char *str, int len, ap_file_t *thefile);

/*

=head1 ap_status_t ap_puts(const char *str, ap_file_t *thefile)

B<Put the string into a specified file.>

    arg 1) The string to write. 
    arg 2) The file descriptor to write to

=cut
 */
ap_status_t ap_puts(const char *str, ap_file_t *thefile);

/*

=head1 ap_status_t ap_flush(ap_file_t *thefile)

B<Flush the file's buffer.>

    arg 1) The file descriptor to flush

=cut
 */
ap_status_t ap_flush(ap_file_t *thefile);
APR_EXPORT(int) ap_fprintf(ap_file_t *fptr, const char *format, ...)
        __attribute__((format(printf,2,3)));

/*

=head1 ap_status_t ap_dupfile(ap_file_t **new_file, ap_file_t *old_file, ap_pool_t *p)

B<duplicate the specified file descriptor.>

    arg 1) The structure to duplicate into. 
    arg 2) The file to duplicate.
    arg 3) The pool to use for the new file.

B<NOTE>: *arg1 must point to a valid ap_file_t, or point to NULL

=cut
 */         
ap_status_t ap_dupfile(ap_file_t **new_file, ap_file_t *old_file, ap_pool_t *p);

/*

=head1 ap_status_t ap_getfileinfo(ap_finfo_t *finfo, ap_file_t *thefile)

B<get the specified file's stats.>

    arg 1) Where to store the information about the file.
    arg 2) The file to get information about.

=cut
 */ 
ap_status_t ap_getfileinfo(ap_finfo_t *finfo, ap_file_t *thefile);

/*

=head1 ap_status_t ap_setfileperms(const char *fname, ap_fileperms_t perms)

B<set the specified file's permission bits.>

    arg 1) The file (name) to apply the permissions to.
    arg 2) The permission bits to apply to the file.

   Some platforms may not be able to apply all of the available permission
   bits; APR_INCOMPLETE will be returned if some permissions are specified
   which could not be set.

   Platforms which do not implement this feature will return APR_ENOTIMPL.
=cut
 */
ap_status_t ap_setfileperms(const char *fname, ap_fileperms_t perms);

/*

=head1 ap_status_t ap_stat(ap_finfo_t *finfo, const char *fname, ap_pool_t *cont)

B<get the specified file's stats.  The file is specified by filename, instead of using a pre-opened file.>

    arg 1) Where to store the information about the file.
    arg 2) The name of the file to stat.
    arg 3) the pool to use to allocate the new file. 

=cut
 */ 
ap_status_t ap_stat(ap_finfo_t *finfo, const char *fname, ap_pool_t *cont);

/*

=head1 ap_status_t ap_lstat(ap_finfo_t *finfo, const char *fname, ap_pool_t *cont)

B<get the specified file's stats.  The file is specified by filename, instead of using a pre-opened file.  If the file is a symlink, this function will get the stats for the symlink not the file the symlink refers to.>

    arg 1) Where to store the information about the file.
    arg 2) The name of the file to stat.
    arg 3) the pool to use to allocate the new file. 

=cut
 */ 
ap_status_t ap_lstat(ap_finfo_t *finfo, const char *fname, ap_pool_t *cont);

/*

=head1 ap_status_t ap_seek(ap_file_t *thefile, ap_seek_where_t where, ap_off_t *offset)

B<Move the read/write file offset to a specified byte within a file.>

    arg 1) The file descriptor
    arg 2) How to move the pointer, one of:
              APR_SET  --  set the offset to offset
              APR_CUR  --  add the offset to the current position 
              APR_END  --  add the offset to the current file size 
    arg 3) The offset to move the pointer to.

B<NOTE>:  The third argument is modified to be the offset the pointer
          was actually moved to.

=cut
 */
ap_status_t ap_seek(ap_file_t *thefile, ap_seek_where_t where,ap_off_t *offset);

/*

=head1 ap_status_t ap_opendir(ap_dir_t **new_dir, const char *dirname, ap_pool_t *cont)

B<Open the specified directory.>

    arg 1) The opened directory descriptor.
    arg 2) The full path to the directory (use / on all systems)
    arg 3) The pool to use.

=cut
 */                        
ap_status_t ap_opendir(ap_dir_t **new_dir, const char *dirname, ap_pool_t *cont);

/*

=head1 ap_status_t ap_closedir(ap_dir_t *thedir)

B<close the specified directory.> 

    arg 1) the directory descriptor to close.

=cut
 */                        
ap_status_t ap_closedir(ap_dir_t *thedir);

/*

=head1 ap_status_t ap_readdir(ap_dir_t *thedir)

B<Read the next entry from the specified directory.> 

    arg 1) the directory descriptor to read from, and fill out.

B<NOTE>: All systems return . and .. as the first two files.

=cut
 */                        
ap_status_t ap_readdir(ap_dir_t *thedir);

/*

=head1 ap_status_t ap_rewinddir(ap_dir_t *thedir)

B<Rewind the directory to the first entry.>

     arg 1) the directory descriptor to rewind.

=cut
 */                        
ap_status_t ap_rewinddir(ap_dir_t *thedir);

/*

=head1 ap_status_t ap_make_dir(const char *path, ap_fileperms_t perm, ap_pool_t *cont)

B<Create a new directory on the file system.>

    arg 1) the path for the directory to be created.  (use / on all systems)
    arg 2) Permissions for the new direcoty.
    arg 3) the pool to use.

=cut
 */                        
ap_status_t ap_make_dir(const char *path, ap_fileperms_t perm, 
                        ap_pool_t *cont);

/*

=head1 ap_status_t ap_remove_dir(const char *path, ap_pool_t *cont)

B<Remove directory from the file system.>

    arg 1) the path for the directory to be removed.  (use / on all systems)
    arg 2) the pool to use.

=cut
 */                        
ap_status_t ap_remove_dir(const char *path, ap_pool_t *cont);

/*

=head1 ap_status_t ap_create_pipe(ap_file_t **in, ap_file_t **out, ap_pool_t *cont)

B<Create an anonymous pipe.>

    arg 1) The file descriptor to use as input to the pipe.
    arg 2) The file descriptor to use as output from the pipe.
    arg 3) The pool to operate on.

=cut
 */
ap_status_t ap_create_pipe(ap_file_t **in, ap_file_t **out, ap_pool_t *cont);

/*

=head1 ap_status_t ap_create_namedpipe(const char *filename, ap_fileperms_t perm, ap_pool_t *cont)

B<Create a named pipe.>

    arg 1) The filename of the named pipe
    arg 2) The permissions for the newly created pipe.
    arg 3) The pool to operate on.

=cut
 */
ap_status_t ap_create_namedpipe(const char *filename, ap_fileperms_t perm, 
                                ap_pool_t *cont);

/*

=head1 ap_status_t ap_set_pipe_timeout(ap_file_t *thepipe, ap_interval_time_t timeout)

B<Set the timeout value for a pipe or manipulate the blocking state.>

    arg 1) The pipe we are setting a timeout on.
    arg 2) The timeout value in microseconds.  Values < 0 mean wait forever, 0
           means do not wait at all.

=cut
 */
ap_status_t ap_set_pipe_timeout(ap_file_t *thepipe, ap_interval_time_t timeout);

/*accessor and general file_io functions. */

/*

=head1 ap_status_t ap_get_filename(char **new_path, ap_file_t *thefile)

B<return the file name of the current file.>

    arg 1) The path of the file.  
    arg 2) The currently open file.

=cut
 */                     
ap_status_t ap_get_filename(char **new_path, ap_file_t *thefile);

/*

=head1 ap_status_t ap_get_dir_filename(char **new_path, ap_dir_t *thedir) 

B<Get the file name of the current directory entry.>

    arg 1) the file name of the directory entry. 
    arg 2) the currently open directory.

=cut
 */                        
ap_status_t ap_get_dir_filename(char **new_path, ap_dir_t *thedir);

/*

=head1 ap_status_t ap_get_filedata(void **data, const char *key, ap_file_t *file)

B<Return the data associated with the current file.>

    arg 1) The user data associated with the file.  
    arg 2) The key to use for retreiving data associated with this file.
    arg 3) The currently open file.

=cut
 */                     
ap_status_t ap_get_filedata(void **data, const char *key, ap_file_t *file);

/*

=head1 ap_status_t ap_set_filedata(ap_file_t *file, void *data, const char *key, ap_status_t (*cleanup) (void *))

B<Set the data associated with the current file.>

    arg 1) The currently open file.
    arg 2) The user data to associate with the file.  
    arg 3) The key to use for assocaiteing data with the file.
    arg 4) The cleanup routine to use when the file is destroyed.

=cut
 */                     
ap_status_t ap_set_filedata(ap_file_t *file, void *data, const char *key,
                            ap_status_t (*cleanup) (void *));

/*

=head1 ap_status_t ap_dir_entry_size(ap_ssize_t *size, ap_dir_t *thedir)

B<Get the size of the current directory entry.>

    arg 1) the size of the directory entry. 
    arg 2) the currently open directory.

=cut
 */                        
ap_status_t ap_dir_entry_size(ap_ssize_t *size, ap_dir_t *thedir);

/*

=head1 ap_status_t ap_dir_entry_mtime(ap_time_t *mtime, ap_dir_t *thedir)

B<Get the last modified time of the current directory entry.>

    arg 1) the last modified time of the directory entry. 
    arg 2) the currently open directory.

=cut
 */                        
ap_status_t ap_dir_entry_mtime(ap_time_t *mtime, ap_dir_t *thedir);

/*

=head1 ap_status_t ap_dir_entry_ftype(ap_filetype_e *type, ap_dir_t *thedir)

B<Get the file type of the current directory entry.>

    arg 1) the file type of the directory entry. 
    arg 2) the currently open directory.

=cut
 */                        
ap_status_t ap_dir_entry_ftype(ap_filetype_e *type, ap_dir_t *thedir);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_FILE_IO_H */


