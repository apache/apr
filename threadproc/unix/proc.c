/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
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
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "threadproc.h"
#include "apr_portable.h"

/* ***APRDOC********************************************************
 * ap_status_t ap_createprocattr_init(ap_procattr_t **new, ap_context_t *cont)
 *    Create and initialize a new procattr variable 
 * arg 1) The newly created procattr. 
 * arg 2) The context to use
 */
ap_status_t ap_createprocattr_init(struct procattr_t **new, ap_context_t *cont)
{
    (*new) = (struct procattr_t *)ap_palloc(cont, 
              sizeof(struct procattr_t));

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
    (*new)->detached = 0; 
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_setprocattr_io(ap_procattr_t *attr, ap_int32_t in, 
 *                               ap_int32_t *out, ap_int32_t err)
 *    Determine if any of stdin, stdout, or stderr should be linked
 *    to pipes when starting a child process. 
 * arg 1) The procattr we care about. 
 * arg 2) Should stdin be a pipe bnack to the parent?
 * arg 3) Should stdout be a pipe bnack to the parent?
 * arg 4) Should stderr be a pipe bnack to the parent?
 */
ap_status_t ap_setprocattr_io(struct procattr_t *attr, ap_int32_t in, 
                                 ap_int32_t out, ap_int32_t err)
{
    ap_status_t status;
    if (in != 0) {
        if ((status = ap_create_pipe(&attr->child_in, &attr->parent_in, 
                                   attr->cntxt)) != APR_SUCCESS) {
            return status;
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
        if ((status = ap_create_pipe(&attr->parent_out, &attr->child_out, 
                                   attr->cntxt)) != APR_SUCCESS) {
            return status;
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
        if ((status = ap_create_pipe(&attr->parent_err, &attr->child_err, 
                                   attr->cntxt)) != APR_SUCCESS) {
            return status;
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


/* ***APRDOC********************************************************
 * ap_status_t ap_setprocattr_childin(ap_procattr_t *attr, ap_file_t *child_in,
 *                                    ap_file_t *parent_in)
 *    Set the child_in and/or parent_in values to existing ap_file_t
 *    values. This is NOT a required initializer function. This is
 *    useful if you have already opened a pipe (or multiple files)
 *    that you wish to use, perhaps persistently across mutiple
 *    process invocations - such as a log file. You can save some 
 *    extra function calls by not creating your own pipe since this
 *    creates one in the process space for you.
 * arg 1) The procattr we care about. 
 * arg 2) ap_file_t value to use as child_in. Must be a valid file.
 * arg 3) ap_file_t value to use as parent_in. Must be a valid file.
 */
ap_status_t ap_setprocattr_childin(struct procattr_t *attr, ap_file_t *child_in,
                                   ap_file_t *parent_in)
{
    if (attr->child_in == NULL && attr->parent_in == NULL)
        ap_create_pipe(&attr->child_in, &attr->parent_in, attr->cntxt);

    if (child_in != NULL)
        ap_dupfile(&attr->child_in, child_in);

    if (parent_in != NULL)
        ap_dupfile(&attr->parent_in, parent_in);

    return APR_SUCCESS;
}


/* ***APRDOC********************************************************
 * ap_status_t ap_setprocattr_childout(ap_procattr_t *attr, 
 *                                     ap_file_t *child_out, 
 *                                     ap_file_t *parent_out)
 *    Set the child_out and parent_out values to existing ap_file_t
 *    values. This is NOT a required initializer function. This is
 *    useful if you have already opened a pipe (or multiple files)
 *    that you wish to use, perhaps persistently across mutiple
 *    process invocations - such as a log file. 
 * arg 1) The procattr we care about. 
 * arg 2) ap_file_t value to use as child_out. Must be a valid file.
 * arg 3) ap_file_t value to use as parent_out. Must be a valid file.
 */
ap_status_t ap_setprocattr_childout(struct procattr_t *attr, ap_file_t *child_out,
                                    ap_file_t *parent_out)
{
    if (attr->child_out == NULL && attr->parent_out == NULL)
        ap_create_pipe(&attr->child_out, &attr->parent_out, attr->cntxt);

    if (child_out != NULL)
        ap_dupfile(&attr->child_out, child_out);

    if (parent_out != NULL)
        ap_dupfile(&attr->parent_out, parent_out);

    return APR_SUCCESS;
}


/* ***APRDOC********************************************************
 * ap_status_t ap_setprocattr_childerr(ap_procattr_t *attr, 
 *                                     ap_file_t *child_err,
 *                                     ap_file_t *parent_err)
 *    Set the child_err and parent_err values to existing ap_file_t
 *    values. This is NOT a required initializer function. This is
 *    useful if you have already opened a pipe (or multiple files)
 *    that you wish to use, perhaps persistently across mutiple
 *    process invocations - such as a log file. 
 * arg 1) The procattr we care about. 
 * arg 2) ap_file_t value to use as child_err. Must be a valid file.
 * arg 3) ap_file_t value to use as parent_err. Must be a valid file.
 */
ap_status_t ap_setprocattr_childerr(struct procattr_t *attr, ap_file_t *child_err,
                                   ap_file_t *parent_err)
{
    if (attr->child_err == NULL && attr->parent_err == NULL)
        ap_create_pipe(&attr->child_err, &attr->parent_err, attr->cntxt);

    if (child_err != NULL)
        ap_dupfile(&attr->child_err, child_err);

    if (parent_err != NULL)
        ap_dupfile(&attr->parent_err, parent_err);

    return APR_SUCCESS;
}


/* ***APRDOC********************************************************
 * ap_status_t ap_setprocattr_dir(ap_procattr_t *attr, constchar *dir) 
 *    Set which directory the child process should start executing in. 
 * arg 1) The procattr we care about. 
 * arg 2) Which dir to start in.  By default, this is the same dir as
 *        the parent currently resides in, when the createprocess call
 *        is made. 
 */
ap_status_t ap_setprocattr_dir(struct procattr_t *attr, 
                               const char *dir) 
{
    attr->currdir = ap_pstrdup(attr->cntxt, dir);
    if (attr->currdir) {
        return APR_SUCCESS;
    }
    return APR_ENOMEM;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_setprocattr_cmdtype(ap_procattr_t *attr, ap_cmdtype_e cmd) 
 *    Set what type of command the child process will call. 
 * arg 1) The procattr we care about. 
 * arg 2) The type of command.  One of:
 *            APR_SHELLCMD --  Shell script
 *            APR_PROGRAM  --  Executable program   (default) 
 */
ap_status_t ap_setprocattr_cmdtype(struct procattr_t *attr,
                                     ap_cmdtype_e cmd) 
{
    attr->cmdtype = cmd;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_setprocattr_detach(ap_procattr_t *attr, ap_int32_t detach) 
 *    Determine if the chlid should start in detached state.
 * arg 1) The procattr we care about. 
 * arg 2) Should the child start in detached state?  Default is no. 
 */
ap_status_t ap_setprocattr_detach(struct procattr_t *attr, ap_int32_t detach) 
{
    attr->detached = detach;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_fork(ap_proc_t **proc, ap_context_t *cont) 
 *    This is currently the only non-portable call in APR.  This executes
 *    a standard unix fork.
 * arg 1) The resulting process handle. 
 * arg 2) The context to use. 
 */
ap_status_t ap_fork(struct proc_t **proc, ap_context_t *cont)
{
    int pid;
    
    (*proc) = ap_palloc(cont, sizeof(struct proc_t));

    if ((pid = fork()) < 0) {
        return errno;
    }
    else if (pid == 0) {
        (*proc)->pid = pid;
        (*proc)->attr = NULL;
        return APR_INCHILD;
    }
    (*proc)->pid = pid;
    (*proc)->attr = NULL;
    return APR_INPARENT;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_create_process(ap_proc_t **new, const char *progname,
 *                               char *const args[], char **env, 
 *                               ap_procattr_t *attr, ap_context_t *cont) 
 *    Create a new process and execute a new program within that process.
 * arg 1) The resulting process handle.
 * arg 2) The program to run 
 * arg 3) the arguments to pass to the new program.  The first one should
 *        be the program name.
 * arg 4) The new environment ap_table_t for the new process.  This should be a
 *        list of NULL-terminated strings.
 * arg 5) the procattr we should use to determine how to create the new
 *        process
 * arg 6) The context to use. 
 */
