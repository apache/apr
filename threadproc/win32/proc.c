/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include "win32/threadproc.h"
#include "win32/fileio.h"

#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_strings.h"
#include "apr_portable.h"
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <process.h>

/* 
 * some of the ideas expressed herein are based off of Microsoft
 * Knowledge Base article: Q190351
 *
 */

APR_DECLARE(apr_status_t) apr_procattr_create(apr_procattr_t **new,
                                                  apr_pool_t *cont)
{
    (*new) = (apr_procattr_t *)apr_pcalloc(cont, sizeof(apr_procattr_t));
    (*new)->cntxt = cont;
    (*new)->cmdtype = APR_PROGRAM;
    return APR_SUCCESS;
}

static apr_status_t open_nt_process_pipe(apr_file_t **read, apr_file_t **write,
                                         apr_int32_t iBlockingMode,
                                         apr_pool_t *cntxt)
{
    apr_status_t stat;
    BOOLEAN bAsyncRead, bAsyncWrite;

    switch (iBlockingMode) {
    case APR_FULL_BLOCK:
        bAsyncRead = bAsyncWrite = FALSE;
        break;
    case APR_PARENT_BLOCK:
        bAsyncRead = FALSE;
        bAsyncWrite = TRUE;
        break;
    case APR_CHILD_BLOCK:
        bAsyncRead = TRUE;
        bAsyncWrite = FALSE;
        break;
    default:
        bAsyncRead = TRUE;
        bAsyncWrite = TRUE;
    }
    if ((stat = apr_create_nt_pipe(read, write, bAsyncRead, bAsyncWrite,
                                   cntxt)) != APR_SUCCESS)
        return stat;

    return APR_SUCCESS;
}

static apr_status_t make_handle_private(apr_file_t *file)
{
    HANDLE hproc = GetCurrentProcess();
    HANDLE filehand = file->filehand;

    /* Create new non-inheritable versions of handles that
     * the child process doesn't care about. Otherwise, the child
     * inherits these handles; resulting in non-closeable handles
     * to the respective pipes.
     */
    if (!DuplicateHandle(hproc, filehand,
                         hproc, &file->filehand, 0,
                         FALSE, DUPLICATE_SAME_ACCESS))
        return apr_get_os_error();
    /* 
     * Close the inerhitable handle we don't need anymore.
     */
    CloseHandle(filehand);
    return APR_SUCCESS;
}

