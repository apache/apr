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

#ifndef APR_THREAD_PROC_H
#define APR_THREAD_PROC_H

#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_errno.h"
#if APR_HAVE_STRUCT_RLIMIT
#include <sys/time.h>
#include <sys/resource.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @package APR Thread library
 */

typedef enum {APR_SHELLCMD, APR_PROGRAM} ap_cmdtype_e;
typedef enum {APR_WAIT, APR_NOWAIT} ap_wait_how_e;

#define APR_NO_PIPE          0
#define APR_FULL_BLOCK       1
#define APR_FULL_NONBLOCK    2
#define APR_PARENT_BLOCK     3
#define APR_CHILD_BLOCK      4

#define APR_LIMIT_CPU        0
#define APR_LIMIT_MEM        1
#define APR_LIMIT_NPROC      2

#if APR_HAS_OTHER_CHILD
#define APR_OC_REASON_DEATH         0     /* child has died, caller must call
                                           * unregister still */
#define APR_OC_REASON_UNWRITABLE    1     /* write_fd is unwritable */
#define APR_OC_REASON_RESTART       2     /* a restart is occuring, perform
                                           * any necessary cleanup (including
                                           * sending a special signal to child)
                                           */
#define APR_OC_REASON_UNREGISTER    3     /* unregister has been called, do
                                           * whatever is necessary (including
                                           * kill the child) */
#define APR_OC_REASON_LOST          4     /* somehow the child exited without
                                           * us knowing ... buggy os? */
#endif /* APR_HAS_OTHER_CHILD */

typedef struct ap_proc_t {
    pid_t pid;
    ap_file_t *in;          /* Parent's side of pipe to child's stdin */
    ap_file_t *out;         /* Parent's side of pipe to child's stdout */
    ap_file_t *err;         /* Parent's side of pipe to child's stdouterr */
} ap_proc_t;

typedef struct ap_thread_t           ap_thread_t;
typedef struct ap_threadattr_t       ap_threadattr_t;
typedef struct ap_procattr_t         ap_procattr_t;

typedef struct ap_threadkey_t        ap_threadkey_t;
#if APR_HAS_OTHER_CHILD
typedef struct ap_other_child_rec_t  ap_other_child_rec_t;
#endif /* APR_HAS_OTHER_CHILD */

typedef void *(APR_THREAD_FUNC *ap_thread_start_t)(void *);

/* Thread Function definitions */

/**
 * Create and initialize a new threadattr variable
 * @param new_attr The newly created threadattr.
 * @param cont The pool to use
 */
ap_status_t ap_create_threadattr(ap_threadattr_t **new_attr, ap_pool_t *cont);

/**
 * Set if newly created threads should be created in detach mode.
 * @param attr The threadattr to affect 
 * @param on Thread detach state on or off
 */
ap_status_t ap_setthreadattr_detach(ap_threadattr_t *attr, ap_int32_t on);

/**
 * Get the detach mode for this threadattr.
 * @param attr The threadattr to reference 
 */
ap_status_t ap_getthreadattr_detach(ap_threadattr_t *attr);

/**
 * Create a new thread of execution
 * @param new_thread The newly created thread handle.
 * @param attr The threadattr to use to determine how to create the thread
 * @param func The function to start the new thread in
 * @param data Any data to be passed to the starting function
 * @param cont The pool to use
 */
ap_status_t ap_create_thread(ap_thread_t **new_thread, ap_threadattr_t *attr, 
                             ap_thread_start_t func, void *data, 
                             ap_pool_t *cont);

/**
 * stop the current thread
 * @param thd The thread to stop
 * @param retval The return value to pass back to any thread that cares
 */
ap_status_t ap_thread_exit(ap_thread_t *thd, ap_status_t *retval);

/**
 * block until the desired thread stops executing.
 * @param retval The return value from the dead thread.
 * @param thd The thread to join
 */
ap_status_t ap_thread_join(ap_status_t *retval, ap_thread_t *thd); 

/**
 * detach a thread
 * @param thd The thread to detach 
 */
ap_status_t ap_thread_detach(ap_thread_t *thd);

/**
 * Return the pool associated with the current thread.
 * @param data The user data associated with the thread.
 * @param key The key to associate with the data
 * @param thread The currently open thread.
 */
ap_status_t ap_get_threaddata(void **data, const char *key, ap_thread_t *thread);

/**
 * Return the pool associated with the current thread.
 * @param data The user data to associate with the thread.
 * @param key The key to use for associating the data with the tread
 * @param cleanup The cleanup routine to use when the thread is destroyed.
 * @param thread The currently open thread.
 */
ap_status_t ap_set_threaddata(void *data, const char *key,
                              ap_status_t (*cleanup) (void *), 
                              ap_thread_t *thread);

/**
 * Create and initialize a new thread private address space
 * @param key The thread private handle.
 * @param dest The destructor to use when freeing the private memory.
 * @param cont The pool to use
 */
ap_status_t ap_create_thread_private(ap_threadkey_t **key, void (*dest)(void *),
                                     ap_pool_t *cont);

