/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include <errno.h>

#ifndef APR_ERRNO_H
#define APR_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* If this definition of APRStatus changes, then we can remove this, but right
   now, the decision was to use an errno-like implementation.
*/
typedef int ap_status_t;

#define APR_SUCCESS 0

#ifdef EACCES
#define APR_EACCES EACCES
#else
#define APR_EACCES 3000
#endif

#ifdef EEXIST
#define APR_EEXIST EEXIST
#else
#define APR_EEXIST 3001
#endif

#ifdef EISDIR
#define APR_EISDIR EISDIR
#else
#define APR_EISDIR 3002
#endif

#ifdef ENAMETOOLONG
#define APR_ENAMETOOLONG ENAMETOOLONG
#else
#define APR_ENAMETOOLONG 3003
#endif

#ifdef ENOENT
#define APR_ENOENT ENOENT
#else
#define APR_ENOENT 3004
#endif

#ifdef ENOTDIR
#define APR_ENOTDIR ENOTDIR
#else
#define APR_ENOTDIR 3005
#endif

#ifdef ENXIO
#define APR_ENXIO ENXIO
#else
#define APR_ENXIO 3006
#endif

#ifdef ENODEV
#define APR_ENODEV ENODEV
#else
#define APR_ENODEV 3007
#endif

#ifdef EROFS
#define APR_EROFS EROFS
#else
#define APR_EROFS 3008
#endif

#ifdef ETXTBSY
#define APR_ETXTBSY ETXTBSY
#else
#define APR_ETXTBSY 3009
#endif

#ifdef EFAULT
#define APR_EFAULT EFAULT
#else
#define APR_EFAULT 3010
#endif

#ifdef ELOOP
#define APR_ELOOP ELOOP
#else
#define APR_ELOOP 3011
#endif

#ifdef ENOSPC
#define APR_ENOSPC ENOSPC
#else
#define APR_ENOSPC 3012
#endif

#ifdef ENONOMEM
#define APR_ENOMEM ENOMEM
#else
#define APR_ENOMEM 3013
#endif

#ifdef EMFILE
#define APR_EMFILE EMFILE
#else
#define APR_EMFILE 3014
#endif

#ifdef ENFILE
#define APR_ENFILE ENFILE
#else
#define APR_ENFILE 3015
#endif

#ifdef EBADF
#define APR_EBADF EBADF
#else
#define APR_EBADF 3016
#endif

#ifdef EPERM
#define APR_EPERM EPERM
#else
#define APR_EPERM 3017
#endif

#ifdef EIO
#define APR_EIO EIO
#else
#define APR_EIO 3018
#endif

#ifdef EINVAL
#define APR_EINVAL EINVAL
#else
#define APR_EINVAL 3019
#endif

#ifdef ENOEMPTY
#define APR_ENOEMPTY ENOEMPTY
#else
#define APR_ENOEMPTY 3020
#endif

#ifdef EBUSY
#define APR_EBUSY EBUSY
#else
#define APR_EBUSY 3021
#endif

#ifdef ESPIPE
#define APR_ESPIPE ESPIPE
#else
#define APR_ESPIPE 3022
#endif

#ifdef EIDRM
#define APR_EIDRM EIDRM
#else
#define APR_EIDRM 3023
#endif

#ifdef ERANGE
#define APR_ERANGE ERANGE
#else
#define APR_ERANGE 3024
#endif

#ifdef E2BIG
#define APR_E2BIG E2BIG
#else
#define APR_E2BIG 3025
#endif

#ifdef EAGAIN
#define APR_EAGAIN EAGAIN
#else
#define APR_EAGAIN 3026
#endif

#ifdef EFBIG
#define APR_EFBIG EFBIG
#else
#define APR_EFBIG 3027
#endif

#ifdef EINTR
#define APR_EINTR EINTR
#else
#define APR_EINTR 3028
#endif

#ifdef EDEADLK
#define APR_EDEADLK EDEADLK
#else
#define APR_EDEADLK 3029
#endif

#ifdef ENOLCK
#define APR_ENOLCK ENOLCK
#else
#define APR_ENOLCK 3030
#endif

#ifdef EWOULDBLOCK
#define APR_EWOULDBLOCK EWOULDBLOCK
#else
#define APR_EWOULDBLOCK 3031
#endif