static apr_status_t make_inheritable_duplicate(apr_file_t *original,
                                               apr_file_t *duplicate)
{
    if (original == NULL)
        return APR_SUCCESS;

    /* Can't use apr_file_dup here because it creates a non-inhertible 
     * handle, and apr_open_file'd apr_file_t's are non-inheritable,
     * so we must assume we need to make an inheritable handle.
     */
    if (!CloseHandle(duplicate->filehand))
        return apr_get_os_error();
    else
    {
        HANDLE hproc = GetCurrentProcess();
        if (!DuplicateHandle(hproc, original->filehand, 
                             hproc, &duplicate->filehand, 0,
                             TRUE, DUPLICATE_SAME_ACCESS))
            return apr_get_os_error();
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_procattr_io_set(apr_procattr_t *attr,
                                             apr_int32_t in, 
                                             apr_int32_t out,
                                             apr_int32_t err)
{
    apr_status_t stat;

    if (in) {
        stat = open_nt_process_pipe(&attr->child_in, &attr->parent_in, in,
                                    attr->cntxt);
        if (stat == APR_SUCCESS)
            stat = make_handle_private(attr->parent_in);
        if (stat != APR_SUCCESS)
            return stat;
    }
    if (out) {
        stat = open_nt_process_pipe(&attr->parent_out, &attr->child_out, out,
                                    attr->cntxt);
        if (stat == APR_SUCCESS)
            stat = make_handle_private(attr->parent_out);
        if (stat != APR_SUCCESS)
            return stat;
    }
    if (err) {
        stat = open_nt_process_pipe(&attr->parent_err, &attr->child_err, err,
                                    attr->cntxt);
        if (stat == APR_SUCCESS)
            stat = make_handle_private(attr->parent_err);
        if (stat != APR_SUCCESS)
            return stat;
    }
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_procattr_child_in_set(apr_procattr_t *attr, 
                                                  apr_file_t *child_in, 
                                                  apr_file_t *parent_in)
{
    apr_status_t stat;

    if (attr->child_in == NULL && attr->parent_in == NULL) {
        stat = open_nt_process_pipe(&attr->child_in, &attr->parent_in,
                                    APR_FULL_BLOCK,
                                    attr->cntxt);
        if (stat == APR_SUCCESS)
            stat = make_handle_private(attr->parent_in);
        if (stat != APR_SUCCESS)
            return stat;
    }

    stat = make_inheritable_duplicate (child_in, attr->child_in);
    if (stat == APR_SUCCESS)
        stat = make_inheritable_duplicate (parent_in, attr->parent_in);

    return stat;
}

APR_DECLARE(apr_status_t) apr_procattr_child_out_set(apr_procattr_t *attr,
                                                   apr_file_t *child_out,
                                                   apr_file_t *parent_out)
{
    apr_status_t stat;

    if (attr->child_out == NULL && attr->parent_out == NULL) {
        stat = open_nt_process_pipe(&attr->child_out, &attr->parent_out,
                                    APR_FULL_BLOCK,
                                    attr->cntxt);
        if (stat == APR_SUCCESS)
            stat = make_handle_private(attr->parent_out);
        if (stat != APR_SUCCESS)
            return stat;
    }        
        
    stat = make_inheritable_duplicate (child_out, attr->child_out);
    if (stat == APR_SUCCESS)
        stat = make_inheritable_duplicate (parent_out, attr->parent_out);

    return stat;
}

APR_DECLARE(apr_status_t) apr_procattr_child_err_set(apr_procattr_t *attr,
                                                   apr_file_t *child_err,
                                                   apr_file_t *parent_err)
{
    apr_status_t stat;

    if (attr->child_err == NULL && attr->parent_err == NULL) {
        stat = open_nt_process_pipe(&attr->child_err, &attr->parent_err,
                                    APR_FULL_BLOCK,
                                    attr->cntxt);
        if (stat == APR_SUCCESS)
            stat = make_handle_private(attr->parent_err);
        if (stat != APR_SUCCESS)
            return stat;
    }        
        
    stat = make_inheritable_duplicate (child_err, attr->child_err);
    if (stat == APR_SUCCESS)
        stat = make_inheritable_duplicate (parent_err, attr->parent_err);

    return stat;
}

APR_DECLARE(apr_status_t) apr_procattr_dir_set(apr_procattr_t *attr,
                                              const char *dir) 
{
    /* curr dir must be in native format, there are all sorts of bugs in
     * the NT library loading code that flunk the '/' parsing test.
     */
    return apr_filepath_merge(&attr->currdir, NULL, dir, 
                              APR_FILEPATH_NATIVE, attr->cntxt);
}

APR_DECLARE(apr_status_t) apr_procattr_cmdtype_set(apr_procattr_t *attr,
                                                  apr_cmdtype_e cmd) 
{
    attr->cmdtype = cmd;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_procattr_detach_set(apr_procattr_t *attr,
                                                 apr_int32_t det) 
{
    attr->detached = det;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_proc_create(apr_proc_t *new,
                                          const char *progname,
                                          const char * const *args,
                                          const char * const *env,
                                          apr_procattr_t *attr,
                                          apr_pool_t *cont)
{
    apr_status_t rv;
    apr_oslevel_e os_level;
    apr_size_t i;
    char *cmdline;
    char *pEnvBlock;
    PROCESS_INFORMATION pi;
    DWORD dwCreationFlags = 0;

    new->in = attr->parent_in;
    new->err = attr->parent_err;
    new->out = attr->parent_out;

    (void) apr_get_oslevel(cont, &os_level);

    if (attr->detached) {
        /* If we are creating ourselves detached, Then we should hide the
         * window we are starting in.  And we had better redfine our
         * handles for STDIN, STDOUT, and STDERR. Do not set the
         * detached attribute for Win9x. We have found that Win9x does
         * not manage the stdio handles properly when running old 16
         * bit executables if the detached attribute is set.
         */
        if (os_level >= APR_WIN_NT) {
            /* 
             * XXX DETACHED_PROCESS won't on Win9x at all; on NT/W2K 
             * 16 bit executables fail (MS KB: Q150956)
             */
            dwCreationFlags |= DETACHED_PROCESS;
        }
    }

    /* progname must be the unquoted, actual name, or NULL if this is
     * a 16 bit app running in the VDM or WOW context.
     */
    if (progname[0] == '\"') {
        progname = apr_pstrndup(cont, progname + 1, strlen(progname) - 2);
    }
    
    /* progname must be unquoted, in native format, as there are all sorts 
     * of bugs in the NT library loader code that fault when parsing '/'.
     * We do not directly manipulate cmdline, and it will become a copy,
     * so we've casted past the constness issue.
     */
    if (strchr(progname, ' '))
        cmdline = apr_pstrcat(cont, "\"", progname, "\"", NULL);
    else
        cmdline = (char*)progname;

    i = 1;
    while (args && args[i]) {
        if (strchr(args[i], ' '))
            cmdline = apr_pstrcat(cont, cmdline, " \"", args[i], "\"", NULL);
        else
            cmdline = apr_pstrcat(cont, cmdline, " ", args[i], NULL);
        i++;
    }

    if (attr->cmdtype == APR_SHELLCMD) {
        char *shellcmd = getenv("COMSPEC");
        if (!shellcmd)
            shellcmd = SHELL_PATH;
        if (shellcmd[0] == '"')
            progname = apr_pstrndup(cont, shellcmd + 1, strlen(shellcmd) - 1);
        else if (strchr(shellcmd, ' '))
            shellcmd = apr_pstrcat(cont, "\"", shellcmd, "\"", NULL);
        cmdline = apr_pstrcat(cont, shellcmd, " /C \"", cmdline, "\"", NULL);
    }

    if (!env) 
        pEnvBlock = NULL;
    else {
        apr_size_t iEnvBlockLen;
        /*
         * Win32's CreateProcess call requires that the environment
         * be passed in an environment block, a null terminated block of
         * null terminated strings.
         */  
        i = 0;
        iEnvBlockLen = 1;
        while (env[i]) {
            iEnvBlockLen += strlen(env[i]) + 1;
            i++;
        }
        if (!i) 
            ++iEnvBlockLen;

#if APR_HAS_UNICODE_FS
        if (os_level >= APR_WIN_NT) {
            apr_wchar_t *pNext;
            pEnvBlock = (char *)apr_palloc(cont, iEnvBlockLen * 2);
            dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;

            i = 0;
            pNext = (apr_wchar_t*)pEnvBlock;
            while (env[i]) {
                apr_size_t in = strlen(env[i]) + 1;
                if ((rv = conv_utf8_to_ucs2(env[i], &in, 
                                            pNext, &iEnvBlockLen)) 
                        != APR_SUCCESS) {
                    return rv;
                }
                pNext = wcschr(pNext, L'\0') + 1;
                i++;
            }
	    if (!i)
                *(pNext++) = L'\0';
	    *pNext = L'\0';
        }
        else 
#endif /* APR_HAS_UNICODE_FS */
        {
            char *pNext;
            pEnvBlock = (char *)apr_palloc(cont, iEnvBlockLen);
    
            i = 0;
            pNext = pEnvBlock;
            while (env[i]) {
                strcpy(pNext, env[i]);
                pNext = strchr(pNext, '\0') + 1;
                i++;
            }
	    if (!i)
                *(pNext++) = '\0';
	    *pNext = '\0';
        }
    } 

#if APR_HAS_UNICODE_FS
    if (os_level >= APR_WIN_NT)
    {
        STARTUPINFOW si;
        apr_wchar_t wprg[APR_PATH_MAX];
        apr_size_t ncmd = strlen(cmdline) + 1, nwcmd = ncmd;
        apr_size_t ncwd = 0, nwcwd = 0;
        apr_wchar_t *wcmd = apr_palloc(cont, ncmd * sizeof(wcmd[0]));
        apr_wchar_t *wcwd = NULL;

        if (attr->currdir)
        {
            ncwd = nwcwd = strlen(attr->currdir) + 1;
            wcwd = apr_palloc(cont, ncwd * sizeof(wcwd[0]));
        }

        if (((rv = utf8_to_unicode_path(wprg, sizeof(wprg)/sizeof(wprg[0]),
                                        progname))
                    != APR_SUCCESS)
         || ((rv = conv_utf8_to_ucs2(cmdline, &ncmd, wcmd, &nwcmd)) 
                    != APR_SUCCESS)
         || (attr->currdir &&
             (rv = conv_utf8_to_ucs2(attr->currdir, &ncwd, wcwd, &nwcwd)) 
                    != APR_SUCCESS)) {
            return rv;
        }

        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        if (attr->detached) {
            si.dwFlags |= STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
        }
        if (attr->child_in || attr->child_out || attr->child_err)
        {
            si.dwFlags |= STARTF_USESTDHANDLES;
            if (attr->child_in)
                si.hStdInput = attr->child_in->filehand;
            if (attr->child_out)
                si.hStdOutput = attr->child_out->filehand;
            if (attr->child_err)
                si.hStdError = attr->child_err->filehand;
        }

        rv = CreateProcessW(wprg, wcmd,        /* Command line */
                            NULL, NULL,        /* Proc & thread security attributes */
                            TRUE,              /* Inherit handles */
                            dwCreationFlags,   /* Creation flags */
                            pEnvBlock,         /* Environment block */
                            wcwd,     /* Current directory name */
                            &si, &pi);
    }
    else {
#endif /* APR_HAS_UNICODE_FS */
        STARTUPINFOA si;
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        if (attr->detached) {
            si.dwFlags |= STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
        }
        if (attr->child_in || attr->child_out || attr->child_err)
        {
            si.dwFlags |= STARTF_USESTDHANDLES;
            if (attr->child_in)
                si.hStdInput = attr->child_in->filehand;
            if (attr->child_out)
                si.hStdOutput = attr->child_out->filehand;
            if (attr->child_err)
                si.hStdError = attr->child_err->filehand;
        }

        rv = CreateProcessA(progname, cmdline, /* Command line */
                            NULL, NULL,        /* Proc & thread security attributes */
                            TRUE,              /* Inherit handles */
                            dwCreationFlags,   /* Creation flags */
                            pEnvBlock,         /* Environment block */
                            attr->currdir,     /* Current directory name */
                            &si, &pi);
    }

    /* Check CreateProcess result 
     */
    if (!rv)
        return apr_get_os_error();

    /* XXX Orphaned handle warning - no fix due to broken apr_proc_t api.
     */
    new->hproc = pi.hProcess;
    new->pid = pi.dwProcessId;

    if (attr->child_in) {
        apr_file_close(attr->child_in);
    }
    if (attr->child_out) {
        apr_file_close(attr->child_out);
    }
    if (attr->child_err) {
        apr_file_close(attr->child_err);
    }
    CloseHandle(pi.hThread);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_proc_wait(apr_proc_t *proc, apr_wait_how_e wait)
{
    DWORD stat;
    if (!proc)
        return APR_ENOPROC;
    if (wait == APR_WAIT) {
        if ((stat = WaitForSingleObject(proc->hproc, 
                                        INFINITE)) == WAIT_OBJECT_0) {
            CloseHandle(proc->hproc);
            proc->hproc = NULL;
            return APR_CHILD_DONE;
        }
        else if (stat == WAIT_TIMEOUT) {
            return APR_CHILD_NOTDONE;
        }
        return apr_get_os_error();
    }
    if ((stat = WaitForSingleObject((HANDLE)proc->hproc, 0)) == WAIT_OBJECT_0) {
        CloseHandle(proc->hproc);
        proc->hproc = NULL;
        return APR_CHILD_DONE;
    }
    else if (stat == WAIT_TIMEOUT) {
        return APR_CHILD_NOTDONE;
    }
    return apr_get_os_error();
}
