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

#include "networkio.h"

#ifdef HAVE_SENDFILE
/* This file is needed to allow us access to the ap_file_t internals. */
#include "../../file_io/unix/fileio.h"

/* Glibc2.1.1 fails to define TCP_CORK.  This is a bug that will be 
 *fixed in the next release.  It should be 3
 */
#if !defined(TCP_CORK) && defined(__linux__)
#define TCP_CORK 3
#endif

#endif /* HAVE_SENDFILE */

static ap_status_t wait_for_io_or_timeout(ap_socket_t *sock, int for_read)
{
    struct timeval tv;
    fd_set fdset;
    int srv;

    do {
	FD_ZERO(&fdset);
	FD_SET(sock->socketdes, &fdset);
	tv.tv_sec = sock->timeout;
	tv.tv_usec = 0;
	srv = select(FD_SETSIZE,
	    for_read ? &fdset : NULL,
	    for_read ? NULL : &fdset,
	    NULL,
	    sock->timeout < 0 ? NULL : &tv);
    } while (srv == -1 && errno == EINTR);

    if (srv == 0) {
	return APR_TIMEUP;
    }
    else if (srv < 0) {
	return errno;
    }
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_send(ap_socket_t *sock, const char *buf, ap_ssize_t *len)
 *    Send data over a network.
 * arg 1) The socket to send the data over.
 * arg 2) The buffer which contains the data to be sent. 
 * arg 3) The maximum number of bytes to send 
 * NOTE:  This functions acts like a blocking write by default.  To change 
 *        this behavior, use ap_setsocketopt with the APR_SO_TIMEOUT option.
 *        The number of bytes actually sent is stored in argument 3.
 */
ap_status_t ap_send(ap_socket_t *sock, const char *buf, ap_ssize_t *len)
{
    ssize_t rv;
    
    do {
        rv = write(sock->socketdes, buf, (*len));
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout != 0) {
	ap_status_t arv = wait_for_io_or_timeout(sock, 0);
	if (arv != APR_SUCCESS) {
	    *len = 0;
	    return arv;
	}
        else {
            do {
                rv = write(sock->socketdes, buf, (*len));
            } while (rv == -1 && errno == EINTR);
        }
    }
    if (rv == -1) {
	*len = 0;
	return errno;
    }
    (*len) = rv;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_recv(ap_socket_t *sock, char *buf, ap_ssize_t *len)
 *    Read data from a network.
 * arg 1) The socket to read the data from.
 * arg 2) The buffer to store the data in. 
 * arg 3) The maximum number of bytes to read 
 * NOTE:  This functions acts like a blocking write by default.  To change 
 *        this behavior, use ap_setsocketopt with the APR_SO_TIMEOUT option.
 *        The number of bytes actually sent is stored in argument 3.
 */
ap_status_t ap_recv(ap_socket_t *sock, char *buf, ap_ssize_t *len)
{
    ssize_t rv;
    
    do {
        rv = read(sock->socketdes, buf, (*len));
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout != 0) {
	ap_status_t arv = wait_for_io_or_timeout(sock, 1);
	if (arv != APR_SUCCESS) {
	    *len = 0;
	    return arv;
	}
        else {
            do {
                rv = read(sock->socketdes, buf, (*len));
            } while (rv == -1 && errno == EINTR);
        }
    }
    if (rv == -1) {
        (*len) = 0;
        return errno;
    }
    (*len) = rv;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_sendv(ap_socket_t *sock, const struct iovec *vec, 
                        ap_int32_t nvec, ap_int32_t *len)
 *    Send multiple packets of data over a network.
 * arg 1) The socket to send the data over.
 * arg 2) The array of iovec structs containing the data to send 
 * arg 3) The number of iovec structs in the array
 * arg 4) Receives the number of bytes actually written
 * NOTE:  This functions acts like a blocking write by default.  To change 
 *        this behavior, use ap_setsocketopt with the APR_SO_TIMEOUT option.
 *        The number of bytes actually sent is stored in argument 3.
 */
ap_status_t ap_sendv(ap_socket_t * sock, const struct iovec *vec,
                     ap_int32_t nvec, ap_int32_t *len)
{
    ssize_t rv;

    do {
        rv = writev(sock->socketdes, vec, nvec);
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout != 0) {
	ap_status_t arv = wait_for_io_or_timeout(sock, 0);
	if (arv != APR_SUCCESS) {
	    *len = 0;
	    return arv;
	}
        else {
            do {
        	rv = writev(sock->socketdes, vec, nvec);
            } while (rv == -1 && errno == EINTR);
        }
    }
    if (rv == -1) {
	*len = 0;
	return errno;
    }
    (*len) = rv;
    return APR_SUCCESS;
}

