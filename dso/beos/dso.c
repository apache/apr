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

#include "beos/apr_arch_dso.h"
#include "apr_portable.h"

#if APR_HAS_DSO

static apr_status_t dso_cleanup(void *thedso)
{
    apr_dso_handle_t *dso = thedso;

    if (dso->handle > 0 && unload_add_on(dso->handle) < B_NO_ERROR)
        return APR_EINIT;
    dso->handle = -1;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_dso_load(apr_dso_handle_t **res_handle, 
                                       const char *path, apr_pool_t *pool)
{
    image_id newid;

    if((newid = load_add_on(path)) < B_NO_ERROR)
        return APR_EDSOOPEN;

    *res_handle = apr_pcalloc(pool, sizeof(*res_handle));
    (*res_handle)->handle = newid;
    (*res_handle)->pool = pool;

    apr_pool_cleanup_register(pool, *res_handle, dso_cleanup, apr_pool_cleanup_null);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_dso_unload(apr_dso_handle_t *handle)
{
    return apr_pool_cleanup_run(handle->pool, handle, dso_cleanup);
}

APR_DECLARE(apr_status_t) apr_dso_sym(apr_dso_handle_sym_t *ressym, apr_dso_handle_t *handle,
               const char *symname)
{
    int err;

    if (symname == NULL)
        return APR_ESYMNOTFOUND;

    err = get_image_symbol(handle->handle, symname, B_SYMBOL_TYPE_ANY, 
			 ressym);

    if(err != B_OK)
        return APR_ESYMNOTFOUND;

    return APR_SUCCESS;
}

APR_DECLARE(const char *) apr_dso_error(apr_dso_handle_t *dso, char *buffer, apr_size_t buflen)
{
    strncpy(buffer, strerror(errno), buflen);
    return buffer;
}

APR_DECLARE(apr_status_t) apr_os_dso_handle_put(apr_dso_handle_t **aprdso,
                                                apr_os_dso_handle_t osdso,
                                                apr_pool_t *pool)
{
    *aprdso = apr_pcalloc(pool, sizeof **aprdso);
    (*aprdso)->handle = osdso;
    (*aprdso)->pool = pool;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_os_dso_handle_get(apr_os_dso_handle_t *osdso,
                                                apr_dso_handle_t *aprdso)
{
    *osdso = aprdso->handle;
    return APR_SUCCESS;
}

#endif
