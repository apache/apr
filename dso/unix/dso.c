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

/* ***APRDOC********************************************************
 * ap_status_t ap_dso_init(void)
 *    Initialize the underlying DSO library.
 */
ap_status_t ap_dso_init(void){
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
#if defined(HPUX) || defined(HPUX10) || defined(HPUX11)
    shl_t os_handle = shl_load(path, BIND_IMMEDIATE|BIND_VERBOSE|BIND_NOSTART, 0L);
#elif defined(OSF1) || defined(SEQUENT) ||\
    (defined(__FreeBSD_version) && (__FreeBSD_version >= 220000))
    void *os_handle = dlopen((char *)path, RTLD_NOW | RTLD_GLOBAL);
#else
    void *os_handle = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
#endif    

    if(os_handle == NULL)
        return APR_EDSOOPEN;

fprintf(stderr,"handle is %Lx\n",os_handle);

    *res_handle = ap_pcalloc(ctx, sizeof(*res_handle));
    (*res_handle)->handle = (void*)os_handle;
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
#if defined(HPUX) || defined(HPUX10) || defined(HPUX11)
    shl_unload((shl_t)handle);
#else
    if (dlclose(handle) != 0)
        return APR_EINIT;
#endif

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
#if defined(HPUX) || defined(HPUX10) || defined(HPUX11)
    void *symaddr = NULL;
    int status;

    errno = 0;
    status = shl_findsym((shl_t *)&handle->handle, symname, TYPE_PROCEDURE, &symaddr);
    if (status == -1 && errno == 0) /* try TYPE_DATA instead */
        status = shl_findsym((shl_t *)&handle->handle, symname, TYPE_DATA, &symaddr);
    if (status = -1)
        return APR_EINIT;
    ressym = symaddr;


#elif defined(DLSYM_NEEDS_UNDERSCORE)
    char *symbol = (char*)malloc(sizeof(char)*(strlen(symname)+2));
    void *retval;
    sprintf(symbol, "_%s", symname);
    retval = dlsym(handle->handle, symbol);
    free(symbol);

#elif defined(SEQUENT)
    void *retval = dlsym(handle->handle, (char *)symname);
#else
    void *retval = dlsym(handle->handle, symname);
#endif

    if (retval == NULL)
        return APR_EINIT;
    
    ressym = retval;
    
    return APR_SUCCESS;
}
