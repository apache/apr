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

#if APR_HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @package Error Codes
 */

/**
 * Type for specifying an error or status code.
 */
typedef int apr_status_t;

/**
 * Return a human readable string describing the specified error.
 * @param statcode The error code the get a string for.
 * @param buf A buffer to hold the error string.
 * @param bufsize Size of the buffer to hold the string.
 * @deffunc char *apr_strerror(apr_status_t statcode, char *buf, apr_size_t bufsize)
 */
APR_DECLARE(char *) apr_strerror(apr_status_t statcode, char *buf, 
                                 apr_size_t bufsize);

/**
 * Fold a platform specific error into an apr_status_t code.
 * @param syserr The platform os error code.
 * @deffunc apr_status_t APR_FROM_OS_ERROR(os_err_type syserr)
 * @tip Warning: macro implementation; the syserr argument may be evaluated
 *      multiple times.
 */

/**
 * Fold an apr_status_t code back to the native platform defined error.
 * @param syserr The apr_status_t folded platform os error code.
 * @deffunc os_err_type APR_TO_OS_ERROR(apr_status_t statcode)
 * @tip Warning: macro implementation; the statcode argument may be evaluated
 *      multiple times.  If the statcode was not created by apr_get_os_error 
 *      or APR_FROM_OS_ERROR, the results are undefined.
 */

/**
 * Return the last platform error, folded into apr_status_t, on some platforms
 * @deffunc apr_status_t apr_get_os_error()
 * @tip This retrieves errno, or calls a GetLastError() style function, and
 *      folds it with APR_FROM_OS_ERROR.  Some platforms (such as OS2) have no
 *      such mechanism, so this call may be unsupported.  Some platforms
 *      require the alternate apr_get_netos_error() to retrieve the last
 *      socket error.
 */

/**
 * Return the last socket error, folded into apr_status_t, on some platforms
 * @deffunc apr_status_t apr_get_netos_error()
 * @tip This retrieves errno, h_errno, or calls a GetLastSocketError() style
 *      function, and folds it with APR_FROM_OS_ERROR.  Some platforms (such
 *      as OS2) have no such mechanism, so this call may be unsupported.
 */

/**
 * Reset the last platform error, unfolded from an apr_status_t, on some platforms
 * @param statcode The OS error folded in a prior call to APR_FROM_OS_ERROR()
 * @deffunc void apr_set_os_error(apr_status_t statcode)
 * @tip Warning: macro implementation; the statcode argument may be evaluated
 *      multiple times.  If the statcode was not created by apr_get_os_error
 *      or APR_FROM_OS_ERROR, the results are undefined.  This macro sets
 *      errno, or calls a SetLastError() style function, unfolding statcode
 *      with APR_TO_OS_ERROR.  Some platforms (such as OS2) have no such
 *      mechanism, so this call may be unsupported.
 */

#define apr_set_os_error(e)  (SetLastError(APR_TO_OS_ERROR(e)))

/**
 * APR_OS_START_ERROR is where the APR specific error values start.
 */
/**
 * APR_OS_START_STATUS is where the APR specific status codes start.
 */
/**
 * APR_OS_START_USEERR are reserved for applications that use APR that
 *     layer their own error codes along with APR's.
 */
/**
 * APR_OS_START_CANONERR is where APR versions of errno values are defined
 *     on systems which don't have the corresponding errno.
 */
/**
 * APR_OS_START_SYSERR folds platform-specific system error values into 
 *     apr_status_t values.
 */
#define APR_OS_START_ERROR     20000
#define APR_OS_START_STATUS    (APR_OS_START_ERROR + 500)
#define APR_OS_START_USEERR    (APR_OS_START_STATUS + 500)
#define APR_OS_START_CANONERR  (APR_OS_START_USEERR + 500)
#define APR_OS_START_SYSERR    (APR_OS_START_CANONERR + 500)

#define APR_SUCCESS 0

