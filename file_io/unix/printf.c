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

#include "apr_file_io.h"
#include "apr_strings.h"
#include "apr_lib.h"

#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif

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

    /* don't really need a HUGE_STRING_LEN anymore */
    data.buf = malloc(HUGE_STRING_LEN);
    if (data.buf == NULL) {
        return -1;
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
