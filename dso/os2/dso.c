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

#include "dso.h"
#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <string.h>

#if APR_HAS_DSO

ap_status_t ap_dso_init() 
{
    return APR_SUCCESS;
}


static ap_status_t dso_cleanup(void *thedso)
{
    ap_dso_handle_t *dso = thedso;
    return ap_dso_unload(dso);
}


ap_status_t ap_dso_load(ap_dso_handle_t **res_handle, const char *path, ap_pool_t *ctx)
{
    char failed_module[20];
    HMODULE handle;
    int rc;

    *res_handle = ap_pcalloc(ctx, sizeof(*res_handle));
    (*res_handle)->cont = ctx;
    (*res_handle)->load_error = APR_SUCCESS;
    (*res_handle)->failed_module = NULL;

    if ((rc = DosLoadModule(failed_module, sizeof(failed_module), path, &handle)) != 0) {
        (*res_handle)->load_error = APR_OS2_STATUS(rc);
        (*res_handle)->failed_module = ap_pstrdup(ctx, failed_module);
        return APR_OS2_STATUS(rc);
    }

    (*res_handle)->handle  = handle;
    ap_register_cleanup(ctx, *res_handle, dso_cleanup, ap_null_cleanup);
    return APR_SUCCESS;
}



ap_status_t ap_dso_unload(ap_dso_handle_t *handle)
{
    int rc;

    if (handle->handle == 0)
        return APR_SUCCESS;
       
    rc = DosFreeModule(handle->handle);

    if (rc == 0)
        handle->handle = 0;

    return APR_OS2_STATUS(rc);
}



ap_status_t ap_dso_sym(ap_dso_handle_sym_t *ressym, 
                       ap_dso_handle_t *handle, 
                       const char *symname)
{
    PFN func;
    int rc;

    if (symname == NULL || ressym == NULL)
        return APR_EINIT;

    if ((rc = DosQueryProcAddr(handle->handle, 0, symname, &func)) != 0)
        return APR_EINIT;

    *ressym = func;
    return APR_SUCCESS;
}



char *ap_dso_error(ap_dso_handle_t *dso, char *buffer, ap_size_t buflen)
{
    char message[200];
    ap_strerror(dso->load_error, message, sizeof(message));
    strcat(message, " (");
    strcat(message, dso->failed_module);
    strcat(message, ")");
    ap_cpystrn(buffer, message, buflen);
    return buffer;
}

#endif
