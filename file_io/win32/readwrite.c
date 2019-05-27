/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_arch_file_io.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_errno.h"
#include <malloc.h>
#include "apr_arch_atime.h"
#include "apr_arch_misc.h"

/*
 * read_with_timeout() 
 * Uses async i/o to emulate unix non-blocking i/o with timeouts.
 */
static apr_status_t read_with_timeout(apr_file_t *file, void *buf, apr_size_t len_in, apr_size_t *nbytes)
{
    apr_status_t rv;
    DWORD len = (DWORD)len_in;
    DWORD bytesread = 0;

    /* Handle the zero timeout non-blocking case */
    if (file->timeout == 0) {
        /* Peek at the pipe. If there is no data available, return APR_EAGAIN.
         * If data is available, go ahead and read it.
         */
        if (file->pipe) {
            DWORD bytes;
            if (!PeekNamedPipe(file->filehand, NULL, 0, NULL, &bytes, NULL)) {
                rv = apr_get_os_error();
                if (rv == APR_FROM_OS_ERROR(ERROR_BROKEN_PIPE)) {
                    rv = APR_EOF;
                }
                *nbytes = 0;
                return rv;
            }
            else {
                if (bytes == 0) {
                    *nbytes = 0;
                    return APR_EAGAIN;
                }
                if (len > bytes) {
                    len = bytes;
                }
            }
        }
        else {
            /* ToDo: Handle zero timeout non-blocking file i/o 
             * This is not needed until an APR application needs to
             * timeout file i/o (which means setting file i/o non-blocking)
             */
        }
    }

    if (file->pOverlapped && !file->pipe) {
        file->pOverlapped->Offset     = (DWORD)file->filePtr;
        file->pOverlapped->OffsetHigh = (DWORD)(file->filePtr >> 32);
    }

    if (ReadFile(file->filehand, buf, len, 
                 &bytesread, file->pOverlapped)) {
        rv = APR_SUCCESS;
    }
    else {
        rv = apr_get_os_error();
        if (rv == APR_FROM_OS_ERROR(ERROR_IO_PENDING)) {
            DWORD res;

            /* It seems that ReadFile() return ERROR_IO_PENDING even
             * when I/O operation completed syncronously.
             * Use fast macro to check that overlapped I/O already
             * completed to avoid kernel call.
             */ 
            if (HasOverlappedIoCompleted(file->pOverlapped)) {
                res = WAIT_OBJECT_0;
            }
            else {
                /* Wait for the pending i/o, timeout converted from us to ms
                 * Note that we loop if someone gives up the event.
                 *
                 * NOTE: We do not handle WAIT_ABANDONED here because they
                 * can be returned only when waiting for mutex.
                 */
                res = apr_wait_for_single_object(file->pOverlapped->hEvent,
                                                 file->timeout);
            }

            /* There is one case that represents entirely
             * successful operations, otherwise we will cancel
             * the operation in progress.
             */
            if (res != WAIT_OBJECT_0) {
                CancelIoEx(file->filehand, file->pOverlapped);
            }

            /* Ignore any failures above.  Attempt to complete
             * the overlapped operation and use only _its_ result.
             * For example, CancelIo or WaitForSingleObject can
             * fail if the handle is closed, yet the read may have
             * completed before we attempted to CancelIo...
             */
            if (GetOverlappedResult(file->filehand, file->pOverlapped, 
                                    &bytesread, TRUE)) {
                rv = APR_SUCCESS;
            }
            else {
                rv = apr_get_os_error();
                if (((rv == APR_FROM_OS_ERROR(ERROR_IO_INCOMPLETE))
                        || (rv == APR_FROM_OS_ERROR(ERROR_OPERATION_ABORTED)))
                    && (res == WAIT_TIMEOUT))
                    rv = APR_TIMEUP;
            }
        }
        if (rv == APR_FROM_OS_ERROR(ERROR_BROKEN_PIPE)) {
            /* Assume ERROR_BROKEN_PIPE signals an EOF reading from a pipe */
            rv = APR_EOF;
        } else if (rv == APR_FROM_OS_ERROR(ERROR_HANDLE_EOF)) {
            /* Did we hit EOF reading from the handle? */
            rv = APR_EOF;
        }
    }
    
    /* OK and 0 bytes read ==> end of file */
    if (rv == APR_SUCCESS && bytesread == 0)
        rv = APR_EOF;
    
    if (rv == APR_SUCCESS && file->pOverlapped && !file->pipe) {
        file->filePtr += bytesread;
    }
    *nbytes = bytesread;
    return rv;
}

