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

/* ***APRDOC********************************************************
 * ap_status_t ap_read(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
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

    if (thefile->filedes < 0 && !thefile->buffered) {
        *nbytes = 0;
        return APR_EBADF;
    }
    
    if (thefile->buffered) {
        rv = fread(buf, *nbytes, 1, thefile->filehand);
    }
    else {
        do {
            rv = read(thefile->filedes, buf, *nbytes);
        } while (rv == -1 && errno == EINTR);

        if (rv == -1 && errno == EAGAIN && thefile->timeout != 0) {
            struct timeval *tv;
            fd_set fdset;
            int srv;

            do {
                FD_ZERO(&fdset);
                FD_SET(thefile->filedes, &fdset);
                if (thefile->timeout == -1) {
                    tv = NULL;
                }
                else {
                    tv = ap_palloc(thefile->cntxt, sizeof(struct timeval));
                    tv->tv_sec  = thefile->timeout;
                    tv->tv_usec = 0;
                }

                srv = select(FD_SETSIZE, &fdset, NULL, NULL, tv);
            } while (srv == -1 && errno == EINTR);

            if (srv == 0) {
                (*nbytes) = 0;
                return APR_TIMEUP;
            }
            else if (srv < 0) {
                (*nbytes) = 0;
                return errno;
            }
            else {
                do {
                    rv = read(thefile->filedes, buf, *nbytes);
                } while (rv == -1 && errno == EINTR);
            }
        }  
    }  /* buffered? */

    /* getting less data than requested does not signify an EOF when
       dealing with a pipe.
     */
    if ((*nbytes != rv) && ((errno == EPIPE && thefile->pipe == 1)
        || (errno != EINTR && !thefile->buffered && thefile->pipe == 0 ))) {
        thefile->eof_hit = 1;
    }
    if (rv == -1) {
        (*nbytes) = 0;
        return errno;
    }
    *nbytes = rv;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_write(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
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

    if (thefile->filedes < 0) {
        *nbytes = 0;
        return APR_EBADF;
    }

    if (thefile->buffered) {
        rv = fwrite(buf, *nbytes, 1, thefile->filehand);
    }
    else {
        do {
            rv = write(thefile->filedes, buf, *nbytes);
        } while (rv == -1 && errno == EINTR);

        if (rv == -1 && errno == EAGAIN && thefile->timeout != 0) {
            struct timeval *tv;
            fd_set fdset;
            int srv;

            do {
                FD_ZERO(&fdset);
                FD_SET(thefile->filedes, &fdset);
                if (thefile->timeout == -1) {
                    tv = NULL;
                }
                else {
                    tv = ap_palloc(thefile->cntxt, sizeof(struct timeval));
                    tv->tv_sec  = thefile->timeout;
                    tv->tv_usec = 0;
                }

                srv = select(FD_SETSIZE, NULL, &fdset, NULL, tv);
            } while (srv == -1 && errno == EINTR);

            if (srv == 0) {
                (*nbytes) = 0;
                return APR_TIMEUP;
            }
            else if (srv < 0) {
                (*nbytes) = 0;
                return errno;
            }
            else {
                do {
                    rv = write(thefile->filedes, buf, *nbytes);
                } while (rv == -1 && errno == EINTR);
            }
        }  
    }   /* BUFFERED ?? */

    if (rv == -1) {
        (*nbytes) = 0;
        return errno;
    }
    *nbytes = rv;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_writev(ap_file_t *thefile, struct iovec *vec, ap_size_t nvec,
 *                       ap_ssize_t *nbytes)
 *    Write data from iovec array to the specified file.
 * arg 1) The file descriptor to write to.
 * arg 2) The array from which to get the data to write to the file.
 * arg 3) The number of elements in the struct iovec array. This must be
 *        smaller than AP_MAX_IOVEC_SIZE.  If it isn't, the function will
 *        fail with APR_EINVAL.
 * arg 4) The number of bytes written.
 */
#ifdef HAVE_WRITEV
ap_status_t ap_writev(struct file_t *thefile, const struct iovec *vec,
                      ap_size_t nvec, ap_ssize_t *nbytes)
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

/* ***APRDOC********************************************************
 * ap_status_t ap_putc(char ch, ap_file_t *thefile)
 *    put a character into the specified file.
 * arg 1) The character to write.
 * arg 2) The file descriptor to write to
 */
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

/* ***APRDOC********************************************************
 * ap_status_t ap_ungetc(char ch, ap_file_t *thefile)
 *    put a character back onto a specified stream.
 * arg 1) The character to write.
 * arg 2) The file descriptor to write to
 */
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

/* ***APRDOC********************************************************
 * ap_status_t ap_getc(char *ch, ap_file_t *thefil)
 *    get a character from the specified file.
 * arg 1) The character to write.
 * arg 2) The file descriptor to write to
 */
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

/* ***APRDOC********************************************************
 * ap_status_t ap_puts(char *str, ap_file_t *thefile)
 *    Put the string into a specified file.
 * arg 1) The string to write. 
 * arg 2) The file descriptor to write to from
 */
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

/* ***APRDOC********************************************************
 * ap_status_t ap_flush(ap_file_t *thefile)
 *    Flush the file's buffer.
 * arg 1) The file descriptor to flush
 */
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

/* ***APRDOC********************************************************
 * ap_status_t ap_fgets(char *str, int len, ap_file_t *thefile)
 *    Get a string from a specified file.
 * arg 1) The buffer to store the string in. 
 * arg 2) The length of the string
 * arg 3) The file descriptor to read from
 */
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

