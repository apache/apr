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

#ifndef APR_MMAP_H
#define APR_MMAP_H

#include "apr_general.h"
#include "apr_errno.h"
#include "apr_network_io.h"
#include "apr_portable.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct ap_mmap_t            ap_mmap_t;

/*   Function definitions */

/* 

=head1 ap_status_t ap_mmap_create(ap_mmap_t **new, ap_file_t *file, ap_off_t offset)

B<Create a new mmap'ed file out of an existing APR file.>

    arg 1) The newly created mmap'ed file.
    arg 2) The file turn into an mmap.
    arg 3) The offset into the file to start the data pointer at.
    arg 4) The size of the file
    arg 5) The pool to use when creating the mmap.

=cut
 */
ap_status_t ap_mmap_create(ap_mmap_t ** newmmap, ap_file_t *file, ap_off_t offset,
                 ap_size_t size, ap_pool_t *cntxt);

/* 

=head1 ap_status_t ap_mmap_delete(ap_mmap_t *mmap)

B<Remove a mmap'ed.>

    arg 1) The mmap'ed file.

=cut
 */
ap_status_t ap_mmap_delete(ap_mmap_t *mmap);

/* 

=head1 ap_status_t ap_mmap_offset(void **addr, ap_mmap_t *mmap, ap_offset_t offset)

B<Move the pointer into the mmap'ed file to the specified offset.>

    arg 1) The pointer to the offset specified.
    arg 2) The mmap'ed file.
    arg 3) The offset to move to.

=cut
 */
ap_status_t ap_mmap_offset(void **addr, ap_mmap_t *mmap, ap_off_t offset);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_MMAP_H */


