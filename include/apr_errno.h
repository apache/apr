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

#include <errno.h>
#if WIN32
#include "apr_win.h"
#else
#include "apr.h"
#endif

#ifndef APR_ERRNO_H
#define APR_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Define four layers of status code offsets so that we don't interfere
 * with the predefined errno codes on the operating system.  Codes beyond
 * APR_OS_START_USEERR are reserved for applications that use APR that
 * layer their own error codes along with APR's.
 */
#ifndef APR_OS_START_ERROR
#define APR_OS_START_ERROR   4000
#endif
#ifndef APR_OS_START_STATUS
#define APR_OS_START_STATUS  (APR_OS_START_ERROR + 500)
#endif
#ifndef APR_OS_START_SYSERR
#define APR_OS_START_SYSERR  (APR_OS_START_STATUS + 500)
#endif
#ifndef APR_OS_START_USEERR
#define APR_OS_START_USEERR  (APR_OS_START_SYSERR + 500)
#endif

/* If this definition of APRStatus changes, then we can remove this, but right
 * now, the decision was to use an errno-like implementation.
 */
typedef int ap_status_t;

#define APR_SUCCESS 0

/* APR ERROR VALUES */
#define APR_ENOSTAT        (APR_OS_START_ERROR + 1)
#define APR_ENOPOOL        (APR_OS_START_ERROR + 2)
#define APR_ENOFILE        (APR_OS_START_ERROR + 3)
#define APR_EBADDATE       (APR_OS_START_ERROR + 4)
#define APR_ENOCONT        (APR_OS_START_ERROR + 5)
#define APR_ENOPROC        (APR_OS_START_ERROR + 6)
#define APR_ENOTIME        (APR_OS_START_ERROR + 7)
#define APR_ENODIR         (APR_OS_START_ERROR + 8)
#define APR_ENOLOCK        (APR_OS_START_ERROR + 9)
#define APR_ENOPOLL        (APR_OS_START_ERROR + 10)
#define APR_ENOSOCKET      (APR_OS_START_ERROR + 11)
#define APR_ENOTHREAD      (APR_OS_START_ERROR + 12)
#define APR_ENOTHDKEY      (APR_OS_START_ERROR + 13)
#define APR_ENOTTHREADSAFE (APR_OS_START_ERROR + 14)
#define APR_ESHMLOCK       (APR_OS_START_ERROR + 15)
#define APR_EFUNCNOTFOUND  (APR_OS_START_ERROR + 16)
#define APR_ENOFUNCPOINTER (APR_OS_START_ERROR + 17)
#define APR_ENODSOHANDLE   (APR_OS_START_ERROR + 18)
#define APR_EDSOOPEN       (APR_OS_START_ERROR + 19)
#define APR_EBADARG        (APR_OS_START_ERROR + 20)

/* APR STATUS VALUES */
#define APR_INCHILD        (APR_OS_START_STATUS + 1)
#define APR_INPARENT       (APR_OS_START_STATUS + 2)
#define APR_DETACH         (APR_OS_START_STATUS + 3)
#define APR_NOTDETACH      (APR_OS_START_STATUS + 4)
#define APR_CHILD_DONE     (APR_OS_START_STATUS + 5)
#define APR_CHILD_NOTDONE  (APR_OS_START_STATUS + 6)
#define APR_TIMEUP         (APR_OS_START_STATUS + 7)
#define APR_INVALSOCK      (APR_OS_START_STATUS + 8)
#define APR_ALLSTD         (APR_OS_START_STATUS + 9)
#define APR_STDOUT         (APR_OS_START_STATUS + 10)
#define APR_STDERR         (APR_OS_START_STATUS + 11)
#define APR_BADCH          (APR_OS_START_STATUS + 12)
#define APR_BADARG         (APR_OS_START_STATUS + 13)
#define APR_EOF            (APR_OS_START_STATUS + 14)
#define APR_NOTFOUND       (APR_OS_START_STATUS + 15)
#define APR_LESS           (APR_OS_START_STATUS + 16)
#define APR_EQUAL          (APR_OS_START_STATUS + 17)
#define APR_MORE           (APR_OS_START_STATUS + 18)
#define APR_ANONYMOUS      (APR_OS_START_STATUS + 19)
#define APR_FILEBASED      (APR_OS_START_STATUS + 20)
#define APR_KEYBASED       (APR_OS_START_STATUS + 21)

/* A simple value to be used to initialze a status variable. */
#define APR_EINIT          (APR_OS_START_STATUS + 16)  

/* Not implemented either because we haven't gotten to it yet, or 
 * because it is not possible to do correctly.  
 */
