/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#ifndef APR_SHMEM_H
#define APR_SHMEM_H

#include "apr.h"
#include "apr_general.h"
#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if APR_USES_ANONYMOUS_SHM
typedef   void           ap_shm_name_t;
#elif APR_USES_FILEBASED_SHM
typedef   char *         ap_shm_name_t;
#elif APR_USES_KEYBASED_SHM
typedef   key_t          ap_shm_name_t;
#endif

typedef   struct shmem_t ap_shmem_t;

ap_status_t ap_shm_init(ap_shmem_t **m, ap_size_t reqsize, const char *file, ap_context_t *cont);
ap_status_t ap_shm_destroy(ap_shmem_t *m);
void *ap_shm_malloc(ap_shmem_t *c, ap_size_t reqsize);
void *ap_shm_calloc(ap_shmem_t *shared, ap_size_t size);
ap_status_t ap_shm_free(ap_shmem_t *shared, void *free);
ap_status_t ap_get_shm_name(ap_shmem_t *c, ap_shm_name_t **name);
ap_status_t ap_set_shm_name(ap_shmem_t *c, ap_shm_name_t *name);
ap_status_t ap_open_shmem(ap_shmem_t *c);
ap_status_t ap_shm_avail(ap_shmem_t *c, ap_size_t *avail);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_FILE_IO_H */


