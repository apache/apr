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

#include "apr.h"
#include "apr_private.h"
#include "apr_pools.h"
#include "apr_general.h"
#include "apr_tables.h"
#include "apr_lock.h"
#include "apr_file_io.h"
#include "apr_errno.h"
#include "misc.h"

#if APR_HAVE_SYS_STAT_H
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
#if APR_HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#ifdef HAVE_UIO_H
#include <sys/uio.h>
#endif

#define APR_FILE_BUFSIZE 4096

typedef apr_int16_t apr_wchar_t;

typedef enum apr_canon_case_e {
    APR_CANON_CASE_GIVEN,
    APR_CANON_CASE_LOWER,
    APR_CANON_CASE_TRUE
} apr_canon_case_e;

/*
 * Internal canonical filename elements for the apr_canon_t elems
 *  ccase   tracks the mechanism used to resolve this element
 *  pathlen is the full path length to the end of this element
 *  name    slash is prefix, as appropriate 
               
 */
typedef struct apr_canon_elem_t {
    apr_canon_case_e ccase;
    int pathlen;
    char *name;
} apr_canon_elem_t;

/* warning: win32 canonical path "/" resolves to a
 * zero'th element of the empty string for testing the
 * psudo-root for the system
 */
struct apr_canon_t {
    apr_pool_t *cntxt;
    apr_array_header_t *elems;
};

/* quick run-down of fields in windows' apr_file_t structure that may have 
 * obvious uses.
 * fname --  the filename as passed to the open call.
 * dwFileAttricutes -- Attributes used to open the file.
 * demonfname -- the canonicalized filename.  Used to store the result from
 *               apr_os_canonicalize_filename.
 * lowerdemonfname -- inserted at Ken Parzygnat's request, because of the
 *                    ugly way windows deals with case in the filesystem.
 * append -- Windows doesn't support the append concept when opening files.
 *           APR needs to keep track of this, and always make sure we append
 *           correctly when writing to a file with this flag set TRUE.
 */

struct apr_file_t {
    apr_pool_t *cntxt;
    HANDLE filehand;
    BOOLEAN pipe;              // Is this a pipe of a file?
    OVERLAPPED *pOverlapped;
    apr_interval_time_t timeout;

    /* File specific info */
    union {
#if APR_HAS_UNICODE_FS
        struct {
            apr_wchar_t *fname;
        } w;
#endif
        struct {
            char *fname;
        } n;
    };
    apr_canon_t *canonname;
    
    DWORD dwFileAttributes;
    int eof_hit;
    BOOLEAN buffered;          // Use buffered I/O?
    int ungetchar;             // Last char provided by an unget op. (-1 = no char)
    int append; 
    off_t size;
    apr_time_t atime;
    apr_time_t mtime;
    apr_time_t ctime;

    /* Stuff for buffered mode */
    char *buffer;
    apr_ssize_t bufpos;        // Read/Write position in buffer
    apr_ssize_t dataRead;      // amount of valid data read into buffer
    int direction;            // buffer being used for 0 = read, 1 = write
    apr_ssize_t filePtr;       // position in file of handle
    apr_lock_t *mutex;         // mutex semaphore, must be owned to access the above fields

    /* Pipe specific info */
    
};

struct apr_dir_t {
    apr_pool_t *cntxt;
    HANDLE dirhand;
    union {
#if APR_HAS_UNICODE_FS
        struct {
            apr_wchar_t *dirname;
            WIN32_FIND_DATAW *entry;
        } w;
#endif
        struct {
            char *dirname;
            WIN32_FIND_DATA *entry;
        } n;
    };
};

apr_status_t file_cleanup(void *);

/**
 * Internal function to create a Win32/NT pipe that respects some async
 * timeout options.
 */
apr_status_t apr_create_nt_pipe(apr_file_t **in, apr_file_t **out, 
                                BOOLEAN bAsyncRead, BOOLEAN bAsyncWrite, 
                                apr_pool_t *p);
#endif  /* ! FILE_IO_H */

