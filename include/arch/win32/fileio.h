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

#ifndef FILE_IO_H
#define FILE_IO_H

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UIO_H
#include <sys/uio.h>
#endif
#include "apr_config.h"
#include "apr_pools.h"
#include "apr_general.h"
#include "apr_file_io.h"
#include "apr_errno.h"

/* quick run-down of fields in windows' ap_file_t structure that may have 
 * obvious uses.
 * fname --  the filename as passed to the open call.
 * dwFileAttricutes -- Attributes used to open the file.
 * demonfname -- the canonicalized filename.  Used to store the result from
 *               ap_os_canonicalize_filename.
 * lowerdemonfname -- inserted at Ken Parzygnat's request, because of the
 *                    ugly way windows deals with case in the filesystem.
 * append -- Windows doesn't support the append concept when opening files.
 *           APR needs to keep track of this, and always make sure we append
 *           correctly when writing to a file with this flag set TRUE.
 */

struct ap_file_t {
    ap_pool_t *cntxt;
    HANDLE filehand;
    char *fname;
    DWORD dwFileAttributes;
    char *demonfname; 
    char *lowerdemonfname; 
    int stated;
    int append; 
    int eof_hit;
    off_t size;
    ap_time_t atime;
    ap_time_t mtime;
    ap_time_t ctime;
    int pipe;
    int timeout;
};

struct ap_dir_t {
    ap_pool_t *cntxt;
    char *dirname;
    HANDLE dirhand;
    WIN32_FIND_DATA *entry;
};

ap_status_t file_cleanup(void *);
/*mode_t get_fileperms(ap_fileperms_t);
*/
API_EXPORT(char *) ap_os_systemcase_filename(struct ap_pool_t *pCont, 
                                             const char *szFile);
char * canonical_filename(struct ap_pool_t *pCont, const char *szFile);

#endif  /* ! FILE_IO_H */

