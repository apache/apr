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

#ifndef MISC_H
#define MISC_H

#include "apr.h"
#include "apr_portable.h"
#include "apr_private.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_getopt.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_errno.h"
#include "apr_getopt.h"

#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_SIGNAL_H
#include <signal.h>
#endif
#if APR_HAVE_PTHREAD_H
#include <pthread.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif
#ifndef _WIN32_WCE
#include <tlhelp32.h>
#endif

#if defined(HAVE_IF_INDEXTONAME) && defined(_MSC_VER)
#include <Iphlpapi.h>
#endif

struct apr_other_child_rec_t {
    apr_pool_t *p;
    struct apr_other_child_rec_t *next;
    apr_proc_t *proc;
    void (*maintenance) (int, void *, int);
    void *data;
};

#define WSAHighByte 2
#define WSALowByte 0

/* start.c and apr_app.c helpers and communication within misc.c
 *
 * They are not for public consumption, although apr_app_init_complete
 * must be an exported symbol to avoid reinitialization.
 */
extern int APR_DECLARE_DATA apr_app_init_complete;

int apr_wastrtoastr(char const * const * *retarr, 
                    wchar_t const * const *arr, int args);

/* Platform specific designation of run time os version.
 * Gaps allow for specific service pack levels that
 * export new kernel or winsock functions or behavior.
 */
typedef enum {
        APR_WIN_UNK =       0,
        APR_WIN_UNSUP =     1,
        APR_WIN_95 =       10,
        APR_WIN_95_B =     11,
        APR_WIN_95_OSR2 =  12,
        APR_WIN_98 =       14,
        APR_WIN_98_SE =    16,
        APR_WIN_ME =       18,

	APR_WIN_UNICODE =  20, /* Prior versions support only narrow chars */

        APR_WIN_CE_3 =     23, /* CE is an odd beast, not supporting */
                               /* some pre-NT features, such as the    */
        APR_WIN_NT =       30, /* narrow charset APIs (fooA fns), while  */
        APR_WIN_NT_3_5 =   35, /* not supporting some NT-family features.  */
        APR_WIN_NT_3_51 =  36,

        APR_WIN_NT_4 =     40,
        APR_WIN_NT_4_SP2 = 42,
        APR_WIN_NT_4_SP3 = 43,
        APR_WIN_NT_4_SP4 = 44,
        APR_WIN_NT_4_SP5 = 45,
        APR_WIN_NT_4_SP6 = 46,

        APR_WIN_2000 =     50,
        APR_WIN_2000_SP1 = 51,
        APR_WIN_2000_SP2 = 52,
        APR_WIN_XP =       60,
        APR_WIN_XP_SP1 =   61,
        APR_WIN_XP_SP2 =   62,
        APR_WIN_2003 =     70,
        APR_WIN_VISTA =    80,
        APR_WIN_7  =       90,
        APR_WIN_7_SP1 =    91,
        APR_WIN_8  =       100,
        APR_WIN_8_1 =      110,
        APR_WIN_10 =       120
} apr_oslevel_e;

extern APR_DECLARE_DATA apr_oslevel_e apr_os_level;

apr_status_t apr_get_oslevel(apr_oslevel_e *);

/* The APR_HAS_ANSI_FS symbol is PRIVATE, and internal to APR.
 * APR only supports char data for filenames.  Like most applications,
 * characters >127 are essentially undefined.  APR_HAS_UNICODE_FS lets
 * the application know that utf-8 is the encoding method of APR, and
 * only incidently hints that we have Wide OS calls.
 *
 * APR_HAS_ANSI_FS is simply an OS flag to tell us all calls must be
 * the unicode eqivilant.
 */

#define APR_HAS_ANSI_FS           0

/* IF_WIN_OS_IS_UNICODE / ELSE_WIN_OS_IS_ANSI help us keep the code trivial
 * where have runtime tests for unicode-ness, that aren't needed in any
 * build which supports only WINNT or WCE.
 */
#if APR_HAS_ANSI_FS && APR_HAS_UNICODE_FS
#define IF_WIN_OS_IS_UNICODE if (apr_os_level >= APR_WIN_UNICODE)
#define ELSE_WIN_OS_IS_ANSI else
#else /* APR_HAS_UNICODE_FS */
#define IF_WIN_OS_IS_UNICODE
#define ELSE_WIN_OS_IS_ANSI
#endif /* APR_HAS_ANSI_FS && APR_HAS_UNICODE_FS */

#if defined(_MSC_VER) && !defined(_WIN32_WCE)
#include "crtdbg.h"

static APR_INLINE void* apr_malloc_dbg(size_t size, const char* filename,
                                       int linenumber)
{
    return _malloc_dbg(size, _CRT_BLOCK, filename, linenumber);
}

