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

/*

=head1 ap_status_t ap_shm_init(ap_shmem_t *m, ap_size_t reqsize, char *file)

B<Create a pool of shared memory for use later.>

    arg 1) The shared memory block.
    arg 2) The size of the shared memory pool.
    arg 3) The file to use for the shared memory on platforms that
           require it.
    arg 4) The pool to use

=cut
 */
ap_status_t ap_shm_init(ap_shmem_t **m, ap_size_t reqsize, const char *file, ap_pool_t *cont);

/*

=head1 ap_status_t ap_shm_destroy(ap_shmem_t *m)

B<Destroy the shared memory block.>

    arg 1) The shared memory block to destroy. 

=cut
 */
ap_status_t ap_shm_destroy(ap_shmem_t *m);

/*

=head1 ap_status_t ap_shm_malloc(ap_shmem_t *c, ap_size_t reqsize)

B<allocate memory from the block of shared memory.>

    arg 1) The shared memory block to destroy. 
    arg 2) How much memory to allocate

=cut
 */
void *ap_shm_malloc(ap_shmem_t *c, ap_size_t reqsize);

/*

=head1 void *ap_shm_calloc(ap_shmem_t *shared, ap_size_t size)

B<allocate memory from the block of shared memory and initialize it to zero.>

    arg 1) The shared memory block to destroy. 
    arg 2) How much memory to allocate

=cut
 */
void *ap_shm_calloc(ap_shmem_t *shared, ap_size_t size);

/*

=head1 ap_status_t ap_shm_free(ap_shmem_t *shared, void *entity)

B<free shared memory previously allocated.>

    arg 1) The shared memory block to destroy. 

=cut
 */
ap_status_t ap_shm_free(ap_shmem_t *shared, void *free);

/*

=head1 ap_status_t ap_get_shm_name(ap_shmem_t *c, ap_shm_name_t **name)

B<Get the name of the shared memory segment if not using anonymous shared memory.>

    arg 1)  The shared memory block to destroy. 
    arg 2)  The name of the shared memory block, NULL if anonymous 
            shared memory.
   return) APR_USES_ANONYMOUS_SHM if we are using anonymous shared
           memory.  APR_USES_FILEBASED_SHM if our shared memory is
           based on file access.  APR_USES_KEYBASED_SHM if shared
           memory is based on a key value such as shmctl.  If the
           shared memory is anonymous, the name is NULL.

=cut
 */
ap_status_t ap_get_shm_name(ap_shmem_t *c, ap_shm_name_t **name);

/*

=head1 ap_status_t ap_set_shm_name(ap_shmem_t *c, ap_shm_name_t *name)

B<Set the name of the shared memory segment if not using
anonymous shared memory.>  This is to allow processes to open
shared memory created by another process.

    arg 1)  The shared memory block to destroy. 
    arg 2)  The name of the shared memory block, NULL if anonymous 
            shared memory.
   return) APR_USES_ANONYMOUS_SHM if we are using anonymous shared
           memory.  APR_SUCCESS if we are using named shared memory
           and we were able to assign the name correctly. 

=cut
 */
ap_status_t ap_set_shm_name(ap_shmem_t *c, ap_shm_name_t *name);

/*

=head1 ap_status_t ap_open_shmem(ap_shmem_t *c)

B<Open the shared memory block in a child process.> 

    arg 1)  The shared memory block to open in the child. 
   return) This should be called after ap_set_shm_name.  The ap_shmem_t 
           variable must refer to the memory segment to open.

=cut
 */
ap_status_t ap_open_shmem(ap_shmem_t *c);

/*

=head1 ap_status_t ap_shm_avail(ap_shmem_t *c, ap_size_t *size)

B<Determine how much memory is available in the specified shared memory block>

    arg 1)  The shared memory block to open in the child. 
    arg 2)  The amount of space available in the shared memory block.

=cut
 */
ap_status_t ap_shm_avail(ap_shmem_t *c, ap_size_t *avail);

#ifdef __cplusplus
}
#endif

#endif  /* ! APR_FILE_IO_H */


