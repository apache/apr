/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include "apr_shm.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_time.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#if APR_HAS_SHARED_MEMORY

typedef struct mbox {
    char msg[1024]; 
    int msgavail; 
} mbox;
mbox *boxes;

#define N_BOXES 10
#define SHARED_SIZE (apr_size_t)(N_BOXES * sizeof(mbox))
#define SHARED_FILENAME "/tmp/apr.testshm.shm"

static void msgwait(int sleep_sec, int first_box, int last_box)
{
    int i;
    apr_time_t start = apr_time_now();
    while (apr_time_now() - start < sleep_sec * APR_USEC_PER_SEC) {
        for (i = first_box; i < last_box; i++) {
            if (boxes[i].msgavail) {
                fprintf(stdout, "received a message in box %d, message was: %s\n", 
                        i, boxes[i].msg); 
                boxes[i].msgavail = 0; /* reset back to 0 */
            }
        }
        apr_sleep(1*APR_USEC_PER_SEC);
    }
    fprintf(stdout, "done waiting on mailboxes...\n");
}

static void msgput(int boxnum, char *msg)
{
    fprintf(stdout, "Sending message to box %d\n", boxnum);
    apr_cpystrn(boxes[boxnum].msg, msg, strlen(msg));
    boxes[boxnum].msgavail = 1;
}

int main(void)
{
    apr_status_t rv;
    apr_pool_t *pool;
    apr_shm_t *shm;
    int i;
    char errmsg[200];

    apr_initialize();
    
    printf("APR Shared Memory Test: PRODUCER\n");

    printf("Initializing the pool............................"); 
    if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
        printf("could not initialize pool\n");
        exit(-1);
    }
    printf("OK\n");

    rv = apr_shm_attach(&shm, SHARED_FILENAME, pool);
    if (rv != APR_SUCCESS) {
        printf("Unable to attach to name-based shared memory segment: "
               "[%d] %s \n", rv, apr_strerror(rv, errmsg, sizeof(errmsg)));
        exit(-2);
    }

    boxes = apr_shm_baseaddr_get(shm);

    /* produce messages on all of the boxes, in descending order */
    for (i = N_BOXES - 1; i > 0; i--) {
        msgput(i, "Sending a message\n");
        apr_sleep(1*APR_USEC_PER_SEC);
    }

    rv = apr_shm_detach(shm);
    if (rv != APR_SUCCESS) {
        printf("Unable to detach from name-based shared memory segment: "
               "[%d] %s \n", rv, apr_strerror(rv, errmsg, sizeof(errmsg)));
        exit(-3);
    }

    return 0;
}

#else /* APR_HAS_SHARED_MEMORY */

int main(void)
{
    printf("APR SHMEM test not run!\n");
    printf("shmem is not supported on this platform\n"); 
    return -1;
}

#endif /* APR_HAS_SHARED_MEMORY */