static apr_status_t read_buffered(apr_file_t *thefile, void *buf, apr_size_t *len)
{
    apr_status_t rv;
    char *pos = (char *)buf;
    apr_size_t bytes_read;
    apr_size_t size;
    apr_size_t remaining = *len;

    if (thefile->direction == 1) {
        rv = apr_file_flush(thefile);
        if (rv != APR_SUCCESS) {
            return rv;
        }
        thefile->bufpos = 0;
        thefile->direction = 0;
        thefile->dataRead = 0;
    }

    /* Copy the data we have in the buffer. */
    size = thefile->dataRead - thefile->bufpos;
    if (size > remaining) {
        size = remaining;
    }
    memcpy(pos, thefile->buffer + thefile->bufpos, size);
    pos += size;
    remaining -= size;
    thefile->bufpos += size;

    if (remaining == 0) {
        /* Nothing to do more, keep *LEN unchanged and return. */
        return APR_SUCCESS;
    }
    /* The buffer is empty, but the caller wants more.
     * Decide on the most appropriate way to read from the file:
     */
    if (remaining > thefile->bufsize) {
        /* If the remaining chunk won't fit into the buffer, read it into
         * the destination buffer with a single syscall.
         */
        rv = read_with_timeout(thefile, pos, remaining, &bytes_read);
        thefile->filePtr += bytes_read;
        pos += bytes_read;
        /* Also, copy the last BUFSIZE (or less in case of a short read) bytes
         * from the chunk to our buffer so that seeking backwards and reading
         * would work from the buffer.
         */
        size = thefile->bufsize;
        if (size > bytes_read) {
            size = bytes_read;
        }
        memcpy(thefile->buffer, pos - size, size);
        thefile->bufpos = size;
        thefile->dataRead = size;
    }
    else {
        /* The remaining chunk fits into the buffer.  Read up to BUFSIZE bytes
         * from the file to our internal buffer.
         */
        rv = read_with_timeout(thefile, thefile->buffer, thefile->bufsize, &bytes_read);
        thefile->filePtr += bytes_read;
        thefile->bufpos = 0;
        thefile->dataRead = bytes_read;
        /* Copy the required part to the caller. */
        size = remaining;
        if (size > bytes_read) {
            size = bytes_read;
        }
        memcpy(pos, thefile->buffer, size);
        pos += size;
        thefile->bufpos += size;
    }

    if (bytes_read == 0 && rv == APR_EOF) {
        thefile->eof_hit = TRUE;
    }

    *len = pos - (char *)buf;
    if (*len) {
        rv = APR_SUCCESS;
    }

    return rv;
}

APR_DECLARE(apr_status_t) apr_file_read(apr_file_t *thefile, void *buf, apr_size_t *len)
{
    apr_status_t rv;
    DWORD bytes_read = 0;

    if (*len <= 0) {
        *len = 0;
        return APR_SUCCESS;
    }

    /* If the file is open for xthread support, allocate and
     * initialize the overlapped and io completion event (hEvent). 
     * Threads should NOT share an apr_file_t or its hEvent.
     */
    if ((thefile->flags & APR_FOPEN_XTHREAD) && !thefile->pOverlapped ) {
        thefile->pOverlapped = (OVERLAPPED*) apr_pcalloc(thefile->pool, 
                                                         sizeof(OVERLAPPED));
        thefile->pOverlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!thefile->pOverlapped->hEvent) {
            rv = apr_get_os_error();
            return rv;
        }
    }

    /* Handle the ungetchar if there is one */
    if (thefile->ungetchar != -1) {
        bytes_read = 1;
        *(char *)buf = (char)thefile->ungetchar;
        buf = (char *)buf + 1;
        (*len)--;
        thefile->ungetchar = -1;
        if (*len == 0) {
            *len = bytes_read;
            return APR_SUCCESS;
        }
    }
    if (thefile->buffered) {
        if (thefile->flags & APR_FOPEN_XTHREAD) {
            apr_thread_mutex_lock(thefile->mutex);
        }
        rv = read_buffered(thefile, buf, len);
        if (thefile->flags & APR_FOPEN_XTHREAD) {
            apr_thread_mutex_unlock(thefile->mutex);
        }
    } else {  
        /* Unbuffered i/o */
        apr_size_t nbytes;
        rv = read_with_timeout(thefile, buf, *len, &nbytes);
        if (rv == APR_EOF)
            thefile->eof_hit = TRUE;
        *len = nbytes;
    }

    return rv;
}

