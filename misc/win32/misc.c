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

#include "apr_private.h"
#include "misc.h"

apr_status_t ap_get_oslevel(apr_pool_t *cont, ap_oslevel_e *level)
{
    static OSVERSIONINFO oslev;
    static unsigned int servpack = 0;
    static BOOL first = TRUE;
    char *pservpack;

    if (first) {
        first = FALSE;
        oslev.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&oslev);
        if (oslev.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            for (pservpack = oslev.szCSDVersion; 
                 *pservpack && !isdigit(*pservpack); pservpack++)
                ;
            if (*pservpack)
                servpack = atoi(pservpack);
        }
    }
    if (oslev.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        if (oslev.dwMajorVersion == 5) {
            (*level) = APR_WIN_2000;
        }
        else if (oslev.dwMajorVersion == 4) {
            if (servpack >= 6) {
                (*level) = APR_WIN_NT_4_SP6;
            }
            else if (servpack >= 4) {
                (*level) = APR_WIN_NT_4_SP4;
            }
            else if (servpack >= 3) {
                (*level) = APR_WIN_NT_4_SP3;
            }
            else if (servpack >= 2) {
                (*level) = APR_WIN_NT_4_SP2;
            }
            else {
                (*level) = APR_WIN_NT_4;
            }
        }
        else {
            (*level) = APR_WIN_NT;
        }
        return APR_SUCCESS;
    }
    else if (oslev.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        if (oslev.dwMinorVersion == 0) {
            (*level) = APR_WIN_95;
            return APR_SUCCESS;
        }
        else if (oslev.dwMinorVersion > 0) {
            (*level) = APR_WIN_98;
            return APR_SUCCESS;
        }
    }
    return APR_EEXIST;
}


/* This is the helper code to resolve late bound entry points 
 * missing from one or more releases of the Win32 API
 */

static const char* const lateDllName[DLL_defined] = {
    "kernel32", "advapi32", "mswsock",  "ws2_32"  };
static HMODULE lateDllHandle[DLL_defined] = {
    NULL,       NULL,       NULL,       NULL      };

FARPROC ap_load_dll_func(ap_dlltoken_e fnLib, char* fnName, int ordinal)
{
    if (!lateDllHandle[fnLib]) { 
        lateDllHandle[fnLib] = LoadLibrary(lateDllName[fnLib]);
        if (!lateDllHandle[fnLib])
            return NULL;
    }
    if (ordinal)
        return GetProcAddress(lateDllHandle[fnLib], (char *) ordinal);
    else
        return GetProcAddress(lateDllHandle[fnLib], fnName);
}
