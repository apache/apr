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

#define INCL_DOS
#define INCL_DOSERRORS

#include "threadproc.h"
#include "fileio.h"
#include "apr_config.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <process.h>
#include <stdlib.h>
#include <os2.h>

ap_status_t ap_createprocattr_init(ap_procattr_t **new, ap_pool_t *cont)
{
    (*new) = (ap_procattr_t *)ap_palloc(cont, 
              sizeof(ap_procattr_t));

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
    (*new)->detached = FALSE;
    return APR_SUCCESS;
}

ap_status_t ap_setprocattr_io(ap_procattr_t *attr, ap_int32_t in, 
                                 ap_int32_t out, ap_int32_t err)
{
    ap_status_t stat;
    if (in) {
        if ((stat = ap_create_pipe(&attr->child_in, &attr->parent_in,
                                   attr->cntxt)) != APR_SUCCESS) {
            return stat;
        }
        switch (in) {
        case APR_FULL_BLOCK:
            ap_block_pipe(attr->child_in);
            ap_block_pipe(attr->parent_in);
        case APR_PARENT_BLOCK:
            ap_block_pipe(attr->parent_in);
        case APR_CHILD_BLOCK:
            ap_block_pipe(attr->child_in);
        }
    } 
    if (out) {
        if ((stat = ap_create_pipe(&attr->parent_out, &attr->child_out,
                                   attr->cntxt)) != APR_SUCCESS) {
            return stat;
        }
        switch (out) {
        case APR_FULL_BLOCK:
            ap_block_pipe(attr->child_out);
            ap_block_pipe(attr->parent_out);
        case APR_PARENT_BLOCK:
            ap_block_pipe(attr->parent_out);
        case APR_CHILD_BLOCK:
            ap_block_pipe(attr->child_out);
        }
    } 
    if (err) {
        if ((stat = ap_create_pipe(&attr->parent_err, &attr->child_err,
                                   attr->cntxt)) != APR_SUCCESS) {
            return stat;
        }
        switch (err) {
        case APR_FULL_BLOCK:
            ap_block_pipe(attr->child_err);
            ap_block_pipe(attr->parent_err);
        case APR_PARENT_BLOCK:
            ap_block_pipe(attr->parent_err);
        case APR_CHILD_BLOCK:
            ap_block_pipe(attr->child_err);
        }
    } 
    return APR_SUCCESS;
}

ap_status_t ap_setprocattr_dir(ap_procattr_t *attr, const char *dir)
{
    attr->currdir = ap_pstrdup(attr->cntxt, dir);
    if (attr->currdir) {
        return APR_SUCCESS;
    }
    return APR_ENOMEM;
}

ap_status_t ap_setprocattr_cmdtype(ap_procattr_t *attr,
                                     ap_cmdtype_e cmd) 
{
    attr->cmdtype = cmd;
    return APR_SUCCESS;
}

ap_status_t ap_setprocattr_detach(ap_procattr_t *attr, ap_int32_t detach) 
{
    attr->detached = detach;
    return APR_SUCCESS;
}

ap_status_t ap_fork(ap_proc_t **proc, ap_pool_t *cont)
{
    int pid;
    
    (*proc) = ap_palloc(cont, sizeof(ap_proc_t));

    if ((pid = fork()) < 0) {
        return errno;
    } else if (pid == 0) {
        (*proc)->pid = pid;
        (*proc)->attr = NULL;
        (*proc)->running = TRUE;
        return APR_INCHILD;
    }

    (*proc)->pid = pid;
    (*proc)->attr = NULL;
    (*proc)->running = TRUE;
    return APR_INPARENT;
}



/* quotes in the string are doubled up.
 * Used to escape quotes in args passed to OS/2's cmd.exe
 */
static char *double_quotes(ap_pool_t *cntxt, char *str)
{
    int num_quotes = 0;
    int len = 0;
    char *quote_doubled_str, *dest;
    
    while (str[len]) {
        num_quotes += str[len++] == '\"';
    }
    
    quote_doubled_str = ap_palloc(cntxt, len + num_quotes + 1);
    dest = quote_doubled_str;
    
    while (*str) {
        if (*str == '\"')
            *(dest++) = '\"';
        *(dest++) = *(str++);
    }
    
    *dest = 0;
    return quote_doubled_str;
}