#if defined(HAVE_SENDFILE)
/* ***APRDOC********************************************************
 * ap_status_t ap_sendfile(ap_socket_t *sock, ap_file_t *file, ap_hdtr_t *hdtr, 
 *                         ap_off_t *offset, ap_size_t *len, ap_int32_t flags)
 *    Send a file from an open file descriptor to a socket, along with 
 *    optional headers and trailers
 * arg 1) The socket to which we're writing
 * arg 2) The open file from which to read
 * arg 3) A structure containing the headers and trailers to send
 * arg 4) Offset into the file where we should begin writing
 * arg 5) Number of bytes to send 
 * arg 6) OS-specific flags to pass to sendfile()
 * NOTE:  This functions acts like a blocking write by default.  To change 
 *        this behavior, use ap_setsocketopt with the APR_SO_TIMEOUT option.
 *        The number of bytes actually sent is stored in argument 5.
 */

 /* TODO: Verify that all platforms handle the fd the same way 
  *     (i.e. not moving current file pointer)
  *     - Should flags be an int_32 or what?
  */

#if defined(__linux__) && defined(HAVE_WRITEV)
ap_status_t ap_sendfile(ap_socket_t *sock, ap_file_t *file,
        		ap_hdtr_t *hdtr, ap_off_t *offset, ap_size_t *len,
        		ap_int32_t flags)
{
    off_t off = *offset;
    int corkflag = 1;
    int rv, nbytes = 0;
    ap_status_t arv;

    /* TCP_CORK keeps us from sending partial frames when we shouldn't */
    rv = setsockopt(sock->socketdes, SOL_TCP, TCP_CORK,
        	    (const void *) &corkflag, sizeof(corkflag));
    if (rv == -1) {
	*len = 0;
        return errno;
    }

    /* Now write the headers */
    if (hdtr->numheaders > 0) {
        ap_int32_t hdrbytes;
        arv = ap_sendv(sock, hdtr->headers, hdtr->numheaders, &hdrbytes);
        if (arv != APR_SUCCESS) {
	    *len = 0;
            return errno;
        }
        nbytes += hdrbytes;
    }

    do {
        rv = sendfile(sock->socketdes,	/* socket */
        	      file->filedes,	/* open file descriptor of the file to be sent */
        	      &off,	/* where in the file to start */
        	      *len	/* number of bytes to send */
            );
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout != 0) {
	arv = wait_for_io_or_timeout(sock, 0);
	if (arv != APR_SUCCESS) {
	    *len = 0;
	    return arv;
	}
        else {
            do {
        	rv = sendfile(sock->socketdes,	/* socket */
        		      file->filedes,	/* open file descriptor of the file to be sent */
        		      &off,	/* where in the file to start */
        		      *len);	/* number of bytes to send */
            } while (rv == -1 && errno == EINTR);
        }
    }

    if (rv == -1) {
	*len = nbytes;
        return errno;
    }

    nbytes += rv;

    /* Now write the footers */
    if (hdtr->numtrailers > 0) {
        ap_int32_t trbytes;
        arv = ap_sendv(sock, hdtr->trailers, hdtr->numtrailers, &trbytes);
        nbytes += trbytes;
        if (arv != APR_SUCCESS) {
	    *len = nbytes;
            return errno;
        }
    }

    /* Uncork to send queued frames */
    corkflag = 0;
    rv = setsockopt(sock->socketdes, SOL_TCP, TCP_CORK,
                    (const void *) &corkflag, sizeof(corkflag));

    (*len) = nbytes;
    return rv < 0 ? errno : APR_SUCCESS;
}

/* These are just demos of how the code for the other OSes.
 * I haven't tested these, but they're right in terms of interface.
 * I just wanted to see what types of vars would be required from other OSes. 
 */

#elif defined(__FreeBSD__)

/* Release 3.1 or greater */
ap_status_t ap_sendfile(ap_socket_t * sock, ap_file_t * file,
        		ap_hdtr_t * hdtr, ap_off_t * offset, ap_size_t * len,
        		ap_int32_t flags)
{
    off_t nbytes;
    int rv;
    struct sf_hdtr headerstruct;

    headerstruct.headers = hdtr->headers;
    headerstruct.hdr_cnt = hdtr->numheaders;
    headerstruct.trailers = hdtr->trailers;
    headerstruct.trl_cnt = hdtr->numtrailers;


    /* FreeBSD can send the headers/footers as part of the system call */
    do {
        rv = sendfile(file->filedes,	/* open file descriptor of the file to be sent */
        	      sock->socketdes,	/* socket */
        	      *offset,	/* where in the file to start */
        	      (size_t) * len,	/* number of bytes to send */
        	      &headerstruct,	/* Headers/footers */
        	      &nbytes,	/* number of bytes written */
        	      flags	/* undefined, set to 0 */
            );
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout != 0) {
	ap_status_t arv = wait_for_io_or_timeout(sock, 0);
	if (arv != APR_SUCCESS) {
	    *len = 0;
	    return arv;
	}
        else {
            do {
        	rv = sendfile(file->filedes,	/* open file descriptor of the file to be sent */
        		      sock->socketdes,	/* socket */
        		      *offset,	/* where in the file to start */
        		      (size_t) * len,	/* number of bytes to send */
        		      &headerstruct,	/* Headers/footers */
        		      &nbytes,	/* number of bytes written */
        		      flags	/* undefined, set to 0 */
        	    );
            } while (rv == -1 && errno == EINTR);
        }
    }

    (*len) = nbytes;
    if (rv == -1) {
        return errno;
    }
    return APR_SUCCESS;
}

