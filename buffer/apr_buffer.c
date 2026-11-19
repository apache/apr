/*    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 */

#include "apr.h"
#include "apr_lib.h"
#if 0
#define APR_WANT_STDIO
#define APR_WANT_STRFUNC
#endif
#include "apr_want.h"
#include "apr_buffer.h"
#include "apr_encode.h"
#include "apr_strings.h"
#include "apr_private.h"

#define APR_BUFFER_MAX (APR_SIZE_MAX/2)

APR_DECLARE(apr_status_t) apr_buffer_mem_set(apr_buffer_t *buf,
                                             void *mem, apr_size_t len)
{

    if (len > APR_BUFFER_MAX) {
        return APR_EINVAL;
    }

    buf->d.mem = mem;
    buf->size = len;
    buf->zero_terminated = 0;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_buffer_mem_create(apr_buffer_t **mb,
                                                apr_pool_t *pool,
                                                void *mem, apr_size_t len)
{
    apr_buffer_t *buf;

    if (len > APR_BUFFER_MAX) {
        return APR_EINVAL;
    }

    buf = apr_palloc(pool, sizeof(apr_buffer_t));

    if (buf) {

        buf->d.mem = mem;
        buf->size = len;
        buf->zero_terminated = 0;

        *mb = buf;
    }
    else {
        return APR_ENOMEM;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_buffer_str_set(apr_buffer_t *buf,
                                             char *str, apr_ssize_t len)
{

    if (!str) {
        buf->d.str = NULL;
        buf->size = 0;
        buf->zero_terminated = 0;
    }
    else if (len < 0) {
        apr_size_t slen = strlen(str);
        if (slen <= APR_BUFFER_MAX) {
            buf->d.str = str;
            buf->size = slen;
            buf->zero_terminated = 1;
        }
        else {
            return APR_EINVAL;
        }
    }
    else {
        if (str[len]) {
            return APR_EINVAL;
        }
        buf->d.str = str;
        buf->size = len;
        buf->zero_terminated = 1;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_buffer_str_create(apr_buffer_t **sb,
                                                apr_pool_t *pool,
                                                char *str, apr_ssize_t len)
{
    apr_buffer_t *buf;
    apr_int64_t size;
    apr_size_t slen;
    unsigned int zero_terminated;

    if (!str) {
        str = NULL;
        size = 0;
        zero_terminated = 0;
    }
    if (APR_BUFFER_STRING == len) {
        slen = strlen(str);
        if (slen <= APR_BUFFER_MAX) {
            size = slen;
            zero_terminated = 1;
        }
        else {
            return APR_EINVAL;
        }
    }
    else if (str[len]) {
        return APR_EINVAL;
    }
    else {
        size = (apr_size_t)len;
    }

    buf = apr_palloc(pool, sizeof(apr_buffer_t));

    if (buf) {
        buf->d.str = str;
        buf->size = size;
        buf->zero_terminated = zero_terminated;

        *sb = buf;
    }
    else {
        return APR_ENOMEM;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_buffer_null_create(apr_buffer_t **nb,
                                                 apr_pool_t *pool)
{
    apr_buffer_t *buf;

    buf = apr_pcalloc(pool, sizeof(apr_buffer_t));

    if (!buf) {
        return APR_ENOMEM;
    }

    *nb = buf;

    return APR_SUCCESS;
}

APR_DECLARE(apr_size_t) apr_buffer_len(const apr_buffer_t *buf)
{
    return buf->size;
}

APR_DECLARE(apr_size_t) apr_buffer_allocated(const apr_buffer_t *buf)
{   
    return buf->size + buf->zero_terminated;
}

APR_DECLARE(int) apr_buffer_is_null(const apr_buffer_t *buf)
{
    if (!buf->d.mem) {
        return 1;
    }
    else {
        return 0;
    }
}

APR_DECLARE(int) apr_buffer_is_str(const apr_buffer_t *buf)
{
    return buf->zero_terminated;
}

APR_DECLARE(char *) apr_buffer_str(const apr_buffer_t *buf)
{
    if (buf->zero_terminated) {
        return buf->d.str;
    }
    else {  
        return NULL;
    }
}

APR_DECLARE(char *) apr_buffer_pstrdup(apr_pool_t *pool, const apr_buffer_t *buf)
{
    return apr_pstrmemdup(pool, buf->d.str, buf->size);
}

APR_DECLARE(void *) apr_buffer_mem(const apr_buffer_t *buf, apr_size_t *size)
{
    if (size) {
        size[0] = apr_buffer_len(buf);
    }

    return buf->d.mem;
}

APR_DECLARE(void *) apr_buffer_pmemdup(apr_pool_t *pool, const apr_buffer_t *buf, apr_size_t *size)
{
    apr_size_t len = apr_buffer_len(buf);

    if (size) {
        size[0] = len;
    }

    return apr_pmemdup(pool, buf->d.mem, len);
}

APR_DECLARE(apr_status_t) apr_buffer_arraydup(apr_buffer_t **out,
                                              const apr_buffer_t *in,
                                              apr_buffer_alloc alloc, void *ctx,
                                              int nelts)
{
    apr_buffer_t *dst = alloc(ctx, nelts * sizeof(apr_buffer_t));
    const apr_buffer_t *src = in;

    *out = dst;

    if (!dst) {
        return APR_ENOMEM;
    }

    int i;
    for (i = 0; i < nelts; i++) {

        /* absolute value is size of mem buffer including optional terminating zero */
        apr_size_t size = src->size + src->zero_terminated;

        void *mem = alloc(ctx, size);

        if (!mem) {
            return APR_ENOMEM;
        }

        memcpy(mem, src->d.mem, size);

        dst->zero_terminated = src->zero_terminated;
        dst->size = src->size;
        dst->d.mem = mem;

        src++;
        dst++;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_buffer_dup(apr_buffer_t **out,
                                         const apr_buffer_t *in,
                                         apr_buffer_alloc alloc, void *ctx)
{
    return apr_buffer_arraydup(out, in, alloc, ctx, 1);
}

APR_DECLARE(apr_buffer_t *) apr_buffer_cpy(apr_buffer_t *dst,
                                           const apr_buffer_t *src,
                                           apr_buffer_alloc alloc, void *ctx)
{
    if (!src) {

        dst->d.mem = NULL;
        dst->size = 0;
        dst->zero_terminated = 0;

    }
    else if (!alloc) {

        dst->d.mem = src->d.mem;
        dst->size = src->size;
        dst->zero_terminated = src->zero_terminated;

    }
    else {

        /* absolute value is size of mem buffer including optional terminating zero */
        apr_size_t size = src->size + src->zero_terminated;

        void *mem = alloc(ctx, size);
        memcpy(mem, src->d.mem, size);

        dst->zero_terminated = src->zero_terminated;
        dst->size = src->size;
        dst->d.mem = mem;

    }

    return dst;
}

APR_DECLARE(int) apr_buffer_cmp(const apr_buffer_t *src,
                                 const apr_buffer_t *dst)
{
    if (!src || !src->d.mem) {
        return (!dst || !dst->d.mem) ? 0 : -1;
    }
    else {
        if (!dst || !dst->d.mem) {
            return 1;
        }
        else {

            apr_size_t slen = apr_buffer_len(src);
            apr_size_t dlen = apr_buffer_len(dst);

            if (slen != dlen) {
                return slen < dlen ? -1 : 1;
            }
            else {
                return memcmp(src->d.mem, dst->d.mem, slen);
            }

        }
    }
}

APR_DECLARE(char *) apr_buffer_pstrncat(apr_pool_t *p, const apr_buffer_t *buf,
                                        int nelts, const char *sep, int flags,
                                        apr_size_t *nbytes)
{
    const apr_buffer_t *src = buf;
    apr_size_t seplen = sep ? strlen(sep) : 0;
    apr_size_t size = 0;

    char *dst, *str;

    int i;
    for (i = 0; i < nelts; i++) {

        if (i > 0) {
            size += seplen;
        }

        if (src->zero_terminated) {
            size += src->size;
        }
        else {
            if (APR_BUFFER_PLAIN == flags) {
                size += src->size;
            }
            else if (APR_BUFFER_BASE64 == flags) {
                apr_size_t b64len;

                if (APR_SUCCESS != apr_encode_base64(NULL, src->d.mem, src->size,
                                                     APR_ENCODE_NONE, &b64len)) {
                    return NULL;
                }
                size += b64len - 1;
            }
        }

        src++;
    }

    if (nbytes) {
        *nbytes = size;
    }

    str = dst = apr_palloc(p, size + 1);

    if (!str) {
        return NULL;
    }

    src = buf;

    for (i = 0; i < nelts; i++) {

        if (i > 0 && sep) {
            memcpy(dst, sep, seplen);
            dst += seplen;
        }

        if (src->zero_terminated) {
            memcpy(dst, src->d.str, src->size);
            dst += src->size;
        }
        else {
            if (APR_BUFFER_PLAIN == flags) {
                memcpy(dst, src->d.mem, src->size);
                dst += src->size;
            }
            else if (APR_BUFFER_BASE64 == flags) {
                apr_size_t b64len;

                if (APR_SUCCESS != apr_encode_base64(dst, src->d.mem, src->size,
                                                     APR_ENCODE_NONE, &b64len)) {
                    return NULL;
                }
                dst += b64len;
            }
        }

        src++;
    }

    dst[0] = 0;

    return str;
}

