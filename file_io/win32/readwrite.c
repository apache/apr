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

#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_errno.h"
#include <windows.h>

#define GetFilePointer(hfile) SetFilePointer(hfile,0,NULL, FILE_CURRENT)

ap_status_t ap_read(struct file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    DWORD bread;
    int lasterror;

    if (thefile->filehand == INVALID_HANDLE_VALUE) {
        *nbytes = -1;
        return APR_EBADF;
    }
    
    if (ReadFile(thefile->filehand, buf, *nbytes, &bread, NULL)) {
        *nbytes = bread;
        return APR_SUCCESS;
    }
    *nbytes = -1;
    lasterror = GetLastError();
    return APR_EEXIST;
}

ap_status_t ap_write(struct file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    DWORD bwrote;
    FILETIME atime, mtime, ctime;
	
    if (thefile->filehand == INVALID_HANDLE_VALUE) {
        *nbytes = -1;
        return APR_EBADF;
    }

    if (WriteFile(thefile->filehand, buf, *nbytes, &bwrote, NULL)) {
        if (strcmp(thefile->fname, "PIPE")) {
            FlushFileBuffers(thefile->filehand);
            thefile->size = GetFileSize(thefile->filehand, NULL);
            GetFileTime(thefile->filehand, &ctime, &atime, &mtime);
            thefile->atime = WinTimeToUnixTime(&atime);
            thefile->mtime = WinTimeToUnixTime(&mtime);
            thefile->ctime = WinTimeToUnixTime(&ctime);
        }
        *nbytes = bwrote;
        return APR_SUCCESS;
    }
    (*nbytes) = -1;
    return APR_EEXIST;
}
/*
 * Too bad WriteFileGather() is not supported on 95&98 (or NT prior to SP2) 
 */
ap_status_t ap_writev(struct file_t *thefile, const struct iovec_t *vec, ap_ssize_t *iocnt)
{
    int i;
    DWORD bwrote = 0;
    int numvec = *iocnt;
    *iocnt = 0;

    for (i = 0; i < numvec; i++) {
        if (!WriteFile(thefile->filehand,
                       vec->iov[i].iov_base, vec->iov[i].iov_len, &bwrote, NULL)) {
            return GetLastError(); /* TODO: Yes, I know this is broken... */
        }
        *iocnt += bwrote;
    }
    return APR_SUCCESS;
}

ap_status_t ap_putc(char ch, ap_file_t *thefile)
{
    DWORD bwrote;

    if (!WriteFile(thefile->filehand, &ch, 1, &bwrote, NULL)) {
        return APR_EEXIST;
    }
    return APR_SUCCESS; 
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
        return !APR_SUCCESS; /* is there no generic failure code? */
    }
    /* that's buffered... */
    if (!thefile->buffered) {
        return !APR_SUCCESS; /* is there no generic failure code? */
    }
    /* and the file pointer is not pointing to the start of the file. */
    if (GetFilePointer(thefile->filehand)) {
        if (SetFilePointer(thefile->filehand, -1, NULL, FILE_CURRENT) 
            == 0xFFFFFFFF) {
            return !APR_SUCCESS;
        }
    }

    thefile->stated = 0;
    return APR_SUCCESS; 
}

ap_status_t ap_getc(char *ch, ap_file_t *thefile)
{
    DWORD bread;
    if (!ReadFile(thefile->filehand, ch, 1, &bread, NULL)) {
        return APR_EEXIST;
    }
    if (bread == 0) {
        thefile->eof_hit = TRUE;
        return APR_EOF;
    }
    return APR_SUCCESS; 
}

ap_status_t ap_puts(char *str, ap_file_t *thefile)
{
    DWORD bwrote;
    int len;

    len = strlen(str);
    str[len] = '\n';
    if (!WriteFile(thefile->filehand, str, len+1, &bwrote, NULL)) {
        str[len] = '\0';
        return APR_EEXIST;
    }
    str[len] = '\0';

    return APR_SUCCESS; 
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
            return APR_EEXIST;
        }
    }
    if (bread == 0) {
        thefile->eof_hit = TRUE;
        return APR_EOF;
    }
    for (i=0; i<len; i++) {
        if (str[i] == '\n') {
            str[i] = '\0';
            return APR_SUCCESS;
        }
        str[i] = '\0';
    }
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

API_EXPORT(int) ap_fprintf(struct file_t *fptr, const char *format, ...)
{
    int cc;
    va_list ap;
    ap_vformatter_buff_t vbuff;
    char *buf;
    int len;

    buf = malloc(HUGE_STRING_LEN);
    if (buf == NULL) {
        return 0;
    }
    /* save one byte for nul terminator */
    vbuff.curpos = buf;
    vbuff.endpos = buf + len - 1;
    va_start(ap, format);
#if 0
    cc = ap_vformatter(printf_flush, &vbuff, format, ap);
    va_end(ap);
    *vbuff.curpos = '\0';
#endif
    vsprintf(buf, format, ap);
    len = strlen(buf);
    cc = ap_write(fptr, buf, &len);
    va_end(ap);
    return (cc == -1) ? len : cc;
}