/**
 * <PRE>
 * <b>APR ERROR VALUES</b>
 * APR_ENOSTAT      APR was unable to perform a stat on the file 
 * APR_ENOPOOL      APR was not provided a pool with which to allocate memory
 * APR_EBADDATE     APR was given an invalid date 
 * APR_EINVALSOCK   APR was given an invalid socket
 * APR_ENOFILE      APR was not given a file structure
 * APR_ENOPROC      APR was not given a process structure
 * APR_ENOTIME      APR was not given a time structure
 * APR_ENODIR       APR was not given a directory structure
 * APR_ENOLOCK      APR was not given a lock structure
 * APR_ENOPOLL      APR was not given a poll structure
 * APR_ENOSOCKET    APR was not given a socket
 * APR_ENOTHREAD    APR was not given a thread structure
 * APR_ENOTHDKEY    APR was not given a thread key structure
 * APR_ENOSHMAVAIL  There is no more shared memory available
 * APR_EDSOOPEN     APR was unable to open the dso object.  For more 
 *                  information call apr_dso_error().
 * </PRE>
 *
 * <PRE>
 * <b>APR STATUS VALUES</b>
 * APR_INCHILD        Program is currently executing in the child
 * APR_INPARENT       Program is currently executing in the parent
 * APR_DETACH         The thread is detached
 * APR_NOTDETACH      The thread is not detached
 * APR_CHILD_DONE     The child has finished executing
 * APR_CHILD_NOTDONE  The child has not finished executing
 * APR_TIMEUP         The operation did not finish before the timeout
 * APR_INCOMPLETE     The character conversion stopped because of an 
 *                    incomplete character or shift sequence at the end 
 *                    of the input buffer.
 * APR_BADCH          Getopt found an option not in the option string
 * APR_BADARG         Getopt found an option that is missing an argument 
 *                    and and argument was specified in the option string
 * APR_EOF            APR has encountered the end of the file
 * APR_NOTFOUND       APR was unable to find the socket in the poll structure
 * APR_ANONYMOUS      APR is using anonymous shared memory
 * APR_FILEBASED      APR is using a file name as the key to the shared memory
 * APR_KEYBASED       APR is using a shared key as the key to the shared memory
 * APR_EINIT          Ininitalizer value.  If no option has been found, but 
 *                    the status variable requires a value, this should be used
 * APR_ENOTIMPL       The APR function has not been implemented on this 
 *                    platform, either because nobody has gotten to it yet, 
 *                    or the function is impossible on this platform.
 * APR_EMISMATCH      Two passwords do not match.
 * </PRE>
 * 
 * @param status The APR_status code to check.
 * @param statcode The apr status code to test.
 * @deffunc int APR_STATUS_IS_status(apr_status_t statcode)
 * @tip Warning: macro implementations; the statcode argument may be
 *      evaluated multiple times.  To test for APR_ENOFILE, always test
 *      APR_STATUS_IS_ENOFILE(statcode) because platform-specific codes are
 *      not necessarily translated into the corresponding APR_Estatus code.
 */

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


/* APR ERROR VALUE TESTS */
#define APR_STATUS_IS_ENOSTAT(s)        ((s) == APR_ENOSTAT)
#define APR_STATUS_IS_ENOPOOL(s)        ((s) == APR_ENOPOOL)
#define APR_STATUS_IS_ENOFILE(s)        ((s) == APR_ENOFILE)
#define APR_STATUS_IS_EBADDATE(s)       ((s) == APR_EBADDATE)
#define APR_STATUS_IS_EINVALSOCK(s)     ((s) == APR_EINVALSOCK)
#define APR_STATUS_IS_ENOPROC(s)        ((s) == APR_ENOPROC)
#define APR_STATUS_IS_ENOTIME(s)        ((s) == APR_ENOTIME)
#define APR_STATUS_IS_ENODIR(s)         ((s) == APR_ENODIR)
#define APR_STATUS_IS_ENOLOCK(s)        ((s) == APR_ENOLOCK)
#define APR_STATUS_IS_ENOPOLL(s)        ((s) == APR_ENOPOLL)
#define APR_STATUS_IS_ENOSOCKET(s)      ((s) == APR_ENOSOCKET)
#define APR_STATUS_IS_ENOTHREAD(s)      ((s) == APR_ENOTHREAD)
#define APR_STATUS_IS_ENOTHDKEY(s)      ((s) == APR_ENOTHDKEY)
/* empty slot: +14 */
#define APR_STATUS_IS_ENOSHMAVAIL(s)    ((s) == APR_ENOSHMAVAIL)
/* empty slot: +16 */
/* empty slot: +17 */
/* empty slot: +18 */
#define APR_STATUS_IS_EDSOOPEN(s)       ((s) == APR_EDSOOPEN)


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
#define APR_EINIT          (APR_OS_START_STATUS + 22)  
#define APR_ENOTIMPL       (APR_OS_START_STATUS + 23)
#define APR_EMISMATCH      (APR_OS_START_STATUS + 24)


