/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "apr_private.h"
#include "apr_general.h"
#include "apr_thread_mutex.h"
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"

/* We have an implementation of mkstemp but it's not very multi-threading 
 * friendly & is part of the POSIX emulation rather than native so don't
 * use it.
 */
#undef HAVE_MKSTEMP

#define APR_FILE_BUFSIZE 4096

struct apr_file_t {
    apr_pool_t *pool;
    HFILE filedes;
    char * fname;
    int isopen;
    int buffered;
    int eof_hit;
    apr_int32_t flags;
    int timeout;
    int pipe;
    HEV pipeSem;
    enum { BLK_UNKNOWN, BLK_OFF, BLK_ON } blocking;

    /* Stuff for buffered mode */
    char *buffer;
    int bufpos;               // Read/Write position in buffer
    unsigned long dataRead;   // amount of valid data read into buffer
    int direction;            // buffer being used for 0 = read, 1 = write
    unsigned long filePtr;    // position in file of handle
    apr_thread_mutex_t *mutex;// mutex semaphore, must be owned to access the above fields
};

struct apr_dir_t {
    apr_pool_t *pool;
    char *dirname;
    ULONG handle;
    FILEFINDBUF3 entry;
    int validentry;
};

apr_status_t apr_file_cleanup(void *);
apr_status_t apr_os2_time_to_apr_time(apr_time_t *result, FDATE os2date, 
                                      FTIME os2time);

/* see win32/fileio.h for description of these */
extern const char c_is_fnchar[256];

#define IS_FNCHAR(c) c_is_fnchar[(unsigned char)c]

apr_status_t filepath_root_test(char *path, apr_pool_t *p);
apr_status_t filepath_drive_get(char **rootpath, char drive, 
                                apr_int32_t flags, apr_pool_t *p);
apr_status_t filepath_root_case(char **rootpath, char *root, apr_pool_t *p);

#endif  /* ! FILE_IO_H */

