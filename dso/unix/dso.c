/* ===================================================================
 * Copyright (c) 1996-2000 The Apache Software Foundation.  All rights reserved.
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
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ===================================================================+ *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "dso.h"

/* ***APRDOC********************************************************
 * ap_status_t ap_dso_init(void)
 *    Initialize the underlying DSO library.
 */
ap_status_t ap_dso_init(void){
    if(lt_dlinit())
        return APR_EINIT;

    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_dso_load(ap_dso_handle_t **res_handle, const char *path,
 *                         ap_context_t *ctx)
 *    Load a DSO library.
 * arg 1) Location to store new handle for the DSO.
 * arg 2) Path to the DSO library
 * arg 3) Context to use. 
 */
ap_status_t ap_dso_load(struct dso_handle_t **res_handle, const char *path, 
                        ap_context_t *ctx)
{
    lt_dlhandle dlhandle;

    if((dlhandle = lt_dlopen(path)) == NULL)
        return APR_EDSOOPEN;

    *res_handle = ap_pcalloc(ctx, sizeof(*res_handle));
    (*res_handle)->handle = dlhandle;
    (*res_handle)->cont = ctx;
    return APR_SUCCESS;
}
    
/* ***APRDOC********************************************************
 * ap_status_t ap_dso_unload(ap_dso_handle_t *handle)
 *    Close a DSO library.
 * arg 1) handle to close.
 */
ap_status_t ap_dso_unload(struct dso_handle_t *handle)
{
    if(lt_dlclose(handle->handle))
        return APR_EINIT;

    return APR_SUCCESS;
}

/* ***APRDOC********************************************************
 * ap_status_t ap_dso_sym(ap_dso_handle_sym_t *ressym, ap_dso_handle_t *handle
 *                        const char *symname)
 *    Load a symbol from a DSO handle.
 * arg 1) Location to store the loaded symbol
 * arg 2) handle to load from.
 * arg 3) Name of the symbol to load.
 */
ap_status_t ap_dso_sym(ap_dso_handle_sym_t *ressym, 
                       struct dso_handle_t *handle, 
                       const char *symname)
{
    lt_ptr_t sym;

    if (ressym == NULL) {
        return APR_ENOFUNCPOINTER;
    }
    if (handle == NULL) {
        return APR_ENODSOHANDLE;
    }

    if((sym = lt_dlsym(handle->handle, symname)) == NULL) {
        return APR_EFUNCNOTFOUND;
    }

    *ressym = sym;
    return APR_SUCCESS;
}
