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

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include "apr_network_io.h"
#include "apr_errno.h"
#include "apr_general.h"

#if !APR_HAS_SENDFILE
int main(void)
{
    fprintf(stderr, 
            "This program won't work on this platform because there is no "
            "support for sendfile().\n");
    return 0;
}
#else /* !APR_HAS_SENDFILE */

#define FILE_LENGTH    200000

#define FILE_DATA_CHAR '0'

#define HDR1           "1234567890ABCD\n"
#define HDR2           "EFGH\n"
#define TRL1           "IJKLMNOPQRSTUVWXYZ\n"
#define TRL2           "!@#$%&*()\n"

#define TESTSF_PORT    8021

#define TESTFILE       "testsf.dat"

static void apr_setup(ap_pool_t **p, ap_socket_t **sock)
{
    char buf[120];
    ap_status_t rv;

    rv = ap_initialize();
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_initialize()->%d/%s\n",
                rv,
                ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    atexit(ap_terminate);

    rv = ap_create_pool(p, NULL);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_create_pool()->%d/%s\n",
                rv,
                ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    *sock = NULL;
    rv = ap_create_tcp_socket(sock, *p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_create_tcp_socket()->%d/%s\n",
                rv,
                ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }
}

static void create_testfile(ap_pool_t *p, const char *fname)
{
    ap_file_t *f = NULL;
    ap_status_t rv;
    char buf[120];
    int i;
    ap_ssize_t nbytes;
    ap_finfo_t finfo;

    printf("Creating a test file...\n");
    rv = ap_open(&f, fname, 
                 APR_CREATE | APR_WRITE | APR_TRUNCATE | APR_BUFFERED,
                 APR_UREAD | APR_UWRITE, p);
    if (rv) {
        fprintf(stderr, "ap_open()->%d/%s\n",
                rv, ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    
    buf[0] = FILE_DATA_CHAR;
    for (i = 0; i < FILE_LENGTH; i++) {
        nbytes = 1;
        rv = ap_write(f, buf, &nbytes);
        if (rv) {
            fprintf(stderr, "ap_write()->%d/%s\n",
                    rv, ap_strerror(rv, buf, sizeof buf));
            exit(1);
        }
    }

    rv = ap_close(f);
    if (rv) {
        fprintf(stderr, "ap_close()->%d/%s\n",
                rv, ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = ap_stat(&finfo, fname, p);
    if (rv) {
        fprintf(stderr, "ap_close()->%d/%s\n",
                rv, ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    if (finfo.size != FILE_LENGTH) {
        fprintf(stderr, 
                "test file %s should be %ld-bytes long\n"
                "instead it is %ld-bytes long\n",
                fname,
                (long int)FILE_LENGTH,
                (long int)finfo.size);
        exit(1);
    }
}

static int client(int socket_mode)
{
    ap_status_t rv;
    ap_socket_t *sock;
    ap_pool_t *p;
    char buf[120];
    ap_file_t *f = NULL;
    ap_size_t len, expected_len;
    ap_off_t offset;
    ap_hdtr_t hdtr;
    struct iovec headers[2];
    struct iovec trailers[2];
    ap_ssize_t bytes_read;

    apr_setup(&p, &sock);
    create_testfile(p, TESTFILE);

    rv = ap_open(&f, TESTFILE, APR_READ, 0, p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_open()->%d/%s\n",
                rv,
                ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = ap_set_remote_port(sock, TESTSF_PORT);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_set_remote_port()->%d/%s\n",
                rv,
                ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = ap_connect(sock, "127.0.0.1");
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_connect()->%d/%s\n", 
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    switch(socket_mode) {
    case 1:
        /* leave it blocking */
        break;
    case 0:
        /* set it non-blocking */
        rv = ap_setsocketopt(sock, APR_SO_NONBLOCK, 1);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "ap_setsocketopt(APR_SO_NONBLOCK)->%d/%s\n", 
                    rv,
                    ap_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        break;
    case 2:
        /* set a timeout */
        rv = ap_setsocketopt(sock, APR_SO_TIMEOUT, 
                             100 * AP_USEC_PER_SEC);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "ap_setsocketopt(APR_SO_NONBLOCK)->%d/%s\n", 
                    rv,
                    ap_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        break;
    default:
        assert(1 != 1);
    }

    printf("Sending the file...\n");

    hdtr.headers = headers;
    hdtr.numheaders = 2;
    hdtr.headers[0].iov_base = HDR1;
    hdtr.headers[0].iov_len  = strlen(hdtr.headers[0].iov_base);
    hdtr.headers[1].iov_base = HDR2;
    hdtr.headers[1].iov_len  = strlen(hdtr.headers[1].iov_base);

    hdtr.trailers = trailers;
    hdtr.numtrailers = 2;
    hdtr.trailers[0].iov_base = TRL1;
    hdtr.trailers[0].iov_len  = strlen(hdtr.trailers[0].iov_base);
    hdtr.trailers[1].iov_base = TRL2;
    hdtr.trailers[1].iov_len  = strlen(hdtr.trailers[1].iov_base);

    offset = 0;
    len = FILE_LENGTH;
    rv = ap_sendfile(sock, f, &hdtr, &offset, &len, 0);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_sendfile()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    printf("ap_sendfile() updated offset with %ld\n",
           (long int)offset);

    printf("ap_sendfile() updated len with %ld\n",
           (long int)len);

    expected_len = 
        strlen(HDR1) + strlen(HDR2) + 
        strlen(TRL1) + strlen(TRL2) + 
        FILE_LENGTH;

    printf("bytes really sent: %d\n",
           expected_len);

    if (len != expected_len) {
        fprintf(stderr, "ap_sendfile() didn't report the correct "
                "number of bytes sent!\n");
        exit(1);
    }
    
    offset = 0;
    rv = ap_seek(f, APR_CUR, &offset);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_seek()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    printf("After ap_sendfile(), the kernel file pointer is "
           "at offset %ld.\n",
           (long int)offset);

    rv = ap_shutdown(sock, APR_SHUTDOWN_WRITE);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_shutdown()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    bytes_read = 1;
    rv = ap_recv(sock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_recv()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != 0) {
        fprintf(stderr, "We expected the EOF condition on the connected\n"
                "socket but instead we read %ld bytes.\n",
                (long int)bytes_read);
        exit(1);
    }

    printf("client: ap_sendfile() worked as expected!\n");

    rv = ap_remove_file(TESTFILE, p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_remove_file()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    return 0;
}

static int server(void)
{
    ap_status_t rv;
    ap_socket_t *sock;
    ap_pool_t *p;
    char buf[120];
    int i;
    ap_socket_t *newsock = NULL;
    ap_ssize_t bytes_read;

    apr_setup(&p, &sock);

    rv = ap_set_local_port(sock, TESTSF_PORT);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_set_local_port()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = ap_setsocketopt(sock, APR_SO_REUSEADDR, 1);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_setsocketopt()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = ap_bind(sock);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_bind()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    rv = ap_listen(sock, 5);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_listen()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    printf("Waiting for a client to connect...\n");

    rv = ap_accept(&newsock, sock, p);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_accept()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }

    printf("Processing a client...\n");

    assert(sizeof buf > strlen(HDR1));
    bytes_read = strlen(HDR1);
    rv = ap_recv(newsock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_recv()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != strlen(HDR1)) {
        fprintf(stderr, "wrong data read (1)\n");
        exit(1);
    }
    if (memcmp(buf, HDR1, strlen(HDR1))) {
        fprintf(stderr, "wrong data read (2)\n");
        fprintf(stderr, "received: `%.*s'\nexpected: `%s'\n",
                bytes_read, buf, HDR1);
        exit(1);
    }
        
    assert(sizeof buf > strlen(HDR2));
    bytes_read = strlen(HDR2);
    rv = ap_recv(newsock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_recv()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != strlen(HDR2)) {
        fprintf(stderr, "wrong data read (3)\n");
        exit(1);
    }
    if (memcmp(buf, HDR2, strlen(HDR2))) {
        fprintf(stderr, "wrong data read (4)\n");
        fprintf(stderr, "received: `%.*s'\nexpected: `%s'\n",
                bytes_read, buf, HDR2);
        exit(1);
    }

    for (i = 0; i < FILE_LENGTH; i++) {
        bytes_read = 1;
        rv = ap_recv(newsock, buf, &bytes_read);
        if (rv != APR_SUCCESS) {
            fprintf(stderr, "ap_recv()->%d/%s\n",
                    rv,
                    ap_strerror(rv, buf, sizeof buf));
            exit(1);
        }
        if (bytes_read != 1) {
            fprintf(stderr, "ap_recv()->%ld bytes instead of 1\n",
                    (long int)bytes_read);
            exit(1);
        }
        if (buf[0] != FILE_DATA_CHAR) {
            fprintf(stderr,
                    "problem with data read (byte %d of file):\n",
                    i);
            fprintf(stderr, "read `%c' (0x%x) from client; expected "
                    "`%c'\n",
                    buf[0], buf[0], FILE_DATA_CHAR);
            exit(1);
        }
    }
        
    assert(sizeof buf > strlen(TRL1));
    bytes_read = strlen(TRL1);
    rv = ap_recv(newsock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_recv()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != strlen(TRL1)) {
        fprintf(stderr, "wrong data read (5)\n");
        exit(1);
    }
    if (memcmp(buf, TRL1, strlen(TRL1))) {
        fprintf(stderr, "wrong data read (6)\n");
        fprintf(stderr, "received: `%.*s'\nexpected: `%s'\n",
                bytes_read, buf, TRL1);
        exit(1);
    }
        
    assert(sizeof buf > strlen(TRL2));
    bytes_read = strlen(TRL2);
    rv = ap_recv(newsock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_recv()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != strlen(TRL2)) {
        fprintf(stderr, "wrong data read (7)\n");
        exit(1);
    }
    if (memcmp(buf, TRL2, strlen(TRL2))) {
        fprintf(stderr, "wrong data read (8)\n");
        fprintf(stderr, "received: `%.*s'\nexpected: `%s'\n",
                bytes_read, buf, TRL2);
        exit(1);
    }

    bytes_read = 1;
    rv = ap_recv(newsock, buf, &bytes_read);
    if (rv != APR_SUCCESS) {
        fprintf(stderr, "ap_recv()->%d/%s\n",
                rv,
		ap_strerror(rv, buf, sizeof buf));
        exit(1);
    }
    if (bytes_read != 0) {
        fprintf(stderr, "We expected the EOF condition on the connected\n"
                "socket but instead we read %ld bytes.\n",
                (long int)bytes_read);
        exit(1);
    }

    printf("server: ap_sendfile() worked as expected!\n");

    return 0;
}

int main(int argc, char *argv[])
{
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    /* Gee whiz this is goofy logic but I wanna drive sendfile right now, 
     * not dork around with the command line!
     */
    if (argc == 3 && !strcmp(argv[1], "client")) {
        if (!strcmp(argv[2], "blocking")) {
            return client(1);
        }
        else if (!strcmp(argv[2], "timeout")) {
            return client(2);
        }
        else if (!strcmp(argv[2], "nonblocking")) {
            return client(0);
        }
    }
    else if (argc == 2 && !strcmp(argv[1], "server")) {
        return server();
    }

    fprintf(stderr, 
            "Usage: %s client {blocking|nonblocking|timeout}\n"
            "       %s server\n",
            argv[0], argv[0]);
    return -1;
}

#endif /* !APR_HAS_SENDFILE */
