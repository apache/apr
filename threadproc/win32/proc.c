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

#include "threadproc.h"
#include "fileio.h"

#include "apr_thread_proc.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_portable.h"
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <process.h>

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
	(*new)->detached = TRUE;

    memset(&(*new)->si, 0, sizeof((*new)->si));

	return APR_SUCCESS;
}

ap_status_t ap_setprocattr_io(struct procattr_t *attr, ap_int32_t in, 
                                 ap_int32_t out, ap_int32_t err)
{
    ap_status_t stat;
    if (in) {
        if ((stat = ap_create_pipe(&attr->child_in, &attr->parent_in, 
                                   attr->cntxt)) != APR_SUCCESS) {
            return stat;
        }
    } 
    if (out) {
        if ((stat = ap_create_pipe(&attr->parent_out, &attr->child_out, 
                                   attr->cntxt)) != APR_SUCCESS) {
            return stat;
        }
    } 
    if (err) {
        if ((stat = ap_create_pipe(&attr->parent_err, &attr->child_err, 
                                   attr->cntxt)) != APR_SUCCESS) {
            return stat;
        }
    } 
    return APR_SUCCESS;
}

ap_status_t ap_setprocattr_dir(struct procattr_t *attr, 
                                 char *dir) 
{
    attr->currdir = ap_pstrdup(attr->cntxt, dir);
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

ap_status_t ap_setprocattr_detach(struct procattr_t *attr,
									ap_int32_t det) 
{
    attr->detached = det;
    return APR_SUCCESS;
}

ap_status_t ap_create_process(ap_context_t *cont, char *progname, 
                               char *const args[], char **env, 
                               struct procattr_t *attr, struct proc_t **new)
{
    int i, iEnvBlockLen;
	char *cmdline;
	HANDLE hCurrentProcess;
	HANDLE hParentindup, hParentoutdup,hParenterrdup;
    char ppid[20];
    char *envstr;
    char *pEnvBlock, *pNext;



    (*new) = (struct proc_t *)ap_palloc(cont, sizeof(struct proc_t));

    if ((*new) == NULL) {
        return APR_ENOMEM;
    }

    (*new)->cntxt = cont;
	(*new)->attr = attr;

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

	cmdline = args[0];
	i = 1;
	while (args[i]) {
		cmdline = ap_pstrcat(cont, cmdline, " ", args[i], NULL);
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
		if (attr->child_in) {
			ap_close(attr->child_in);
			ap_close(attr->parent_in);
		}
		if (attr->child_out) {
			ap_close(attr->child_out);
			ap_close(attr->parent_out);
		}
		if (attr->child_err) {
			ap_close(attr->child_err);
			ap_close(attr->parent_err);
		}
		return APR_EEXIST;
    }
    else {
		if (attr->child_in) {
			ap_close(attr->parent_in);
		    attr->parent_in->filehand = hParentindup;
		}
		if (attr->child_out) {
			ap_close(attr->parent_out);
			attr->parent_out->filehand = hParentoutdup;
		}
		if (attr->child_err) {
		    ap_close(attr->parent_err);
		    attr->parent_err->filehand = hParenterrdup;
		}
    }

    _itoa(_getpid(), ppid, 10);
    if (env) {

        envstr = ap_pstrcat(cont, "parentpid=", ppid, NULL);
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
  
        pEnvBlock = (char *)ap_pcalloc(cont, iEnvBlockLen + strlen(envstr));
    
        i = 0;
        pNext = pEnvBlock;
        while (env[i]) {
            strcpy(pNext, env[i]);
            pNext = pNext + strlen(pNext) + 1;
            i++;
        }
        strcpy(pNext, envstr);        
    }
    else {
        SetEnvironmentVariable("parentpid", ppid);
        pEnvBlock = NULL;
   } 
    

    if (CreateProcess(NULL, cmdline, NULL, NULL, TRUE, 0, pEnvBlock, attr->currdir, 
		&attr->si, &(*new)->pi)) {
		if (attr->detached) {
			CloseHandle((*new)->pi.hProcess);
		}
		if (attr->child_in) {
			ap_close(attr->child_in);
		}
		if (attr->child_out) {
			ap_close(attr->child_out);
		}
		if (attr->child_err) {
			ap_close(attr->child_err);
		}
		CloseHandle((*new)->pi.hThread);
		return APR_SUCCESS;
	}

	return GetLastError();
}

ap_status_t ap_get_childin(ap_file_t **new, struct proc_t *proc)
{
    (*new) = proc->attr->parent_in;
    return APR_SUCCESS; 
}

ap_status_t ap_get_childout(ap_file_t **new, struct proc_t *proc)
{
    (*new) = proc->attr->parent_out; 
    return APR_SUCCESS;
}

ap_status_t ap_get_childerr(ap_file_t **new, struct proc_t *proc)
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
        if ((stat = WaitForSingleObject(proc->pi.hProcess, INFINITE)) == WAIT_OBJECT_0) {
            return APR_CHILD_DONE;
        }
        else if (stat == WAIT_TIMEOUT) {
            return APR_CHILD_NOTDONE;
        }
        return APR_EEXIST;
    }
    if ((stat = WaitForSingleObject(proc->pi.hProcess, 0)) == WAIT_OBJECT_0) {
            return APR_CHILD_DONE;
        }
        else if (stat == WAIT_TIMEOUT) {
            return APR_CHILD_NOTDONE;
        }
        return APR_EEXIST;
} 

ap_status_t ap_get_procdata(struct proc_t *proc, char *key, void *data)
{
    if (proc != NULL) {
        return ap_get_userdata(&data, key, proc->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOPROC;
    }
}

ap_status_t ap_set_procdata(struct proc_t *proc, void *data, char *key,
                            ap_status_t (*cleanup) (void *))
{
    if (proc != NULL) {
        return ap_set_userdata(data, key, cleanup, proc->cntxt);
    }
    else {
        data = NULL;
        return APR_ENOPROC;
    }
}

ap_status_t ap_get_os_proc(ap_proc_t *proc, ap_os_proc_t *theproc)
{
    if (proc == NULL) {
        return APR_ENOPROC;
    }
    theproc = &(proc->pi);
    return APR_SUCCESS;
}

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
    (*proc)->pi = *theproc;
    return APR_SUCCESS;
}              
