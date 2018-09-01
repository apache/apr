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

#include <ctype.h>
#include <stdlib.h>

#include "apr_json.h"

#if !APR_CHARSET_EBCDIC

typedef struct _json_link_t {
    apr_json_value_t *value;
    struct _json_link_t *next;
} json_link_t;

typedef struct apr_json_scanner_t {
    apr_pool_t *pool;
    const char *p;
    const char *e;
    int flags;
    int level;
} apr_json_scanner_t;

static apr_status_t apr_json_decode_space(apr_json_scanner_t * self,
                                          const char **space);
static apr_status_t apr_json_decode_value(apr_json_scanner_t * self,
                                          apr_json_value_t ** retval);

/* stolen from mod_mime_magic.c :) */
/* Single hex char to int; -1 if not a hex char. */
static int hex_to_int(int c)
{
    if (isdigit(c))
        return c - '0';
    if ((c >= 'a') && (c <= 'f'))
        return c + 10 - 'a';
    if ((c >= 'A') && (c <= 'F'))
        return c + 10 - 'A';
    return -1;
}

static apr_ssize_t ucs4_to_utf8(char *out, int code)
{
    if (code < 0x00000080) {
        out[0] = code;
        return 1;
    }
    else if (code < 0x00000800) {
        out[0] = 0xc0 + (code >> 6);
        out[1] = 0x80 + (code & 0x3f);
        return 2;
    }
    else if (code < 0x00010000) {
        out[0] = 0xe0 + (code >> 12);
        out[1] = 0x80 + ((code >> 6) & 0x3f);
        out[2] = 0x80 + (code & 0x3f);
        return 3;
    }
    else if (code < 0x00200000) {
        out[0] = 0xd0 + (code >> 18);
        out[1] = 0x80 + ((code >> 12) & 0x3f);
        out[2] = 0x80 + ((code >> 6) & 0x3f);
        out[3] = 0x80 + (code & 0x3F);
        return 4;
    }
    return 0;
}