ap_status_t ap_create_process(struct proc_t **new, const char *progname, 
                              char *const args[], char **env,
                              struct procattr_t *attr, ap_context_t *cont)
{
    int i;
    typedef const char *my_stupid_string;
    my_stupid_string *newargs;
    struct proc_t *pgrp; 

    (*new) = (struct proc_t *)ap_palloc(cont, sizeof(struct proc_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;

    if (((*new)->pid = fork()) < 0) {
        return errno;
    }
    else if ((*new)->pid == 0) { 
        /* child process */
        if (attr->child_in) {
            ap_close(attr->parent_in);
            dup2(attr->child_in->filedes, STDIN_FILENO);
            ap_close(attr->child_in);
        }
        if (attr->child_out) {
            ap_close(attr->parent_out);
            dup2(attr->child_out->filedes, STDOUT_FILENO);
            ap_close(attr->child_out);
        }
        if (attr->child_err) {
            ap_close(attr->parent_err);
            dup2(attr->child_err->filedes, STDERR_FILENO);
            ap_close(attr->child_err);
        }
        
        ap_signal(SIGCHLD, SIG_DFL); /*not sure if this is needed or not */

        if (attr->currdir != NULL) {
            if (chdir(attr->currdir) == -1) {
                exit(-1);   /* We have big problems, the child should exit. */
            }
        }

        ap_cleanup_for_exec();

        if (attr->cmdtype == APR_SHELLCMD) {
            i = 0;
            while (args[i]) {
                i++;
            }
            newargs =
               (my_stupid_string *) ap_palloc(cont, sizeof (char *) * (i + 3));
            newargs[0] = SHELL_PATH;
            newargs[1] = "-c";
            i = 0;
            while (args[i]) {
                newargs[i + 2] = args[i]; 
                i++;
            }
            newargs[i + 3] = NULL;
            if (attr->detached) {
                ap_detach(&pgrp, attr->cntxt);
            }
            execve(SHELL_PATH, (char **) newargs, env);
        }
        else {
            if (attr->detached) {
                ap_detach(&pgrp, attr->cntxt);
            }
            execve(progname, args, env);
        }
        exit(-1);  /* if we get here, there is a problem, so exit with an */ 
                   /* error code. */
    }
    /* Parent process */
    if (attr->child_in) {
        ap_close(attr->child_in);
    }
    if (attr->child_out) {
        ap_close(attr->child_out);
    }
    if (attr->child_err) {
        ap_close(attr->child_err);
    }
    
    (*new)->attr = attr;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_childin(ap_file_t **new, ap_proc_t *proc) 
 *    Get the file handle that is assocaited with a child's stdin.
 * arg 1) The returned file handle. 
 * arg 2) The process handle that corresponds to the desired child process 
 */
ap_status_t ap_get_childin(ap_file_t **new, struct proc_t *proc)
{
    (*new) = proc->attr->parent_in;
    return APR_SUCCESS; 
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_childout(ap_file_t **new, ap_proc_t *proc) 
 *    Get the file handle that is assocaited with a child's stdout.
 * arg 1) The returned file handle. 
 * arg 2) The process handle that corresponds to the desired child process 
 */
