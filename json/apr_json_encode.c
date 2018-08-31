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

#include "apr_json.h"

#if !APR_CHARSET_EBCDIC

typedef struct apr_json_serializer_t {
    apr_pool_t *pool;
    apr_bucket_brigade *brigade;
    apr_brigade_flush flush;
    void *ctx;
    int flags;
} apr_json_serializer_t;

static apr_status_t apr_json_encode_value(apr_json_serializer_t * self,
                                            const apr_json_value_t * value);

static apr_status_t apr_json_brigade_write(apr_json_serializer_t * self,
               const char *chunk, apr_size_t chunk_len, const char *escaped)
{
    apr_status_t status;

    status = apr_brigade_write(self->brigade, self->flush, self->ctx, chunk, chunk_len);
    if (APR_SUCCESS == status) {
        status = apr_brigade_puts(self->brigade, self->flush, self->ctx, escaped);
    }

    return status;
}

static apr_status_t apr_json_brigade_printf(apr_json_serializer_t * self,
               const char *chunk, apr_size_t chunk_len, const char *fmt, ...)
{
    va_list ap;
    apr_status_t status;

    status = apr_brigade_write(self->brigade, self->flush, self->ctx, chunk,
            chunk_len);
    if (APR_SUCCESS == status) {
        va_start(ap, fmt);
        status = apr_brigade_vprintf(self->brigade, self->flush, self->ctx, fmt,
                ap);
        va_end(ap);
    }

    return status;
}

static apr_status_t apr_json_encode_string(apr_json_serializer_t * self,
        const apr_json_string_t * string)
{
    apr_status_t status;
    const char *p, *e, *chunk;
    const char invalid[4] = { 0xEF, 0xBF, 0xBD, 0x00 };
    unsigned char c;

    status = apr_brigade_putc(self->brigade, self->flush, self->ctx, '\"');
    if (APR_SUCCESS != status) {
        return status;
    }

    for (p = chunk = string->p, e = string->p
            + (APR_JSON_VALUE_STRING == string->len ?
                    strlen(string->p) : string->len); p < e; p++) {
        switch (*p) {
        case '\n':
            status = apr_json_brigade_write(self, chunk, p - chunk, "\\n");
            chunk = p + 1;
            break;
        case '\r':
            status = apr_json_brigade_write(self, chunk, p - chunk, "\\r");
            chunk = p + 1;
            break;
        case '\t':
            status = apr_json_brigade_write(self, chunk, p - chunk, "\\t");
            chunk = p + 1;
            break;
        case '\b':
            status = apr_json_brigade_write(self, chunk, p - chunk, "\\b");
            chunk = p + 1;
            break;
        case '\f':
            status = apr_json_brigade_write(self, chunk, p - chunk, "\\f");
            chunk = p + 1;
            break;
        case '\\':
            status = apr_json_brigade_write(self, chunk, p - chunk, "\\\\");
            chunk = p + 1;
            break;
        case '"':
            status = apr_json_brigade_write(self, chunk, p - chunk, "\\\"");
            chunk = p + 1;
            break;
        default:
            c = (unsigned char)(*p);
            apr_size_t left = e - p;
            if (c < 0x20) {
                status = apr_json_brigade_printf(self, chunk, p - chunk,
                        "\\u%04x", c);
                chunk = p + 1;
            }
            else if (((c >> 7) == 0x00)) {
                /* 1 byte */
            }
            else if (left > 1 && ((c >> 5) == 0x06) && p[1]) {
                /* 2 bytes */
                if (left < 2 || (p[1] >> 6) != 0x02) {
                    status = apr_json_brigade_write(self, chunk, p - chunk,
                            invalid);
                    chunk = p + 1;
                }
            }
            else if (((c >> 4) == 0x0E)) {
                /* 3 bytes */
                if (left < 3 || (p[1] >> 6) != 0x02 || (p[2] >> 6) != 0x02) {
                    status = apr_json_brigade_write(self, chunk, p - chunk,
                            invalid);
                    chunk = p + 1;
                }
            }
            else if ((c >> 3) == 0x1E) {
                /* 4 bytes */
                if (left < 4 || (p[1] >> 6) != 0x02 || (p[2] >> 6) != 0x02 || (p[3] >> 6) != 0x02) {
                    status = apr_json_brigade_write(self, chunk, p - chunk,
                            invalid);
                    chunk = p + 1;
                }
            }
            else {
                status = apr_json_brigade_write(self, chunk, p - chunk,
                        invalid);
                chunk = p + 1;
            }
            break;
        }

        if (APR_SUCCESS != status) {
            return status;
        }
    }

    if (chunk < p) {
        status = apr_brigade_write(self->brigade, self->flush, self->ctx, chunk, p - chunk);
        if (APR_SUCCESS != status) {
            return status;
        }
    }

    return apr_brigade_putc(self->brigade, self->flush, self->ctx, '\"');
}