static apr_status_t apr_json_decode_string(apr_json_scanner_t * self, apr_json_string_t * retval)
{
    apr_status_t status = APR_SUCCESS;
    apr_json_string_t string;
    const char *p = self->p;
    const char *e;
    char *q;
    apr_ssize_t len;

    if (self->p >= self->e) {
        status = APR_EOF;
        goto out;
    }

    self->p++; /* eat the leading '"' */

    /* advance past the \ " */
    len = 0;
    for (p = self->p, e = self->e; p < e;) {
        if (*p == '"')
            break;
        else if (*p == '\\') {
            p++;
            if (p >= e) {
                status = APR_EOF;
                goto out;
            }
            if (*p == 'u') {
                if (p + 4 >= e) {
                    status = APR_EOF;
                    goto out;
                }
                p += 5;
                len += 4;/* an UTF-8 character spans at most 4 bytes */
            }
            else {
                len++;
                p++;
            }
        }
        else {
            len++;
            p++;
        }
    }

    string.p = q = apr_pcalloc(self->pool, len + 1);
    e = p;

#define VALIDATE_UTF8_SUCCEEDING_BYTE(p) \
    if (*(unsigned char *)(p) < 0x80 || *(unsigned char *)(p) >= 0xc0) { \
        status = APR_BADCH; \
        goto out; \
    }

    for (p = self->p; p < e;) {
        switch (*(unsigned char *)p) {
        case '\\':
            p++;
            switch (*p) {
            case 'u':
                /* THIS IS REQUIRED TO BE A 4 DIGIT HEX NUMBER */
                {
                    int i, d, cp = 0;
                    for (i = 0, p++; i < 4 && p < e; i++, p++) {
                        d = hex_to_int(*p);
                        if (d < 0) {
                            status = APR_BADCH;
                            goto out;
                        }
                        cp = (cp << 4) | d;
                    }
                    if (cp >= 0xd800 && cp < 0xdc00) {
                        /* surrogate pair */
                        int sc = 0;
                        if (p + 6 > e) {
                            status = APR_EOF;
                            goto out;
                        }
                        if (p[0] != '\\' && p[1] != 'u') {
                            status = APR_BADCH;
                            goto out;
                        }
                        for (i = 0, p += 2; i < 4 && p < e; i++, p++) {
                            d = hex_to_int(*p);
                            if (d < 0) {
                                status = APR_BADCH;
                                goto out;
                            }
                            sc = (sc << 4) | d;
                        }
                        cp = ((cp & 0x3ff) << 10) | (sc & 0x3ff);
                        if ((cp >= 0xd800 && cp < 0xe000) || (cp >= 0x110000)) {
                            status = APR_BADCH;
                            goto out;
                        }
                    }
                    else if (cp >= 0xdc00 && cp < 0xe000) {
                        status = APR_BADCH;
                        goto out;
                    }
                    q += ucs4_to_utf8(q, cp);
                }
                break;
            case '\\':
                *q++ = '\\';
                p++;
                break;
            case '/':
                *q++ = '/';
                p++;
                break;
            case 'n':
                *q++ = '\n';
                p++;
                break;
            case 'r':
                *q++ = '\r';
                p++;
                break;
            case 't':
                *q++ = '\t';
                p++;
                break;
            case 'f':
                *q++ = '\f';
                p++;
                break;
            case 'b':
                *q++ = '\b';
                p++;
                break;
            case '"':
                *q++ = '"';
                p++;
                break;
            default:
                status = APR_BADCH;
                goto out;
            }
            break;

        case 0xc0:
        case 0xc1:
        case 0xc2:
        case 0xc3:
        case 0xc4:
        case 0xc5:
        case 0xc6:
        case 0xc7:
        case 0xc8:
        case 0xc9:
        case 0xca:
        case 0xcb:
        case 0xcc:
        case 0xcd:
        case 0xce:
        case 0xcf:
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xd3:
        case 0xd4:
        case 0xd5:
        case 0xd6:
        case 0xd7:
        case 0xd8:
        case 0xd9:
        case 0xda:
        case 0xdb:
        case 0xdc:
        case 0xdd:
        case 0xde:
        case 0xdf:
            if (p + 1 >= e) {
                status = APR_EOF;
                goto out;
            }
            *q++ = *p++;
            VALIDATE_UTF8_SUCCEEDING_BYTE(p);
            *q++ = *p++;
            break;

        case 0xe0:
        case 0xe1:
        case 0xe2:
        case 0xe3:
        case 0xe4:
        case 0xe5:
        case 0xe6:
        case 0xe7:
        case 0xe8:
        case 0xe9:
        case 0xea:
        case 0xeb:
        case 0xec:
        case 0xed:
        case 0xee:
        case 0xef:
            if (p + 2 >= e) {
                status = APR_EOF;
                goto out;
            }
            *q++ = *p++;
            VALIDATE_UTF8_SUCCEEDING_BYTE(p);
            *q++ = *p++;
            VALIDATE_UTF8_SUCCEEDING_BYTE(p);
            *q++ = *p++;
            break;

        case 0xf0:
        case 0xf1:
        case 0xf2:
        case 0xf3:
        case 0xf4:
        case 0xf5:
        case 0xf6:
        case 0xf7:
            if (p + 3 >= e) {
                status = APR_EOF;
                goto out;
            }
            if (((unsigned char *)p)[0] >= 0xf5 || ((unsigned char *)p)[1] >= 0x90) {
                status = APR_BADCH;
                goto out;
            }
            *q++ = *p++;
            VALIDATE_UTF8_SUCCEEDING_BYTE(p);
            *q++ = *p++;
            VALIDATE_UTF8_SUCCEEDING_BYTE(p);
            *q++ = *p++;
            VALIDATE_UTF8_SUCCEEDING_BYTE(p);
            *q++ = *p++;
            break;

        case 0xf8:
        case 0xf9:
        case 0xfa:
        case 0xfb:
            if (p + 4 >= e) {
                status = APR_EOF;
                goto out;
            }
            status = APR_BADCH;
            goto out;

        case 0xfc:
        case 0xfd:
            if (p + 5 >= e) {
                status = APR_EOF;
                goto out;
            }
            status = APR_BADCH;
            goto out;

        default:
            *q++ = *p++;
            break;
        }
    }
#undef VALIDATE_UTF8_SUCCEEDING_BYTE
    p++; /* eat the trailing '"' */
    *q = 0;
    string.len = q - string.p;
    *retval = string;
out:
    self->p = p;
    return status;
}

