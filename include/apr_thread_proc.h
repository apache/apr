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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {APR_SHELLCMD, APR_PROGRAM} ap_cmdtype_e;
typedef enum {APR_WAIT, APR_NOWAIT} ap_wait_how_e;

#define APR_NO_PIPE          0
#define APR_FULL_BLOCK       1
#define APR_FULL_NONBLOCK    2
#define APR_PARENT_BLOCK     3
#define APR_CHILD_BLOCK      4

#define APR_CANCEL_ASYNCH      1
#define APR_CANCEL_DEFER       2
#define APR_CANCEL_ENABLE      3
#define APR_CANCEL_DISABLE     4

typedef struct ap_thread_t           ap_thread_t;
typedef struct ap_threadattr_t       ap_threadattr_t;
typedef struct ap_proc_t		  ap_proc_t;
typedef struct ap_procattr_t         ap_procattr_t;

typedef struct ap_threadkey_t        ap_key_t;

typedef void *(API_THREAD_FUNC *ap_thread_start_t)(void *);

/* Thread Function definitions */
ap_status_t ap_create_threadattr(ap_threadattr_t **new, ap_context_t *cont);
ap_status_t ap_setthreadattr_detach(ap_threadattr_t *attr, ap_int32_t on);
ap_status_t ap_getthreadattr_detach(ap_threadattr_t *iattr);
ap_status_t ap_create_thread(ap_thread_t **new, ap_threadattr_t *attr, 
                             ap_thread_start_t func, void *data, 
                             ap_context_t *cont);
ap_status_t ap_thread_exit(ap_thread_t *thd, ap_status_t *retval);
ap_status_t ap_thread_join(ap_status_t *retval, ap_thread_t *thd); 
ap_status_t ap_thread_detach(ap_thread_t *thd);

ap_status_t ap_cancel_thread(ap_thread_t *thd);
ap_status_t ap_setcanceltype(ap_int32_t type, ap_context_t *cont);
ap_status_t ap_setcancelstate(ap_int32_t type, ap_context_t *cont);
ap_status_t ap_get_threaddata(void **data, char *key, ap_thread_t *thread);
ap_status_t ap_set_threaddata(void *data, char *key,
                              ap_status_t (*cleanup) (void *), 
                              ap_thread_t *thread);

ap_status_t ap_create_thread_private(ap_key_t **key, void (*dest)(void *), 
                                     ap_context_t *cont);
ap_status_t ap_get_thread_private(void **new, ap_key_t *key);
ap_status_t ap_set_thread_private(void *priv, ap_key_t *key);
ap_status_t ap_delete_thread_private(ap_key_t *key);
ap_status_t ap_get_threadkeydata(void **data, char *key, ap_key_t *threadkey);
ap_status_t ap_set_threadkeydata(void *data, char *key,
                                 ap_status_t (*cleanup) (void *), 
                                 ap_key_t *threadkey);

/* Process Function definitions */
ap_status_t ap_createprocattr_init(ap_procattr_t **new, ap_context_t *cont);
ap_status_t ap_setprocattr_io(ap_procattr_t *attr, ap_int32_t in, 
                              ap_int32_t out, ap_int32_t err);
ap_status_t ap_setprocattr_childin(struct ap_procattr_t *attr, ap_file_t *child_in,
                                   ap_file_t *parent_in);
ap_status_t ap_setprocattr_childout(struct ap_procattr_t *attr, 
                                    ap_file_t *child_out, 
                                    ap_file_t *parent_out);
ap_status_t ap_setprocattr_childerr(struct ap_procattr_t *attr, 
                                    ap_file_t *child_err,
                                    ap_file_t *parent_err);
ap_status_t ap_setprocattr_dir(ap_procattr_t *attr, const char *dir);
ap_status_t ap_setprocattr_cmdtype(ap_procattr_t *attr, ap_cmdtype_e cmd);
ap_status_t ap_setprocattr_detach(ap_procattr_t *attr, ap_int32_t detach);
ap_status_t ap_get_procdata(char *key, void *data, ap_proc_t *proc);
ap_status_t ap_set_procdata(void *data, char *key,
                            ap_status_t (*cleanup) (void *), ap_proc_t *proc);

ap_status_t ap_get_childin(ap_file_t **new, ap_proc_t *proc);
ap_status_t ap_get_childout(ap_file_t **new, ap_proc_t *proc);
ap_status_t ap_get_childerr(ap_file_t **new, ap_proc_t *proc);

ap_status_t ap_fork(ap_proc_t **proc, ap_context_t *cont);
ap_status_t ap_create_process(ap_proc_t **new, const char *progname, 
                              char *const args[], char **env, 
                              ap_procattr_t *attr, ap_context_t *cont);
ap_status_t ap_wait_proc(ap_proc_t *proc, ap_wait_how_e waithow);
ap_status_t ap_detach(ap_proc_t **new, ap_context_t *cont);

ap_status_t ap_kill(ap_proc_t *proc, int sig);
#ifdef __cplusplus
}
#endif

#endif  /* ! APR_FILE_IO_H */








