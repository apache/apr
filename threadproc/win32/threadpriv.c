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
#include "apr_thread_proc.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_errno.h"
#include "apr_portable.h"

ap_status_t ap_create_thread_private(ap_context_t *cont, void (*dest)(void *),
                                     struct threadkey_t **key)
{
	(*key)->key = TlsAlloc();
	return APR_SUCCESS;
}

ap_status_t ap_get_thread_private(void **new, struct threadkey_t *key)
{
    if ((*new) = TlsGetValue(key->key)) {
        return APR_SUCCESS;
    }
    return APR_EEXIST;
}

ap_status_t ap_set_thread_private(struct threadkey_t *key, void *priv)
{
    if (TlsSetValue(key->key, priv)) {
        return APR_SUCCESS;
    }
    return APR_EEXIST;
}

ap_status_t ap_delete_thread_private(struct threadkey_t *key)
{
    if (TlsFree(key->key)) {
        return APR_SUCCESS; 
    }
    return APR_EEXIST;
}

ap_status_t ap_get_threadkeydata(struct threadkey_t *threadkey, char *key, void *data)
{
    if (threadkey != NULL) {
        return ap_get_userdata(&data, threadkey->cntxt, key);
    }
    else {
        data = NULL;
        return APR_ENOTHDKEY;
    }
}

ap_status_t ap_set_threadkeydata(struct threadkey_t *threadkey, void *data,
                                 char *key, ap_status_t (*cleanup) (void *))
{
    if (threadkey != NULL) {
        return ap_set_userdata(threadkey->cntxt, data, key, cleanup);
    }
    else {
        data = NULL;
        return APR_ENOTHDKEY;
    }
}

ap_status_t ap_get_os_threadkey(struct threadkey_t *key, ap_os_threadkey_t *thekey)
{
    if (key == NULL) {
        return APR_ENOFILE;
    }
    thekey = &(key->key);
    return APR_SUCCESS;
}

ap_status_t ap_put_os_threadkey(struct threadkey_t **key, 
                                ap_os_threadkey_t *thekey, ap_context_t *cont)
{
    if (cont == NULL) {
        return APR_ENOCONT;
    }
    if ((*key) == NULL) {
        (*key) = (struct threadkey_t *)ap_palloc(cont, sizeof(struct threadkey_t));
        (*key)->cntxt = cont;
    }
    (*key)->key = *thekey;
    return APR_SUCCESS;
}           