/* APR STATUS VALUE TESTS */
#define APR_STATUS_IS_INCHILD(s)        ((s) == APR_INCHILD)
#define APR_STATUS_IS_INPARENT(s)       ((s) == APR_INPARENT)
#define APR_STATUS_IS_DETACH(s)         ((s) == APR_DETACH)
#define APR_STATUS_IS_NOTDETACH(s)      ((s) == APR_NOTDETACH)
#define APR_STATUS_IS_CHILD_DONE(s)     ((s) == APR_CHILD_DONE)
#define APR_STATUS_IS_CHILD_NOTDONE(s)  ((s) == APR_CHILD_NOTDONE)
#define APR_STATUS_IS_TIMEUP(s)         ((s) == APR_TIMEUP)
#define APR_STATUS_IS_INCOMPLETE(s)     ((s) == APR_INCOMPLETE)
/* empty slot: +9 */
/* empty slot: +10 */
/* empty slot: +11 */
#define APR_STATUS_IS_BADCH(s)          ((s) == APR_BADCH)
#define APR_STATUS_IS_BADARG(s)         ((s) == APR_BADARG)
#define APR_STATUS_IS_EOF(s)            ((s) == APR_EOF)
#define APR_STATUS_IS_NOTFOUND(s)       ((s) == APR_NOTFOUND)
/* empty slot: +16 */
/* empty slot: +17 */
/* empty slot: +18 */
#define APR_STATUS_IS_ANONYMOUS(s)      ((s) == APR_ANONYMOUS)
#define APR_STATUS_IS_FILEBASED(s)      ((s) == APR_FILEBASED)
#define APR_STATUS_IS_KEYBASED(s)       ((s) == APR_KEYBASED)
#define APR_STATUS_IS_EINIT(s)          ((s) == APR_EINIT)
#define APR_STATUS_IS_ENOTIMPL(s)       ((s) == APR_ENOTIMPL)
#define APR_STATUS_IS_EMISMATCH(s)      ((s) == APR_EMISMATCH)


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
#elif defined(EWOULDBLOCK)
#define APR_EAGAIN EWOULDBLOCK
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

#ifdef ECONNABORTED
#define APR_ECONNABORTED ECONNABORTED
#else
#define APR_ECONNABORTED   (APR_OS_START_CANONERR + 18)
#endif

#ifdef ECONNRESET
#define APR_ECONNRESET ECONNRESET
#else
#define APR_ECONNRESET     (APR_OS_START_CANONERR + 19)
#endif

#ifdef ETIMEDOUT
#define APR_ETIMEDOUT ETIMEDOUT
#else
#define APR_ETIMEDOUT      (APR_OS_START_CANONERR + 20)
#endif

#ifdef EHOSTUNREACH
#define APR_EHOSTUNREACH EHOSTUNREACH
#else
#define APR_EHOSTUNREACH   (APR_OS_START_CANONERR + 21)
#endif

#ifdef ENETUNREACH
#define APR_ENETUNREACH ENETUNREACH
#else
#define APR_ENETUNREACH    (APR_OS_START_CANONERR + 22)
#endif


#if defined(OS2)

