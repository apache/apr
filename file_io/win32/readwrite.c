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

#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_errno.h"
#include <malloc.h>
#include <windows.h>
#include "atime.h"

#define GetFilePointer(hfile) SetFilePointer(hfile,0,NULL, FILE_CURRENT)

ap_status_t ap_read(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    DWORD bread;
    int lasterror;

    if (ReadFile(thefile->filehand, buf, *nbytes, &bread, NULL)) {
        *nbytes = bread;
        return APR_SUCCESS;
    }

    *nbytes = 0;
    lasterror = GetLastError();
    if (lasterror == ERROR_BROKEN_PIPE) {
        /* Assume ERROR_BROKEN_PIPE signals an EOF reading from a pipe */
        return APR_SUCCESS;
    } else if (lasterror == ERROR_NO_DATA) {
        /* Receive this error on a read to a pipe in nonblocking mode */
        return APR_EAGAIN;
    }

    return lasterror;
}

ap_status_t ap_write(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ap_status_t rv;
    DWORD bwrote;

    if (!thefile->pipe) {
        SetFilePointer(thefile->filehand, 0, NULL, FILE_END);
    }
    if (WriteFile(thefile->filehand, buf, *nbytes, &bwrote, NULL)) {
        *nbytes = bwrote;
        rv = APR_SUCCESS;
    } 
    else {
        (*nbytes) = 0;
        rv = GetLastError();
    }

    return rv;
}
/*
 * Too bad WriteFileGather() is not supported on 95&98 (or NT prior to SP2) 
 */
ap_status_t ap_writev(ap_file_t *thefile, const struct iovec *vec, ap_size_t nvec, 
                      ap_ssize_t *nbytes)
{
    ap_status_t rv = APR_SUCCESS;
    int i;
    DWORD bwrote = 0;
    char *buf;

    *nbytes = 0;
    for (i = 0; i < nvec; i++) {
        buf = vec[i].iov_base;
        bwrote = vec[i].iov_len;
        rv = ap_write(thefile, buf, &bwrote);
        *nbytes += bwrote;
        if (rv != APR_SUCCESS) {
            break;
        }
    }
    return rv;
}

ap_status_t ap_putc(char ch, ap_file_t *thefile)
{
    DWORD len = 1;

    return ap_write(thefile, &ch, &len);
}

ap_status_t ap_ungetc(char ch, ap_file_t *thefile)
{
    /* 
     * Your application must provide its own serialization (locking) if
     * it allows multiple threads to access the same file handle 
     * concurrently.
     *
     * ToDo: This function does not use the char ch argument. Could add 
     * gorpy code to read the file after the SetFilePointer() call to 
     * make sure the character pushed back on the stream is the same as
     * arg ch. Then, need to SetFilePointer() once more to reset the 
     * file pointer to the point before the read. Yech... Just assume 
     * the caller knows what he is doing.  There may be a nifty Win32 
     * call for this I've not discovered....
     */

    /* SetFilePointer is only valid for a file device ...*/
    if (GetFileType(thefile->filehand) != FILE_TYPE_DISK) {
        return GetLastError();
    }
    /* and the file pointer is not pointing to the start of the file. */
    if (GetFilePointer(thefile->filehand)) {
        if (SetFilePointer(thefile->filehand, -1, NULL, FILE_CURRENT) 
            == 0xFFFFFFFF) {
            return GetLastError();
        }
    }

    thefile->stated = 0;
    return APR_SUCCESS; 
}

ap_status_t ap_getc(char *ch, ap_file_t *thefile)
{
    DWORD bread;
    if (!ReadFile(thefile->filehand, ch, 1, &bread, NULL)) {
        return GetLastError();
    }
    if (bread == 0) {
        thefile->eof_hit = TRUE;
        return APR_EOF;
    }
    return APR_SUCCESS; 
}

ap_status_t ap_puts(char *str, ap_file_t *thefile)
{
    DWORD len = strlen(str);

    return ap_write(thefile, str, &len);
}

ap_status_t ap_fgets(char *str, int len, ap_file_t *thefile)
{
    DWORD bread;
    int i;
    if (!ReadFile(thefile->filehand, str, len, &bread, NULL)) {
        switch(GetLastError()) {
        case ERROR_HANDLE_EOF:
            return APR_EOF;
        default:
            return GetLastError();
        }
    }
    if (bread == 0) {
        thefile->eof_hit = TRUE;
        return APR_EOF;
    }
    for (i=0; i<len; i++) {
        if (str[i] == '\n') {
            ++i;
            if (i < len)
                str[i] = '\0';
            else
                str [--i] = '\0';
            SetFilePointer(thefile->filehand, (i - bread), NULL, FILE_CURRENT);
            return APR_SUCCESS;
        }
    }
    str[i] = '\0';
    return APR_SUCCESS; 
}
ap_status_t ap_flush(ap_file_t *thefile)
{
    FlushFileBuffers(thefile->filehand);
    return APR_SUCCESS; 
}

static int printf_flush(ap_vformatter_buff_t *vbuff)
{
    /* I would love to print this stuff out to the file, but I will
     * get that working later.  :)  For now, just return.
     */
    return -1;
}

API_EXPORT(int) ap_fprintf(ap_file_t *fptr, const char *format, ...)
{
    int cc;
    va_list ap;
    char *buf;
    int len;

    buf = malloc(HUGE_STRING_LEN);
    if (buf == NULL) {
        return 0;
    }
    va_start(ap, format);
    len = ap_vsnprintf(buf, HUGE_STRING_LEN, format, ap);
    cc = ap_puts(buf, fptr);
    va_end(ap);
    free(buf);
    return (cc == APR_SUCCESS) ? len : -1;
}