static APR_INLINE void* apr_realloc_dbg(void* userData, size_t newSize,
                                        const char* filename, int linenumber)
{
    return _realloc_dbg(userData, newSize, _CRT_BLOCK, filename, linenumber);
}

#else

static APR_INLINE void* apr_malloc_dbg(size_t size, const char* filename,
                                       int linenumber)
{
    return malloc(size);
}

static APR_INLINE void* apr_realloc_dbg(void* userData, size_t newSize,
                                        const char* filename, int linenumber)
{
    return realloc(userData, newSize);
}

#endif  /* ! _MSC_VER */

/* Wrapper around WaitForSingleObject() that accepts apr_interval_time_t
 * in microseconds instead of milliseconds. Values < 0 mean wait 
 * forever, 0 means do not wait at all. */
DWORD apr_wait_for_single_object(HANDLE handle, apr_interval_time_t timeout);

typedef enum {
    DLL_WINBASEAPI = 0,    /* kernel32 From WinBase.h       */
    DLL_WINADVAPI = 1,     /* advapi32 From WinBase.h       */
    DLL_WINSOCKAPI = 2,    /* mswsock  From WinSock.h       */
    DLL_WINSOCK2API = 3,   /* ws2_32   From WinSock2.h      */
    DLL_SHSTDAPI = 4,      /* shell32  From ShellAPI.h      */
    DLL_NTDLL = 5,         /* ntdll    From our real kernel */
    DLL_IPHLPAPI = 6,      /* Iphlpapi From Iphlpapi.h      */
    DLL_defined = 7        /* must define as last idx_ + 1  */
} apr_dlltoken_e;

FARPROC apr_load_dll_func(apr_dlltoken_e fnLib, char *fnName, int ordinal);

/* The apr_load_dll_func call WILL return 0 set error to
 * ERROR_INVALID_FUNCTION if the function cannot be loaded
 */