#define APR_FROM_OS_ERROR(e) (e == 0 ? APR_SUCCESS : e + APR_OS_START_SYSERR)
#define APR_TO_OS_ERROR(e)   (e == 0 ? APR_SUCCESS : e - APR_OS_START_SYSERR)

#define INCL_DOSERRORS
#define INCL_DOS
#include <os2.h>
#include "../network_io/os2/os2nerrno.h"
/* And this needs to be greped away for good:
 */
#define APR_OS2_STATUS(e) (APR_FROM_OS_ERROR(e))

#define APR_STATUS_IS_SUCCESS(s)           ((s) == APR_SUCCESS \
                || (s) == APR_OS_START_SYSERR + NO_ERROR)

/* APR CANONICAL ERROR TESTS */
#define APR_STATUS_IS_EACCES(s)         ((s) == APR_EACCES \
                || (s) == APR_OS_START_SYSERR + ERROR_ACCESS_DENIED \
                || (s) == APR_OS_START_SYSERR + ERROR_SHARING_VIOLATION)
#define APR_STATUS_IS_EEXIST(s)         ((s) == APR_EEXIST)
#define APR_STATUS_IS_ENAMETOOLONG(s)   ((s) == APR_ENAMETOOLONG \
                || (s) == APR_OS_START_SYSERR + ERROR_FILENAME_EXCED_RANGE \
                || (s) == APR_OS_START_SYSERR + SOCENAMETOOLONG)
#define APR_STATUS_IS_ENOENT(s)         ((s) == APR_ENOENT \
                || (s) == APR_OS_START_SYSERR + ERROR_FILE_NOT_FOUND \
                || (s) == APR_OS_START_SYSERR + ERROR_PATH_NOT_FOUND \
                || (s) == APR_OS_START_SYSERR + ERROR_OPEN_FAILED)
#define APR_STATUS_IS_ENOTDIR(s)        ((s) == APR_ENOTDIR)
#define APR_STATUS_IS_ENOSPC(s)         ((s) == APR_ENOSPC \
                || (s) == APR_OS_START_SYSERR + ERROR_DISK_FULL)
#define APR_STATUS_IS_ENOMEM(s)         ((s) == APR_ENOMEM)
#define APR_STATUS_IS_EMFILE(s)         ((s) == APR_EMFILE \
                || (s) == APR_OS_START_SYSERR + ERROR_TOO_MANY_OPEN_FILES)
#define APR_STATUS_IS_ENFILE(s)         ((s) == APR_ENFILE)
#define APR_STATUS_IS_EBADF(s)          ((s) == APR_EBADF \
                || (s) == APR_OS_START_SYSERR + ERROR_INVALID_HANDLE)
#define APR_STATUS_IS_EINVAL(s)         ((s) == APR_EINVAL \
                || (s) == APR_OS_START_SYSERR + ERROR_INVALID_PARAMETER \
                || (s) == APR_OS_START_SYSERR + ERROR_INVALID_FUNCTION)
#define APR_STATUS_IS_ESPIPE(s)         ((s) == APR_ESPIPE \
                || (s) == APR_OS_START_SYSERR + ERROR_NEGATIVE_SEEK)
#define APR_STATUS_IS_EAGAIN(s)         ((s) == APR_EAGAIN \
                || (s) == APR_OS_START_SYSERR + ERROR_NO_DATA \
                || (s) == APR_OS_START_SYSERR + SOCEWOULDBLOCK \
                || (s) == APR_OS_START_SYSERR + ERROR_LOCK_VIOLATION)
#define APR_STATUS_IS_EINTR(s)          ((s) == APR_EINTR \
                || (s) == APR_OS_START_SYSERR + SOCEINTR)
#define APR_STATUS_IS_ENOTSOCK(s)       ((s) == APR_ENOTSOCK \
                || (s) == APR_OS_START_SYSERR + SOCENOTSOCK)
#define APR_STATUS_IS_ECONNREFUSED(s)   ((s) == APR_ECONNREFUSED \
                || (s) == APR_OS_START_SYSERR + SOCECONNREFUSED)
#define APR_STATUS_IS_EINPROGRESS(s)    ((s) == APR_EINPROGRESS \
                || (s) == APR_OS_START_SYSERR + SOCEINPROGRESS)
