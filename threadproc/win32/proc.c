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

apr_status_t apr_createprocattr_init(apr_procattr_t **new, apr_pool_t *cont)
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

apr_status_t apr_setprocattr_io(apr_procattr_t *attr, apr_int32_t in, 
                              apr_int32_t out, apr_int32_t err)
{
    apr_status_t stat;
    BOOLEAN bAsyncRead, bAsyncWrite;
    if (in) {
        switch (in) {
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
        if ((stat = apr_create_nt_pipe(&attr->child_in, &attr->parent_in, 
                                       bAsyncRead, bAsyncWrite,
                                       attr->cntxt)) != APR_SUCCESS) {
            return stat;
        }
    }
    if (out) {
        switch (out) {
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
        if ((stat = apr_create_nt_pipe(&attr->parent_out, &attr->child_out,
                                       bAsyncRead, bAsyncWrite,
                                       attr->cntxt)) != APR_SUCCESS) {
            return stat;
        }
    }
    if (err) {
        switch (err) {
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
        if ((stat = apr_create_nt_pipe(&attr->parent_err, &attr->child_err,
                                       bAsyncRead, bAsyncWrite,
                                       attr->cntxt)) != APR_SUCCESS) {
            return stat;
        }
    } 
    return APR_SUCCESS;
}
#if 0
apr_status_t apr_setprocattr_childin(apr_procattr_t *attr, apr_file_t *child_in,
                                   apr_file_t *parent_in)
{
}
apr_status_t apr_setprocattr_childout(apr_procattr_t *attr, apr_file_t *child_out,
                                    apr_file_t *parent_out)
{
    if (attr->child_out == NULL && attr->parent_out == NULL)
        apr_create_pipe(&attr->child_out, &attr->parent_out, attr->cntxt);

    if (child_out != NULL)
        apr_dupfile(&attr->child_out, child_out, attr->cntxt);

    if (parent_out != NULL)
        apr_dupfile(&attr->parent_out, parent_out, attr->cntxt);

    return APR_SUCCESS;
}
apr_status_t apr_setprocattr_childerr(apr_procattr_t *attr, apr_file_t *child_err,
                                   apr_file_t *parent_err)
{
    if (attr->child_err == NULL && attr->parent_err == NULL)
        apr_create_pipe(&attr->child_err, &attr->parent_err, attr->cntxt);

    if (child_err != NULL)
        apr_dupfile(&attr->child_err, child_err, attr->cntxt);

    if (parent_err != NULL)
        apr_dupfile(&attr->parent_err, parent_err, attr->cntxt);
    return APR_SUCCESS;
}
#endif
apr_status_t apr_setprocattr_dir(apr_procattr_t *attr, 
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

apr_status_t apr_setprocattr_cmdtype(apr_procattr_t *attr,
                                     apr_cmdtype_e cmd) 
{
    attr->cmdtype = cmd;
    return APR_SUCCESS;
}

apr_status_t apr_setprocattr_detach(apr_procattr_t *attr, apr_int32_t det) 
{
    attr->detached = det;
    return APR_SUCCESS;
}

apr_status_t apr_create_process(apr_proc_t *new, const char *progname, 
                                const char * const *args,
                                const char * const *env, 
                                apr_procattr_t *attr, apr_pool_t *cont)
{
    int i, iEnvBlockLen;
    char *cmdline;
    HANDLE hCurrentProcess;
    HANDLE hParentindup, hParentoutdup,hParenterrdup;
    char ppid[20];
    char *envstr;
    char *pEnvBlock, *pNext;
    apr_status_t rv;
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
    /*
     * When the pipe handles are created, the security descriptor
     * indicates that the handle can be inherited.  However, we do not
     * want the server side handles to the pipe to be inherited by the
     * child CGI process. If the child CGI does inherit the server
     * side handles, then the child may be left around if the server
     * closes its handles (e.g. if the http connection is aborted),
     * because the child will have a valid copy of handles to both
     * sides of the pipes, and no I/O error will occur.  Microsoft
     * recommends using DuplicateHandle to turn off the inherit bit
     * under NT and Win95.
     */
    hCurrentProcess = GetCurrentProcess();
    if ((attr->child_in && !DuplicateHandle(hCurrentProcess, attr->parent_in->filehand, 
                                            hCurrentProcess,
                                            &hParentindup, 0, FALSE,
                                            DUPLICATE_SAME_ACCESS))
	|| (attr->child_out && !DuplicateHandle(hCurrentProcess, attr->parent_out->filehand,
                                                hCurrentProcess, &hParentoutdup,
                                                0, FALSE, DUPLICATE_SAME_ACCESS))
	|| (attr->child_err && !DuplicateHandle(hCurrentProcess, attr->parent_err->filehand,
                                                hCurrentProcess, &hParenterrdup,
                                                0, FALSE, DUPLICATE_SAME_ACCESS))) {
        rv = apr_get_os_error();
        if (attr->child_in) {
            apr_close(attr->child_in);
            apr_close(attr->parent_in);
        }
        if (attr->child_out) {
            apr_close(attr->child_out);
            apr_close(attr->parent_out);
        }
        if (attr->child_err) {
            apr_close(attr->child_err);
            apr_close(attr->parent_err);
        }
        return rv;
    }
    else {
        if (attr->child_in) {
            CloseHandle(attr->parent_in->filehand);
            attr->parent_in->filehand = hParentindup;
        }
        if (attr->child_out) {
            CloseHandle(attr->parent_out->filehand);
            attr->parent_out->filehand = hParentoutdup;
        }
        if (attr->child_err) {
            CloseHandle(attr->parent_err->filehand);
            attr->parent_err->filehand = hParenterrdup;
        }
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

apr_status_t apr_wait_proc(apr_proc_t *proc, apr_wait_how_e wait)
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

