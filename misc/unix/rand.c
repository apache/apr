/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#define APR_WANT_MEMFUNC
#include "apr_want.h"
#include "apr_general.h"

#include "apr_arch_misc.h"
#include <sys/stat.h>
#if APR_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if APR_HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if APR_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#if APR_HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifndef SHUT_RDWR
#define SHUT_RDWR 2
#endif

#if APR_HAS_RANDOM

APR_DECLARE(apr_status_t) apr_generate_random_bytes(unsigned char *buf, 
#ifdef APR_ENABLE_FOR_1_0
                                                    apr_size_t length)
#else
                                                    int length)
#endif
{
#ifdef DEV_RANDOM

    int fd = -1;

    /* On BSD/OS 4.1, /dev/random gives out 8 bytes at a time, then
     * gives EOF, so reading 'length' bytes may require opening the
     * device several times. */
    do {
        apr_ssize_t rc;

        if (fd == -1)
            if ((fd = open(DEV_RANDOM, O_RDONLY)) == -1)
                return errno;
        
        rc = read(fd, buf, length);
        if (rc < 0) {
            int errnum = errno;
            close(fd);
            return errnum;
        }
        else if (rc == 0) {
            close(fd);
            fd = -1; /* force open() again */
        }
        else {
            buf += rc;
            length -= rc;
        }
    } while (length > 0);
    
    close(fd);
#elif defined(OS2)
    static UCHAR randbyte();
    unsigned int idx;

    for (idx=0; idx<length; idx++)
	buf[idx] = randbyte();

#elif defined(HAVE_EGD)
    /* use EGD-compatible socket daemon (such as EGD or PRNGd).
     * message format:
     * 0x00 (get entropy level)
     *   0xMM (msb) 0xmm 0xll 0xLL (lsb)
     * 0x01 (read entropy nonblocking) 0xNN (bytes requested)
     *   0xMM (bytes granted) MM bytes
     * 0x02 (read entropy blocking) 0xNN (bytes desired)
     *   [block] NN bytes
     * 0x03 (write entropy) 0xMM 0xLL (bits of entropy) 0xNN (bytes of data) 
     *      NN bytes
     * (no response - write only) 
     * 0x04 (report PID)
     *   0xMM (length of PID string, not null-terminated) MM chars
     */
    static const char *egd_sockets[] = { EGD_DEFAULT_SOCKET, NULL };
    const char **egdsockname = NULL;

    int egd_socket, egd_path_len, rv, bad_errno;
    struct sockaddr_un addr;
    apr_socklen_t egd_addr_len;
    apr_size_t resp_expected;
    unsigned char req[2], resp[255];
    unsigned char *curbuf = buf;

    for (egdsockname = egd_sockets; *egdsockname && length > 0; egdsockname++) {
        egd_path_len = strlen(*egdsockname);
        
        if (egd_path_len > sizeof(addr.sun_path)) {
            return APR_EINVAL;
        }

        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        memcpy(addr.sun_path, *egdsockname, egd_path_len);
        egd_addr_len = APR_OFFSETOF(struct sockaddr_un, sun_path) + 
          egd_path_len; 

        egd_socket = socket(PF_UNIX, SOCK_STREAM, 0);

        if (egd_socket == -1) {
            return errno;
        }

        rv = connect(egd_socket, (struct sockaddr*)&addr, egd_addr_len);

        if (rv == -1) {
            bad_errno = errno;
            continue;
        }

        /* EGD can only return 255 bytes of data at a time.  Silly.  */ 
        while (length > 0) {
            apr_ssize_t srv;
            req[0] = 2; /* We'll block for now. */
            req[1] = length > 255 ? 255: length;

            srv = write(egd_socket, req, 2);
            if (srv == -1) {
                bad_errno = errno;
                shutdown(egd_socket, SHUT_RDWR);
                close(egd_socket);
                break;
            }

            if (srv != 2) {
                shutdown(egd_socket, SHUT_RDWR);
                close(egd_socket);
                return APR_EGENERAL;
            }
            
            resp_expected = req[1];
            srv = read(egd_socket, resp, resp_expected);
            if (srv == -1) {
                bad_errno = errno;
                shutdown(egd_socket, SHUT_RDWR);
                close(egd_socket);
                return bad_errno;
            }
            
            memcpy(curbuf, resp, srv);
            curbuf += srv;
            length -= srv;
        }
        
        shutdown(egd_socket, SHUT_RDWR);
        close(egd_socket);
    }

    if (length > 0) {
        /* We must have iterated through the list of sockets,
         * and no go. Return the errno.
         */
        return bad_errno;
    }

#elif defined(HAVE_TRUERAND) /* use truerand */

    extern int randbyte(void);	/* from the truerand library */
    unsigned int idx;

    /* this will increase the startup time of the server, unfortunately...
     * (generating 20 bytes takes about 8 seconds)
     */
    for (idx=0; idx<length; idx++)
	buf[idx] = (unsigned char) randbyte();

#endif	/* DEV_RANDOM */

    return APR_SUCCESS;
}

#undef	STR
#undef	XSTR

#ifdef OS2
#include "../os2/randbyte.c"
#endif

#endif /* APR_HAS_RANDOM */
