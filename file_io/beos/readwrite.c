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

#include <errno.h>
#include <unistd.h>
#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_errno.h"
#include "apr_lib.h"

ap_status_t ap_read(struct file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ap_size_t rv;

    if (thefile->filedes < 0) {
        *nbytes = -1;
        return APR_EBADF;
    }

    rv = read(thefile->filedes, buf, *nbytes);

    *nbytes = rv;
    return APR_SUCCESS;
}

ap_status_t ap_write(struct file_t *thefile, void * buf, ap_ssize_t *nbytes)
{
    ap_size_t rv;
    struct stat info;

    if (thefile->filedes < 0) {
        *nbytes = -1;
        return APR_EBADF;
    }

    rv = write(thefile->filedes, buf, *nbytes);

    if (stat(thefile->fname, &info) == 0) {
        thefile->size = info.st_size;
        thefile->atime = info.st_atime;
        thefile->mtime = info.st_mtime;
        thefile->ctime = info.st_ctime;
    }

    *nbytes = rv;
    return APR_SUCCESS;
}	

ap_status_t ap_writev(struct file_t *thefile, const struct iovec_t *vec, ap_ssize_t *iocnt)
{
	ap_ssize_t bytes;
	if ((bytes = writev(thefile->filedes, vec->iovec, *iocnt)) < 0){
		*iocnt = bytes;
		return errno;
	}
	else {
	    *iocnt = bytes;
		return APR_SUCCESS;
	}
}

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

ap_status_t ap_gets(ap_file_t *thefile, char *str, int len)
{
    ssize_t rv;
    int i;    

    if (thefile->buffered) {
        if (fgets(str, len, thefile->filehand)) {
            return APR_SUCCESS;
        }
        if (feof(thefile->filehand)) {
            return APR_EOF;
        }
        return errno;
    }
    for (i = 0; i < len; i++) {
        rv = read(thefile->filedes, &str[i], 1); 
        if (rv == 0) {
            thefile->eof_hit = TRUE;
            return APR_EOF;
        }
        else if (rv != 1) {
            return errno;
        }
        if (str[i] == '\n' || str[i] == '\r')
            break;
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
