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
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_dso.h"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

/*
 * stuffbuffer - like apr_cpystrn() but returns the address of the
 * dest buffer instead of the address of the terminating '\0'
 */
static char *stuffbuffer(char *buf, apr_size_t bufsize, const char *s)
{
    apr_cpystrn(buf,s,bufsize);
    return buf;
}

static char *apr_error_string(apr_status_t statcode)
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
#if APR_HAS_DSO
    case APR_EDSOOPEN:
#ifdef HAVE_LIBDL
        return dlerror();
#else
        return "DSO load failed";
#endif /* HAVE_LIBDL */
#endif /* APR_HAS_DSO */
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
    case APR_INCOMPLETE:
        return "The input data is incomplete";
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
    default:
        return "Error string not specified yet";
    }
}


#ifdef OS2
#define INCL_DOS
#include <os2.h>
#include <ctype.h>

static char *apr_os_strerror(char* buf, apr_size_t bufsize, int err)
{
  char result[200];
  unsigned char message[HUGE_STRING_LEN];
  ULONG len;
  char *pos;
  int c;
  
  if (err >= 10000 && err < 12000) {  /* socket error codes */
      return stuffbuffer(buf, bufsize,
                         strerror(apr_canonical_error(err+APR_OS_START_SYSERR)));
  } 
  else if (DosGetMessage(NULL, 0, message, HUGE_STRING_LEN, err,
			 "OSO001.MSG", &len) == 0) {
      len--;
      message[len] = 0;
      pos = result;
  
      if (len >= sizeof(result))
        len = sizeof(result-1);

      for (c=0; c<len; c++) {
	  /* skip multiple whitespace */
          while (apr_isspace(message[c]) && apr_isspace(message[c+1]))
              c++;
          *(pos++) = apr_isspace(message[c]) ? ' ' : message[c];
      }
  
      *pos = 0;
  } 
  else {
      sprintf(result, "OS/2 error %d", err);
  }

  /* Stuff the string into the caller supplied buffer, then return 
   * a pointer to it.
   */
  return stuffbuffer(buf, bufsize, result);  
}

#elif defined(WIN32)

static char *apr_os_strerror(char *buf, apr_size_t bufsize, apr_status_t errcode)
{
    DWORD len;
    DWORD i;

    
    len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        errcode,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
                        (LPTSTR) buf,
                        bufsize,
                        NULL);

    if (len) {
        /* FormatMessage put the message in the buffer, but it may
         * have embedded a newline (\r\n), and possible more than one.
         * Remove the newlines replacing them with a space. This is not
         * as visually perfect as moving all the remaining message over,
         * but more efficient.
         */
        i = len;
        while (i) {
            i--;
            if ((buf[i] == '\r') || (buf[i] == '\n'))
                buf[i] = ' ';
        }
    }
    else {
        /* Windows didn't provide us with a message.  Even stuff like                    * WSAECONNREFUSED won't get a message.
         */
        apr_cpystrn(buf, "Unrecognized error code", bufsize);
    }

    return buf;
}

#else
/* On Unix, apr_os_strerror() handles error codes from the resolver 
 * (h_errno). 
 e*/
static char *apr_os_strerror(char* buf, apr_size_t bufsize, int err) 
{
#ifdef HAVE_HSTRERROR
    return stuffbuffer(buf, bufsize, hstrerror(err));
#else /* HAVE_HSTRERROR */
    const char *msg;

    switch(err) {
    case HOST_NOT_FOUND:
        msg = "Unknown host";
        break;
#if defined(NO_DATA)
    case NO_DATA:
#if defined(NO_ADDRESS) && (NO_DATA != NO_ADDRESS)
    case NO_ADDRESS:
#endif
        msg = "No address for host";
        break;
#elif defined(NO_ADDRESS)
    case NO_ADDRESS:
        msg = "No address for host";
        break;
#endif /* NO_DATA */
    default:
        msg = "Unrecognized resolver error";
    }
    return stuffbuffer(buf, bufsize, msg);
#endif /* HAVE_STRERROR */
}
#endif

char *apr_strerror(apr_status_t statcode, char *buf, apr_size_t bufsize)
{
    if (statcode < APR_OS_START_ERROR) {
        return stuffbuffer(buf, bufsize, strerror(statcode));
    }
    else if (statcode < APR_OS_START_USEERR) {
        return stuffbuffer(buf, bufsize, apr_error_string(statcode));
    }
    else if (statcode < APR_OS_START_SYSERR) {
        return stuffbuffer(buf, bufsize, "APR does not understand this error code");
    }
    else {
        return apr_os_strerror(buf, bufsize, statcode - APR_OS_START_SYSERR);
    }
}

