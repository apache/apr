/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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
#include "apr_arch_misc.h"
#include "crtdbg.h"
#include "apr_arch_file_io.h"
#include "assert.h"
#include "apr_lib.h"

APR_DECLARE_DATA apr_oslevel_e apr_os_level = APR_WIN_UNK;

apr_status_t apr_get_oslevel(apr_oslevel_e *level)
{
    if (apr_os_level == APR_WIN_UNK) 
    {
        static OSVERSIONINFO oslev;
        oslev.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&oslev);

        if (oslev.dwPlatformId == VER_PLATFORM_WIN32_NT) 
        {
            static unsigned int servpack = 0;
            char *pservpack;
            if (pservpack = oslev.szCSDVersion) {
                while (*pservpack && !apr_isdigit(*pservpack)) {
                    pservpack++;
                }
                if (*pservpack)
                    servpack = atoi(pservpack);
            }

            if (oslev.dwMajorVersion < 3) {
                apr_os_level = APR_WIN_UNSUP;
            }
            else if (oslev.dwMajorVersion == 3) {
                if (oslev.dwMajorVersion < 50) {
                    apr_os_level = APR_WIN_UNSUP;
                }
                else if (oslev.dwMajorVersion == 50) {
                    apr_os_level = APR_WIN_NT_3_5;
                }
                else {
                    apr_os_level = APR_WIN_NT_3_51;
                }
            }
            else if (oslev.dwMajorVersion == 4) {
                if (servpack < 2)
                    apr_os_level = APR_WIN_NT_4;
                else if (servpack <= 2)
                    apr_os_level = APR_WIN_NT_4_SP2;
                else if (servpack <= 3)
                    apr_os_level = APR_WIN_NT_4_SP3;
                else if (servpack <= 4)
                    apr_os_level = APR_WIN_NT_4_SP4;
                else if (servpack <= 5)
                    apr_os_level = APR_WIN_NT_4_SP5;
                else 
                    apr_os_level = APR_WIN_NT_4_SP6;
            }
            else if (oslev.dwMajorVersion == 5) {
                if (oslev.dwMinorVersion == 0) {
                    if (servpack == 0)
                        apr_os_level = APR_WIN_2000;
                    else if (servpack == 1)
                        apr_os_level = APR_WIN_2000_SP1;
                    else
                        apr_os_level = APR_WIN_2000_SP2;
                }
                else {
                    apr_os_level = APR_WIN_XP;
                }
            }
            else {
                apr_os_level = APR_WIN_XP;
            }
        }
#ifndef WINNT
        else if (oslev.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            char *prevision;
            if (prevision = oslev.szCSDVersion) {
                while (*prevision && !apr_isupper(*prevision)) {
                     prevision++;
                }
            }
            else prevision = "";

            if (oslev.dwMinorVersion < 10) {
                if (*prevision < 'C')
                    apr_os_level = APR_WIN_95;
                else
                    apr_os_level = APR_WIN_95_OSR2;
            }
            else if (oslev.dwMinorVersion < 90) {
                if (*prevision < 'A')
                    apr_os_level = APR_WIN_98;
                else
                    apr_os_level = APR_WIN_98_SE;
            }
            else {
                apr_os_level = APR_WIN_ME;
            }
        }
#endif
#ifdef _WIN32_WCE
        else if (oslev.dwPlatformId == VER_PLATFORM_WIN32_CE) 
        {
            if (oslev.dwMajorVersion < 3) {
                apr_os_level = APR_WIN_UNSUP;
            }
            else {
                apr_os_level = APR_WIN_CE_3;
            }
        }
#endif
        else {
            apr_os_level = APR_WIN_UNSUP;
        }
    }

    *level = apr_os_level;

    if (apr_os_level < APR_WIN_UNSUP) {
        return APR_EGENERAL;
    }

    return APR_SUCCESS;
}


/* This is the helper code to resolve late bound entry points 
 * missing from one or more releases of the Win32 API
 */

static const char* const lateDllName[DLL_defined] = {
    "kernel32", "advapi32", "mswsock",  "ws2_32", "shell32", "ntdll.dll"  };
static HMODULE lateDllHandle[DLL_defined] = {
     NULL,       NULL,       NULL,       NULL,     NULL,       NULL       };

FARPROC apr_load_dll_func(apr_dlltoken_e fnLib, char* fnName, int ordinal)
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

/* Declared in include/arch/win32/apr_dbg_win32_handles.h
 */
APR_DECLARE_NONSTD(HANDLE) apr_dbg_log(char* fn, HANDLE ha, char* fl, int ln, 
                                       int nh, /* HANDLE hv, char *dsc */...)
{
    static DWORD tlsid = 0xFFFFFFFF;
    static HANDLE fh = NULL;
    static long ctr = 0;
    static CRITICAL_SECTION cs;
    long seq;
    DWORD wrote;
    char *sbuf;
    
    seq = (InterlockedIncrement)(&ctr);

    if (tlsid == 0xFFFFFFFF) {
        tlsid = (TlsAlloc)();
    }

    sbuf = (TlsGetValue)(tlsid);
    if (!fh || !sbuf) {
        sbuf = (malloc)(1024);
        (TlsSetValue)(tlsid, sbuf);
        sbuf[1023] = '\0';
        if (!fh) {
            (GetModuleFileName)(NULL, sbuf, 250);
            sprintf(strchr(sbuf, '\0'), ".%d",
                    (GetCurrentProcessId)());
            fh = (CreateFile)(sbuf, GENERIC_WRITE, 0, NULL, 
                            CREATE_ALWAYS, 0, NULL);
            (InitializeCriticalSection)(&cs);
        }
    }

    if (!nh) {
        (sprintf)(sbuf, "%08x %08x %08x %s() %s:%d\n",
                  (DWORD)ha, seq, GetCurrentThreadId(), fn, fl, ln);
        (EnterCriticalSection)(&cs);
        (WriteFile)(fh, sbuf, strlen(sbuf), &wrote, NULL);
        (LeaveCriticalSection)(&cs);
    } 
    else {
        va_list a;
        va_start(a,nh);
        (EnterCriticalSection)(&cs);
        do {
            HANDLE *hv = va_arg(a, HANDLE*);
            char *dsc = va_arg(a, char*);
            if (strcmp(dsc, "Signaled") == 0) {
                if ((DWORD)ha >= STATUS_WAIT_0 
                       && (DWORD)ha < STATUS_ABANDONED_WAIT_0) {
                    hv += (DWORD)ha;
                }
                else if ((DWORD)ha >= STATUS_ABANDONED_WAIT_0
                            && (DWORD)ha < STATUS_USER_APC) {
                    hv += (DWORD)ha - STATUS_ABANDONED_WAIT_0;
                    dsc = "Abandoned";
                }
                else if ((DWORD)ha == WAIT_TIMEOUT) {
                    dsc = "Timed Out";
                }
            }
            (sprintf)(sbuf, "%08x %08x %08x %s(%s) %s:%d\n",
                      (DWORD*)*hv, seq, GetCurrentThreadId(), 
                      fn, dsc, fl, ln);
            (WriteFile)(fh, sbuf, strlen(sbuf), &wrote, NULL);
        } while (--nh);
        (LeaveCriticalSection)(&cs);
        va_end(a);
    }
    return ha;
}
