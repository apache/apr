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

#ifndef APR_DSO_DOT_H
#define APR_DSO_DOT_H

/**
 * @file apr_dso.h
 * @brief APR Dynamic Object Handling Routines
 */

#include "apr.h"
#include "apr_pools.h"
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup apr_dso Dynamic Object Handling
 * @ingroup APR 
 * @{
 */

#if APR_HAS_DSO || defined(DOXYGEN)

/**
 * Structure for referencing dynamic objects
 */
typedef struct apr_dso_handle_t       apr_dso_handle_t;

/**
 * Structure for referencing symbols from dynamic objects
 */
typedef void *                        apr_dso_handle_sym_t;

/**
 * Load a DSO library.
 * @param res_handle Location to store new handle for the DSO.
 * @param path Path to the DSO library
 * @param ctx Pool to use.
 * @bug We aught to provide an alternative to RTLD_GLOBAL, which
 * is the only supported method of loading DSOs today.
 */
APR_DECLARE(apr_status_t) apr_dso_load(apr_dso_handle_t **res_handle, 
                                       const char *path, apr_pool_t *ctx);

/**
 * Close a DSO library.
 * @param handle handle to close.
 */
APR_DECLARE(apr_status_t) apr_dso_unload(apr_dso_handle_t *handle);

/**
 * Load a symbol from a DSO handle.
 * @param ressym Location to store the loaded symbol
 * @param handle handle to load the symbol from.
 * @param symname Name of the symbol to load.
 */
APR_DECLARE(apr_status_t) apr_dso_sym(apr_dso_handle_sym_t *ressym, 
                                      apr_dso_handle_t *handle,
                                      const char *symname);

/**
 * Report more information when a DSO function fails.
 * @param dso The dso handle that has been opened
 * @param buf Location to store the dso error
 * @param bufsize The size of the provided buffer
 */
APR_DECLARE(const char *) apr_dso_error(apr_dso_handle_t *dso, char *buf, apr_size_t bufsize);

#endif /* APR_HAS_DSO */

/** @} */

#ifdef __cplusplus
}
#endif

#endif
