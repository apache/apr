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
#include "apr_lock.h"

/* The only case where we don't use wait_for_io_or_timeout is on
 * pre-BONE BeOS, so this check should be sufficient and simpler */
#if !BEOS_R5
#define USE_WAIT_FOR_IO
#endif

#ifdef USE_WAIT_FOR_IO
static apr_status_t wait_for_io_or_timeout(apr_file_t *file, int for_read)
{
    struct timeval tv, *tvptr;
    fd_set fdset;
    int srv;

    /* TODO - timeout should be less each time through this loop */

    do {
        FD_ZERO(&fdset);
        FD_SET(file->filedes, &fdset);
        if (file->timeout >= 0) {
            tv.tv_sec = file->timeout / APR_USEC_PER_SEC;
            tv.tv_usec = file->timeout % APR_USEC_PER_SEC;
            tvptr = &tv;
        }
        else {
            tvptr = NULL;
        }
        srv = select(FD_SETSIZE,
            for_read ? &fdset : NULL,
            for_read ? NULL : &fdset,
            NULL,
            tvptr);
    } while (srv == -1 && errno == EINTR);

    if (srv == 0) {
        return APR_TIMEUP;
    }
    else if (srv < 0) {
        return errno;
    }
    return APR_SUCCESS;
}
#endif

/* problems: 
 * 1) ungetchar not used for buffered files
 */
