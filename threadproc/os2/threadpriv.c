/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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

#include "apr_arch_threadproc.h"
#include "apr_thread_proc.h"
#include "apr_portable.h"
#include "apr_general.h"
#include "apr_errno.h"
#include "apr_lib.h"
#include "apr_arch_file_io.h"

APR_DECLARE(apr_status_t) apr_threadkey_private_create(apr_threadkey_t **key,
                                                       void (*dest)(void *), 
                                                       apr_pool_t *pool)
{
    (*key) = (apr_threadkey_t *)apr_palloc(pool, sizeof(apr_threadkey_t));

    if ((*key) == NULL) {
        return APR_ENOMEM;
    }

    (*key)->pool = pool;
    return APR_OS2_STATUS(DosAllocThreadLocalMemory(1, &((*key)->key)));
}

APR_DECLARE(apr_status_t) apr_threadkey_private_get(void **new, apr_threadkey_t *key)
{
    (*new) = (void *)*(key->key);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_threadkey_private_set(void *priv, apr_threadkey_t *key)
{
    *(key->key) = (ULONG)priv;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_threadkey_private_delete(apr_threadkey_t *key)
{
    return APR_OS2_STATUS(DosFreeThreadLocalMemory(key->key));
}

APR_DECLARE(apr_status_t) apr_threadkey_data_get(void **data, const char *key,
                                                 apr_threadkey_t *threadkey)
{
    return apr_pool_userdata_get(data, key, threadkey->pool);
}

APR_DECLARE(apr_status_t) apr_threadkey_data_set(void *data, const char *key,
                                                 apr_status_t (*cleanup) (void *),
                                                 apr_threadkey_t *threadkey)
{
    return apr_pool_userdata_set(data, key, cleanup, threadkey->pool);
}

APR_DECLARE(apr_status_t) apr_os_threadkey_get(apr_os_threadkey_t *thekey, apr_threadkey_t *key)
{
    *thekey = key->key;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_threadkey_put(apr_threadkey_t **key, 
                                               apr_os_threadkey_t *thekey, 
                                               apr_pool_t *pool)
{
    if (pool == NULL) {
        return APR_ENOPOOL;
    }
    if ((*key) == NULL) {
        (*key) = (apr_threadkey_t *)apr_pcalloc(pool, sizeof(apr_threadkey_t));
        (*key)->pool = pool;
    }
    (*key)->key = *thekey;
    return APR_SUCCESS;
}           
