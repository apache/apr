/* ====================================================================
 * Copyright (c) 1996-1999 The Apache Group.  All rights reserved.
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
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "apr_private.h"
#include "apr_buf.h"
#include <stdlib.h>

#ifndef DEFAULT_RWBUF_SIZE
#define DEFAULT_RWBUF_SIZE (4096)
#endif

APR_EXPORT(ap_bucket_rwmem *) ap_rwmem_create(void)
{
    ap_bucket_rwmem *newbuf;
    newbuf = malloc(sizeof(*newbuf));
    newbuf->alloc_addr = calloc(DEFAULT_RWBUF_SIZE, 1);
    newbuf->alloc_len  = DEFAULT_RWBUF_SIZE;
    newbuf->start      = newbuf->alloc_addr;
    newbuf->end        = newbuf->alloc_addr;
    return newbuf;
}

APR_EXPORT(void) ap_rwmem_destroy(void *e)
{
    ap_bucket_rwmem *d = (ap_bucket_rwmem *)e;
    free(d->alloc_addr);
    free(d);
}

APR_EXPORT(char *) ap_rwmem_get_char_str(ap_bucket_rwmem *b)
{
    return b->start;
}

APR_EXPORT(int) ap_rwmem_get_len(ap_bucket_rwmem *b)
{
    return b->end - b->start;
}

/*
 * save nbyte bytes to the bucket.
 * Only returns fewer than nbyte if an error ocurred.
 * Returns -1 if no bytes were written before the error ocurred.
 * It is worth noting that if an error occurs, the buffer is in an unknown
 * state.
 */
APR_EXPORT(int) ap_rwmem_write(ap_bucket_rwmem *b, const void *buf,
                               ap_size_t nbyte, ap_ssize_t *bytes_written)
{
    int amt;
    int total;

    if (nbyte == 0) {
        *bytes_written = 0;
        return APR_SUCCESS;
    }

/*
 * At this point, we need to make sure we aren't trying to write too much
 * data to the bucket.  We will need to write to the dist here, but I am
 * leaving that for a later pass.  The basics are presented below, but this
 * is horribly broken.
 */
    amt = b->alloc_len - (b->end - b->start);
    total = 0;
    if (nbyte > amt) {
        /* loop through and write to the disk */
        /* Replace the rwmem buckets with file buckets */
    }
    /* now we know that nbyte < b->alloc_len */
    memcpy(b->end, buf, nbyte);
    b->end += nbyte;
    *bytes_written = total + nbyte;
    return APR_SUCCESS;
}

APR_EXPORT(int) ap_rwmem_vputstrs(ap_bucket_rwmem *b, va_list va)
{
    int j, k;
    ap_ssize_t i;
    const char *x;
    int rv;

    for (k = 0;;) {
        x = va_arg(va, const char *);
        if (x == NULL)
            break;
        j = strlen(x);
        rv = ap_rwmem_write(b, x, j, &i);
        if (i != j) {
            /* Do we need better error reporting?  */
            return -1;
        }
        k += i;
    }

    return k;
}
