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

#ifndef APR_SHM_H
#define APR_SHM_H

/**
 * @file apr_shm.h
 * @brief APR Shared Memory Routines
 */

#include "apr.h"
#include "apr_pools.h"
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup apr_shm Shared Memory Routines
 * @ingroup APR 
 * @{
 */

/**
 * Private, platform-specific data struture representing a shared memory
 * segment.
 */
typedef struct apr_shm_t apr_shm_t;

/**
 * Create and make accessable a shared memory segment.
 * @param m The shared memory structure to create.
 * @param reqsize The desired size of the segment.
 * @param filename The file to use for shared memory on platforms that
 *        require it.
 * @param pool the pool from which to allocate the shared memory
 *        structure.
 * @remark A note about Anonymous vs. Named shared memory segments:
 *         Not all plaforms support anonymous shared memory segments, but in
 *         some cases it is prefered over other types of shared memory
 *         implementations. Passing a NULL 'file' parameter to this function
 *         will cause the subsystem to use anonymous shared memory segments.
 *         If such a system is not available, APR_ENOTIMPL is returned.
 * @remark A note about allocation sizes:
 *         On some platforms it is necessary to store some metainformation
 *         about the segment within the actual segment. In order to supply
 *         the caller with the requested size it may be necessary for the
 *         implementation to request a slightly greater segment length
 *         from the subsystem. In all cases, the apr_shm_baseaddr_get()
 *         function will return the first usable byte of memory.
 * 
 */
APR_DECLARE(apr_status_t) apr_shm_create(apr_shm_t **m,
                                         apr_size_t reqsize,
                                         const char *filename,
                                         apr_pool_t *pool);

/**
 * Destroy a shared memory segment and associated memory.
 * @param m The shared memory segment structure to destroy.
 */
APR_DECLARE(apr_status_t) apr_shm_destroy(apr_shm_t *m);

/**
 * Attach to a shared memory segment that was created
 * by another process.
 * @param m The shared memory structure to create.
 * @param filename The file used to create the original segment.
 *        (This MUST match the original filename.)
 * @param pool the pool from which to allocate the shared memory
 *        structure for this process.
 */
APR_DECLARE(apr_status_t) apr_shm_attach(apr_shm_t **m,
                                         const char *filename,
                                         apr_pool_t *pool);

/**
 * Detach from a shared memory segment without destroying it.
 * @param m The shared memory structure representing the segment
 *        to detach from.
 */
APR_DECLARE(apr_status_t) apr_shm_detach(apr_shm_t *m);

/**
 * Retrieve the base address of the shared memory segment.
 * NOTE: This address is only usable within the callers address
 * space, since this API does not guarantee that other attaching
 * processes will maintain the same address mapping.
 * @param m The shared memory segment from which to retrieve
 *        the base address.
 */
APR_DECLARE(void *) apr_shm_baseaddr_get(const apr_shm_t *m);

/**
 * Retrieve the length of a shared memory segment in bytes.
 * @param m The shared memory segment from which to retrieve
 *        the segment length.
 */
APR_DECLARE(apr_size_t) apr_shm_size_get(const apr_shm_t *m);

/**
 * Get the pool used by this shared memory segment.
 */
APR_POOL_DECLARE_ACCESSOR(shm);

/** @} */ 

#ifdef __cplusplus
}
#endif

#endif  /* APR_SHM_T */