#define APR_STATUS_IS_ECONNABORTED(s)   ((s) == APR_ECONNABORTED \
                || (s) == APR_OS_START_SYSERR + SOCECONNABORTED)
#define APR_STATUS_IS_ECONNRESET(s)     ((s) == APR_ECONNRESET \
                || (s) == APR_OS_START_SYSERR + SOCECONNRESET)
#define APR_STATUS_IS_ETIMEDOUT(s)      ((s) == APR_ETIMEDOUT \
                || (s) == APR_OS_START_SYSERR + SOCETIMEDOUT)    
#define APR_STATUS_IS_EHOSTUNREACH(s)   ((s) == APR_EHOSTUNREACH \
                || (s) == APR_OS_START_SYSERR + SOCEHOSTUNREACH)
#define APR_STATUS_IS_ENETUNREACH(s)    ((s) == APR_ENETUNREACH \
                || (s) == APR_OS_START_SYSERR + SOCENETUNREACH)

/*
    Sorry, too tired to wrap this up for OS2... feel free to
    fit the following into their best matches.

    { ERROR_NO_SIGNAL_SENT,     ESRCH           },
    { SOCEALREADY,              EALREADY        },
    { SOCEDESTADDRREQ,          EDESTADDRREQ    },
    { SOCEMSGSIZE,              EMSGSIZE        },
    { SOCEPROTOTYPE,            EPROTOTYPE      },
    { SOCENOPROTOOPT,           ENOPROTOOPT     },
    { SOCEPROTONOSUPPORT,       EPROTONOSUPPORT },
    { SOCESOCKTNOSUPPORT,       ESOCKTNOSUPPORT },
    { SOCEOPNOTSUPP,            EOPNOTSUPP      },
    { SOCEPFNOSUPPORT,          EPFNOSUPPORT    },
    { SOCEAFNOSUPPORT,          EAFNOSUPPORT    },
    { SOCEADDRINUSE,            EADDRINUSE      },
    { SOCEADDRNOTAVAIL,         EADDRNOTAVAIL   },
    { SOCENETDOWN,              ENETDOWN        },
    { SOCENETRESET,             ENETRESET       },
    { SOCENOBUFS,               ENOBUFS         },
    { SOCEISCONN,               EISCONN         },
    { SOCENOTCONN,              ENOTCONN        },
    { SOCESHUTDOWN,             ESHUTDOWN       },
    { SOCETOOMANYREFS,          ETOOMANYREFS    },
    { SOCELOOP,                 ELOOP           },
    { SOCEHOSTDOWN,             EHOSTDOWN       },
    { SOCENOTEMPTY,             ENOTEMPTY       },
    { SOCEPIPE,                 EPIPE           }
*/

#elif defined(WIN32) /* endif defined(OS2) */

#define APR_FROM_OS_ERROR(e) (e == 0 ? APR_SUCCESS : e + APR_OS_START_SYSERR)
#define APR_TO_OS_ERROR(e)   (e == 0 ? APR_SUCCESS : e - APR_OS_START_SYSERR)

#define apr_get_os_error()   (APR_FROM_OS_ERROR(GetLastError()))
#define apr_set_os_error(e)  (SetLastError(APR_TO_OS_ERROR(e)))

/* A special case, only Win32 winsock calls require this:
 */
#define apr_get_netos_error()   (APR_FROM_OS_ERROR(WSAGetLastError()))

#define APR_STATUS_IS_SUCCESS(s)           ((s) == APR_SUCCESS \
                || (s) == APR_OS_START_SYSERR + ERROR_SUCCESS)

/* APR CANONICAL ERROR TESTS */
#define APR_STATUS_IS_EACCES(s)         ((s) == APR_EACCES \
                || (s) == APR_OS_START_SYSERR + ERROR_ACCESS_DENIED \
                || (s) == APR_OS_START_SYSERR + ERROR_SHARING_VIOLATION)
