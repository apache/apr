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

#include "../unix/fileio.h"

ap_status_t ap_read(struct ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ap_ssize_t rv;

    if (thefile->filedes < 0) {
        (*nbytes) = 0;
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
    if (rv == -1) {
        *nbytes = 0;
        return errno;
    }
    (*nbytes) = rv;
    return APR_SUCCESS;
}

ap_status_t ap_write(struct ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ap_size_t rv;

    if (thefile->filedes < 0) {
        (*nbytes) = 0;
        return APR_EBADF;
    }

    if (thefile->buffered) {
        rv = fwrite(buf, *nbytes, 1, thefile->filehand);
    }
    else {
        rv = write(thefile->filedes, buf, *nbytes);
    }

    if (rv == -1) {
        (*nbytes) = 0;
        return errno;
    }
    (*nbytes) = rv;
    return APR_SUCCESS;
}

ap_status_t ap_writev(struct ap_file_t *thefile, const struct iovec *vec, 
                      ap_size_t nvec, ap_ssize_t *nbytes)
{
    int bytes;
    if ((bytes = writev(thefile->filedes, vec, nvec)) < 0) {
        (*nbytes) = 0;
        return errno;
    }
    else {
        (*nbytes) = bytes;
        return APR_SUCCESS;
    }
}

ap_status_t ap_putc(char ch, ap_file_t *thefile)
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

ap_status_t ap_ungetc(char ch, ap_file_t *thefile)
{
    if (thefile->buffered) {
        if (ungetc(ch, thefile->filehand) == ch) {
            return APR_SUCCESS;
        }
        return errno;
    }
    /* Not sure what to do in this case.  For now, return SUCCESS. */
    return APR_SUCCESS; 
}

ap_status_t ap_getc(char *ch, ap_file_t *thefile)
{
    ssize_t rv;
    
    if (thefile->buffered) {
        int r;

	r=fgetc(thefile->filehand);
	if(r != EOF)
	    {
	    *ch=(char)r;
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

ap_status_t ap_puts(char *str, ap_file_t *thefile)
{
    ssize_t rv;
    int len;

    if (thefile->buffered) {
        if (fputs(str, thefile->filehand)) {
            return APR_SUCCESS;
        }
        return errno;
    }
    len = strlen(str);
    rv = write(thefile->filedes, str, len); 
    if (rv != len) {
        return errno;
    }
    return APR_SUCCESS; 
}

ap_status_t ap_flush(ap_file_t *thefile)
{
    if (thefile->buffered) {
        if (!fflush(thefile->filehand)) {
            return APR_SUCCESS;
        }
        return errno;
    }
    /* There isn't anything to do if we aren't buffering the output
     * so just return success.
     */
    return APR_SUCCESS; 
}

ap_status_t ap_fgets(char *str, int len, ap_file_t *thefile)
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

#if 0 /* not currently used */
static int printf_flush(ap_vformatter_buff_t *vbuff)
{
    /* I would love to print this stuff out to the file, but I will
     * get that working later.  :)  For now, just return.
     */
    return -1;
}
#endif

API_EXPORT(int) ap_fprintf(struct ap_file_t *fptr, const char *format, ...)
{
    int cc;
    va_list ap;
    ap_vformatter_buff_t vbuff;
    char *buf;
    ap_ssize_t len;

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