/**
 * Get a pointer to the thread private memory
 * @param new_mem The data stored in private memory 
 * @param key The handle for the desired thread private memory 
 */
ap_status_t ap_get_thread_private(void **new_mem, ap_threadkey_t *key);

/**
 * Set the data to be stored in thread private memory
 * @param priv The data to be stored in private memory 
 * @param key The handle for the desired thread private memory 
 */
ap_status_t ap_set_thread_private(void *priv, ap_threadkey_t *key);

/**
 * Free the thread private memory
 * @param key The handle for the desired thread private memory 
 */
ap_status_t ap_delete_thread_private(ap_threadkey_t *key);

/**
 * Return the pool associated with the current threadkey.
 * @param data The user data associated with the threadkey.
 * @param key The key associated with the data
 * @param threadkey The currently open threadkey.
 */
ap_status_t ap_get_threadkeydata(void **data, const char *key, ap_threadkey_t *threadkey);

/**
 * Return the pool associated with the current threadkey.
 * @param data The data to set.
 * @param key The key to associate with the data.
 * @param cleanup The cleanup routine to use when the file is destroyed.
 * @param threadkey The currently open threadkey.
 */
ap_status_t ap_set_threadkeydata(void *data, const char *key,
                                 ap_status_t (*cleanup) (void *), 
                                 ap_threadkey_t *threadkey);

/* Process Function definitions */

/**
 * @package APR Process library
 */

/**
 * Create and initialize a new procattr variable
 * @param new_attr The newly created procattr. 
 * @param cont The pool to use
 */
ap_status_t ap_createprocattr_init(ap_procattr_t **new_attr, ap_pool_t *cont);

/**
 * Determine if any of stdin, stdout, or stderr should be linked to pipes 
 * when starting a child process.
 * @param attr The procattr we care about. 
 * @param in Should stdin be a pipe back to the parent?
 * @param out Should stdout be a pipe back to the parent?
 * @param err Should stderr be a pipe back to the parent?
 */
ap_status_t ap_setprocattr_io(ap_procattr_t *attr, ap_int32_t in, 
                              ap_int32_t out, ap_int32_t err);

/**
 * Set the child_in and/or parent_in values to existing ap_file_t values.
 * @param attr The procattr we care about. 
 * @param child_in ap_file_t value to use as child_in. Must be a valid file.
 * @param parent_in ap_file_t value to use as parent_in. Must be a valid file.
 * @tip  This is NOT a required initializer function. This is
 *       useful if you have already opened a pipe (or multiple files)
 *       that you wish to use, perhaps persistently across mutiple
 *       process invocations - such as a log file. You can save some 
 *       extra function calls by not creating your own pipe since this
 *       creates one in the process space for you.
 */
ap_status_t ap_setprocattr_childin(struct ap_procattr_t *attr, ap_file_t *child_in,
                                   ap_file_t *parent_in);

/**
 * Set the child_out and parent_out values to existing ap_file_t values.
 * @param attr The procattr we care about. 
 * @param child_out ap_file_t value to use as child_out. Must be a valid file.
 * @param parent_out ap_file_t value to use as parent_out. Must be a valid file.
 * @tip This is NOT a required initializer function. This is
 *      useful if you have already opened a pipe (or multiple files)
 *      that you wish to use, perhaps persistently across mutiple
 *      process invocations - such as a log file. 
 */
ap_status_t ap_setprocattr_childout(struct ap_procattr_t *attr, 
                                    ap_file_t *child_out, 
                                    ap_file_t *parent_out);

/**
 * Set the child_err and parent_err values to existing ap_file_t values.
 * @param attr The procattr we care about. 
 * @param child_err ap_file_t value to use as child_err. Must be a valid file.
 * @param parent_err ap_file_t value to use as parent_err. Must be a valid file.
 * @tip This is NOT a required initializer function. This is
 *      useful if you have already opened a pipe (or multiple files)
 *      that you wish to use, perhaps persistently across mutiple
 *      process invocations - such as a log file. 
 */
ap_status_t ap_setprocattr_childerr(struct ap_procattr_t *attr, 
                                    ap_file_t *child_err,
                                    ap_file_t *parent_err);

/**
 * Set which directory the child process should start executing in.
 * @param attr The procattr we care about. 
 * @param dir Which dir to start in.  By default, this is the same dir as
 *            the parent currently resides in, when the createprocess call
 *            is made. 
 */
ap_status_t ap_setprocattr_dir(ap_procattr_t *attr, const char *dir);

/**
 * Set what type of command the child process will call.
 * @param attr The procattr we care about. 
 * @param cmd The type of command.  One of:
 * <PRE>
 *            APR_SHELLCMD --  Shell script
 *            APR_PROGRAM  --  Executable program   (default) 
 * </PRE>
 */
ap_status_t ap_setprocattr_cmdtype(ap_procattr_t *attr, ap_cmdtype_e cmd);

/**
 * Determine if the chlid should start in detached state.
 * @param attr The procattr we care about. 
 * @param detach Should the child start in detached state?  Default is no. 
 */
ap_status_t ap_setprocattr_detach(ap_procattr_t *attr, ap_int32_t detach);