static apr_status_t apr_json_decode_array(apr_json_scanner_t * self,
        apr_json_value_t * array)
{
    apr_status_t status = APR_SUCCESS;
    apr_size_t count = 0;

    if (self->p >= self->e) {
        return APR_EOF;
    }

    if (self->level <= 0) {
        return APR_EINVAL;
    }
    self->level--;

    array->value.array = apr_pcalloc(self->pool,
            sizeof(apr_json_array_t));
    if (!array) {
        return APR_ENOMEM;
    }
    APR_RING_INIT(&array->value.array->list, apr_json_value_t, link);
    array->value.array->array = NULL;

    self->p++; /* toss of the leading [ */

    for (;;) {
        apr_json_value_t *element;

        if (self->p == self->e) {
            return APR_EOF;
        }

        if (*self->p == ']') {
            self->p++;
            break;
        }

        if (APR_SUCCESS != (status = apr_json_decode_value(self, &element))) {
            return status;
        }

        if (APR_SUCCESS
                != (status = apr_json_array_add(array, element))) {
            return status;
        }

        count++;

        if (self->p == self->e) {
            return APR_EOF;
        }

        if (*self->p == ',') {
            self->p++;
        }
        else if (*self->p != ']') {
            return APR_BADCH;
        }
    }

    {
        apr_json_value_t *element = apr_json_array_first(array);
        array->value.array->array = apr_array_make(self->pool, count,
                sizeof(apr_json_value_t *));
        while (element) {
            *((apr_json_value_t **) (apr_array_push(array->value.array->array))) =
                    element;
            element = apr_json_array_next(array, element);
        }
    }

    self->level++;

    return status;
}

static apr_status_t apr_json_decode_object(apr_json_scanner_t * self,
        apr_json_value_t *json, apr_json_object_t ** retval)
{
    apr_status_t status = APR_SUCCESS;

    apr_json_object_t *object = apr_pcalloc(self->pool,
            sizeof(apr_json_object_t));
    APR_RING_INIT(&object->list, apr_json_kv_t, link);
    object->hash = apr_hash_make(self->pool);

    *retval = object;

    if (self->p >= self->e) {
        return APR_EOF;
    }

    if (self->level <= 0) {
        return APR_EINVAL;
    }
    self->level--;

    self->p++; /* toss of the leading { */

    for (;;) {
        apr_json_value_t *key;
        apr_json_value_t *value;

        if (self->p == self->e) {
            status = APR_EOF;
            goto out;
        }

        if (*self->p == '}') {
            self->p++;
            break;
        }

        key = apr_json_value_create(self->pool);
        if ((status = apr_json_decode_space(self, &key->pre)))
            goto out;

        if (self->p == self->e) {
            status = APR_EOF;
            goto out;
        }
        if (*self->p != '"') {
            status = APR_BADCH;
            goto out;
        }

        key->type = APR_JSON_STRING;
        if ((status = apr_json_decode_string(self, &key->value.string)))
            goto out;

        if ((status = apr_json_decode_space(self, &key->post)))
            goto out;

        if (self->p == self->e) {
            status = APR_EOF;
            goto out;
        }
        if (*self->p != ':') {
            status = APR_BADCH;
            goto out;
        }

        self->p++; /* eat the ':' */

        if (self->p == self->e) {
            status = APR_EOF;
            goto out;
        }

        if ((status = apr_json_decode_value(self, &value)))
            goto out;

        apr_json_object_set_ex(json, key, value, self->pool);

        if (self->p == self->e) {
            status = APR_EOF;
            goto out;
        }

        if (*self->p == ',') {
            self->p++;
        }
        else if (*self->p != '}') {
            status = APR_BADCH;
            goto out;
        }
    }

    self->level++;

out:
    return status;
}

