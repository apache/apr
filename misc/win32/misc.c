/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr_private.h"
#include "apr_arch_misc.h"
#include "apr_arch_file_io.h"
#include "assert.h"
#include "apr_lib.h"
#include "tchar.h"

APR_DECLARE_DATA apr_oslevel_e apr_os_level = APR_WIN_UNK;

apr_status_t apr_get_oslevel(apr_oslevel_e *level)
{
    if (apr_os_level == APR_WIN_UNK) 
    {
        OSVERSIONINFOEXW oslev;
        oslev.dwOSVersionInfoSize = sizeof(oslev);
        if (!GetVersionExW((OSVERSIONINFOW*) &oslev)) {
            return apr_get_os_error();
        }

        if (oslev.dwPlatformId == VER_PLATFORM_WIN32_NT) 
        {
            unsigned int servpack = oslev.wServicePackMajor;

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
                else if (oslev.dwMinorVersion == 2) {
                    apr_os_level = APR_WIN_2003;
                }
                else {
                    if (servpack < 1)
                        apr_os_level = APR_WIN_XP;
                    else if (servpack == 1)
                        apr_os_level = APR_WIN_XP_SP1;
                    else
                        apr_os_level = APR_WIN_XP_SP2;
                }
            }
            else if (oslev.dwMajorVersion == 6) {
                if (oslev.dwMinorVersion == 0)
                    apr_os_level = APR_WIN_VISTA;
                else if (oslev.dwMinorVersion == 1) {
                    if (servpack < 1)
                        apr_os_level = APR_WIN_7;
                    else
                        apr_os_level = APR_WIN_7_SP1;
                }
                else if (oslev.dwMinorVersion == 2)
                    apr_os_level = APR_WIN_8;
                else
                    apr_os_level = APR_WIN_8_1;
            }
            else {
                apr_os_level = APR_WIN_10;
            }
        }
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

    if (apr_os_level <= APR_WIN_UNSUP) {
        return APR_EGENERAL;
    }

    return APR_SUCCESS;
}


/* This is the helper code to resolve late bound entry points 
 * missing from one or more releases of the Win32 API
 */

typedef struct win32_late_dll_t {
    INIT_ONCE control;
    const apr_wchar_t *dll_name;
    HMODULE dll_handle;
} win32_late_dll_t;

static win32_late_dll_t late_dll[DLL_defined] = {
    {INIT_ONCE_STATIC_INIT, L"kernel32", NULL},
    {INIT_ONCE_STATIC_INIT, L"advapi32", NULL},
    {INIT_ONCE_STATIC_INIT, L"mswsock", NULL},
    {INIT_ONCE_STATIC_INIT, L"ws2_32", NULL},
    {INIT_ONCE_STATIC_INIT, L"shell32", NULL},
    {INIT_ONCE_STATIC_INIT, L"ntdll.dll", NULL},
    {INIT_ONCE_STATIC_INIT, L"Iphplapi", NULL}
};

static BOOL WINAPI load_dll_callback(PINIT_ONCE InitOnce,
                                     PVOID Parameter,
                                     PVOID *Context)
{
    win32_late_dll_t *dll = Parameter;

    dll->dll_handle = LoadLibraryW(dll->dll_name);

    return TRUE;
}

FARPROC apr_load_dll_func(apr_dlltoken_e fnLib, char* fnName, int ordinal)
{
    win32_late_dll_t *dll = &late_dll[fnLib];

    InitOnceExecuteOnce(&dll->control, load_dll_callback, dll, NULL);
    if (!dll->dll_handle)
        return NULL;

#if defined(_WIN32_WCE)
    if (ordinal)
        return GetProcAddressA(dll->dll_handle,
                               (const char *) (apr_ssize_t)ordinal);
    else
        return GetProcAddressA(dll->dll_handle, fnName);
#else
    if (ordinal)
        return GetProcAddress(dll->dll_handle,
                              (const char *) (apr_ssize_t)ordinal);
    else
        return GetProcAddress(dll->dll_handle, fnName);
#endif
}