#if APR_HAVE_STRUCT_RLIMIT
/**
 * Set the Resource Utilization limits when starting a new process.
 * @param attr The procattr we care about. 
 * @param what Which limit to set, one of:
 * <PRE>
 *                 APR_LIMIT_CPU
 *                 APR_LIMIT_MEM
 *                 APR_LIMIT_NPROC
 * </PRE>
 * @param limit Value to set the limit to.
 */
ap_status_t ap_setprocattr_limit(ap_procattr_t *attr, ap_int32_t what, 
                              struct rlimit *limit);
#endif

#if APR_HAS_FORK
/**
 * This is currently the only non-portable call in APR.  This executes 
 * a standard unix fork.
 * @param proc The resulting process handle. 
 * @param cont The pool to use. 
 */
ap_status_t ap_fork(ap_proc_t *proc, ap_pool_t *cont);
#endif

/**
 * Create a new process and execute a new program within that process.
 * @param new_proc The resulting process handle.
 * @param progname The program to run 
 * @param const_args the arguments to pass to the new program.  The first 
 *                   one should be the program name.
 * @param env The new environment ap_table_t for the new process.  This 
 *            should be a list of NULL-terminated strings.
 * @param attr the procattr we should use to determine how to create the new
 *         process
 * @param cont The pool to use. 
 */
ap_status_t ap_create_process(ap_proc_t *new_proc, const char *progname, 
                              char *const args[], char **env, 
                              ap_procattr_t *attr, ap_pool_t *cont);

/**
 * Wait for a child process to die
 * @param proc The process handle that corresponds to the desired child process 
 * @param waithow How should we wait.  One of:
 * <PRE>
 *            APR_WAIT   -- block until the child process dies.
 *            APR_NOWAIT -- return immediately regardless of if the 
 *                          child is dead or not.
 * </PRE>
 * @tip The childs status is in the return code to this process.  It is one of:
 * <PRE>
 *            APR_CHILD_DONE     -- child is no longer running.
 *            APR_CHILD_NOTDONE  -- child is still running.
 * </PRE>
 */
ap_status_t ap_wait_proc(ap_proc_t *proc, ap_wait_how_e waithow);


/**
 * Wait for any current child process to die and return information 
 * about that child.
 * @param proc Pointer to NULL on entry, will be filled out with child's 
 *             information 
 * @param status The returned exit status of the child, if a child process dies
 *               On platforms that don't support obtaining this information, 
 *               the status parameter will be returned as APR_ENOTIMPL.
 * @param waithow How should we wait.  One of:
 * <PRE>
 *            APR_WAIT   -- block until the child process dies.
 *            APR_NOWAIT -- return immediately regardless of if the 
 *                          child is dead or not.
 * </PRE>
 * @param p Pool to allocate child information out of.
 */

ap_status_t ap_wait_all_procs(ap_proc_t *proc, ap_wait_t *status, 
                              ap_wait_how_e waithow, ap_pool_t *p);

/**
 * Detach the process from the controlling terminal.
 */
ap_status_t ap_detach(void);

#if APR_HAS_OTHER_CHILD
/**
 * Register an other_child -- a child which must be kept track of so 
 * that the program knows when it has dies or disappeared.
 * @param pid pid is the pid of the child.
 * @param maintenance maintenance is a function that is invoked with a 
 *                    reason and the data pointer passed here.
 * @param data The data to pass to the maintenance function.
 * @param write_fd An fd that is probed for writing.  If it is ever unwritable
 *                 then the maintenance is invoked with reason 
 *                 OC_REASON_UNWRITABLE.
 * @param p The pool to use for allocating memory.
 */
void ap_register_other_child(ap_proc_t *pid, 
                             void (*maintenance) (int reason, void *, int status),
                             void *data, ap_file_t *write_fd, ap_pool_t *p);

/**
 * Stop watching the specified process.
 * @param data The data to pass to the maintenance function.  This is
 *             used to find the process to unregister.
 * @tip Since this can be called by a maintenance function while we're
 *      scanning the other_children list, all scanners should protect 
 *      themself by loading ocr->next before calling any maintenance 
 *      function.
 */
void ap_unregister_other_child(void *data);

/**
 * Check on the specified process.  If it is gone, call the maintenance 
 * function.
 * @param pid The process to check.
 * @param status The status to pass to the maintenance function.
 */
ap_status_t ap_reap_other_child(ap_proc_t *pid, int status); 

/**
 * Loop through all registered other_children and call the appropriate 
 * maintenance function when necessary.
 */
void ap_check_other_child(void); 

/**
 * Ensure all the registered write_fds are still writable, otherwise 
 * invoke the maintenance functions as appropriate.
 */
void ap_probe_writable_fds(void);
#endif /* APR_HAS_OTHER_CHILD */

/** 
 * Terminate a process.
 * @param proc The process to terminate.
 * @param sig How to kill the process.
 */
ap_status_t ap_kill(ap_proc_t *proc, int sig);
#ifdef __cplusplus
}
#endif

#endif  /* ! APR_FILE_IO_H */

