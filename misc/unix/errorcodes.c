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

#include "misc.h"

#ifdef WIN32
#error "not implemented yet"
#elif OS/2
#error "not implemented yet"
#elif BEOS    
#error "not implemented yet"
#else
#define SYS_ERR_STRING(code, p) \
    if (1) errstr = strerror(code); else
#endif

static const char *apr_error_string(ap_status_t statcode)
{
    switch (statcode) {
    case APR_ENOPOOL:
        return "A new pool could not be created.";
    case APR_ENOFILE:
        return "No file was provided and one was required.";
    case APR_EBADDATE:
        return "An invalid date has been provided";
    case APR_EINVALSOCK:
        return "An invalid socket was returned";
    case APR_ENOPROC:
        return "No process was provided and one was required.";
    case APR_ENOTIME:
        return "No time was provided and one was required.";
    case APR_ENODIR:
        return "No directory was provided and one was required.";
    case APR_ENOLOCK:
        return "No lock was provided and one was required.";
    case APR_ENOPOLL:
        return "No poll structure was provided and one was required.";
    case APR_ENOSOCKET:
        return "No socket was provided and one was required.";
    case APR_ENOTHREAD:
        return "No thread was provided and one was required.";
    case APR_ENOTHDKEY:
        return "No thread key structure was provided and one was required.";
    case APR_ENOSHMAVAIL:
        return "No shared memory is currently available";
    case APR_EDSOOPEN:
        return "Could not open the dso.";
    case APR_INCHILD:
        return
	    "Your code just forked, and you are currently executing in the "
	    "child process";
    case APR_INPARENT:
        return
	    "Your code just forked, and you are currently executing in the "
	    "parent process";
    case APR_DETACH:
        return "The specified thread is detached";
    case APR_NOTDETACH:
        return "The specified thread is not detached";
    case APR_CHILD_DONE:
        return "The specified child process is done executing";
    case APR_CHILD_NOTDONE:
        return "The specified child process is not done executing";
    case APR_TIMEUP:
        return "The timeout specified has expired";
    case APR_BADCH:
        return "Bad character specified on command line";
    case APR_BADARG:
        return "Missing parameter for the specified command line option";
    case APR_EOF:
        return "End of file found";
    case APR_NOTFOUND:
        return "Could not find specified socket in poll list.";
    case APR_ANONYMOUS:
        return "Shared memory is implemented anonymously";
    case APR_FILEBASED:
        return "Shared memory is implemented using files";
    case APR_KEYBASED:
        return "Shared memory is implemented using a key system";
    case APR_EINIT:
        return
	    "There is no error, this value signifies an initialized "
	    "error code";
    case APR_ENOTIMPL:
        return "This function has not been implemented on this platform";
    case APR_EMISMATCH:
        return "passwords do not match";
/* EBADARG needs to be removed soon.  */
    case APR_EBADARG:
    default:
        return "Error string not specified yet";
    }
}

const char *ap_strerror(ap_status_t statcode, ap_pool_t *p)
{
    if (statcode < APR_OS_START_ERROR) {
        return strerror(statcode);
    }
    else if (statcode < APR_OS_START_USEERR) {
        return apr_error_string(statcode);
    }
    else if (statcode < APR_OS_START_SYSERR) {
        return "APR does not understand this error code";
    }
    else {
        const char *errstr;
        SYS_ERR_STRING(statcode - APR_OS_START_SYSERR, p);
        return errstr;
    }
}
