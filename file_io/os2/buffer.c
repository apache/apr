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
#include "apr_thread_mutex.h"

APR_DECLARE(apr_status_t) apr_file_buffer_set(apr_file_t *file, 
                                              char * buffer,
                                              apr_size_t bufsize)
{
    apr_status_t rv = APR_SUCCESS;
    int do_locking = file->mutex != NULL && file->buffered;

    if (do_locking) {
        apr_thread_mutex_lock(file->mutex);
    }
 
    if (file->buffered) {
        /* Flush the existing buffer */
        rv = apr_file_flush(file);
        if (rv != APR_SUCCESS) {
            if (do_locking) {
                apr_thread_mutex_unlock(file->mutex);
            }

            return rv;
        }
    }
        
    file->buffer = buffer;
    file->bufsize = bufsize;
    file->bufpos = 0;
    file->direction = 0;
    file->dataRead = 0;

    if (bufsize > 0 && file->mutex == NULL && (file->flags & APR_FOPEN_XTHREAD)) {
        /* buffering is being turned on but we have no mutex, make one */
        rv = apr_thread_mutex_create(&file->mutex, 0, file->pool);
    }

    /* Setting the buffer size to zero is equivalent to turning 
     * buffering off. 
     */
    file->buffered = file->bufsize > 0;
    
    if (do_locking) {
        apr_thread_mutex_unlock(file->mutex);
    }

    return rv;
}

APR_DECLARE(apr_size_t) apr_file_buffer_size_get(apr_file_t *file)
{
    return file->bufsize;
}