#define APR_STATUS_IS_EEXIST(s)         ((s) == APR_EEXIST \
                || (s) == APR_OS_START_SYSERR + ERROR_FILE_EXISTS \
                || (s) == APR_OS_START_SYSERR + ERROR_ALREADY_EXISTS)
#define APR_STATUS_IS_ENAMETOOLONG(s)   ((s) == APR_ENAMETOOLONG \
                || (s) == APR_OS_START_SYSERR + ERROR_FILENAME_EXCED_RANGE \
                || (s) == APR_OS_START_SYSERR + WSAENAMETOOLONG)
#define APR_STATUS_IS_ENOENT(s)         ((s) == APR_ENOENT \
                || (s) == APR_OS_START_SYSERR + ERROR_FILE_NOT_FOUND \
                || (s) == APR_OS_START_SYSERR + ERROR_PATH_NOT_FOUND \
                || (s) == APR_OS_START_SYSERR + ERROR_OPEN_FAILED)
#define APR_STATUS_IS_ENOTDIR(s)        ((s) == APR_ENOTDIR)
#define APR_STATUS_IS_ENOSPC(s)         ((s) == APR_ENOSPC \
                || (s) == APR_OS_START_SYSERR + ERROR_DISK_FULL)
#define APR_STATUS_IS_ENOMEM(s)         ((s) == APR_ENOMEM)
#define APR_STATUS_IS_EMFILE(s)         ((s) == APR_EMFILE \
                || (s) == APR_OS_START_SYSERR + ERROR_TOO_MANY_OPEN_FILES)
#define APR_STATUS_IS_ENFILE(s)         ((s) == APR_ENFILE)
#define APR_STATUS_IS_EBADF(s)          ((s) == APR_EBADF \
                || (s) == APR_OS_START_SYSERR + ERROR_INVALID_HANDLE)
#define APR_STATUS_IS_EINVAL(s)         ((s) == APR_EINVAL \
                || (s) == APR_OS_START_SYSERR + ERROR_INVALID_PARAMETER \
                || (s) == APR_OS_START_SYSERR + ERROR_INVALID_FUNCTION)
#define APR_STATUS_IS_ESPIPE(s)         ((s) == APR_ESPIPE \
                || (s) == APR_OS_START_SYSERR + ERROR_NEGATIVE_SEEK)
#define APR_STATUS_IS_EAGAIN(s)         ((s) == APR_EAGAIN \
                || (s) == APR_OS_START_SYSERR + ERROR_NO_DATA \
                || (s) == APR_OS_START_SYSERR + WSAEWOULDBLOCK)
#define APR_STATUS_IS_EINTR(s)          ((s) == APR_EINTR \
                || (s) == APR_OS_START_SYSERR + WSAEINTR)
#define APR_STATUS_IS_ENOTSOCK(s)       ((s) == APR_ENOTSOCK \
                || (s) == APR_OS_START_SYSERR + WSAENOTSOCK)
#define APR_STATUS_IS_ECONNREFUSED(s)   ((s) == APR_ECONNREFUSED \
                || (s) == APR_OS_START_SYSERR + WSAECONNREFUSED)
#define APR_STATUS_IS_EINPROGRESS(s)    ((s) == APR_EINPROGRESS \
                || (s) == APR_OS_START_SYSERR + WSAEINPROGRESS)
#define APR_STATUS_IS_ECONNABORTED(s)   ((s) == APR_ECONNABORTED \
                || (s) == APR_OS_START_SYSERR + WSAECONNABORTED)
#define APR_STATUS_IS_ECONNRESET(s)     ((s) == APR_ECONNRESET \
                || (s) == APR_OS_START_SYSERR + WSAECONNRESET)
#define APR_STATUS_IS_ETIMEDOUT(s)      ((s) == APR_ETIMEDOUT \
                || (s) == APR_OS_START_SYSERR + WSAETIMEDOUT)    
#define APR_STATUS_IS_EHOSTUNREACH(s)   ((s) == APR_EHOSTUNREACH \
                || (s) == APR_OS_START_SYSERR + WSAEHOSTUNREACH)
