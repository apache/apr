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

APR_DECLARE(apr_status_t) apr_createprocattr_init(apr_procattr_t **new,
                                                  apr_pool_t *cont)
{
    (*new) = (apr_procattr_t *)apr_palloc(cont, sizeof(apr_procattr_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont;
    (*new)->parent_in = NULL;
    (*new)->child_in = NULL;
    (*new)->parent_out = NULL;
    (*new)->child_out = NULL;
    (*new)->parent_err = NULL;
    (*new)->child_err = NULL;
    (*new)->currdir = NULL; 
    (*new)->cmdtype = APR_PROGRAM;
    (*new)->detached = TRUE;

    memset(&(*new)->si, 0, sizeof((*new)->si));

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
    HANDLE pid = GetCurrentProcess();
    HANDLE filehand = file->filehand;

    /* Create new non-inheritable versions of handles that
     * the child process doesn't care about. Otherwise, the child
     * inherits these handles; resulting in non-closeable handles
     * to the respective pipes.
     */
    if (!DuplicateHandle(pid, filehand,
                         pid, &file->filehand, 0,
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

    /* Can't use apr_dupfile here because it creates a non-inhertible 
     * handle, and apr_open_file'd apr_file_t's are non-inheritable,
     * so we must assume we need to make an inheritable handle.
     */
    if (!CloseHandle(duplicate->filehand))
        return apr_get_os_error();
    else
    {
        HANDLE pid = GetCurrentProcess();
        if (!DuplicateHandle(pid, original->filehand, 
                             pid, &duplicate->filehand, 0,
                             TRUE, DUPLICATE_SAME_ACCESS))
            return apr_get_os_error();
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_setprocattr_io(apr_procattr_t *attr,
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

APR_DECLARE(apr_status_t) apr_setprocattr_childin(apr_procattr_t *attr, 
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

APR_DECLARE(apr_status_t) apr_setprocattr_childout(apr_procattr_t *attr,
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

APR_DECLARE(apr_status_t) apr_setprocattr_childerr(apr_procattr_t *attr,
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

APR_DECLARE(apr_status_t) apr_setprocattr_dir(apr_procattr_t *attr,
                                              const char *dir) 
{
    char path[MAX_PATH];
    int length;

    if (dir[0] != '\\' && dir[0] != '/' && dir[1] != ':') { 
        length = GetCurrentDirectory(MAX_PATH, path);

        if (length == 0 || length + strlen(dir) + 1 >= MAX_PATH)
            return APR_ENOMEM;

        attr->currdir = apr_pstrcat(attr->cntxt, path, "\\", dir, NULL);
    }
    else {
        attr->currdir = apr_pstrdup(attr->cntxt, dir);
    }

    if (attr->currdir) {
        return APR_SUCCESS;
    }
    return APR_ENOMEM;
}

APR_DECLARE(apr_status_t) apr_setprocattr_cmdtype(apr_procattr_t *attr,
                                                  apr_cmdtype_e cmd) 
{
    attr->cmdtype = cmd;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_setprocattr_detach(apr_procattr_t *attr,
                                                 apr_int32_t det) 
{
    attr->detached = det;
    return APR_SUCCESS;
}

/* TODO:  
 *   apr_create_process with APR_SHELLCMD on Win9x won't work due to MS KB:
 *   Q150956
 */

APR_DECLARE(apr_status_t) apr_create_process(apr_proc_t *new,
                                             const char *progname,
                                             const char * const *args,
                                             const char * const *env,
                                             apr_procattr_t *attr,
                                             apr_pool_t *cont)
{
    int i, iEnvBlockLen;
    char *cmdline;
    char ppid[20];
    char *envstr;
    char *pEnvBlock, *pNext;
    PROCESS_INFORMATION pi;

    new->in = attr->parent_in;
    new->err = attr->parent_err;
    new->out = attr->parent_out;

    attr->si.cb = sizeof(attr->si);
    if (attr->detached) {
        /* If we are creating ourselves detached, Then we should hide the
         * window we are starting in.  And we had better redfine our
         * handles for STDIN, STDOUT, and STDERR.
         */
        attr->si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
        attr->si.wShowWindow = SW_HIDE;

        if (attr->child_in) {
            attr->si.hStdInput = attr->child_in->filehand;
        }

        if (attr->child_out) {
            attr->si.hStdOutput = attr->child_out->filehand;
        }

        if (attr->child_err) {
            attr->si.hStdError = attr->child_err->filehand;
        }
    }

    if (attr->cmdtype == APR_PROGRAM) {
        const char *ptr = progname;

        if (*ptr =='"') {
            ptr++;
        }

        if (*ptr == '\\' || *ptr == '/' || *++ptr == ':') {
            cmdline = apr_pstrdup(cont, progname);
        }
        else if (attr->currdir == NULL) {
            cmdline = apr_pstrdup(cont, progname);
        }
        else {
            char lastchar = attr->currdir[strlen(attr->currdir)-1];
            if ( lastchar == '\\' || lastchar == '/') {
                cmdline = apr_pstrcat(cont, attr->currdir, progname, NULL);
            }
            else {
                cmdline = apr_pstrcat(cont, attr->currdir, "\\", progname, NULL);
            }
        }
    }
    else {
        char * shell_cmd = getenv("COMSPEC");
        if (!shell_cmd)
            shell_cmd = SHELL_PATH;
        shell_cmd = apr_pstrdup(cont, shell_cmd);
        cmdline = apr_pstrcat(cont, shell_cmd, " /C ", progname, NULL);
    }

    i = 1;
    while (args && args[i]) {
        cmdline = apr_pstrcat(cont, cmdline, " ", args[i], NULL);
        i++;
    }

    _itoa(_getpid(), ppid, 10);
    if (env) {
        envstr = apr_pstrcat(cont, "parentpid=", ppid, NULL);
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
  
        pEnvBlock = (char *)apr_pcalloc(cont, iEnvBlockLen + strlen(envstr));
    
        i = 0;
        pNext = pEnvBlock;
        while (env[i]) {
            strcpy(pNext, env[i]);
            pNext = pNext + strlen(pNext) + 1;
            i++;
        }
        strcpy(pNext, envstr); 
	pNext = pNext + strlen(pNext) + 1;
	*pNext = '\0';
    }
    else {
        SetEnvironmentVariable("parentpid", ppid);
        pEnvBlock = NULL;
    } 
    

    if (CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, pEnvBlock, attr->currdir, 
                      &attr->si, &pi)) {

        // TODO: THIS IS BADNESS
        // The completion of the apr_proc_t type leaves us ill equiped to track both 
        // the pid (Process ID) and handle to the process, which are entirely
        // different things and each useful in their own rights.
        //
        // Signals are broken since the hProcess varies from process to process, 
        // while the true process ID would not.
        new->pid = (pid_t) pi.hProcess;

        if (attr->child_in) {
            apr_close(attr->child_in);
        }
        if (attr->child_out) {
            apr_close(attr->child_out);
        }
        if (attr->child_err) {
            apr_close(attr->child_err);
        }
        CloseHandle(pi.hThread);

        return APR_SUCCESS;
    }

    return apr_get_os_error();
}

APR_DECLARE(apr_status_t) apr_wait_proc(apr_proc_t *proc, apr_wait_how_e wait)
{
    DWORD stat;
    if (!proc)
        return APR_ENOPROC;
    if (wait == APR_WAIT) {
        if ((stat = WaitForSingleObject((HANDLE)proc->pid, 
                                        INFINITE)) == WAIT_OBJECT_0) {
            return APR_CHILD_DONE;
        }
        else if (stat == WAIT_TIMEOUT) {
            return APR_CHILD_NOTDONE;
        }
        return GetLastError();
    }
    if ((stat = WaitForSingleObject((HANDLE)proc->pid, 0)) == WAIT_OBJECT_0) {
        return APR_CHILD_DONE;
    }
    else if (stat == WAIT_TIMEOUT) {
        return APR_CHILD_NOTDONE;
    }
    return GetLastError();
}