#define APR_ENOTIMPL       (APR_OS_START_STATUS + 17)

/* Passwords do not match.
 */
#define APR_EMISMATCH      (APR_OS_START_STATUS + 18)

/*
 * APR equivalents to what should be standard errno codes.
 */
#ifdef EACCES
#define APR_EACCES EACCES
#else
#define APR_EACCES (APR_OS_START_SYSERR + 0)
#endif

#ifdef EEXIST
#define APR_EEXIST EEXIST
#else
#define APR_EEXIST (APR_OS_START_SYSERR + 1)
#endif

#ifdef EISDIR
#define APR_EISDIR EISDIR
#else
#define APR_EISDIR (APR_OS_START_SYSERR + 2)
#endif

#ifdef ENAMETOOLONG
#define APR_ENAMETOOLONG ENAMETOOLONG
#else
#define APR_ENAMETOOLONG (APR_OS_START_SYSERR + 3)
#endif

#ifdef ENOENT
#define APR_ENOENT ENOENT
#else
#define APR_ENOENT (APR_OS_START_SYSERR + 4)
#endif

#ifdef ENOTDIR
#define APR_ENOTDIR ENOTDIR
#else
#define APR_ENOTDIR (APR_OS_START_SYSERR + 5)
#endif

#ifdef ENXIO
#define APR_ENXIO ENXIO
#else
#define APR_ENXIO (APR_OS_START_SYSERR + 6)
#endif

#ifdef ENODEV
#define APR_ENODEV ENODEV
#else
#define APR_ENODEV (APR_OS_START_SYSERR + 7)
#endif

#ifdef EROFS
#define APR_EROFS EROFS
#else
#define APR_EROFS (APR_OS_START_SYSERR + 8)
#endif

#ifdef ETXTBSY
#define APR_ETXTBSY ETXTBSY
#else
#define APR_ETXTBSY (APR_OS_START_SYSERR + 9)
#endif

#ifdef EFAULT
#define APR_EFAULT EFAULT
#else
#define APR_EFAULT (APR_OS_START_SYSERR + 10)
#endif

#ifdef ELOOP
#define APR_ELOOP ELOOP
#else
#define APR_ELOOP (APR_OS_START_SYSERR + 11)
#endif

#ifdef ENOSPC
#define APR_ENOSPC ENOSPC
#else
#define APR_ENOSPC (APR_OS_START_SYSERR + 12)
#endif

#ifdef ENONOMEM
#define APR_ENOMEM ENOMEM
#else
#define APR_ENOMEM (APR_OS_START_SYSERR + 13)
#endif

#ifdef EMFILE
#define APR_EMFILE EMFILE
#else
#define APR_EMFILE (APR_OS_START_SYSERR + 14)
#endif

#ifdef ENFILE
#define APR_ENFILE ENFILE
#else
#define APR_ENFILE (APR_OS_START_SYSERR + 15)
#endif

#ifdef EBADF
#define APR_EBADF EBADF
#else
#define APR_EBADF (APR_OS_START_SYSERR + 16)
#endif

#ifdef EPERM
#define APR_EPERM EPERM
#else
#define APR_EPERM (APR_OS_START_SYSERR + 17)
#endif

#ifdef EIO
#define APR_EIO EIO
#else
#define APR_EIO (APR_OS_START_SYSERR + 18)
#endif

#ifdef EINVAL
#define APR_EINVAL EINVAL
#else
#define APR_EINVAL (APR_OS_START_SYSERR + 19)
#endif

#ifdef ENOEMPTY
#define APR_ENOEMPTY ENOEMPTY
#else
#define APR_ENOEMPTY (APR_OS_START_SYSERR + 20)
#endif

#ifdef EBUSY
#define APR_EBUSY EBUSY
#else
#define APR_EBUSY (APR_OS_START_SYSERR + 21)
#endif

#ifdef ESPIPE
#define APR_ESPIPE ESPIPE
#else
#define APR_ESPIPE (APR_OS_START_SYSERR + 22)
#endif

#ifdef EIDRM
#define APR_EIDRM EIDRM
#else
#define APR_EIDRM (APR_OS_START_SYSERR + 23)
#endif

#ifdef ERANGE
#define APR_ERANGE ERANGE
#else
#define APR_ERANGE (APR_OS_START_SYSERR + 24)
#endif

#ifdef E2BIG
#define APR_E2BIG E2BIG
#else
#define APR_E2BIG (APR_OS_START_SYSERR + 25)
#endif

#ifdef EAGAIN
#define APR_EAGAIN EAGAIN
#else
#define APR_EAGAIN (APR_OS_START_SYSERR + 26)
#endif