ap_status_t ap_create_process(ap_proc_t **new, const char *progname,
                              char *const args[], char **env,
                              ap_procattr_t *attr, ap_pool_t *cont)
{
    int i, arg, numargs, cmdlen;
    ap_status_t status;
    char **newargs;
    char savedir[300];
    HFILE save_in, save_out, save_err, dup;
    int criticalsection = FALSE;
    char *extension, *newprogname, *extra_arg = NULL, *cmdline, *cmdline_pos;
    char interpreter[1024];
    char error_object[260];
    ap_file_t *progfile;
    int env_len, e;
    char *env_block, *env_block_pos;
    RESULTCODES rescodes;

    (*new) = (ap_proc_t *)ap_palloc(cont, sizeof(ap_proc_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
    (*new)->running = FALSE;

    /* Prevent other threads from running while these process-wide resources are modified */
    if (attr->child_in || attr->child_out || attr->child_err || attr->currdir) {
        criticalsection = TRUE;
        DosEnterCritSec();
    }

    if (attr->child_in) {
        save_in = -1;
        DosDupHandle(STDIN_FILENO, &save_in);
        dup = STDIN_FILENO;
        DosDupHandle(attr->child_in->filedes, &dup);
        DosSetFHState(attr->parent_in->filedes, OPEN_FLAGS_NOINHERIT);
    }
    
    if (attr->child_out) {
        save_out = -1;
        DosDupHandle(STDOUT_FILENO, &save_out);
        dup = STDOUT_FILENO;
        DosDupHandle(attr->child_out->filedes, &dup);
        DosSetFHState(attr->parent_out->filedes, OPEN_FLAGS_NOINHERIT);
    }
    
    if (attr->child_err) {
        save_err = -1;
        DosDupHandle(STDERR_FILENO, &save_err);
        dup = STDERR_FILENO;
        DosDupHandle(attr->child_err->filedes, &dup);
        DosSetFHState(attr->parent_err->filedes, OPEN_FLAGS_NOINHERIT);
    }

    ap_signal(SIGCHLD, SIG_DFL); /*not sure if this is needed or not */

    if (attr->currdir != NULL) {
        _getcwd2(savedir, sizeof(savedir));
        
        if (_chdir2(attr->currdir) < 0) {
            if (criticalsection)
                DosExitCritSec();
            return errno;
        }
    }

    interpreter[0] = 0;
    extension = strrchr(progname, '.');

    if (extension == NULL || strchr(extension, '/') || strchr(extension, '\\'))
        extension = "";

    if (attr->cmdtype == APR_SHELLCMD || strcasecmp(extension, ".cmd") == 0) {
        strcpy(interpreter, "#!" SHELL_PATH);
        extra_arg = "/C";
    } else if (stricmp(extension, ".exe") != 0) {
        status = ap_open(&progfile, progname, APR_READ|APR_BUFFERED, 0, cont);

        if (status == APR_ENOENT) {
            progname = ap_pstrcat(cont, progname, ".exe", NULL);
        }

        if (status == APR_SUCCESS) {
            status = ap_fgets(interpreter, sizeof(interpreter), progfile);

            if (status == APR_SUCCESS) {
                if (interpreter[0] == '#' && interpreter[1] == '!') {
                    if (interpreter[2] != '/' && interpreter[2] != '\\' && interpreter[3] != ':') {
                        char buffer[300];

                        if (DosSearchPath(SEARCH_ENVIRONMENT, "PATH", interpreter+2, buffer, sizeof(buffer)) == 0) {
                            strcpy(interpreter+2, buffer);
                        } else {
                            strcat(interpreter, ".exe");
                            if (DosSearchPath(SEARCH_ENVIRONMENT, "PATH", interpreter+2, buffer, sizeof(buffer)) == 0) {
                                strcpy(interpreter+2, buffer);
                            }
                        }
                    }
                } else {
                    interpreter[0] = 0;
                }
            }
        }
        ap_close(progfile);
    }

    i = 0;

    while (args && args[i]) {
        i++;
    }

    newargs = (char **)ap_palloc(cont, sizeof (char *) * (i + 4));
    numargs = 0;

    if (interpreter[0])
        newargs[numargs++] = interpreter + 2;
    if (extra_arg)
        newargs[numargs++] = "/c";

    newargs[numargs++] = newprogname = ap_pstrdup(cont, progname);
    arg = 1;

    while (args && args[arg]) {
        newargs[numargs++] = args[arg++];
    }

    newargs[numargs] = NULL;

    for (i=0; newprogname[i]; i++)
        if (newprogname[i] == '/')
            newprogname[i] = '\\';

    cmdlen = 0;

    for (i=0; i<numargs; i++)
        cmdlen += strlen(newargs[i]) + 3;

    cmdline = ap_pstrndup(cont, newargs[0], cmdlen + 2);
    cmdline_pos = cmdline + strlen(cmdline);

    for (i=1; i<numargs; i++) {
        char *a = newargs[i];

        if (strpbrk(a, "&|<>\" "))
            a = ap_pstrcat(cont, "\"", double_quotes(cont, a), "\"", NULL);

        *(cmdline_pos++) = ' ';
        strcpy(cmdline_pos, a);
        cmdline_pos += strlen(cmdline_pos);
    }

    *(++cmdline_pos) = 0; /* Add required second terminator */
    cmdline_pos = strchr(cmdline, ' ');

    if (cmdline_pos) {
        *cmdline_pos = 0;
        cmdline_pos++;
    }

    /* Create environment block from list of envariables */
    if (env) {
        for (env_len=1, e=0; env[e]; e++)
            env_len += strlen(env[e]) + 1;

        env_block = ap_palloc(cont, env_len);
        env_block_pos = env_block;

        for (e=0; env[e]; e++) {
            strcpy(env_block_pos, env[e]);
            env_block_pos += strlen(env_block_pos) + 1;
        }

        *env_block_pos = 0; /* environment block is terminated by a double null */
    } else
        env_block = NULL;

    status = DosExecPgm(error_object, sizeof(error_object),
                        attr->detached ? EXEC_BACKGROUND : EXEC_ASYNCRESULT,
                        cmdline, env_block, &rescodes, cmdline);

    (*new)->pid = rescodes.codeTerminate;

    if (attr->currdir != NULL) {
        chdir(savedir);
    }

    if (attr->child_in) {
        ap_close(attr->child_in);
        dup = STDIN_FILENO;
        DosDupHandle(save_in, &dup);
        DosClose(save_in);
    }
    
    if (attr->child_out) {
        ap_close(attr->child_out);
        dup = STDOUT_FILENO;
        DosDupHandle(save_out, &dup);
        DosClose(save_out);
    }
    
    if (attr->child_err) {
        ap_close(attr->child_err);
        dup = STDERR_FILENO;
        DosDupHandle(save_err, &dup);
        DosClose(save_err);
    }

    if (criticalsection)
        DosExitCritSec();

    (*new)->attr = attr;
    (*new)->running = status == APR_SUCCESS;
    return status;
}



ap_status_t ap_get_childin(ap_file_t **new, ap_proc_t *proc)
{
    (*new) = proc->attr->parent_in;
    return APR_SUCCESS; 
}

ap_status_t ap_get_childout(ap_file_t **new, ap_proc_t *proc)
{
    (*new) = proc->attr->parent_out; 
    return APR_SUCCESS;
}

ap_status_t ap_get_childerr(ap_file_t **new, ap_proc_t *proc)
{
    (*new) = proc->attr->parent_err; 
    return APR_SUCCESS;
}    

ap_status_t ap_wait_proc(ap_proc_t *proc, 
                           ap_wait_how_e wait)
{
    RESULTCODES codes;
    ULONG rc;
    PID pid;

    if (!proc)
        return APR_ENOPROC;

    if (!proc->running)
        return APR_CHILD_DONE;

    rc = DosWaitChild(DCWA_PROCESS, wait == APR_WAIT ? DCWW_WAIT : DCWW_NOWAIT, &codes, &pid, proc->pid);

    if (rc == 0) {
        proc->running = 0;
        return APR_CHILD_DONE;
    } else if (rc == ERROR_CHILD_NOT_COMPLETE) {
        return APR_CHILD_NOTDONE;
    }

    return APR_OS2_STATUS(rc);
} 



ap_status_t ap_get_os_proc(ap_os_proc_t *theproc, ap_proc_t *proc)
{
    if (proc == NULL) {
        return APR_ENOPROC;
    }
    *theproc = proc->pid;
    return APR_SUCCESS;
}
