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

static const char * rwmem_get_str(ap_bucket *e)
{
    ap_bucket_rwmem *b = (ap_bucket_rwmem *)e->data;
    return b->start;
}

static int rwmem_get_len(ap_bucket *e)
{
    ap_bucket_rwmem *b = (ap_bucket_rwmem *)e->data;
    return (char *)b->end - (char *)b->start;
}

static void rwmem_destroy(void *e)
{
    ap_bucket_rwmem *d = (ap_bucket_rwmem *)e;
    free(d->alloc_addr);
}

static ap_status_t rwmem_split(ap_bucket *e, ap_size_t nbyte)
{
    ap_bucket *newbuck;
    ap_bucket_rwmem *a = (ap_bucket_rwmem *)e;
    ap_bucket_rwmem *b;

    newbuck = ap_bucket_rwmem_create();
    b = (ap_bucket_rwmem *)newbuck;

    b->alloc_addr = a->alloc_addr;
    b->alloc_len = a->alloc_len;
    b->end = a->end;
    a->end = a->start + nbyte;
    b->start = a->end + 1;

    newbuck->prev = e;
    newbuck->next = e->next;
    e->next = newbuck;

    return APR_SUCCESS;
}

/*
 * save nbyte bytes to the bucket.
 * Only returns fewer than nbyte if an error occurred.
 * Returns -1 if no bytes were written before the error occurred.
 * It is worth noting that if an error occurs, the buffer is in an unknown
 * state.
 */
static ap_status_t rwmem_insert(ap_bucket *e, const void *buf,
                                ap_size_t nbyte, ap_ssize_t *w)
{
    int amt;
    int total;
    ap_bucket_rwmem *b = (ap_bucket_rwmem *)e->data;

    if (nbyte == 0) {
        *w = 0;
        return APR_SUCCESS;
    }

/*
 * At this point, we need to make sure we aren't trying to write too much
 * data to the bucket.  We will need to write to the dist here, but I am
 * leaving that for a later pass.  The basics are presented below, but this
 * is horribly broken.
 */
    amt = b->alloc_len - ((char *)b->end - (char *)b->start);
    total = 0;
    if (nbyte > amt) {
        /* loop through and write to the disk */
        /* Replace the rwmem buckets with file buckets */
    }
    /* now we know that nbyte < b->alloc_len */
    memcpy(b->end, buf, nbyte);
    b->end = (char *)b->end + nbyte;
    *w = total + nbyte;
    return APR_SUCCESS;
}

APR_EXPORT(ap_bucket *) ap_bucket_rwmem_create(void)
{
    ap_bucket *newbuf;
    ap_bucket_rwmem *b;

    newbuf = calloc(1, sizeof(*newbuf));
    b = malloc(sizeof(*b));
    
    b->alloc_addr = calloc(DEFAULT_RWBUF_SIZE, 1);
    b->alloc_len  = DEFAULT_RWBUF_SIZE;
    b->start      = b->alloc_addr;
    b->end        = b->alloc_addr;

    newbuf->color      = AP_BUCKET_rwmem;
    newbuf->getstr     = rwmem_get_str;
    newbuf->getlen     = rwmem_get_len;
    newbuf->insert     = rwmem_insert;
    newbuf->split      = rwmem_split;
    newbuf->free       = rwmem_destroy;
    newbuf->data       = b;

    return newbuf;
}

