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
#include "apr_pools.h"
#include "apr_lib.h"
#include "apr_errno.h"
#include <stdlib.h>
#include <sys/uio.h>
#include "apr_buf.h"

APR_EXPORT(ap_status_t) ap_bucket_destroy(ap_bucket *e)
{
    if (e->free) {
        e->free(e);
    }
    free(e);
    return APR_SUCCESS;
}

static ap_status_t ap_bucket_list_destroy(ap_bucket *e)
{
    ap_bucket *cur = e;
    ap_bucket *next;

    while (cur) {
        next = cur->next;
        ap_bucket_destroy(cur);
        cur = next;
    }
    return APR_SUCCESS;
}

APR_EXPORT(ap_status_t) ap_brigade_destroy(void *data)
{
    ap_bucket_brigade *b = data;

    ap_bucket_list_destroy(b->head);
    /* The brigade itself is allocated out of a pool, so we don't actually 
     * want to free it.  If we did, we would do that free() here.
     */

    return APR_SUCCESS;
}

APR_EXPORT(ap_bucket_brigade *) ap_brigade_create(ap_pool_t *p)
{
    ap_bucket_brigade *b;

    b = ap_palloc(p, sizeof(*b));
    b->p = p;
    b->head = b->tail = NULL;

    ap_register_cleanup(b->p, b, ap_brigade_destroy, 
                        ap_brigade_destroy);
    return b;
}

APR_EXPORT(void) ap_brigade_append_buckets(ap_bucket_brigade *b, 
                                                  ap_bucket *e)
{
    ap_bucket *cur = e;

    if (b->tail) {
        b->tail->next = e;
        e->prev = b->tail;
        while (cur->next) {
           cur = cur->next;
        }
        b->tail = cur;
    }
    else {
        b->head = b->tail = e;
    }
}

APR_EXPORT(int) ap_brigade_to_iovec(ap_bucket_brigade *b, 
                                           struct iovec *vec, int nvec)
{
    ap_bucket *e;
    struct iovec *orig;

    orig = vec;
    e = b->head;
    while (e && nvec) {
	vec->iov_len = ap_get_bucket_len(e);
	vec->iov_base = (void *)e->read(e);
	e = e->next;
	--nvec;
	++vec;
    }
    return vec - orig;
}

APR_EXPORT(void) ap_brigade_catenate(ap_bucket_brigade *a, 
                                            ap_bucket_brigade *b)
{
    if (b->head) {
        if (a->tail) {
            a->tail->next = b->head;
        }
        b->head->prev = a->tail;
	a->tail = b->tail;
        if (!a->head) {
            a->head = b->head;
        }
	b->head = NULL;
	b->tail = b->head;
    }
}

APR_EXPORT(void) ap_consume_buckets(ap_bucket_brigade *b, int nvec)
{
    int i;   

    for (i=0; i < nvec; i++) {
        if (b->head == b->tail) {
            ap_bucket_destroy(b->head);
            b->head = b->tail = NULL;
            break;
        }
        b->head = b->head->next;
        ap_bucket_destroy(b->head->prev);
        b->head->prev = NULL;
    }
}

APR_EXPORT(ap_status_t) ap_brigade_to_iol(ap_ssize_t *total_bytes,
                                                 ap_bucket_brigade *b, 
                                                 ap_iol *iol)
{
    ap_status_t status;
    int iov_used;
    struct iovec vec[16];   /* seems like a reasonable number to me */
    ap_ssize_t bytes = 0;

    *total_bytes = 0;
    do {
        iov_used = ap_brigade_to_iovec(b, vec, 16);
        status = iol_writev(iol, vec, iov_used, &bytes);

        ap_consume_buckets(b, 16);

        if (status != APR_SUCCESS) {
            return status;
        }
        *total_bytes += bytes;
    } while (iov_used == 16);
    return APR_SUCCESS;
}

APR_EXPORT(int) ap_get_bucket_len(ap_bucket *b)
{
    if (b) {
        return b->getlen(b);
    }
    return 0;
}    

APR_EXPORT(int) ap_brigade_vputstrs(ap_bucket_brigade *b, va_list va)
{
    ap_bucket *r;
    const char *x;
    int j, k, rv;
    ap_ssize_t i;

    if (b->tail && b->tail->color == AP_BUCKET_rwmem) {
        ap_bucket *rw;
        rw = b->tail;
        /* I have no idea if this is a good idea or not.  Probably not.
         * Basically, if the last bucket in the list is a rwmem bucket,
         * then we just add to it instead of allocating a new read only
         * bucket.  This is incredibly easy to take out if it is a bad 
         * idea.  RBB
         */
        for (k = 0;;) {
            x = va_arg(va, const char *);
            if (x == NULL)
                break;
            j = strlen(x);
           
            rv = rw->write(rw, x, j, &i);
            if (i != j) {
                /* Do we need better error reporting?  */
                return -1;
            }
            k += i;

            ap_brigade_append_buckets(b, rw);
        }        
    }
    
    for (k = 0;;) {
        x = va_arg(va, const char *);
        if (x == NULL)
            break;
        j = strlen(x);
       
        r = ap_bucket_rwmem_create(x, j, &i);
        if (i != j) {
            /* Do we need better error reporting?  */
            return -1;
        }
        k += i;

        ap_brigade_append_buckets(b, r);
    }

    return k;
}

APR_EXPORT(int) ap_brigade_printf(ap_bucket_brigade *b, const char *fmt, ...)
{
    va_list ap;
    int res;

    va_start(ap, fmt);
    res = ap_brigade_vprintf(b, fmt, ap);
    va_end(ap);
    return res;
}

APR_EXPORT(int) ap_brigade_vprintf(ap_bucket_brigade *b, const char *fmt, va_list va)
{
    /* THIS IS A HACK.  This needs to be replaced with a function to printf
     * directly into a bucket.  I'm being lazy right now.  RBB
     */
    char buf[4096];
    ap_bucket *r;
    int res, i;

    res = ap_vsnprintf(buf, 4096, fmt, va);

    r = ap_bucket_rwmem_create(buf, strlen(buf), &i);
    ap_brigade_append_buckets(b, r);

    return res;
}
