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

#include "apr_shmem.h"
#include "apr_lock.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_config.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
/*#include <process.h>*/
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

typedef struct mbox {
    char msg[1024]; 
    int msgavail; 
} mbox;
ap_pool_t *context;
mbox *boxes;

void msgwait(int boxnum)
{
    volatile int test = 0;
    while (test == 0) {
        sleep(0);
        test = boxes[boxnum].msgavail;
    }    
    fprintf(stdout, "\nreceived a message in box %d, message was: %s\n", 
            boxnum, boxes[boxnum].msg); 
}

void msgput(int boxnum, char *msg)
{
    fprintf(stdout, "Sending message to box %d\n", boxnum);
    ap_cpystrn(boxes[boxnum].msg, msg, strlen(msg));
    boxes[boxnum].msgavail = 1;
}

int main()
{
    ap_shmem_t *shm;
    pid_t pid;
    int size;

    ap_initialize();

    fprintf(stdout, "Initializing the context......."); 
    if (ap_create_pool(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "could not initialize\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Creating shared memory block......."); 
    if (ap_shm_init(&shm, 1048576, NULL, context) != APR_SUCCESS) { 
        fprintf(stderr, "Error allocating shared memory block\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Allocating shared memory......."); 
    size = sizeof(mbox) * 2;
    boxes = ap_shm_calloc(shm, size);
    if (boxes == NULL) { 
        fprintf(stderr, "Error creating message boxes.\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Creating a child process\n");
    pid = fork();
    if (pid == 0) {
sleep(1);
        if (ap_open_shmem(shm) == APR_SUCCESS) {
            msgwait(1);
            msgput(0, "Msg received\n");
        } else {
            puts( "Child: unable to get access to shared memory" );
        }
        exit(1);
    }
    else if (pid > 0) {
        msgput(1, "Sending a message\n");
sleep(1);
        msgwait(0);
        exit(1);
    }
    else {
        fprintf(stderr, "Error creating a child process\n");
        exit(1);
    }
}
