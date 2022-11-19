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

#ifndef TESTSHM_H
#define TESTSHM_H

#include "apr.h"
#include "apr_time.h"
#include "apr_atomic.h"
#include "apr_strings.h"

#include <assert.h>

typedef struct mbox {
    char msg[1024];
    apr_uint32_t msgavail;
} mbox;
mbox *boxes;

#define N_BOXES 10
#define N_MESSAGES 100
#define SHARED_SIZE (apr_size_t)(N_BOXES * sizeof(mbox))
#define SHARED_FILENAME "data/apr.testshm.shm"
#define MSG "Sending a message"

#if APR_HAS_SHARED_MEMORY

static APR_INLINE
int msgwait(const char *msg, int count, int duration, int sleep_ms)
{
    int recvd = 0, i;
    apr_time_t start = apr_time_now();
    apr_interval_time_t sleep_duration = apr_time_from_sec(duration);
    apr_interval_time_t sleep_delay = apr_time_from_msec(sleep_ms);
    while (apr_time_now() - start < sleep_duration) {
        for (i = 0; i < N_BOXES; i++) {
            if (apr_atomic_cas32(&boxes[i].msgavail, 0, 1) == 1) {
                if (msg) {
                    assert(strcmp(boxes[i].msg, msg) == 0);
                }
                *boxes[i].msg = '\0';
                if (++recvd == count) {
                    return recvd;
                }
            }
        }
        apr_sleep(sleep_delay);
    }
    return recvd;
}

static APR_INLINE
int msgput(const char *msg, int boxnum)
{
    if (apr_atomic_cas32(&boxes[boxnum].msgavail, -1, 0) != 0) {
        return 0;
    }
    if (msg) {
        apr_cpystrn(boxes[boxnum].msg, msg, sizeof(boxes[boxnum].msg));
    }
    else {
        *boxes[boxnum].msg = '\0';
    }
    apr_atomic_set32(&boxes[boxnum].msgavail, 1);
    return 1;
}

#endif /* APR_HAS_SHARED_MEMORY */

#endif