#ifdef EFBIG
#define APR_EFBIG EFBIG
#else
#define APR_EFBIG (APR_OS_START_SYSERR + 27)
#endif

#ifdef EINTR
#define APR_EINTR EINTR
#else
#define APR_EINTR (APR_OS_START_SYSERR + 28)
#endif

#ifdef EDEADLK
#define APR_EDEADLK EDEADLK
#else
#define APR_EDEADLK (APR_OS_START_SYSERR + 29)
#endif

#ifdef ENOLCK
#define APR_ENOLCK ENOLCK
#else
#define APR_ENOLCK (APR_OS_START_SYSERR + 30)
#endif

#ifdef EWOULDBLOCK
#define APR_EWOULDBLOCK EWOULDBLOCK
#else
#define APR_EWOULDBLOCK (APR_OS_START_SYSERR + 31)
#endif

#ifdef EPROTONOSUPPORT
#define APR_EPROTONOSUPPORT EPROTONOSUPPORT
#else
#define APR_EPROTONOSUPPORT (APR_OS_START_SYSERR + 32)
#endif

#ifdef ENOTSOCK
#define APR_ENOTSOCK ENOTSOCK
#else
#define APR_ENOTSOCK (APR_OS_START_SYSERR + 33)
#endif

#ifdef ENOTCONN
#define APR_ENOTCONN ENOTCONN
#else
#define APR_ENOTCONN (APR_OS_START_SYSERR + 34)
#endif

#ifdef EOPNOTSUPP
#define APR_EOPNOTSUPP EOPNOTSUPP
#else
#define APR_EOPNOTSUPP (APR_OS_START_SYSERR + 35)
#endif

/* never use h_errno values as-is because doing so makes them 
 * indistinguishable from errno values
 * APR_EHOSTNOTFOUND corresponds to HOST_NOT_FOUND
 * APR_ENODATA corresponds to NO_DATA
 * APR_ENOADDRESS corresponds to NO_ADDRESS
 * APR_ENORECOVERY corresponds to NO_RECOVERY
 */
#define APR_EHOSTNOTFOUND (APR_OS_START_SYSERR + 36)

#define APR_ENODATA (APR_OS_START_SYSERR + 37)

#define APR_ENOADDRESS (APR_OS_START_SYSERR + 38)

#define APR_ENORECOVERY (APR_OS_START_SYSERR + 39)

#ifdef EISCONN
#define APR_EISCONN EISCONN
#else
#define APR_EISCONN (APR_OS_START_SYSERR + 40)
#endif

#ifdef ETIMEDOUT
#define APR_ETIMEDOUT ETIMEDOUT
#else
#define APR_ETIMEDOUT (APR_OS_START_SYSERR + 41)
#endif

#ifdef ECONNREFUSED
#define APR_ECONNREFUSED ECONNREFUSED
#else
#define APR_ECONNREFUSED (APR_OS_START_SYSERR + 42)
#endif

#ifdef ENETUNREACH
#define APR_ENETUNREACH ENETUNREACH
#else
#define APR_ENETUNREACH (APR_OS_START_SYSERR + 43)
#endif

#ifdef EADDRINUSE
#define APR_EADDRINUSE EADDRINUSE
#else
#define APR_EADDRINUSE (APR_OS_START_SYSERR + 44)
#endif

#ifdef EINPROGRESS
#define APR_EINPROGRESS EINPROGRESS
#else
#define APR_EINPROGRESS (APR_OS_START_SYSERR + 45)
#endif

#ifdef EALREADY
#define APR_EALREADY EALREADY
#else
#define APR_EALREADY (APR_OS_START_SYSERR + 46)
#endif

#ifdef EAFNOSUPPORT
#define APR_EAFNOSUPPORT EAFNOSUPPORT
#else
#define APR_EAFNOSUPPORT (APR_OS_START_SYSERR + 47)
#endif

#ifdef ENOPROTOOPT
#define APR_ENOPROTOOPT ENOPROTOOPT
#else
#define APR_ENOPROTOOPT (APR_OS_START_SYSERR + 48)
#endif

#ifdef ENOCHILD
#define APR_ENOCHILD ENOCHILD
#else
#define APR_ENOCHILD (APR_OS_START_SYSERR + 49)
#endif

#ifdef ESRCH
#define APR_ESRCH ESRCH
#else
#define APR_ESRCH (APR_OS_START_SYSERR + 50)
#endif

#ifdef ENOTSUP
#define APR_ENOTSUP ENOTSUP
#else
#define APR_ENOTSUP (APR_OS_START_SYSERR + 51)
#endif

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_ERRNO_H */