APR_DECLARE(apr_status_t) apr_file_rotating_check(apr_file_t *thefile)
{
    return APR_ENOTIMPL;
}

APR_DECLARE(apr_status_t) apr_file_rotating_manual_check(apr_file_t *thefile,
                                                         apr_time_t n)
{
    return APR_ENOTIMPL;
}

/* Helper function that adapts WriteFile() to apr_size_t instead
 * of DWORD. */
static apr_status_t write_helper(HANDLE filehand, const char *buf,
                                 apr_size_t len, apr_size_t *pwritten)
{
    apr_size_t remaining = len;

    *pwritten = 0;
    do {
        DWORD to_write;
        DWORD written;

        if (remaining > APR_DWORD_MAX) {
            to_write = APR_DWORD_MAX;
        }
        else {
            to_write = (DWORD)remaining;
        }

        if (!WriteFile(filehand, buf, to_write, &written, NULL)) {
            *pwritten += written;
            return apr_get_os_error();
        }

        *pwritten += written;
        remaining -= written;
        buf += written;
    } while (remaining);

    return APR_SUCCESS;
}

static apr_status_t write_buffered(apr_file_t *thefile, const char *buf,
                                   apr_size_t len, apr_size_t *pwritten)
{
    apr_status_t rv;

    if (thefile->direction == 0) {
        /* Position file pointer for writing at the offset we are logically reading from */
        apr_off_t offset = thefile->filePtr - thefile->dataRead + thefile->bufpos;
        DWORD offlo = (DWORD)offset;
        LONG offhi = (LONG)(offset >> 32);
        if (offset != thefile->filePtr)
            SetFilePointer(thefile->filehand, offlo, &offhi, FILE_BEGIN);
        thefile->bufpos = thefile->dataRead = 0;
        thefile->direction = 1;
    }

    *pwritten = 0;

    while (len > 0) {
        if (thefile->bufpos == thefile->bufsize) { /* write buffer is full */
            rv = apr_file_flush(thefile);
            if (rv) {
                return rv;
            }
        }
        /* If our buffer is empty, and we cannot fit the remaining chunk
         * into it, write the chunk with a single syscall and return.
         */
        if (thefile->bufpos == 0 && len > thefile->bufsize) {
            apr_size_t written;

            rv = write_helper(thefile->filehand, buf, len, &written);
            thefile->filePtr += written;
            *pwritten += written;
            return rv;
        }
        else {
            apr_size_t blocksize = len;

            if (blocksize > thefile->bufsize - thefile->bufpos) {
                blocksize = thefile->bufsize - thefile->bufpos;
            }
            memcpy(thefile->buffer + thefile->bufpos, buf, blocksize);
            thefile->bufpos += blocksize;
            buf += blocksize;
            len -= blocksize;
            *pwritten += blocksize;
        }
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_write(apr_file_t *thefile, const void *buf, apr_size_t *nbytes)
{
    apr_status_t rv;
    DWORD bwrote;

    /* If the file is open for xthread support, allocate and
     * initialize the overlapped and io completion event (hEvent). 
     * Threads should NOT share an apr_file_t or its hEvent.
     */
    if ((thefile->flags & APR_FOPEN_XTHREAD) && !thefile->pOverlapped ) {
        thefile->pOverlapped = (OVERLAPPED*) apr_pcalloc(thefile->pool, 
                                                         sizeof(OVERLAPPED));
        thefile->pOverlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!thefile->pOverlapped->hEvent) {
            rv = apr_get_os_error();
            return rv;
        }
    }

    if (thefile->buffered) {
        if (thefile->flags & APR_FOPEN_XTHREAD) {
            apr_thread_mutex_lock(thefile->mutex);
        }
        rv = write_buffered(thefile, buf, *nbytes, nbytes);
        if (thefile->flags & APR_FOPEN_XTHREAD) {
            apr_thread_mutex_unlock(thefile->mutex);
        }
        return rv;
    } else {
        if (thefile->pipe) {
            rv = WriteFile(thefile->filehand, buf, (DWORD)*nbytes, &bwrote,
                           thefile->pOverlapped);
        }
        else if (thefile->append && !thefile->pOverlapped) {
            OVERLAPPED ov = {0};

            /* If the file is opened for synchronous I/O, take advantage of the
             * documented way to atomically append data by calling WriteFile()
             * with both the OVERLAPPED.Offset and OffsetHigh members set to
             * 0xFFFFFFFF.  This avoids calling LockFile() that is otherwise
             * required to avoid a race condition between seeking to the end
             * and writing data.  Not locking the file improves robustness of
             * such appends and avoids a deadlock when appending to an already
             * locked file, as described in PR50058.
             *
             * We use this approach only for files opened for synchronous I/O
             * because in this case the I/O Manager maintains the current file
             * position.  Otherwise, the file offset returned or changed by
             * the SetFilePointer() API is not guaranteed to be valid and that
             * could, for instance, break apr_file_seek() calls after appending
             * data.  Sadly, if a file is opened for asynchronous I/O, this
             * call doesn't update the OVERLAPPED.Offset member to reflect the
             * actual offset used when appending the data (which we could then
             * use to make seeking and other operations involving filePtr work).
             * Therefore, when appending to files opened for asynchronous I/O,
             * we still use the LockFile + SetFilePointer + WriteFile approach.
             *
             * References:
             * https://bz.apache.org/bugzilla/show_bug.cgi?id=50058
             * https://msdn.microsoft.com/en-us/library/windows/desktop/aa365747
             * https://msdn.microsoft.com/en-us/library/windows/hardware/ff567121
             */
            ov.Offset = MAXDWORD;
            ov.OffsetHigh = MAXDWORD;
            rv = WriteFile(thefile->filehand, buf, (DWORD)*nbytes, &bwrote, &ov);
        }
        else {
            apr_off_t offset = 0;
            apr_status_t rc;
            if (thefile->append) {
                if (thefile->flags & APR_FOPEN_XTHREAD) {
                    /* apr_file_lock will mutex the file across processes.
                     * The call to apr_thread_mutex_lock is added to avoid
                     * a race condition between LockFile and WriteFile
                     * that occasionally leads to deadlocked threads.
                     */
                    apr_thread_mutex_lock(thefile->mutex);
                }
                rc = apr_file_lock(thefile, APR_FLOCK_EXCLUSIVE);
                if (rc != APR_SUCCESS) {
                    if (thefile->flags & APR_FOPEN_XTHREAD) {
                        apr_thread_mutex_unlock(thefile->mutex);
                    }
                    return rc;
                }
                rc = apr_file_seek(thefile, APR_END, &offset);
                if (rc != APR_SUCCESS) {
                    if (thefile->flags & APR_FOPEN_XTHREAD) {
                        apr_thread_mutex_unlock(thefile->mutex);
                    }
                    return rc;
                }
            }
            if (thefile->pOverlapped) {
                thefile->pOverlapped->Offset     = (DWORD)thefile->filePtr;
                thefile->pOverlapped->OffsetHigh = (DWORD)(thefile->filePtr >> 32);
            }
            rv = WriteFile(thefile->filehand, buf, (DWORD)*nbytes, &bwrote,
                           thefile->pOverlapped);
            if (thefile->append) {
                apr_file_unlock(thefile);
                if (thefile->flags & APR_FOPEN_XTHREAD) {
                    apr_thread_mutex_unlock(thefile->mutex);
                }
            }
        }
        if (rv) {
            *nbytes = bwrote;
            rv = APR_SUCCESS;
        }
        else {
            (*nbytes) = 0;
            rv = apr_get_os_error();

            if (rv == APR_FROM_OS_ERROR(ERROR_IO_PENDING)) {
 
                DWORD res;

                /* It seems that WriteFile() return ERROR_IO_PENDING even
                 * when I/O operation completed syncronously.
                 * Use fast macro to check that overlapped I/O already
                 * completed to avoid kernel call.
                 */
                if (HasOverlappedIoCompleted(thefile->pOverlapped)) {
                    res = WAIT_OBJECT_0;
                }
                else {
                    res = apr_wait_for_single_object(thefile->pOverlapped->hEvent,
                                                     thefile->timeout);
                }

                /* There is one case that represents entirely
                 * successful operations, otherwise we will cancel
                 * the operation in progress.
                 */
                if (res != WAIT_OBJECT_0) {
                    CancelIoEx(thefile->filehand, thefile->pOverlapped);
                }

                /* Ignore any failures above.  Attempt to complete
                 * the overlapped operation and use only _its_ result.
                 * For example, CancelIo or WaitForSingleObject can
                 * fail if the handle is closed, yet the read may have
                 * completed before we attempted to CancelIo...
                 */
                if (GetOverlappedResult(thefile->filehand, thefile->pOverlapped,
                                        &bwrote, TRUE)) {
                    *nbytes = bwrote;
                    rv = APR_SUCCESS;
                }
                else {
                    rv = apr_get_os_error();
                    if (((rv == APR_FROM_OS_ERROR(ERROR_IO_INCOMPLETE))
                        || (rv == APR_FROM_OS_ERROR(ERROR_OPERATION_ABORTED)))
                        && (res == WAIT_TIMEOUT))
                        rv = APR_TIMEUP;

                    if (rv == APR_TIMEUP && thefile->timeout == 0) {
                        rv = APR_EAGAIN;
                    }
                }
            }
        }
        if (rv == APR_SUCCESS && thefile->pOverlapped && !thefile->pipe) {
            thefile->filePtr += *nbytes;
        }
    }
    return rv;
}
/* ToDo: Write for it anyway and test the oslevel!
 * Too bad WriteFileGather() is not supported on 95&98 (or NT prior to SP2)
 */
APR_DECLARE(apr_status_t) apr_file_writev(apr_file_t *thefile,
                                     const struct iovec *vec,
                                     apr_size_t nvec, 
                                     apr_size_t *nbytes)
{
    apr_status_t rv = APR_SUCCESS;
    apr_size_t i;
    apr_size_t bwrote = 0;
    char *buf;

    *nbytes = 0;
    for (i = 0; i < nvec; i++) {
        buf = vec[i].iov_base;
        bwrote = vec[i].iov_len;
        rv = apr_file_write(thefile, buf, &bwrote);
        *nbytes += bwrote;
        if (rv != APR_SUCCESS) {
            break;
        }
    }
    return rv;
}

APR_DECLARE(apr_status_t) apr_file_putc(char ch, apr_file_t *thefile)
{
    apr_size_t len = 1;

    return apr_file_write(thefile, &ch, &len);
}

APR_DECLARE(apr_status_t) apr_file_ungetc(char ch, apr_file_t *thefile)
{
    thefile->ungetchar = (unsigned char) ch;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_file_getc(char *ch, apr_file_t *thefile)
{
    apr_status_t rc;
    apr_size_t bread;

    bread = 1;
    rc = apr_file_read(thefile, ch, &bread);

    if (rc) {
        return rc;
    }
    
    if (bread == 0) {
        thefile->eof_hit = TRUE;
        return APR_EOF;
    }
    return APR_SUCCESS; 
}

APR_DECLARE(apr_status_t) apr_file_puts(const char *str, apr_file_t *thefile)
{
    apr_size_t len = strlen(str);

    return apr_file_write(thefile, str, &len);
}

APR_DECLARE(apr_status_t) apr_file_gets(char *str, int len, apr_file_t *thefile)
{
    apr_status_t rv = APR_SUCCESS;
    apr_size_t nbytes;
    const char *str_start = str;
    char *final = str + len - 1;

    /* If the file is open for xthread support, allocate and
     * initialize the overlapped and io completion event (hEvent).
     * Threads should NOT share an apr_file_t or its hEvent.
     */
    if ((thefile->flags & APR_FOPEN_XTHREAD) && !thefile->pOverlapped) {
        thefile->pOverlapped = (OVERLAPPED*) apr_pcalloc(thefile->pool,
                                                         sizeof(OVERLAPPED));
        thefile->pOverlapped->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (!thefile->pOverlapped->hEvent) {
            rv = apr_get_os_error();
            return rv;
        }
    }

    /* Handle the ungetchar if there is one. */
    if (thefile->ungetchar != -1 && str < final) {
        *str = thefile->ungetchar;
        thefile->ungetchar = -1;
        if (*str == '\n') {
            *(++str) = '\0';
            return APR_SUCCESS;
        }
        ++str;
    }

    /* If we have an underlying buffer, we can be *much* more efficient
     * and skip over the read_with_timeout() calls.
     */
    if (thefile->buffered) {
        if (thefile->flags & APR_FOPEN_XTHREAD) {
            apr_thread_mutex_lock(thefile->mutex);
        }

        if (thefile->direction == 1) {
            rv = apr_file_flush(thefile);
            if (rv) {
                if (thefile->flags & APR_FOPEN_XTHREAD) {
                    apr_thread_mutex_unlock(thefile->mutex);
                }
                return rv;
            }

            thefile->direction = 0;
            thefile->bufpos = 0;
            thefile->dataRead = 0;
        }

        while (str < final) { /* leave room for trailing '\0' */
            if (thefile->bufpos < thefile->dataRead) {
                *str = thefile->buffer[thefile->bufpos++];
            }
            else {
                nbytes = 1;
                rv = read_buffered(thefile, str, &nbytes);
                if (rv != APR_SUCCESS) {
                    break;
                }
            }
            if (*str == '\n') {
                ++str;
                break;
            }
            ++str;
        }
        if (thefile->flags & APR_FOPEN_XTHREAD) {
            apr_thread_mutex_unlock(thefile->mutex);
        }
    }
    else {
        while (str < final) { /* leave room for trailing '\0' */
            nbytes = 1;
            rv = read_with_timeout(thefile, str, nbytes, &nbytes);
            if (rv == APR_EOF)
                thefile->eof_hit = TRUE;

            if (rv != APR_SUCCESS) {
                break;
            }
            if (*str == '\n') {
                ++str;
                break;
            }
            ++str;
        }
    }

    /* We must store a terminating '\0' if we've stored any chars. We can
     * get away with storing it if we hit an error first.
     */
    *str = '\0';
    if (str > str_start) {
        /* We stored chars; don't report EOF or any other errors;
         * the app will find out about that on the next call.
         */
        return APR_SUCCESS;
    }
    return rv;
}

APR_DECLARE(apr_status_t) apr_file_flush(apr_file_t *thefile)
{
    if (thefile->buffered) {
        apr_status_t rc = 0;

        if (thefile->direction == 1 && thefile->bufpos) {
            apr_size_t written;

            rc = write_helper(thefile->filehand, thefile->buffer,
                              thefile->bufpos, &written);
            thefile->filePtr += written;

            if (rc == 0)
                thefile->bufpos = 0;
        }

        return rc;
    }

    /* There isn't anything to do if we aren't buffering the output
     * so just return success.
     */
    return APR_SUCCESS; 
}

APR_DECLARE(apr_status_t) apr_file_sync(apr_file_t *thefile){
    apr_status_t rv;

    rv = apr_file_flush(thefile);
    if (rv != APR_SUCCESS) {
        return rv;
    }

    if (!FlushFileBuffers(thefile->filehand)) {
        rv = apr_get_os_error();
    }

    return rv;
}

APR_DECLARE(apr_status_t) apr_file_datasync(apr_file_t *thefile){
    return apr_file_sync(thefile);
}

struct apr_file_printf_data {
    apr_vformatter_buff_t vbuff;
    apr_file_t *fptr;
    char *buf;
};

static int file_printf_flush(apr_vformatter_buff_t *buff)
{
    struct apr_file_printf_data *data = (struct apr_file_printf_data *)buff;

    if (apr_file_write_full(data->fptr, data->buf,
                            data->vbuff.curpos - data->buf, NULL)) {
        return -1;
    }

    data->vbuff.curpos = data->buf;
    return 0;
}

APR_DECLARE_NONSTD(int) apr_file_printf(apr_file_t *fptr, 
                                        const char *format, ...)
{
    struct apr_file_printf_data data;
    va_list ap;
    int count;

    data.buf = malloc(HUGE_STRING_LEN);
    if (data.buf == NULL) {
        return 0;
    }
    data.vbuff.curpos = data.buf;
    data.vbuff.endpos = data.buf + HUGE_STRING_LEN;
    data.fptr = fptr;
    va_start(ap, format);
    count = apr_vformatter(file_printf_flush,
                           (apr_vformatter_buff_t *)&data, format, ap);
    /* apr_vformatter does not call flush for the last bits */
    if (count >= 0) file_printf_flush((apr_vformatter_buff_t *)&data);

    va_end(ap);

    free(data.buf);
    return count;
}

APR_DECLARE(apr_status_t) apr_file_pipe_wait(apr_file_t *thepipe,
                                             apr_wait_type_t direction)
{
    return APR_ENOTIMPL;
}

