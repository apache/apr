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

struct send_pipe {
	int in;
	int out;
	int err;
};

apr_status_t apr_createprocattr_init(apr_procattr_t **new, apr_pool_t *cont)
{
    (*new) = (apr_procattr_t *)apr_palloc(cont, 
              sizeof(apr_procattr_t));

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

apr_status_t apr_setprocattr_dir(apr_procattr_t *attr, 
                                 const char *dir) 
{
    char * cwd;
    if (strncmp("/",dir,1) != 0 ) {
        cwd = (char*)malloc(sizeof(char) * PATH_MAX);
        getcwd(cwd, PATH_MAX);
        strncat(cwd,"/\0",2);
        strcat(cwd,dir);
        attr->currdir = (char *)apr_pstrdup(attr->cntxt, cwd);
        free(cwd);
    } else {
        attr->currdir = (char *)apr_pstrdup(attr->cntxt, dir);
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


apr_status_t apr_create_process(apr_proc_t *new, const char *progname, 
                              char *const args[], char **env, 
                              apr_procattr_t *attr, apr_pool_t *cont)
{
    int i=0,nargs=0;
    char **newargs = NULL;
    thread_id newproc, sender;
    struct send_pipe *sp;        
	char * dir = NULL;
	    
	sp = (struct send_pipe *)apr_palloc(cont, sizeof(struct send_pipe));

    new->in = attr->parent_in;
    new->err = attr->parent_err;
    new->out = attr->parent_out;
	sp->in  = attr->child_in?attr->child_in->filedes:-1;
	sp->out = attr->child_out?attr->child_out->filedes:-1;
	sp->err = attr->child_err?attr->child_err->filedes:-1;

    i = 0;
    while (args && args[i]) {
        i++;
    }

	newargs = (char**)malloc(sizeof(char *) * (i + 4));
	newargs[0] = strdup("/boot/home/config/bin/apr_proc_stub");
    if (attr->currdir == NULL) {
        /* we require the directory , so use a temp. variable */
        dir = malloc(sizeof(char) * PATH_MAX);
        getcwd(dir, PATH_MAX);
        newargs[1] = strdup(dir);
        free(dir);
    } else {
        newargs[1] = strdup(attr->currdir);
    }
    newargs[2] = strdup(progname);
    i=0;nargs = 3;

    while (args && args[i]) {
        newargs[nargs] = strdup(args[i]);
        i++;nargs++;
    }
    newargs[nargs] = NULL;

    newproc = load_image(nargs, (const char**)newargs, (const char**)env);

    /* load_image copies the data so now we can free it... */
    while (--nargs >= 0)
        free (newargs[nargs]);
    free(newargs);
        
    if ( newproc < B_NO_ERROR) {
        return errno;
    }
    resume_thread(newproc);

    if (attr->child_in) {
        apr_close(attr->child_in);
    }
    if (attr->child_out) {
        apr_close(attr->child_out);
    }
    if (attr->child_err) {
        apr_close(attr->child_err);
    }

    send_data(newproc, 0, (void*)sp, sizeof(struct send_pipe));
    new->pid = newproc;

    /* before we go charging on we need the new process to get to a 
     * certain point.  When it gets there it'll let us know and we
     * can carry on. */
    receive_data(&sender, (void*)NULL,0);
    
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
                           apr_wait_how_e wait)
{
    status_t exitval;
    thread_info tinfo;
    
    if (!proc)
        return APR_ENOPROC;
    /* when we run processes we are actually running threads, so here
       we'll wait on the thread dying... */
    if (wait == APR_WAIT) {
        if (wait_for_thread(proc->pid, &exitval) == B_OK) {
            return APR_CHILD_DONE;
        }
        return errno;
    }
    /* if the thread is still alive then it's not done...
       this won't hang or holdup the thread checking... */
    if (get_thread_info(proc->pid, &tinfo) == B_BAD_VALUE) {
        return APR_CHILD_DONE;
    }
    /* if we get this far it's still going... */
    return APR_CHILD_NOTDONE;
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

apr_status_t apr_setprocattr_limit(apr_procattr_t *attr, apr_int32_t what, 
                          struct rlimit *limit)
{
    return APR_ENOTIMPL;
}