ap_status_t ap_get_childout(ap_file_t **new, struct proc_t *proc)
{
    (*new) = proc->attr->parent_out; 
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_get_childerr(ap_file_t **new, ap_proc_t *proc) 
 *    Get the file handle that is assocaited with a child's stderr.
 * arg 1) The returned file handle. 
 * arg 2) The process handle that corresponds to the desired child process 
 */
ap_status_t ap_get_childerr(ap_file_t **new, struct proc_t *proc)
{
    (*new) = proc->attr->parent_err; 
    return APR_SUCCESS;
}    

/* ***APRDOC********************************************************
 * ap_status_t ap_wait_proc(ap_proc_t *proc, ap_wait_how waithow) 
 *    Wait for a child process to die 
 * arg 1) The process handle that corresponds to the desired child process 
 * arg 2) How should we wait.  One of:
 *            APR_WAIT   -- block until the child process dies.
 *            APR_NOWAIT -- return immediately regardless of if the 
 *                          child is dead or not.
 * NOTE:  The childs status is in the return code to this process.  It is 
 *        one of:
 *            APR_CHILD_DONE     -- child is no longer running.
 *            APR_CHILD_NOTDONE  -- child is still running.
 */
ap_status_t ap_wait_proc(struct proc_t *proc, 
                           ap_wait_how_e waithow)
{
    pid_t status;
    if (!proc)
        return APR_ENOPROC;
    if (waithow == APR_WAIT) {
        if ((status = waitpid(proc->pid, NULL, WUNTRACED)) > 0) {
            return APR_CHILD_DONE;
        }
        else if (status == 0) {
            return APR_CHILD_NOTDONE;
        }
        return errno;
    }
    if ((status = waitpid(proc->pid, NULL, WUNTRACED | WNOHANG)) > 0) {
        return APR_CHILD_DONE;
    }
    else if (status == 0) {
        return APR_CHILD_NOTDONE;
    }
    return errno;
} 

/* ***APRDOC********************************************************
 * ap_status_t ap_get_os_proc(ap_os_proc_t *theproc, ap_proc_t *proc)
 *    convert the proc from os specific type to apr type.
 * arg 1) The os specific proc we are converting to
 * arg 2) The apr proc we are converting
 */
ap_status_t ap_get_os_proc(ap_os_proc_t *theproc, ap_proc_t *proc)
{
    if (proc == NULL) {
        return APR_ENOPROC;
    }
    *theproc = proc->pid;
    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_put_os_proc(ap_proc_t *proc, ap_os_proc_t *theproc,
 *                            ap_context_t *cont)
 *    convert the proc from os specific type to apr type.
 * arg 1) The apr proc we are converting to.
 * arg 2) The os specific proc to convert
 * arg 3) The context to use if it is needed.
 */
ap_status_t ap_put_os_proc(struct proc_t **proc, ap_os_proc_t *theproc, 
                           ap_context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if ((*proc) == NULL) {
        (*proc) = (struct proc_t *)ap_palloc(cont, sizeof(struct proc_t));
        (*proc)->cntxt = cont;
    }
    (*proc)->pid = *theproc;
    return APR_SUCCESS;
}              

