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
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

/* ***APRDOC********************************************************
 * ap_status_t ap_read(ap_file_t *, void *, ap_ssize_t *)
 *    Read data from the specified file.
 * arg 1) The file descriptor to read from.
 * arg 2) The buffer to store the data to.
 * arg 3) The number of bytes to read.
 * NOTE:  ap_read will read up to the specified number of bytes, but never
 * more.  If there isn't enough data to fill that number of bytes, all of
 * the available data is read.  The third argument is modified to reflect the
 * number of bytes read. 
 */
ap_status_t ap_read(struct file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ap_ssize_t rv;

    if (thefile->filedes < 0) {
        *nbytes = -1;
        return APR_EBADF;
    }
    
    if (thefile->buffered) {
        rv = fread(buf, *nbytes, 1, thefile->filehand);
    }
    else {
        rv = read(thefile->filedes, buf, *nbytes);
    }

    if ((*nbytes != rv) && (errno != EINTR) && !thefile->buffered) {
        thefile->eof_hit = 1;
    }
    *nbytes = rv;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_write(ap_file_t *, void *, ap_ssize_t *)
 *    Write data to the specified file.
 * arg 1) The file descriptor to write to.
 * arg 2) The buffer which contains the data.
 * arg 3) The number of bytes to write.
 * NOTE:  ap_write will write up to the specified number of bytes, but never
 * more.  If the OS cannot write that many bytes, it will write as many as it
 * can.  The third argument is modified to reflect the * number of bytes 
 * written. 
 */
ap_status_t ap_write(struct file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ap_size_t rv;
    struct stat info;

    if (thefile->filedes < 0) {
        *nbytes = -1;
        return APR_EBADF;
    }

    if (thefile->buffered) {
        rv = fwrite(buf, *nbytes, 1, thefile->filehand);
    }
    else {
        rv = write(thefile->filedes, buf, *nbytes);
    }

    if (strcmp(thefile->fname, "PIPE")) {
        if (stat(thefile->fname, &info) == 0) {
            thefile->size = info.st_size;
            thefile->atime = info.st_atime;
            thefile->mtime = info.st_mtime;
            thefile->ctime = info.st_ctime;
        }
    }
    *nbytes = rv;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_writev(ap_file_t *, ap_iovec_t *, ap_ssize_t *)
 *    Write data from ap_iovec array to the specified file.
 * arg 1) The file descriptor to write to.
 * arg 2) The array from which to get the data to write to the file.
 * arg 3) The number of elements in the ap_iovec array.  This must be
 *        smaller than AP_MAX_IOVEC_SIZE.  If it isn't, the function will
 *        fail with APR_EINVAL.
 * NOTE:  The third arguement is updated with the number of bytes actually 
 *        written on function exit. 
 */
#ifdef HAVE_WRITEV
ap_status_t ap_writev(struct file_t *thefile, const struct iovec_t *vec, ap_ssize_t *iocnt)
{
    int bytes;
    if ((bytes = writev(thefile->filedes, vec->iovec, *iocnt)) < 0) {
        *iocnt = bytes;
        return errno;
    }
    else {
        *iocnt = bytes;
        return APR_SUCCESS;
    }
}
#endif

/* ***APRDOC********************************************************
 * ap_status_t ap_putc(ap_file_t *, char)
 *    put a character into the specified file.
 * arg 1) The file descriptor to write to
 * arg 2) The character to write.
 */
ap_status_t ap_putc(ap_file_t *thefile, char ch)
{
    if (thefile->buffered) {
        if (fputc(ch, thefile->filehand) == ch) {
            return APR_SUCCESS;
        }
        return errno;
    }
    if (write(thefile->filedes, &ch, 1) != 1) {
        return errno;
    }
    return APR_SUCCESS; 
}

/* ***APRDOC********************************************************
 * ap_status_t ap_getc(ap_file_t *, char *)
 *    put a character into the specified file.
 * arg 1) The file descriptor to write to
 * arg 2) The character to write.
 */
ap_status_t ap_getc(ap_file_t *thefile, char *ch)
{
    ssize_t rv;
    
    if (thefile->buffered) {
        if ((*ch) = fgetc(thefile->filehand)) {
            return APR_SUCCESS;
        }
        if (feof(thefile->filehand)) {
            return APR_EOF;
        }
        return errno;
    }
    rv = read(thefile->filedes, ch, 1); 
    if (rv == 0) {
        thefile->eof_hit = TRUE;
        return APR_EOF;
    }
    else if (rv != 1) {
        return errno;
    }
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