DWORD apr_wait_for_single_object(HANDLE handle, apr_interval_time_t timeout)
{
    if (APR_HAVE_LATE_DLL_FUNC(NtWaitForSingleObject)){
        LONG ntstatus;

        if (timeout < 0) {
            ntstatus = apr_winapi_NtWaitForSingleObject(handle, FALSE, NULL);
        }
        else {
            LARGE_INTEGER timeout_100ns;

            /* Negative timeout means relative time. In 100 nanoseconds. */
            timeout_100ns.QuadPart = -timeout * 10;
            ntstatus = apr_winapi_NtWaitForSingleObject(handle, FALSE,
                                                        &timeout_100ns);
        }

        switch(ntstatus)
        {
        case STATUS_WAIT_0:
            return WAIT_OBJECT_0;
        case STATUS_ABANDONED_WAIT_0:
            return WAIT_ABANDONED_0;
        case STATUS_TIMEOUT:
            return WAIT_TIMEOUT;
        case STATUS_USER_APC:
            return WAIT_IO_COMPLETION;
        default:
            SetLastError(ERROR_INVALID_HANDLE);
            return WAIT_FAILED;
        }
    }
    else {
        apr_interval_time_t t = timeout;
        DWORD res;
        DWORD timeout_ms = 0;

        do {
            if (t < 0) {
                timeout_ms = INFINITE;
            }
            else if (t > 0) {
                /* Given timeout is 64bit usecs whereas Windows timeouts are
                 * 32bit msecs and below INFINITE (2^32 - 1), so we may need
                 * multiple timed out waits...
                 */
                if (t > apr_time_from_msec(INFINITE - 1)) {
                    timeout_ms = INFINITE - 1;
                    t -= apr_time_from_msec(INFINITE - 1);
                }
                else {
                    timeout_ms = (DWORD)apr_time_as_msec(t);
                    t = 0;
                }
            }
            res = WaitForSingleObject(handle, timeout_ms);
        } while (res == WAIT_TIMEOUT && t > 0);

        return res;
    }
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
            (GetModuleFileNameA)(NULL, sbuf, 250);
            sprintf(strchr(sbuf, '\0'), ".%u",
                    (unsigned int)(GetCurrentProcessId)());
            fh = (CreateFileA)(sbuf, GENERIC_WRITE, 0, NULL, 
                            CREATE_ALWAYS, 0, NULL);
            (InitializeCriticalSection)(&cs);
        }
    }

    if (!nh) {
        (sprintf)(sbuf, "%p %08x %08x %s() %s:%d\n",
                  ha, (unsigned int)seq, (unsigned int)GetCurrentThreadId(),
                  fn, fl, ln);
        (EnterCriticalSection)(&cs);
        (WriteFile)(fh, sbuf, (DWORD)strlen(sbuf), &wrote, NULL);
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
                if ((apr_ssize_t)ha >= STATUS_WAIT_0 
                       && (apr_ssize_t)ha < STATUS_ABANDONED_WAIT_0) {
                    hv += (apr_ssize_t)ha;
                }
                else if ((apr_ssize_t)ha >= STATUS_ABANDONED_WAIT_0
                            && (apr_ssize_t)ha < STATUS_USER_APC) {
                    hv += (apr_ssize_t)ha - STATUS_ABANDONED_WAIT_0;
                    dsc = "Abandoned";
                }
                else if ((apr_ssize_t)ha == WAIT_TIMEOUT) {
                    dsc = "Timed Out";
                }
            }
            (sprintf)(sbuf, "%p %08x %08x %s(%s) %s:%d\n",
                      *hv, (unsigned int)seq,
                      (unsigned int)GetCurrentThreadId(), 
                      fn, dsc, fl, ln);
            (WriteFile)(fh, sbuf, (DWORD)strlen(sbuf), &wrote, NULL);
        } while (--nh);
        (LeaveCriticalSection)(&cs);
        va_end(a);
    }
    return ha;
}