#define APR_STATUS_IS_ENETUNREACH(s)    ((s) == APR_ENETUNREACH \
                || (s) == APR_OS_START_SYSERR + WSAENETUNREACH)

#else /* !def OS2 || WIN32 */


/*
 *  os error codes are clib error codes
 */
#define APR_FROM_OS_ERROR(e)  (e)
#define APR_TO_OS_ERROR(e)    (e)

#define apr_get_os_error()    (errno)
#define apr_set_os_error(e)   (errno = (e))

#define APR_STATUS_IS_SUCCESS(s)           ((s) == APR_SUCCESS)

/* APR CANONICAL ERROR TESTS */
#define APR_STATUS_IS_EACCES(s)         ((s) == APR_EACCES)
#define APR_STATUS_IS_EEXIST(s)         ((s) == APR_EEXIST)
#define APR_STATUS_IS_ENAMETOOLONG(s)   ((s) == APR_ENAMETOOLONG)
#define APR_STATUS_IS_ENOENT(s)         ((s) == APR_ENOENT)
#define APR_STATUS_IS_ENOTDIR(s)        ((s) == APR_ENOTDIR)
#define APR_STATUS_IS_ENOSPC(s)         ((s) == APR_ENOSPC)
#define APR_STATUS_IS_ENOMEM(s)         ((s) == APR_ENOMEM)
#define APR_STATUS_IS_EMFILE(s)         ((s) == APR_EMFILE)
#define APR_STATUS_IS_ENFILE(s)         ((s) == APR_ENFILE)
#define APR_STATUS_IS_EBADF(s)          ((s) == APR_EBADF)
#define APR_STATUS_IS_EINVAL(s)         ((s) == APR_EINVAL)
#define APR_STATUS_IS_ESPIPE(s)         ((s) == APR_ESPIPE)

#if !defined(EWOULDBLOCK) || !defined(EAGAIN)
#define APR_STATUS_IS_EAGAIN(s)         ((s) == APR_EAGAIN)
#elif (EWOULDBLOCK == EAGAIN)
#define APR_STATUS_IS_EAGAIN(s)         ((s) == APR_EAGAIN)
#else
#define APR_STATUS_IS_EAGAIN(s)         ((s) == APR_EAGAIN \
                                      || (s) == EWOULDBLOCK)
#endif

#define APR_STATUS_IS_EINTR(s)          ((s) == APR_EINTR)
#define APR_STATUS_IS_ENOTSOCK(s)       ((s) == APR_ENOTSOCK)
#define APR_STATUS_IS_ECONNREFUSED(s)   ((s) == APR_ECONNREFUSED)
#define APR_STATUS_IS_EINPROGRESS(s)    ((s) == APR_EINPROGRESS)

/* EPROTO on certain older kernels really means ECONNABORTED, so we need to 
 * ignore it for them.  See discussion in new-httpd archives nh.9701 & nh.9603
 *
 * There is potentially a bug in Solaris 2.x x<6, and other boxes that 
 * implement tcp sockets in userland (i.e. on top of STREAMS).  On these
 * systems, EPROTO can actually result in a fatal loop.  See PR#981 for 
 * example.  It's hard to handle both uses of EPROTO.
 */
#ifdef EPROTO
#define APR_STATUS_IS_ECONNABORTED(s)    ((s) == APR_ECONNABORTED \
                                       || (s) == EPROTO)
#else
#define APR_STATUS_IS_ECONNABORTED(s)    ((s) == APR_ECONNABORTED)
#endif

#define APR_STATUS_IS_ECONNRESET(s)      ((s) == APR_ECONNRESET)
#define APR_STATUS_IS_ETIMEDOUT(s)       ((s) == APR_ETIMEDOUT)    
#define APR_STATUS_IS_EHOSTUNREACH(s)    ((s) == APR_EHOSTUNREACH)
#define APR_STATUS_IS_ENETUNREACH(s)     ((s) == APR_ENETUNREACH)

#endif /* !def OS2 || WIN32 */


#ifdef __cplusplus
}
#endif

#endif  /* ! APR_ERRNO_H */
