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
#include "apr_lib.h"

#define INCL_DOS
#include <os2.h>

ap_status_t ap_read(struct file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ULONG rc;
    ULONG bytesread;

    if (!thefile->isopen) {
        *nbytes = 0;
        return APR_EBADF;
    }

    rc = DosRead(thefile->filedes, buf, *nbytes, &bytesread);

    if (rc) {
        *nbytes = 0;
        return os2errno(rc);
    }
    
    if (bytesread == 0) {
        thefile->eof_hit = TRUE;
        return APR_EOF;
    }
    
    *nbytes = bytesread;
    return APR_SUCCESS;
}



ap_status_t ap_write(struct file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ULONG rc;
    ULONG byteswritten;

    if (!thefile->isopen) {
        *nbytes = 0;
        return APR_EBADF;
    }

    rc = DosWrite(thefile->filedes, buf, *nbytes, &byteswritten);

    if (rc) {
        *nbytes = 0;
        return os2errno(rc);
    }
    
    *nbytes = byteswritten;
    thefile->validstatus = FALSE;
    return APR_SUCCESS;
}



#ifdef HAVE_WRITEV

ap_status_t ap_make_iov(struct iovec_t **new, struct iovec *iova, ap_context_t *cntxt)
{
    (*new) = ap_palloc(cntxt, sizeof(struct iovec_t));
    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cntxt;
    (*new)->theiov = iova;
    return APR_SUCCESS;
}



ap_status_t ap_writev(struct file_t *thefile, const struct iovec_t *vec, ap_ssize_t *iocnt)
{
    int bytes;
    if ((bytes = writev(thefile->filedes, vec->theiov, *iocnt)) < 0) {
        *iocnt = bytes;
        return errno;
    }
    else {
        *iocnt = bytes;
        thefile->validstatus = FALSE;
        return APR_SUCCESS;
    }
}
#endif



ap_status_t ap_putc(char ch, ap_file_t *thefile)
{
    ULONG rc;
    ULONG byteswritten;

    if (!thefile->isopen) {
        return APR_EBADF;
    }

    rc = DosWrite(thefile->filedes, &ch, 1, &byteswritten);

    if (rc) {
        return os2errno(rc);
    }
    
    thefile->validstatus = FALSE;
    return APR_SUCCESS;
}



ap_status_t ap_ungetc(char ch, ap_file_t *thefile)
{
    /* Not sure what to do in this case.  For now, return SUCCESS. */
    return APR_SUCCESS;
}



ap_status_t ap_getc(char *ch, ap_file_t *thefile)
{
    ULONG rc;
    ULONG bytesread;

    if (!thefile->isopen) {
        return APR_EBADF;
    }

    rc = DosRead(thefile->filedes, ch, 1, &bytesread);

    if (rc) {
        return os2errno(rc);
    }
    
    if (bytesread == 0) {
        thefile->eof_hit = TRUE;
        return APR_EOF;
    }
    
    return APR_SUCCESS;
}



ap_status_t ap_puts(char *str, ap_file_t *thefile)
{
    ap_ssize_t len;

    len = strlen(str);
    return ap_write(thefile, str, &len); 
}



ap_status_t ap_flush(ap_file_t *thefile)
{
    /* There isn't anything to do if we aren't buffering the output
     * so just return success.
     */
    return APR_SUCCESS; 
}



ap_status_t ap_fgets(char *str, int len, ap_file_t *thefile)
{
    ssize_t readlen;
    ap_status_t rv;
    int i;    

    for (i = 0; i < len-1; i++) {
        readlen = 1;
        rv = ap_read(thefile, str+i, &readlen);
        
        if (rv != APR_SUCCESS) {
            return rv;
        }
        
        if (str[i] == '\r')
            i--;
        else if (str[i] == '\n')
            break;
    }
    str[i] = 0;
    return APR_SUCCESS; 
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
    vsprintf(buf, format, ap);
    len = strlen(buf);
    cc = ap_write(fptr, buf, &len);
    va_end(ap);
    return (cc == -1) ? len : cc;
}