#ifdef EPROTONOSUPPORT
#define APR_EPROTONOSUPPORT EPROTONOSUPPORT
#else
#define APR_EPROTONOSUPPORT 3032
#endif

#ifdef ENOTSOCK
#define APR_ENOTSOCK ENOTSOCK
#else
#define APR_ENOTSOCK 3033
#endif

#ifdef ENOTCONN
#define APR_ENOTCONN ENOTCONN
#else
#define APR_ENOTCONN 3034
#endif

#ifdef EOPNOTSUPP
#define APR_EOPNOTSUPP EOPNOTSUPP
#else
#define APR_EOPNOTSUPP 3035
#endif

#ifdef HOST_NOT_FOUND
#define APR_EHOSTNOTFOUND HOST_NOT_FOUND
#else
#define APR_EHOSTNOTFOUND 3036
#endif

#ifdef NO_DATA
#define APR_ENODATA NO_DATA
#else
#define APR_ENODATA 3037
#endif

#ifdef NO_ADDRESS
#define APR_ENOADDRESS NO_ADDRESS
#else
#define APR_ENOADDRESS 3038
#endif

#ifdef NO_RECOVERY
#define APR_ENORECOVERY NO_RECOVERY
#else
#define APR_ENORECOVERY 3039
#endif

#ifdef EISCONN
#define APR_EISCONN EISCONN
#else
#define APR_EISCONN 3040
#endif

#ifdef ETIMEDOUT
#define APR_ETIMEDOUT ETIMEDOUT
#else
#define APR_ETIMEDOUT 3041
#endif

#ifdef ECONNREFUSED
#define APR_ECONNREFUSED ECONNREFUSED
#else
#define APR_ECONNREFUSED 3042
#endif

#ifdef ENETUNREACH
#define APR_ENETUNREACH ENETUNREACH
#else
#define APR_ENETUNREACH 3043
#endif

#ifdef EADDRINUSE
#define APR_EADDRINUSE EADDRINUSE
#else
#define APR_EADDRINUSE 3044
#endif

#ifdef EINPROGRESS
#define APR_EINPROGRESS EINPROGRESS
#else
#define APR_EINPROGRESS 3045
#endif

#ifdef EALREADY
#define APR_EALREADY EALREADY
#else
#define APR_EALREADY 3046
#endif

#ifdef EAFNOSUPPORT
#define APR_EAFNOSUPPORT EAFNOSUPPORT
#else
#define APR_EAFNOSUPPORT 3047
#endif

#ifdef ENOPROTOOPT
#define APR_ENOPROTOOPT ENOPROTOOPT
#else
#define APR_ENOPROTOOPT 3048
#endif

#ifdef ENOCHILD
#define APR_ENOCHILD ENOCHILD
#else
#define APR_ENOCHILD 3049
#endif

#ifdef ESRCH
#define APR_ESRCH ESRCH
#else
#define APR_ESRCH 3050
#endif

#ifdef ENOTSUP
#define APR_ENOTSUP ENOTSUP
#else
#define APR_ENOTSUP 3051
#endif


/*  APR ERROR VALUES */
#define APR_ENOSTAT        4001
#define APR_ENOPOOL        4002
#define APR_ENOFILE        4003
#define APR_EBADDATE       4004
#define APR_ENOCONT        4005
#define APR_ENOPROC        4006
#define APR_ENOTIME        4007
#define APR_ENODIR         4008
#define APR_ENOLOCK        4009
#define APR_ENOPOLL        4010
#define APR_ENOSOCKET      4011
#define APR_ENOTHREAD      4012
#define APR_ENOTHDKEY      4013

/*  APR STATUS VALUES */
#define APR_INCHILD        5001
#define APR_INPARENT       5002
#define APR_DETACH         5003
#define APR_NOTDETACH      5004
#define APR_CHILD_DONE     5005
#define APR_CHILD_NOTDONE  5006
#define APR_TIMEUP         5007
#define APR_INVALSOCK      5008
#define APR_ALLSTD         5009
#define APR_STDOUT         5010
#define APR_STDERR         5011
#define APR_BADCH          5012
#define APR_BADARG         5013
#define APR_EOF            5014
#define APR_NOTFOUND       5015
#ifdef __cplusplus
}
#endif

#endif  /* ! APR_ERRNO_H */
