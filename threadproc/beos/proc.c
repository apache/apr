/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */


#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include "threadproc.h"
#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_general.h"

ap_status_t ap_createprocattr_init(ap_context_t *cont, struct procattr_t **new)
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
    return APR_SUCCESS;
}

ap_status_t ap_setprocattr_io(struct procattr_t *attr, ap_int32_t in, 
                                 ap_int32_t out, ap_int32_t err)
{
    ap_status_t stat;
    if (in) {
        if ((stat = ap_create_pipe(attr->cntxt, &attr->child_in, 
                            &attr->parent_in)) != APR_SUCCESS) {
            return stat;
        }
    } 
    if (out) {
        if ((stat = ap_create_pipe(attr->cntxt, &attr->parent_out, 
                            &attr->child_out)) != APR_SUCCESS) {
            return stat;
        }
    } 
    if (err) {
        if ((stat = ap_create_pipe(attr->cntxt, &attr->parent_err, 
                            &attr->child_err)) != APR_SUCCESS) {
            return stat;
        }
    } 
	return APR_SUCCESS;
}

ap_status_t ap_setprocattr_dir(struct procattr_t *attr, 
                                 char *dir) 
{
    attr->currdir = (char *)ap_pstrdup(attr->cntxt, dir);
    if (attr->currdir) {
        return APR_SUCCESS;
    }
    return APR_ENOMEM;
}

ap_status_t ap_setprocattr_cmdtype(struct procattr_t *attr,
                                     ap_cmdtype_e cmd) 
{
    attr->cmdtype = cmd;
    return APR_SUCCESS;
}

ap_status_t ap_setprocattr_detach(struct procattr_t *attr, ap_int32_t detach) 
{
    attr->detached = detach;
    return APR_SUCCESS;
}

ap_status_t ap_fork(ap_context_t *cont, struct proc_t **proc)
{
    int pid;
    
    (*proc) = (struct proc_t *)ap_palloc(cont, sizeof(struct proc_t));

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

ap_status_t ap_create_process(ap_context_t *cont, char *progname, 
                               char *const args[], char **env, 
                               struct procattr_t *attr, struct proc_t **new)
{
    int i;
    char **newargs;
    (*new) = (struct proc_t *)ap_palloc(cont, sizeof(struct proc_t));

    if ((*new) == NULL){
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
        
        signal(SIGCHLD, SIG_DFL); /*not sure if this is needed or not */

        if (attr->currdir != NULL) {
            if (chdir(attr->currdir) == -1) {
                exit(-1);   /* We have big problems, the child should exit. */
            }
        }
        if (attr->cmdtype == APR_SHELLCMD) {
            i = 0;
            while (args[i]) {
                i++;
            }
            newargs = (char **)ap_palloc(cont, sizeof (char *) * (i + 3));
            newargs[0] = strdup(SHELL_PATH);
            newargs[1] = strdup("-c");
            i = 0;
            while (args[i]) {
                newargs[i + 2] = strdup(args[i]); 
                i++;
            }
            newargs[i + 3] = NULL;
            execve(SHELL_PATH, newargs, env);
        }
        else {
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

ap_status_t ap_get_childin(struct proc_t *proc, ap_file_t **new)
{
    (*new) = proc->attr->parent_in; 
    return APR_SUCCESS;
}

ap_status_t ap_get_childout(struct proc_t *proc, ap_file_t **new)
{
    (*new) = proc->attr->parent_out;
    return APR_SUCCESS;
}

ap_status_t ap_get_childerr(struct proc_t *proc, ap_file_t **new)
{
    (*new) = proc->attr->parent_err; 
    return APR_SUCCESS;
}    

ap_status_t ap_wait_proc(struct proc_t *proc, 
                           ap_wait_how_e wait)
{
    pid_t stat;
    if (!proc)
        return APR_ENOPROC;
    if (wait == APR_WAIT) {
        if ((stat = waitpid(proc->pid, NULL, WUNTRACED)) > 0) {
            return APR_CHILD_DONE;
        }
        else if (stat == 0) {
            return APR_CHILD_NOTDONE;
        }
        return errno;
    }
    if ((stat = waitpid(proc->pid, NULL, WUNTRACED | WNOHANG)) > 0) {
            return APR_CHILD_DONE;
        }
        else if (stat == 0) {
            return APR_CHILD_NOTDONE;
        }
        return errno;
} 