static apr_status_t apr_json_decode_boolean(apr_json_scanner_t * self, int *retval)
{
    if (self->p >= self->e)
        return APR_EOF;

    if (self->e - self->p >= 4 && strncmp("true", self->p, 4) == 0) {
        self->p += 4;
        *retval = 1;
        return APR_SUCCESS;
    }
    else if (self->e - self->p >= 5 && strncmp("false", self->p, 5) == 0) {
        self->p += 5;
        *retval = 0;
        return APR_SUCCESS;
    }
    return APR_BADCH;
}

static apr_status_t apr_json_decode_number(apr_json_scanner_t * self, apr_json_value_t * retval)
{
    apr_status_t status = APR_SUCCESS;
    int treat_as_float = 0, exp_occurred = 0;
    const char *p = self->p, *e = self->e;

    if (p >= e)
        return APR_EOF;

    {
        unsigned char c = *(unsigned char *)p;
        if (c == '-') {
            p++;
            if (p >= e)
                return APR_EOF;
            c = *(unsigned char *)p;
        }
        if (!isdigit(c)) {
            status = APR_BADCH;
            goto out;
        }
        p++;
    }

    if (!treat_as_float) {
        while (p < e) {
            unsigned char c = *(unsigned char *)p;
            if (c == 'e' || c == 'E') {
                p++;
                if (p >= e)
                    return APR_EOF;
                c = *(unsigned char *)p;
                if (c == '-') {
                    p++;
                    if (p >= e)
                        return APR_EOF;
                    c = *(unsigned char *)p;
                }
                if (!isdigit(c)) {
                    status = APR_BADCH;
                    goto out;
                }
                treat_as_float = 1;
                exp_occurred = 1;
                break;
            }
            else if (c == '.') {
                p++;
                treat_as_float = 1;
                break;
            }
            else if (!isdigit(c))
                break;
            p++;
        }
    }
    else {
        while (p < e) {
            unsigned char c = *(unsigned char *)p;
            if (c == 'e' || c == 'E') {
                p++;
                if (p >= e)
                    return APR_EOF;
                c = *(unsigned char *)p;
                if (c == '-') {
                    p++;
                    if (p >= e)
                        return APR_EOF;
                    c = *(unsigned char *)p;
                }
                if (!isdigit(c)) {
                    status = APR_BADCH;
                    goto out;
                }
                exp_occurred = 1;
                break;
            }
            else if (!isdigit(c))
                break;
            p++;
        }
    }

    if (treat_as_float) {
        if (!exp_occurred) {
            while (p < e) {
                unsigned char c = *(unsigned char *)p;
                if (c == 'e' || c == 'E') {
                    p++;
                    if (p >= e)
                        return APR_EOF;
                    c = *(unsigned char *)p;
                    if (c == '-') {
                        p++;
                        if (p >= e)
                            return APR_EOF;
                        c = *(unsigned char *)p;
                    }
                    if (!isdigit(c)) {
                        status = APR_BADCH;
                        goto out;
                    }
                    exp_occurred = 1;
                    break;
                }
                else if (!isdigit(c))
                    break;
                p++;
            }
        }
        if (exp_occurred) {
            if (p >= e || !isdigit(*(unsigned char *)p))
                return APR_EOF;
            while (++p < e && isdigit(*(unsigned char *)p));
        }
    }

    if (treat_as_float) {
        retval->type = APR_JSON_DOUBLE;
        retval->value.dnumber = strtod(self->p, NULL);
    }
    else {
        retval->type = APR_JSON_LONG;
        retval->value.lnumber = strtol(self->p, NULL, 10);
    }

out:
    self->p = p;
    return status;
}