static apr_status_t apr_json_encode_array(apr_json_serializer_t * self,
        const apr_json_value_t * array)
{
    apr_status_t status;
    apr_json_value_t *val;
    apr_size_t count = 0;

    status = apr_brigade_putc(self->brigade, self->flush, self->ctx, '[');
    if (APR_SUCCESS != status) {
        return status;
    }

    val = apr_json_array_first(array);
    while (val) {

        if (count > 0) {
            status = apr_brigade_putc(self->brigade, self->flush, self->ctx, ',');
            if (APR_SUCCESS != status) {
                return status;
            }
        }

        status = apr_json_encode_value(self, val);
        if (APR_SUCCESS != status) {
            return status;
        }

        val = apr_json_array_next(array, val);
        count++;
    }

    return apr_brigade_putc(self->brigade, self->flush, self->ctx, ']');
}

static apr_status_t apr_json_encode_object(apr_json_serializer_t * self, apr_json_object_t * object)
{
    apr_status_t status;
    apr_json_kv_t *kv;
    int first = 1;
    status = apr_brigade_putc(self->brigade, self->flush, self->ctx, '{');
    if (APR_SUCCESS != status) {
        return status;
    }

    for (kv = APR_RING_FIRST(&(object)->list);
         kv != APR_RING_SENTINEL(&(object)->list, apr_json_kv_t, link);
         kv = APR_RING_NEXT((kv), link)) {

        if (!first) {
            status = apr_brigade_putc(self->brigade, self->flush, self->ctx, ',');
            if (APR_SUCCESS != status) {
                return status;
            }
        }

        {
            status = apr_json_encode_value(self, kv->k);
            if (APR_SUCCESS != status) {
                return status;
            }

            status = apr_brigade_putc(self->brigade, self->flush, self->ctx, ':');
            if (APR_SUCCESS != status) {
                return status;
            }

            status = apr_json_encode_value(self, kv->v);
            if (APR_SUCCESS != status) {
                return status;
            }
        }
        first = 0;
    }
    return apr_brigade_putc(self->brigade, self->flush, self->ctx, '}');
}

static apr_status_t apr_json_encode_value(apr_json_serializer_t * self, const apr_json_value_t * value)
{
    apr_status_t status = APR_SUCCESS;

    if (value->pre && (self->flags & APR_JSON_FLAGS_WHITESPACE)) {
        status = apr_brigade_puts(self->brigade, self->flush, self->ctx,
                value->pre);
    }

    if (APR_SUCCESS == status) {
        switch (value->type) {
        case APR_JSON_STRING:
            status = apr_json_encode_string(self, &value->value.string);
            break;
        case APR_JSON_LONG:
            status = apr_brigade_printf(self->brigade, self->flush, self->ctx,
                    "%" APR_INT64_T_FMT, value->value.lnumber);
            break;
        case APR_JSON_DOUBLE:
            status = apr_brigade_printf(self->brigade, self->flush, self->ctx,
                    "%lf", value->value.dnumber);
            break;
        case APR_JSON_BOOLEAN:
            status = apr_brigade_puts(self->brigade, self->flush, self->ctx,
                    value->value.boolean ? "true" : "false");
            break;
        case APR_JSON_NULL:
            status = apr_brigade_puts(self->brigade, self->flush, self->ctx,
                    "null");
            break;
        case APR_JSON_OBJECT:
            status = apr_json_encode_object(self, value->value.object);
            break;
        case APR_JSON_ARRAY:
            status = apr_json_encode_array(self, value);
            break;
        default:
            return APR_EINVAL;
        }
    }

    if (APR_SUCCESS == status && value->post
            && (self->flags & APR_JSON_FLAGS_WHITESPACE)) {
        status = apr_brigade_puts(self->brigade, self->flush, self->ctx,
                value->post);
    }

    return status;
}

apr_status_t apr_json_encode(apr_bucket_brigade * brigade, apr_brigade_flush flush,
                void *ctx, const apr_json_value_t * json, int flags, apr_pool_t * pool)
{
    apr_json_serializer_t serializer = {pool, brigade, flush, ctx, flags};
    return apr_json_encode_value(&serializer, json);
}

#else
 /* we do not yet support JSON on EBCDIC platforms, but will do in future */
apr_status_t apr_json_encode(apr_bucket_brigade * brigade, apr_brigade_flush flush,
                void *ctx, const apr_json_value_t * json, apr_pool_t * pool)
{
    return APR_ENOTIMPL;
}
#endif