#elif defined(__HPUX__)

#error "there's no way this ap_sendfile implementation works -djg"

/* HP-UX Version 10.30 or greater */
ap_status_t ap_sendfile(ap_socket_t * sock, ap_file_t * file,
        		ap_hdtr_t * hdtr, ap_off_t * offset, ap_size_t * len,
        		ap_int32_t flags)
{
    int i, ptr = 0;
    size_t nbytes = 0, headerlen = 0, trailerlen = 0;
    struct sf_hdtr headerstruct;
    struct iovec hdtrarray[2];
    void *headerbuf, *trailerbuf;


    /* HP-UX can only send one header iovec and one footer iovec */

    for (i = 0; i < hdtr->numheaders; i++) {
        headerlen += hdtr->headers[i].iov_len;
    }

    /* XXX:  BUHHH? wow, what a memory leak! */
    headerbuf = ap_palloc(sock->cntxt, headerlen);

    for (i = 0; i < hdtr->numheaders; i++) {
        memcpy(headerbuf + ptr, hdtr->headers[i].iov_base,
               hdtr->headers[i].iov_len);
        ptr += hdtr->headers[i].iov_len;
    }

    for (i = 0; i < hdtr->numtrailers; i++) {
        trailerlen += hdtr->headers[i].iov_len;
    }

    /* XXX:  BUHHH? wow, what a memory leak! */
    trailerbuf = ap_palloc(sock->cntxt, trailerlen);

    for (i = 0; i < hdtr->numtrailers; i++) {
        memcpy(trailerbuf + ptr, hdtr->trailers[i].iov_base,
               hdtr->trailers[i].iov_len);
        ptr += hdtr->trailers[i].iov_len;
    }

    hdtrarray[0].iov_base = headerbuf;
    hdtrarray[0].iov_len = headerlen;
    hdtrarray[1].iov_base = trailerbuf;
    hdtrarray[1].iov_len = trailerlen;

    do {
        rv = sendfile(sock->socketdes,	/* socket  */
        	      file->filedes,	/* file descriptor to send */
        	      *offset,	/* where in the file to start */
		      /* XXX: as far as i can see, nbytes == 0 always here -djg */
        	      nbytes,	/* number of bytes to send */
        	      hdtrarray,	/* Headers/footers */
        	      flags	/* undefined, set to 0 */
            );
    } while (rv == -1 && errno == EINTR);

    if (rv == -1 && errno == EAGAIN && sock->timeout != 0) {
        struct timeval *tv;
        fd_set fdset;
        int srv;

        do {
            FD_ZERO(&fdset);
            FD_SET(sock->socketdes, &fdset);
            if (sock->timeout < 0) {
        	tv = NULL;
            }
            else {
		/* XXX:  BUHHH? wow, what a memory leak! */
        	tv = ap_palloc(sock->cntxt, sizeof(struct timeval));
        	tv->tv_sec = sock->timeout;
        	tv->tv_usec = 0;
            }
            srv = select(FD_SETSIZE, NULL, &fdset, NULL, tv);
        } while (srv == -1 && errno == EINTR);

        if (srv == 0) {
	    /* XXX: -1 is wrong */
            (*len) = -1;
            return APR_TIMEUP;
        }
        else if (srv < 0) {
	    /* XXX: -1 is wrong */
            (*len) = -1;
            return errno;
        }
        else {
            do {
        	rv = sendfile(sock->socketdes,	/* socket  */
        		      file->filedes,	/* file descriptor to send */
        		      *offset,	/* where in the file to start */
				/* XXX: as far as i can see, nbytes == 0 always here -djg */
        		      nbytes,	/* number of bytes to send */
        		      hdtrarray,	/* Headers/footers */
        		      flags	/* undefined, set to 0 */
        	    );
            } while (rv == -1 && errno == EINTR);
        }
    }


    if (rv == -1) {
	*len = 0;
        return errno;
    }


    /* Set len to the number of bytes written */
    (*len) = rv;
    return APR_SUCCESS;
}
#else
/* TODO: Add AIX support */
#endif /* __linux__, __FreeBSD__, __HPUX__ */
#endif /* HAVE_SENDFILE */