#define APR_DECLARE_LATE_DLL_FUNC(lib, rettype, calltype, fn, ord, args, names) \
    typedef rettype (calltype *apr_winapi_fpt_##fn) args; \
    static apr_winapi_fpt_##fn apr_winapi_pfn_##fn = NULL; \
    static int apr_winapi_chk_##fn = 0; \
    static APR_INLINE int apr_winapi_ld_##fn(void) \
    {   if (apr_winapi_pfn_##fn) return 1; \
        if (apr_winapi_chk_##fn ++) return 0; \
        if (!apr_winapi_pfn_##fn) \
            apr_winapi_pfn_##fn = (apr_winapi_fpt_##fn) \
                                      apr_load_dll_func(lib, #fn, ord); \
        if (apr_winapi_pfn_##fn) return 1; else return 0; }; \
    static APR_INLINE rettype apr_winapi_##fn args \
    {   if (apr_winapi_ld_##fn()) \
            return (*(apr_winapi_pfn_##fn)) names; \
        else { SetLastError(ERROR_INVALID_FUNCTION); return 0;} }; \

#define APR_HAVE_LATE_DLL_FUNC(fn) apr_winapi_ld_##fn()

/* Provide late bound declarations of every API function missing from
 * one or more supported releases of the Win32 API
 *
 * lib is the enumerated token from apr_dlltoken_e, and must correspond
 * to the string table entry in start.c used by the apr_load_dll_func().
 * Token names (attempt to) follow Windows.h declarations prefixed by DLL_
 * in order to facilitate comparison.  Use the exact declaration syntax
 * and names from Windows.h to prevent ambigutity and bugs.
 *
 * rettype and calltype follow the original declaration in Windows.h
 * fn is the true function name - beware Ansi/Unicode #defined macros
 * ord is the ordinal within the library, use 0 if it varies between versions
 * args is the parameter list following the original declaration, in parens
 * names is the parameter list sans data types, enclosed in parens
 *
 * #undef/re#define the Ansi/Unicode generic name to abate confusion
 * In the case of non-text functions, simply #define the original name
 */

#if !defined(_WIN32_WCE)
/* This group is NOT available to all versions of WinNT,
 * these we must always look up
 */

APR_DECLARE_LATE_DLL_FUNC(DLL_NTDLL, LONG, WINAPI, NtQueryTimerResolution, 0, (
    ULONG *pMaxRes,  /* Minimum NS Resolution */
    ULONG *pMinRes,  /* Maximum NS Resolution */
    ULONG *pCurRes), /* Current NS Resolution */
    (pMaxRes, pMinRes, pCurRes));
#define QueryTimerResolution apr_winapi_NtQueryTimerResolution

APR_DECLARE_LATE_DLL_FUNC(DLL_NTDLL, LONG, WINAPI, NtSetTimerResolution, 0, (
    ULONG ReqRes,    /* Requested NS Clock Resolution */
    BOOL  Acquire,   /* Aquire (1) or Release (0) our interest */
    ULONG *pNewRes), /* The NS Clock Resolution granted */
    (ReqRes, Acquire, pNewRes));
#define SetTimerResolution apr_winapi_NtSetTimerResolution

typedef struct PBI {
    LONG      ExitStatus;
    PVOID     PebBaseAddress;
    apr_uintptr_t AffinityMask;
    LONG      BasePriority;
    apr_uintptr_t UniqueProcessId;
    apr_uintptr_t InheritedFromUniqueProcessId;
} PBI, *PPBI;

APR_DECLARE_LATE_DLL_FUNC(DLL_NTDLL, LONG, WINAPI, NtQueryInformationProcess, 0, (
    HANDLE hProcess,  /* Obvious */
    INT   info,       /* Use 0 for PBI documented above */
    PVOID pPI,        /* The PIB buffer */
    ULONG LenPI,      /* Use sizeof(PBI) */
    ULONG *pSizePI),  /* returns pPI buffer used (may pass NULL) */
    (hProcess, info, pPI, LenPI, pSizePI));
#define QueryInformationProcess apr_winapi_NtQueryInformationProcess

APR_DECLARE_LATE_DLL_FUNC(DLL_NTDLL, LONG, WINAPI, NtQueryObject, 0, (
    HANDLE hObject,   /* Obvious */
    INT   info,       /* Use 0 for PBI documented above */
    PVOID pOI,        /* The PIB buffer */
    ULONG LenOI,      /* Use sizeof(PBI) */
    ULONG *pSizeOI),  /* returns pPI buffer used (may pass NULL) */
    (hObject, info, pOI, LenOI, pSizeOI));
#define QueryObject apr_winapi_NtQueryObject

/* https://docs.microsoft.com/en-us/windows/desktop/api/winternl/nf-winternl-ntwaitforsingleobject */
APR_DECLARE_LATE_DLL_FUNC(DLL_NTDLL, LONG, WINAPI, NtWaitForSingleObject, 0, (
    HANDLE Handle,          /* The handle to the wait object. */
    BOOLEAN Alertable,      /* Specifies whether an alert can be delivered when
                               the object is waiting. */
    PLARGE_INTEGER Timeout),/* A pointer to an absolute or relative time over
                               which the wait is to occur.  */
    (Handle, Alertable, Timeout));

#ifdef CreateToolhelp32Snapshot
#undef CreateToolhelp32Snapshot
#endif
APR_DECLARE_LATE_DLL_FUNC(DLL_WINBASEAPI, HANDLE, WINAPI, CreateToolhelp32Snapshot, 0, (
    DWORD dwFlags,
    DWORD th32ProcessID),
    (dwFlags, th32ProcessID));
#define CreateToolhelp32Snapshot apr_winapi_CreateToolhelp32Snapshot

#ifdef Process32FirstW
#undef Process32FirstW
#endif
APR_DECLARE_LATE_DLL_FUNC(DLL_WINBASEAPI, BOOL, WINAPI, Process32FirstW, 0, (
    HANDLE hSnapshot,
    LPPROCESSENTRY32W lppe),
    (hSnapshot, lppe));
#define Process32FirstW apr_winapi_Process32FirstW

#ifdef Process32NextW
#undef Process32NextW
#endif
APR_DECLARE_LATE_DLL_FUNC(DLL_WINBASEAPI, BOOL, WINAPI, Process32NextW, 0, (
    HANDLE hSnapshot,
    LPPROCESSENTRY32W lppe),
    (hSnapshot, lppe));
#define Process32NextW apr_winapi_Process32NextW

#define HAVE_POLL   1

#ifdef if_nametoindex
#undef if_nametoindex
#endif
APR_DECLARE_LATE_DLL_FUNC(DLL_IPHLPAPI, NET_IFINDEX, WINAPI, if_nametoindex, 0, (
    IN PCSTR InterfaceName),
    (InterfaceName));
#define if_nametoindex apr_winapi_if_nametoindex

#ifdef if_indextoname
#undef if_indextoname
#endif
APR_DECLARE_LATE_DLL_FUNC(DLL_IPHLPAPI, PCHAR, NETIOAPI_API_, if_indextoname, 0, (
    NET_IFINDEX InterfaceIndex,
    PCHAR       InterfaceName),
    (InterfaceIndex, InterfaceName));
#define if_indextoname apr_winapi_if_indextoname

#endif /* !defined(_WIN32_WCE) */

#endif  /* ! MISC_H */
