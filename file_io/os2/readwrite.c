/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
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
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "fileio.h"
#include "apr_file_io.h"
#include "apr_lib.h"

#define INCL_DOS
#include <os2.h>
#include <malloc.h>

ap_status_t ap_read(struct file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ULONG rc = 0;
    ULONG bytesread;

    if (!thefile->isopen) {
        *nbytes = 0;
        return APR_EBADF;
    }

    if (thefile->buffered) {
        char *pos = (char *)buf;
        ULONG blocksize;
        ULONG size = *nbytes;

        DosEnterCritSec();

        if (thefile->direction == 1) {
            ap_flush(thefile);
            thefile->bufpos = 0;
            thefile->direction = 0;
            thefile->dataRead = 0;
        }

        while (rc == 0 && size > 0) {
            if (thefile->bufpos >= thefile->dataRead) {
                rc = DosRead(thefile->filedes, thefile->buffer, APR_FILE_BUFSIZE, &thefile->dataRead );
                if (thefile->dataRead == 0) {
                    if (rc == 0)
                        thefile->eof_hit = TRUE;
                    break;
                }
                thefile->filePtr += thefile->dataRead;
                thefile->bufpos = 0;
            }

            blocksize = size > thefile->dataRead - thefile->bufpos ? thefile->dataRead - thefile->bufpos : size;
            memcpy(pos, thefile->buffer + thefile->bufpos, blocksize);
            thefile->bufpos += blocksize;
            pos += blocksize;
            size -= blocksize;
        }

        *nbytes = rc == 0 ? pos - (char *)buf : 0;
        DosExitCritSec();
        return os2errno(rc);
    } else {
        rc = DosRead(thefile->filedes, buf, *nbytes, &bytesread);

        if (rc) {
            *nbytes = 0;
            return os2errno(rc);
        }

        *nbytes = bytesread;
        
        if (bytesread == 0) {
            thefile->eof_hit = TRUE;
        }

        return APR_SUCCESS;
    }
}



ap_status_t ap_write(struct file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ULONG rc = 0;
    ULONG byteswritten;

    if (!thefile->isopen) {
        *nbytes = 0;
        return APR_EBADF;
    }

    if (thefile->buffered) {
        char *pos = (char *)buf;
        int blocksize;
        int size = *nbytes;

        DosEnterCritSec();

        if ( thefile->direction == 0 ) {
            // Position file pointer for writing at the offset we are logically reading from
            ULONG offset = thefile->filePtr - thefile->dataRead + thefile->bufpos;
            if (offset != thefile->filePtr)
                DosSetFilePtr(thefile->filedes, offset, FILE_BEGIN, &thefile->filePtr );
            thefile->bufpos = thefile->dataRead = 0;
            thefile->direction = 1;
        }

        while (rc == 0 && size > 0) {
            if (thefile->bufpos == APR_FILE_BUFSIZE)   // write buffer is full
                ap_flush(thefile);

            blocksize = size > APR_FILE_BUFSIZE - thefile->bufpos ? APR_FILE_BUFSIZE - thefile->bufpos : size;
            memcpy(thefile->buffer + thefile->bufpos, pos, blocksize);
            thefile->bufpos += blocksize;
            pos += blocksize;
            size -= blocksize;
        }

        DosExitCritSec();
        return os2errno(rc);
    } else {
        rc = DosWrite(thefile->filedes, buf, *nbytes, &byteswritten);

        if (rc) {
            *nbytes = 0;
            return os2errno(rc);
        }

        *nbytes = byteswritten;
        return APR_SUCCESS;
    }
}



#ifdef HAVE_WRITEV

ap_status_t ap_writev(struct file_t *thefile, const struct iovec *vec, ap_size_t nvec, ap_ssize_t *nbytes)
{
    int bytes;
    if ((bytes = writev(thefile->filedes, vec, nvec)) < 0) {
        *nbytes = 0;
        return errno;
    }
    else {
        *nbytes = bytes;
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
    int bytesread;

    if (!thefile->isopen) {
        return APR_EBADF;
    }

    bytesread = 1;
    rc = ap_read(thefile, ch, &bytesread);

    if (rc) {
        return rc;
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
    if (thefile->buffered) {
        ULONG written = 0;
        int rc = 0;

        if (thefile->direction == 1 && thefile->bufpos) {
            rc = DosWrite(thefile->filedes, thefile->buffer, thefile->bufpos, &written);
            thefile->filePtr += written;

            if (rc == 0)
                thefile->bufpos = 0;
        }

        return os2errno(rc);
    } else {
        /* There isn't anything to do if we aren't buffering the output
         * so just return success.
         */
        return APR_SUCCESS;
    }
}



ap_status_t ap_fgets(char *str, int len, ap_file_t *thefile)
{
    ssize_t readlen;
    ap_status_t rv = APR_SUCCESS;
    int i;    

    for (i = 0; i < len-1; i++) {
        readlen = 1;
        rv = ap_read(thefile, str+i, &readlen);

        if (readlen != 1) {
            rv = APR_EOF;
            break;
        }
        
        if (str[i] == '\r')
            i--;
        else if (str[i] == '\n')
            break;
    }
    str[i] = 0;
    return rv;
}



API_EXPORT(int) ap_fprintf(struct file_t *fptr, const char *format, ...)
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


