/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002-2003 The Apache Software Foundation.  All rights
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

#include "apr_arch_file_io.h"
#include "apr_file_io.h"

static apr_status_t apr_file_transfer_contents(const char *from_path,
                                               const char *to_path,
                                               apr_int32_t flags,
                                               apr_fileperms_t to_perms,
                                               apr_pool_t *pool)
{
    apr_file_t *s = NULL, *d = NULL;  /* init to null important for APR */
    apr_status_t status;
    apr_finfo_t finfo;
    apr_fileperms_t perms;

    /* Open source file. */
    status = apr_file_open(&s, from_path, APR_READ, APR_OS_DEFAULT, pool);
    if (status)
        return status;

    /* Maybe get its permissions. */
    if (to_perms == APR_FILE_SOURCE_PERMS) {
        status = apr_file_info_get(&finfo, APR_FINFO_PROT, s);
        if (status != APR_SUCCESS && status != APR_INCOMPLETE) {
            apr_file_close(s);  /* toss any error */
            return status;
        }
        perms = finfo.protection;
    }
    else
        perms = to_perms;

    /* Open dest file. */
    status = apr_file_open(&d, to_path, flags, perms, pool);
    if (status) {
        apr_file_close(s);  /* toss any error */
        return status;
    }

    /* Copy bytes till the cows come home. */
    while (1) {
        char buf[BUFSIZ];
        apr_size_t bytes_this_time = sizeof(buf);
        apr_status_t read_err;
        apr_status_t write_err;

        /* Read 'em. */
        read_err = apr_file_read(s, buf, &bytes_this_time);
        if (read_err && !APR_STATUS_IS_EOF(read_err)) {
            apr_file_close(s);  /* toss any error */
            apr_file_close(d);  /* toss any error */
            return read_err;
        }

        /* Write 'em. */
        write_err = apr_file_write_full(d, buf, bytes_this_time, NULL);
        if (write_err) {
            apr_file_close(s);  /* toss any error */
            apr_file_close(d);  /* toss any error */
            return write_err;
        }

        if (read_err && APR_STATUS_IS_EOF(read_err)) {
            status = apr_file_close(s);
            if (status) {
                apr_file_close(d);  /* toss any error */
                return status;
            }

            /* return the results of this close: an error, or success */
            return apr_file_close(d);
        }
    }
    /* NOTREACHED */
}

APR_DECLARE(apr_status_t) apr_file_copy(const char *from_path,
                                        const char *to_path,
                                        apr_fileperms_t perms,
                                        apr_pool_t *pool)
{
    return apr_file_transfer_contents(from_path, to_path,
                                      (APR_WRITE | APR_CREATE | APR_TRUNCATE),
                                      perms,
                                      pool);
}

APR_DECLARE(apr_status_t) apr_file_append(const char *from_path,
                                          const char *to_path,
                                          apr_fileperms_t perms,
                                          apr_pool_t *pool)
{
    return apr_file_transfer_contents(from_path, to_path,
                                      (APR_WRITE | APR_CREATE | APR_APPEND),
                                      perms,
                                      pool);
}
