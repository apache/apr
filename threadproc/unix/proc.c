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

#include "threadproc.h"
#include "apr_strings.h"
#include "apr_portable.h"

apr_status_t apr_createprocattr_init(apr_procattr_t **new, apr_pool_t *cont)
{
    (*new) = (apr_procattr_t *)apr_pcalloc(cont, sizeof(apr_procattr_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }
    (*new)->cntxt = cont;
    (*new)->cmdtype = APR_PROGRAM;
    return APR_SUCCESS;
}

apr_status_t apr_setprocattr_io(apr_procattr_t *attr, apr_int32_t in, 
                                 apr_int32_t out, apr_int32_t err)
{
    apr_status_t status;
    if (in != 0) {
        if ((status = apr_create_pipe(&attr->child_in, &attr->parent_in, 
                                   attr->cntxt)) != APR_SUCCESS) {
            return status;
        }
        switch (in) {
        case APR_FULL_BLOCK:
            break;
        case APR_PARENT_BLOCK:
            apr_set_pipe_timeout(attr->child_in, 0);
            break;
        case APR_CHILD_BLOCK:
            apr_set_pipe_timeout(attr->parent_in, 0);
            break;
        default:
            apr_set_pipe_timeout(attr->child_in, 0);
            apr_set_pipe_timeout(attr->parent_in, 0);
        }
    } 
    if (out) {
        if ((status = apr_create_pipe(&attr->parent_out, &attr->child_out, 
                                   attr->cntxt)) != APR_SUCCESS) {
            return status;
        }
        switch (out) {
        case APR_FULL_BLOCK:
            break;
        case APR_PARENT_BLOCK:
            apr_set_pipe_timeout(attr->child_out, 0);
            break;
        case APR_CHILD_BLOCK:
            apr_set_pipe_timeout(attr->parent_out, 0);
            break;
        default:
            apr_set_pipe_timeout(attr->child_out, 0);
            apr_set_pipe_timeout(attr->parent_out, 0);
        }
    } 
    if (err) {
        if ((status = apr_create_pipe(&attr->parent_err, &attr->child_err, 
                                   attr->cntxt)) != APR_SUCCESS) {
            return status;
        }
        switch (err) {
        case APR_FULL_BLOCK:
            break;
        case APR_PARENT_BLOCK:
            apr_set_pipe_timeout(attr->child_err, 0);
            break;
        case APR_CHILD_BLOCK:
            apr_set_pipe_timeout(attr->parent_err, 0);
            break;
        default:
            apr_set_pipe_timeout(attr->child_err, 0);
            apr_set_pipe_timeout(attr->parent_err, 0);
        }
    } 
    return APR_SUCCESS;
}


apr_status_t apr_setprocattr_childin(apr_procattr_t *attr, apr_file_t *child_in,
                                   apr_file_t *parent_in)
{
    if (attr->child_in == NULL && attr->parent_in == NULL)
        apr_create_pipe(&attr->child_in, &attr->parent_in, attr->cntxt);

    if (child_in != NULL)
        apr_dupfile(&attr->child_in, child_in, attr->cntxt);

    if (parent_in != NULL)
        apr_dupfile(&attr->parent_in, parent_in, attr->cntxt);

    return APR_SUCCESS;
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


apr_status_t apr_setprocattr_dir(apr_procattr_t *attr, 
                               const char *dir) 
{
    attr->currdir = apr_pstrdup(attr->cntxt, dir);
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

apr_status_t apr_setprocattr_detach(apr_procattr_t *attr, apr_int32_t detach) 
{
    attr->detached = detach;
    return APR_SUCCESS;
}

apr_status_t apr_fork(apr_proc_t *proc, apr_pool_t *cont)
{
    int pid;
    
    if ((pid = fork()) < 0) {
        return errno;
    }
    else if (pid == 0) {
        proc->pid = pid;
        proc->in = NULL; 
        proc->out = NULL; 
        proc->err = NULL; 
        return APR_INCHILD;
    }
    proc->pid = pid;
    proc->in = NULL; 
    proc->out = NULL; 
    proc->err = NULL; 
    return APR_INPARENT;
}

#if APR_HAVE_STRUCT_RLIMIT
#if defined(RLIMIT_CPU) || defined(RLIMIT_NPROC) || defined(RLIMIT_AS) || defined(RLIMIT_DATA) || defined(RLIMIT_VMEM)
static apr_status_t limit_proc(apr_procattr_t *attr)
{
#ifdef RLIMIT_CPU
    if (attr->limit_cpu != NULL) {
        if ((setrlimit(RLIMIT_CPU, attr->limit_cpu)) != 0) {
            return errno;
        }
    }
#endif
#ifdef RLIMIT_NPROC
    if (attr->limit_nproc != NULL) {
        if ((setrlimit(RLIMIT_NPROC, attr->limit_nproc)) != 0) {
            return errno;
        }
    }
#endif
#if defined(RLIMIT_AS)
    if (attr->limit_mem != NULL) {
        if ((setrlimit(RLIMIT_AS, attr->limit_mem)) != 0) {
            return errno;
        }
    }
#elif defined(RLIMIT_DATA)
    if (attr->limit_mem != NULL) {
        if ((setrlimit(RLIMIT_DATA, attr->limit_mem)) != 0) {
            return errno;
        }
    }
#elif defined(RLIMIT_VMEM)
    if (attr->limit_mem != NULL) {
        if ((setrlimit(RLIMIT_VMEM, attr->limit_mem)) != 0) {
            return errno;
        }
    }
#endif
    return APR_SUCCESS;
}
#endif
#endif

apr_status_t apr_create_process(apr_proc_t *new, const char *progname, 
                              char *const args[], char **env,
                              apr_procattr_t *attr, apr_pool_t *cont)
{
    int i;
    typedef const char *my_stupid_string;
    my_stupid_string *newargs;

    new->in = attr->parent_in;
    new->err = attr->parent_err;
    new->out = attr->parent_out;
    if ((new->pid = fork()) < 0) {
        return errno;
    }
    else if (new->pid == 0) { 
        int status;
        /* child process */
        if (attr->child_in) {
            apr_close(attr->parent_in);
            dup2(attr->child_in->filedes, STDIN_FILENO);
            apr_close(attr->child_in);
        }
        if (attr->child_out) {
            apr_close(attr->parent_out);
            dup2(attr->child_out->filedes, STDOUT_FILENO);
            apr_close(attr->child_out);
        }
        if (attr->child_err) {
            apr_close(attr->parent_err);
            dup2(attr->child_err->filedes, STDERR_FILENO);
            apr_close(attr->child_err);
        }
        
        apr_signal(SIGCHLD, SIG_DFL); /*not sure if this is needed or not */

        if (attr->currdir != NULL) {
            if (chdir(attr->currdir) == -1) {
                exit(-1);   /* We have big problems, the child should exit. */
            }
        }

        apr_cleanup_for_exec();

#if defined(RLIMIT_CPU) || defined(RLIMIT_NPROC) || defined(RLIMIT_AS) || defined(RLIMIT_DATA) || defined(RLIMIT_VMEM)
        if ((status = limit_proc(attr)) != APR_SUCCESS) {
            return status;
        }
#endif 

        if (attr->cmdtype == APR_SHELLCMD) {
            i = 0;
            while (args[i]) {
                i++;
            }
            newargs =
               (my_stupid_string *) apr_palloc(cont, sizeof (char *) * (i + 3));
            newargs[0] = SHELL_PATH;
            newargs[1] = "-c";
            i = 0;
            while (args[i]) {
                newargs[i + 2] = args[i]; 
                i++;
            }
            newargs[i + 3] = NULL;
            if (attr->detached) {
                apr_detach();
            }
            execve(SHELL_PATH, (char **) newargs, env);
        }
        else {
            if (attr->detached) {
                apr_detach();
            }
            execve(progname, args, env);
        }
        exit(-1);  /* if we get here, there is a problem, so exit with an */ 
                   /* error code. */
    }
    /* Parent process */
    if (attr->child_in) {
        apr_close(attr->child_in);
    }
    if (attr->child_out) {
        apr_close(attr->child_out);
    }
    if (attr->child_err) {
        apr_close(attr->child_err);
    }
    return APR_SUCCESS;
}

apr_status_t apr_wait_all_procs(apr_proc_t *proc, apr_wait_t *status,
                              apr_wait_how_e waithow, apr_pool_t *p)
{
    int waitpid_options = WUNTRACED;

    if (waithow != APR_WAIT) {
        waitpid_options |= WNOHANG;
    }

    if ((proc->pid = waitpid(-1, status, waitpid_options)) > 0) {
        return APR_CHILD_DONE;
    }
    else if (proc->pid == 0) {
        return APR_CHILD_NOTDONE;
    }
    return errno;
} 

apr_status_t apr_wait_proc(apr_proc_t *proc, 
                           apr_wait_how_e waithow)
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

apr_status_t apr_setprocattr_limit(apr_procattr_t *attr, apr_int32_t what, 
                          struct rlimit *limit)
{
    switch(what) {
        case APR_LIMIT_CPU:
#ifdef RLIMIT_CPU
            attr->limit_cpu = limit;
            break;
#else
            return APR_ENOTIMPL;
#endif
        case APR_LIMIT_MEM:
#if defined (RLIMIT_DATA) || defined (RLIMIT_VMEM) || defined(RLIMIT_AS)
            attr->limit_mem = limit;
            break;
#else
            return APR_ENOTIMPL;
#endif
        case APR_LIMIT_NPROC:
#ifdef RLIMIT_NPROC
            attr->limit_nproc = limit;
            break;
#else
            return APR_ENOTIMPL;
#endif
    }
    return APR_SUCCESS;
}  