static apr_status_t apr_json_decode_null(apr_json_scanner_t * self)
{
    if (self->e - self->p >= 4 && strncmp("null", self->p, 4) == 0) {
        self->p += 4;
        return APR_SUCCESS;
    }
    return APR_BADCH;
}

static apr_status_t apr_json_decode_space(apr_json_scanner_t * self,
        const char **space)
{
    const char *p = self->p;
    char *s;
    int len = 0;

    *space = NULL;

    if (self->p >= self->e) {
        return APR_SUCCESS;
    }

    while (p < self->e && isspace(*(unsigned char *)p)) {
        p++;
        len++;
    }

    if (self->flags & APR_JSON_FLAGS_WHITESPACE) {
        if (len) {
            *space = s = apr_palloc(self->pool, len + 1);

            while (self->p < self->e && isspace(*(unsigned char *) self->p)) {
                *s++ = *self->p++;
            }
            *s = 0;

        }
    } else {
        self->p = p;
    }

    return APR_SUCCESS;
}

static apr_status_t apr_json_decode_value(apr_json_scanner_t * self, apr_json_value_t ** retval)
{
    apr_json_value_t value;
    apr_status_t status = APR_SUCCESS;

    status = apr_json_decode_space(self, &value.pre);

    if (status == APR_SUCCESS) {
        switch (*(unsigned char *) self->p) {
        case '"':
            value.type = APR_JSON_STRING;
            status = apr_json_decode_string(self, &value.value.string);
            break;
        case '[':
            value.type = APR_JSON_ARRAY;
            status = apr_json_decode_array(self, &value);
            break;
        case '{':
            value.type = APR_JSON_OBJECT;
            status = apr_json_decode_object(self, &value, &value.value.object);
            break;
        case 'n':
            value.type = APR_JSON_NULL;
            status = apr_json_decode_null(self);
            break;
        case 't':
        case 'f':
            value.type = APR_JSON_BOOLEAN;
            status = apr_json_decode_boolean(self, &value.value.boolean);
            break;
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            status = apr_json_decode_number(self, &value);
            break;
        default:
            status = APR_BADCH;
        }
    }

    if (status == APR_SUCCESS) {
        status = apr_json_decode_space(self, &value.post);
    }

    if (status == APR_SUCCESS) {
        *retval = apr_pmemdup(self->pool, &value, sizeof(value));
    }
    else {
        *retval = NULL;
    }
    return status;
}

apr_status_t apr_json_decode(apr_json_value_t ** retval, const char *injson,
        apr_ssize_t injson_size, apr_off_t * offset, int flags, int level,
        apr_pool_t * pool)
{
    apr_status_t status;
    apr_json_scanner_t scanner;

    scanner.p = injson;
    scanner.e = injson
            + (injson_size == APR_JSON_VALUE_STRING ? strlen(injson) : injson_size);
    scanner.pool = pool;
    scanner.flags = flags;
    scanner.level = level;

    if (APR_SUCCESS == (status = apr_json_decode_value(&scanner, retval))) {
        if (scanner.p != scanner.e) {
            /* trailing craft */
            status = APR_BADCH;
        }
    }

    if (offset) {
        *offset = scanner.p - injson;
    }

    return status;
}

#else
/* we do not yet support JSON on EBCDIC platforms, but will do in future */
apr_status_t apr_json_decode(apr_json_value_t ** retval, const char *injson,
        apr_size_t injson_size, apr_pool_t * pool)
{
    return APR_ENOTIMPL;
}
#endif
