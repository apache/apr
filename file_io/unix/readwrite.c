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

static ap_status_t wait_for_io_or_timeout(ap_file_t *file, int for_read)
{
    struct timeval tv;
    fd_set fdset;
    int srv;

    do {
        FD_ZERO(&fdset);
        FD_SET(file->filedes, &fdset);
        tv.tv_sec = file->timeout;
        tv.tv_usec = 0;
        srv = select(FD_SETSIZE,
            for_read ? &fdset : NULL,
            for_read ? NULL : &fdset,
            NULL,
            file->timeout < 0 ? NULL : &tv);
    } while (srv == -1 && errno == EINTR);

    if (srv == 0) {
        return APR_TIMEUP;
    }
    else if (srv < 0) {
        return errno;
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_read(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
 *    Read data from the specified file.
 * arg 1) The file descriptor to read from.
 * arg 2) The buffer to store the data to.
 * arg 3) The number of bytes to read.
 * NOTE:  ap_read will read up to the specified number of bytes, but never
 * more.  If there isn't enough data to fill that number of bytes, all of
 * the available data is read.  The third argument is modified to reflect the
 * number of bytes read.  If a char was put back into the stream via
 * ungetc, it will be the first character returned. 
 */
ap_status_t ap_read(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ap_ssize_t rv;
    int used_unget = FALSE;

    if(thefile == NULL || nbytes == NULL || (buf == NULL && *nbytes != 0))
        return APR_EBADARG;

    if(*nbytes <= 0) {
        *nbytes = 0;
	return APR_SUCCESS;
    }

    if (thefile->ungetchar != -1) {
        used_unget = TRUE;
        *(char *)buf++ = (char)thefile->ungetchar;
        (*nbytes)--;
        thefile->ungetchar = -1;
    }
	  
    do {
        rv = read(thefile->filedes, buf, *nbytes);
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && 
        (errno == EAGAIN || errno == EWOULDBLOCK) && 
        thefile->timeout != 0) {
        ap_status_t arv = wait_for_io_or_timeout(thefile, 1);
        if (arv != APR_SUCCESS) {
            *nbytes = 0;
            return arv;
        }
        else {
            do {
                rv = read(thefile->filedes, buf, *nbytes);
            } while (rv == -1 && errno == EINTR);
        }
    }  
    /* getting less data than requested does not signify an EOF when
       dealing with a pipe.
     */
    if ((*nbytes != rv) && ((errno == EPIPE && thefile->pipe == 1)
        || (errno != EINTR && thefile->pipe == 0 ))) {
        thefile->eof_hit = 1;
    }
    if (rv == -1) {
        (*nbytes) = 0;
        return errno;
    }
    *nbytes = rv;
    if (used_unget) {
        thefile->ungetchar = -1;
        *nbytes += 1;
    }
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
ap_status_t ap_write(ap_file_t *thefile, void *buf, ap_ssize_t *nbytes)
{
    ap_size_t rv;

    if(thefile == NULL || nbytes == NULL || (buf == NULL && *nbytes != 0))
        return APR_EBADARG;

    do {
        rv = write(thefile->filedes, buf, *nbytes);
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && 
        (errno == EAGAIN || errno == EWOULDBLOCK) && 
        thefile->timeout != 0) {
        ap_status_t arv = wait_for_io_or_timeout(thefile, 0);
        if (arv != APR_SUCCESS) {
            *nbytes = 0;
            return arv;
        }
        else {
            do {
                rv = write(thefile->filedes, buf, *nbytes);
            } while (rv == -1 && errno == EINTR);
        }
    }  

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
ap_status_t ap_writev(ap_file_t *thefile, const struct iovec *vec,
                      ap_size_t nvec, ap_ssize_t *nbytes)
{
    int bytes;

    if(thefile == NULL || vec == NULL || nvec < 0 || nbytes == NULL)
        return APR_EBADARG;

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
    if(thefile == NULL)
        return APR_EBADARG;

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
    if(thefile == NULL)
        return APR_EBADARG;

    thefile->ungetchar = (unsigned char)ch;
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
    
    if(thefile == NULL || ch == NULL)
        return APR_EBADARG;

    if (thefile->ungetchar != -1) {
        *ch = (char) thefile->ungetchar;
        thefile->ungetchar = -1;
        return APR_SUCCESS;
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

    if(thefile == NULL || str == NULL)
        return APR_EBADARG;

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
/* Another function to get rid of once we finish removing buffered I/O
 * and we are sure nobody is using it.
 */
    if(thefile == NULL)
        return APR_EBADARG;

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
    int i, used_unget = FALSE, beg_idx;

    if(thefile == NULL || str == NULL || len < 0)
        return APR_EBADARG;

    if(len <= 1)  /* as per fgets() */
        return APR_SUCCESS;

    if(thefile->ungetchar != -1){
        str[0] = thefile->ungetchar;
	used_unget = TRUE;
	beg_idx = 1;
	if(str[0] == '\n' || str[0] == '\r'){
	    thefile->ungetchar = -1;
	    str[1] = '\0';
	    return APR_SUCCESS;
	}
    } else
        beg_idx = 0;
    
    for (i = beg_idx; i < len; i++) {
        rv = read(thefile->filedes, &str[i], 1); 
        if (rv == 0) {
            thefile->eof_hit = TRUE;
	    if(used_unget) thefile->filedes = -1;
	    str[i] = '\0';
            return APR_EOF;
        }
        else if (rv != 1) {
            return errno;
        }
        if (str[i] == '\n' || str[i] == '\r')
            break;
    }
    if (i < len-1)
        str[i+1] = '\0';
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

API_EXPORT(int) ap_fprintf(ap_file_t *fptr, const char *format, ...)
{
    int cc;
    va_list ap;
    char *buf;
    int len;

    if(fptr == NULL || format == NULL)
        return APR_EBADARG;

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

