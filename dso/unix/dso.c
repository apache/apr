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
#include "ltdl.h"

/*
 * ap_dso_init:  Initialize the underlying DSO library
 *
 * Arguments:    errstr = Location to store error result on failure. 
 *
 * Return values:  Returns APR_SUCCESS (on success), and APR_EINIT on failure.
 *                 on failure, *errstr is valid.
 */

ap_status_t ap_dso_init(void){
    if(lt_dlinit())
        return APR_EINIT;

    return APR_SUCCESS;
}

/*
 * ap_dso_load:  Load a DSO library.
 *
 * Arguments:    path       = Path to the DSO library
 *               ctx        = Context to use in allocation of the handle.
 *               res_handle = Location to store new handle for the DSO
 *               errstr     = Location to store error string on failure.
 *
 * Return values:  Returns APR_SUCCESS on success, else APR_EINIT
 */

ap_status_t ap_dso_load(const char *path, ap_context_t *ctx,
			ap_dso_handle_t **res_handle)
{
    lt_dlhandle dlhandle;

    if((dlhandle = lt_dlopen(path)) == NULL)
        return APR_EINIT;

    *res_handle = ap_pcalloc(ctx, sizeof(*res_handle));
    (*res_handle)->handle = dlhandle;
    (*res_handle)->cont = ctx;
    return APR_SUCCESS;
}
    
/*
 * ap_dso_unload:  Unload a DSO library.  
 *
 * Arguments:      handle = Handle to unload
 *                 errstr = Location to store error string on failure.
 *
 * Return values:  Returns APR_SUCCESS on success, else APR_EINIT
 */

ap_status_t ap_dso_unload(ap_dso_handle_t *handle)
{
    if(lt_dlclose(handle->handle))
        return APR_EINIT;

    return APR_SUCCESS;
}

/*
 * ap_dso_sym:  Load a symbol from a DSO handle.
 *
 * Arguments:   handle  = DSO handle to load from
 *              symname = Symbol name to load
 *              ressym  = Location to store pointer to the symbol
 *              errstr  = Location to store error string on failure.
 *
 * Return values:  Returns APR_SUCCESS on success, else APR_EINIT
 */

ap_status_t ap_dso_sym(ap_dso_handle_t *handle, const char *symname,
		       ap_dso_handle_sym_t *ressym)
{
    lt_ptr_t sym;

    if (symname == NULL || ressym == NULL)
        return APR_EINIT;

    if((sym = lt_dlsym(handle->handle, symname)) == NULL)
        return APR_EINIT;

    *ressym = sym;
    return APR_SUCCESS;
}