apr_status_t apr_read(apr_file_t *thefile, void *buf, apr_size_t *nbytes)
{
    apr_ssize_t rv;
    apr_size_t bytes_read;

    if (*nbytes <= 0) {
        *nbytes = 0;
	return APR_SUCCESS;
    }

    if (thefile->buffered) {
        char *pos = (char *)buf;
        apr_uint64_t blocksize;
        apr_uint64_t size = *nbytes;

#if APR_HAS_THREADS
        apr_lock(thefile->thlock);
#endif

        if (thefile->direction == 1) {
            apr_flush(thefile);
            thefile->bufpos = 0;
            thefile->direction = 0;
            thefile->dataRead = 0;
        }

	rv = 0;
        while (rv == 0 && size > 0) {
            if (thefile->bufpos >= thefile->dataRead) {
                thefile->dataRead = read(thefile->filedes, thefile->buffer, APR_FILE_BUFSIZE);
                if (thefile->dataRead == 0) {
                    thefile->eof_hit = TRUE;
                    rv = APR_EOF;
                    break;
                }
                else if (thefile->dataRead == -1) {
                    rv = errno;
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

        *nbytes = pos - (char *)buf;
        if (*nbytes) {
            rv = 0;
        }
#if APR_HAS_THREADS
        apr_unlock(thefile->thlock);
#endif
        return rv;
    }
    else {
        bytes_read = 0;
        if (thefile->ungetchar != -1) {
            bytes_read = 1;
            *(char *)buf = (char)thefile->ungetchar;
            buf = (char *)buf + 1;
            (*nbytes)--;
            thefile->ungetchar = -1;
            if (*nbytes == 0) {
	        *nbytes = bytes_read;
	        return APR_SUCCESS;
            }
        }

        do {
            rv = read(thefile->filedes, buf, *nbytes);
        } while (rv == -1 && errno == EINTR);
#ifdef USE_WAIT_FOR_IO
        if (rv == -1 && 
            (errno == EAGAIN || errno == EWOULDBLOCK) && 
            thefile->timeout != 0) {
            apr_status_t arv = wait_for_io_or_timeout(thefile, 1);
            if (arv != APR_SUCCESS) {
                *nbytes = bytes_read;
                return arv;
            }
            else {
                do {
                    rv = read(thefile->filedes, buf, *nbytes);
                } while (rv == -1 && errno == EINTR);
            }
        }  
#endif
        *nbytes = bytes_read;
        if (rv == 0) {
            return APR_EOF;
        }
        if (rv > 0) {
            *nbytes += rv;
            return APR_SUCCESS;
        }
        return errno;
    }
}

apr_status_t apr_write(apr_file_t *thefile, const void *buf, apr_size_t *nbytes)
{
    apr_size_t rv;

    if (thefile->buffered) {
        char *pos = (char *)buf;
        int blocksize;
        int size = *nbytes;

#if APR_HAS_THREADS
        apr_lock(thefile->thlock);
#endif

        if ( thefile->direction == 0 ) {
            /* Position file pointer for writing at the offset we are 
             * logically reading from
             */
            apr_int64_t offset = thefile->filePtr - thefile->dataRead + thefile->bufpos;
            if (offset != thefile->filePtr)
                lseek(thefile->filedes, offset, SEEK_SET);
            thefile->bufpos = thefile->dataRead = 0;
            thefile->direction = 1;
        }

	rv = 0;
        while (rv == 0 && size > 0) {
            if (thefile->bufpos == APR_FILE_BUFSIZE)   /* write buffer is full*/
                apr_flush(thefile);

            blocksize = size > APR_FILE_BUFSIZE - thefile->bufpos ? 
                        APR_FILE_BUFSIZE - thefile->bufpos : size;
            memcpy(thefile->buffer + thefile->bufpos, pos, blocksize);                      
            thefile->bufpos += blocksize;
            pos += blocksize;
            size -= blocksize;
        }

#if APR_HAS_THREADS
        apr_unlock(thefile->thlock);
#endif
        return rv;
    }
    else {
        do {
            rv = write(thefile->filedes, buf, *nbytes);
        } while (rv == (apr_size_t)-1 && errno == EINTR);
#ifdef USE_WAIT_FOR_IO
        if (rv == (apr_size_t)-1 &&
            (errno == EAGAIN || errno == EWOULDBLOCK) && 
            thefile->timeout != 0) {
            apr_status_t arv = wait_for_io_or_timeout(thefile, 0);
            if (arv != APR_SUCCESS) {
                *nbytes = 0;
                return arv;
            }
            else {
                do {
                    rv = write(thefile->filedes, buf, *nbytes);
	        } while (rv == (apr_size_t)-1 && errno == EINTR);
            }
        }  
#endif
        if (rv == (apr_size_t)-1) {
            (*nbytes) = 0;
            return errno;
        }
        *nbytes = rv;
        return APR_SUCCESS;
    }
}

apr_status_t apr_writev(apr_file_t *thefile, const struct iovec *vec,
                      apr_size_t nvec, apr_size_t *nbytes)
{
#ifdef HAVE_WRITEV
    int bytes;

    if ((bytes = writev(thefile->filedes, vec, nvec)) < 0) {
        *nbytes = 0;
        return errno;
    }
    else {
        *nbytes = bytes;
        return APR_SUCCESS;
    }
#else
    *nbytes = vec[0].iov_len;
    return apr_write(thefile, vec[0].iov_base, nbytes);
#endif
}

apr_status_t apr_putc(char ch, apr_file_t *thefile)
{
    apr_size_t nbytes = 1;

    return apr_write(thefile, &ch, &nbytes);
}

apr_status_t apr_ungetc(char ch, apr_file_t *thefile)
{
    thefile->ungetchar = (unsigned char)ch;
    return APR_SUCCESS; 
}

apr_status_t apr_getc(char *ch, apr_file_t *thefile)
{
    apr_size_t nbytes = 1;

    return apr_read(thefile, ch, &nbytes);
}

apr_status_t apr_puts(const char *str, apr_file_t *thefile)
{
    apr_size_t nbytes = strlen(str);

    return apr_write(thefile, str, &nbytes);
}

apr_status_t apr_flush(apr_file_t *thefile)
{
    if (thefile->buffered) {
        apr_int64_t written = 0;

        if (thefile->direction == 1 && thefile->bufpos) {
            do {
                written = write(thefile->filedes, thefile->buffer, thefile->bufpos);
            } while (written == (apr_int64_t)-1 && errno == EINTR);
            if (written == (apr_int64_t)-1) {
                return errno;
            }
            thefile->filePtr += written;
            thefile->bufpos = 0;
        }
    }
    /* There isn't anything to do if we aren't buffering the output
     * so just return success.
     */
    return APR_SUCCESS; 
}

apr_status_t apr_fgets(char *str, int len, apr_file_t *thefile)
{
    apr_status_t rv = APR_SUCCESS; /* get rid of gcc warning */
    apr_size_t nbytes;
    char *final = str + len - 1;

    if (len <= 1) {  
        /* sort of like fgets(), which returns NULL and stores no bytes 
         */
        return APR_SUCCESS;
    }

    while (str < final) { /* leave room for trailing '\0' */
        nbytes = 1;
        rv = apr_read(thefile, str, &nbytes);
        if (rv != APR_SUCCESS) {
            break;
        }
        if (*str == '\n') {
            ++str;
            break;
        }
        ++str;
    }
    /* We must store a terminating '\0' if we've stored any chars. We can
     * get away with storing it if we hit an error first. 
     */
    *str = '\0'; 
    return rv;
}

APR_DECLARE(int) apr_fprintf(apr_file_t *fptr, const char *format, ...)
{
    apr_status_t cc;
    va_list ap;
    char *buf;
    int len;

    buf = malloc(HUGE_STRING_LEN);
    if (buf == NULL) {
        return 0;
    }
    va_start(ap, format);
    len = apr_vsnprintf(buf, HUGE_STRING_LEN, format, ap);
    cc = apr_puts(buf, fptr);
    va_end(ap);
    free(buf);
    return (cc == APR_SUCCESS) ? len : -1;
}
