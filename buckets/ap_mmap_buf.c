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

static const char * mmap_get_str(ap_bucket *e)
{
    ap_bucket_mmap *b = (ap_bucket_mmap *)e->data;
    return b->data->mm;
}

static int mmap_get_len(ap_bucket *e)
{
    ap_bucket_mmap *b = (ap_bucket_mmap *)e->data;
    return b->len;
}

static ap_status_t mmap_bucket_insert(ap_bucket *e, const void *buf, 
                                      ap_size_t nbytes, ap_ssize_t *w)
{
    ap_bucket_mmap *b = (ap_bucket_mmap *)e->data;
    const ap_mmap_t *mm = buf;

    b->data = mm;
    b->len = nbytes;
    *w = nbytes;
    return APR_SUCCESS;
}
    

APR_EXPORT(ap_bucket *) ap_mmap_bucket_create(void)
{
    ap_bucket *newbuf;
    ap_bucket_mmap *b;

    newbuf            = calloc(1, sizeof(*newbuf));
    b                 = malloc(sizeof(*b));

    b->data      = NULL;
    b->len       = 0;

    newbuf->color     = AP_BUCKET_mmap;
    newbuf->getstr    = mmap_get_str;
    newbuf->getlen    = mmap_get_len;
    newbuf->insert    = mmap_bucket_insert;
    newbuf->free      = NULL;
    newbuf->data      = b;
    
    return newbuf;
}

