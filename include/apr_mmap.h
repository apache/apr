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

#ifndef APR_MMAP_H
#define APR_MMAP_H

#include "apr_general.h"
#include "apr_errno.h"
#include "apr_network_io.h"
#include "apr_portable.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct mmap_t            ap_mmap_t;

/*   Function definitions */

/* creation */
ap_status_t ap_mmap_create(ap_mmap_t ** newmmap, const char *fname, ap_context_t *cntxt);
ap_status_t ap_mmap_open_create(ap_mmap_t **newmmap, ap_file_t *file, ap_context_t *cntxt);
ap_status_t ap_mmap_size_create(ap_mmap_t **newmmap, ap_file_t *file, ap_size_t size,
                                ap_context_t *cntxt);

/* destruction */
ap_status_t ap_mmap_delete(ap_mmap_t *mmap);

/* These are designed to be used in qsort, bsearch etc */
ap_int32_t ap_mmap_inode_compare(const void *m1, const void *m2);
ap_int32_t ap_mmap_filename_compare(const void *m1, const void *m2);

ap_status_t ap_mmap_offset(void **addr, ap_mmap_t *mmap, ap_size_t offset);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_MMAP_H */


