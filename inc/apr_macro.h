/* ====================================================================
 * Copyright (c) 1995-1999 The Apache Group.  All rights reserved.
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
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#ifndef AP_MACROS_H
#define AP_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apr_config.h"

#ifndef _POSIX_THREAD_SAFE_FUNCTIONS
#define SAFETY_LOCK(funcname, namestr) \
    { \
    if (lock_##func_name == NULL) \
        if (ap_create_lock(&lock_##func_name, APR_MUTEX, APR_INTRAPROCESS, \
        name_str, NULL) != APR_SUCCESS) \
            return APR_NOTTHREADSAFE; \
    if (ap_lock(lock_##funcname) != APR_SUCCESS) \
        return APR_NOTTHREADSAFE; \
    }
#else
#define SAFETY_LOCK(funcname, namestr)
#endif

#ifndef _POSIX_THREAD_SAFE_FUNCTIONS
#define SAFETY_UNLOCK(func_name) \
    if (ap_unlock(lock_##func_name) != APR_SUCCESS) { \
        return APR_NOTTHREADSAFE; \
    }
#else
#define SAFETY_UNLOCK(func_name)
#endif

#ifdef HAVE_GMTIME_R
#define GMTIME_R(x, y) gmtime_r(x, y);
#else
#define GMTIME_R(x, y) memcpy(y, gmtime(x), sizeof(y));
#endif

#ifdef HAVE_LOCALTIME_R
#define LOCALTIME_R(x, y) localtime_r(x, y);
#else
#define LOCALTIME_R(x, y) memcpy(y, localtime(x), sizeof(y));
#endif

#ifdef _POSIX_THREAD_SAFE_FUNCTIONS 
#define GETHOSTBYNAME(x, y, z) z = gethostbyname(x);
#else
#define GETHOSTBYNAME(x, y, z) \
            y = gethostbyname(x); \
            z = ap_palloc(NULL, sizeof(struct hostent)); \
            memcpy(z, y, sizeof(struct hostent));
#endif

#ifdef _POSIX_THREAD_SAFE_FUNCTIONS 
#define GETHOSTBYADDR(x, y, z) z = gethostbyaddr(x, sizeof(struct in_addr), AF_INET);
#else
#define GETHOSTBYADDR(x, y, z) \
            y = gethostbyaddr(x, sizeof(struct in_addr), AF_INET); \
            z = ap_palloc(NULL, sizeof(struct hostent)); \
            memcpy(z, y, sizeof(struct hostent));
#endif

#ifdef _POSIX_THREAD_SAFE_FUNCTIONS 
#define INET_NTOA(x, y, len) ap_cpystrn(y, inet_ntoa(x), len);
#else
#define INET_NTOA(x, y, len) ap_cpystrn(y, inet_ntoa(x), len);
#endif

#ifdef _POSIX_THREAD_SAFE_FUNCTIONS 
#define READDIR(x, y, z)  y = readdir(x); \
                           if (y == NULL) { \
                               z = errno; \
                           } \
                           else { \
                               z = APR_SUCCESS; \
                           }
#else
#define READDIR(x, y, z) \
                          { \
                          struct dirent *temp = readdir(x); \
                          if (temp == NULL) { \
                              z = errno; \
                          } \
                          else { \
                              memcpy(y, temp, sizeof(struct dirent)); \
                              z = APR_SUCCESS; \
                          } \
                          }
#endif

#ifdef __cplusplus
}
#endif

#endif	/* !AP_MACROS_H */
