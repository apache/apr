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

#ifndef APR_ERRNO_H
#define APR_ERRNO_H

#include "apr.h"
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef int ap_status_t;

/* see lib/apr/APRDesign for why this inane function needs to be used
 * everywhere.
 */
int ap_canonical_error(ap_status_t err);


/* APR_OS_START_ERROR is where the APR specific error values should start.
 * APR_OS_START_STATUS is where the APR specific status codes should start.
 * APR_OS_START_USEERR are reserved for applications that use APR that
 *     layer their own error codes along with APR's.
 * APR_OS_START_SYSERR should be used for system error values on 
 *     each platform.  
 */
#define APR_OS_START_ERROR     20000
#define APR_OS_START_STATUS    (APR_OS_START_ERROR + 500)
#define APR_OS_START_USEERR    (APR_OS_START_STATUS + 500)
#define APR_OS_START_CANONERR  (APR_OS_START_USEERR + 500)
#define APR_OS_START_SYSERR    (APR_OS_START_CANONERR + 500)

#define APR_OS2_STATUS(e) (e == 0 ? APR_SUCCESS : e + APR_OS_START_SYSERR)

#define APR_SUCCESS 0

/* APR ERROR VALUES */
#define APR_ENOSTAT        (APR_OS_START_ERROR + 1)
#define APR_ENOPOOL        (APR_OS_START_ERROR + 2)
#define APR_ENOFILE        (APR_OS_START_ERROR + 3)
#define APR_EBADDATE       (APR_OS_START_ERROR + 4)
#define APR_EINVALSOCK     (APR_OS_START_ERROR + 5)
#define APR_ENOPROC        (APR_OS_START_ERROR + 6)
#define APR_ENOTIME        (APR_OS_START_ERROR + 7)
#define APR_ENODIR         (APR_OS_START_ERROR + 8)
#define APR_ENOLOCK        (APR_OS_START_ERROR + 9)
#define APR_ENOPOLL        (APR_OS_START_ERROR + 10)
#define APR_ENOSOCKET      (APR_OS_START_ERROR + 11)
#define APR_ENOTHREAD      (APR_OS_START_ERROR + 12)
#define APR_ENOTHDKEY      (APR_OS_START_ERROR + 13)
/* empty slot: +14 */
#define APR_ENOSHMAVAIL    (APR_OS_START_ERROR + 15)
/* empty slot: +16 */
/* empty slot: +17 */
/* empty slot: +18 */
#define APR_EDSOOPEN       (APR_OS_START_ERROR + 19)

/* APR STATUS VALUES */
#define APR_INCHILD        (APR_OS_START_STATUS + 1)
#define APR_INPARENT       (APR_OS_START_STATUS + 2)
#define APR_DETACH         (APR_OS_START_STATUS + 3)
#define APR_NOTDETACH      (APR_OS_START_STATUS + 4)
#define APR_CHILD_DONE     (APR_OS_START_STATUS + 5)
#define APR_CHILD_NOTDONE  (APR_OS_START_STATUS + 6)
#define APR_TIMEUP         (APR_OS_START_STATUS + 7)
#define APR_INCOMPLETE     (APR_OS_START_STATUS + 8)
/* empty slot: +9 */
/* empty slot: +10 */
/* empty slot: +11 */
#define APR_BADCH          (APR_OS_START_STATUS + 12)
#define APR_BADARG         (APR_OS_START_STATUS + 13)
#define APR_EOF            (APR_OS_START_STATUS + 14)
#define APR_NOTFOUND       (APR_OS_START_STATUS + 15)
/* empty slot: +16 */
/* empty slot: +17 */
/* empty slot: +18 */
#define APR_ANONYMOUS      (APR_OS_START_STATUS + 19)
#define APR_FILEBASED      (APR_OS_START_STATUS + 20)
#define APR_KEYBASED       (APR_OS_START_STATUS + 21)

/* A simple value to be used to initialize a status variable. */
#define APR_EINIT          (APR_OS_START_STATUS + 22)  

/* Not implemented either because we haven't gotten to it yet, or 
 * because it is not possible to do correctly.  
 */
#define APR_ENOTIMPL       (APR_OS_START_STATUS + 23)

/* Passwords do not match.
 */
#define APR_EMISMATCH      (APR_OS_START_STATUS + 24)


/* APR CANONICAL ERROR VALUES */
#ifdef EACCES
#define APR_EACCES EACCES
#else
#define APR_EACCES         (APR_OS_START_CANONERR + 1)
#endif

#ifdef EEXIST
#define APR_EEXIST EEXIST
#else
#define APR_EEXIST         (APR_OS_START_CANONERR + 2)
#endif

#ifdef ENAMETOOLONG
#define APR_ENAMETOOLONG ENAMETOOLONG
#else
#define APR_ENAMETOOLONG   (APR_OS_START_CANONERR + 3)
#endif

#ifdef ENOENT
#define APR_ENOENT ENOENT
#else
#define APR_ENOENT         (APR_OS_START_CANONERR + 4)
#endif

#ifdef ENOTDIR
#define APR_ENOTDIR ENOTDIR
#else
#define APR_ENOTDIR        (APR_OS_START_CANONERR + 5)
#endif

#ifdef ENOSPC
#define APR_ENOSPC ENOSPC
#else
#define APR_ENOSPC         (APR_OS_START_CANONERR + 6)
#endif

#ifdef ENOMEM
#define APR_ENOMEM ENOMEM
#else
#define APR_ENOMEM         (APR_OS_START_CANONERR + 7)
#endif

#ifdef EMFILE
#define APR_EMFILE EMFILE
#else
#define APR_EMFILE         (APR_OS_START_CANONERR + 8)
#endif

#ifdef ENFILE
#define APR_ENFILE ENFILE
#else
#define APR_ENFILE         (APR_OS_START_CANONERR + 9)
#endif

#ifdef EBADF
#define APR_EBADF EBADF
#else
#define APR_EBADF          (APR_OS_START_CANONERR + 10)
#endif

#ifdef EINVAL
#define APR_EINVAL EINVAL
#else
#define APR_EINVAL         (APR_OS_START_CANONERR + 11)
#endif

#ifdef ESPIPE
#define APR_ESPIPE ESPIPE
#else
#define APR_ESPIPE         (APR_OS_START_CANONERR + 12)
#endif

#ifdef EAGAIN
#define APR_EAGAIN EAGAIN
#else
#define APR_EAGAIN         (APR_OS_START_CANONERR + 13)
#endif

#ifdef EINTR
#define APR_EINTR EINTR
#else
#define APR_EINTR          (APR_OS_START_CANONERR + 14)
#endif

#ifdef ENOTSOCK
#define APR_ENOTSOCK ENOTSOCK
#else
#define APR_ENOTSOCK       (APR_OS_START_CANONERR + 15)
#endif

#ifdef ECONNREFUSED
#define APR_ECONNREFUSED ECONNREFUSED
#else
#define APR_ECONNREFUSED   (APR_OS_START_CANONERR + 16)
#endif

#ifdef EINPROGRESS
#define APR_EINPROGRESS EINPROGRESS
#else
#define APR_EINPROGRESS    (APR_OS_START_CANONERR + 17)
#endif

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_ERRNO_H */
