/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2001 The Apache Software Foundation.  All rights
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

#include "apr.h"
#include "apr_pools.h" /* includes apr_sms.h" */
#include "apr_sms_trivial.h"
#include "apr_errno.h"
#include "apr_lock.h"
#include "apr_portable.h"
#include "apr_lib.h" /* for apr_vformatter */

#include "sms_private.h"

static int initialized = 0;
static apr_pool_t *permanent_pool = NULL;

APR_DECLARE(apr_status_t) apr_pool_create(apr_pool_t **newpool, apr_pool_t *p)
{
    if (!initialized)
        /* Hmm, if we are given a parent here, is this correct?
         * It should never happen, so we're probably OK....
         */
        return apr_sms_std_create(newpool);

    return apr_sms_trivial_create_ex(newpool, p ? p : permanent_pool,
                                     0x2000, 0, 0x80000);
}
    
APR_DECLARE(void) apr_pool_sub_make(apr_pool_t **p,
                                    apr_pool_t *pparent,
                                    apr_abortfunc_t abort)
{
    if (apr_sms_trivial_create(p, pparent) != APR_SUCCESS)
        return NULL;

    apr_sms_set_abort(abort, *p);

    return p;
}

APR_DECLARE(void) apr_pool_cleanup_register(apr_pool_t *pool,
                                            const void *data,
                                         apr_status_t (*plain_cleanup)(void*),
                                         apr_status_t (*child_cleanup)(void*))
{
    if (plain_cleanup == child_cleanup) {
        /* we only need to register one as an ALL_CLEANUP */
        apr_sms_cleanup_register(pool, APR_ALL_CLEANUPS, data, plain_cleanup);
        return;
    }
    if (plain_cleanup)
        apr_sms_cleanup_register(pool, APR_GENERAL_CLEANUP, data,
                                        plain_cleanup);

    if (child_cleanup)
        apr_sms_cleanup_register(pool, APR_CHILD_CLEANUP, data,
                                        child_cleanup);
}

APR_DECLARE(void) apr_pool_cleanup_for_exec(void)
{
#if !defined(WIN32) && !defined(OS2)
    /* See note in apr_pools.c for why we do this :) */
    apr_sms_cleanup_run_type(permanent_pool, APR_CHILD_CLEANUP);
#endif
}

APR_DECLARE(apr_status_t) apr_pool_alloc_init(apr_pool_t *gp)
{
    apr_status_t rv;
    
    if ((rv = apr_sms_trivial_create_ex(&permanent_pool, gp,
                                        0x2000, 0, 0x80000)) != APR_SUCCESS)
        return rv;

    initialized = 1;

    return APR_SUCCESS;
}

APR_DECLARE(void) apr_pool_alloc_term(apr_pool_t *gp)
{
    if (initialized)
        apr_sms_destroy(permanent_pool);

    initialized = 0;
}

APR_DECLARE(int) apr_pool_is_ancestor(apr_pool_t *a, apr_pool_t *b)
{
    while (b && b != a)
        b = b->parent;

    return b == a;
}

/* This stuff needs to be reviewed, but here it is :) */

struct psprintf_data {
    apr_vformatter_buff_t  vbuff;
    char *base;
    apr_sms_t *sms;
};

static int psprintf_flush(apr_vformatter_buff_t *vbuff)
{
    struct psprintf_data *ps = (struct psprintf_data*)vbuff;
    apr_size_t size;
    char *ptr;

    size = (char*) ps->vbuff.curpos - ps->base;
    ptr = apr_sms_realloc(ps->sms, ps->base, 2*size);
    if (ptr == NULL) {
        fputs("[psprintf_flush] Ouch!  Out of memory!\n", stderr);
        exit(1);
    }
    ps->base = ptr;
    ps->vbuff.curpos = ptr + size;
    ps->vbuff.endpos = ptr + 2 * size - 1;
    return 0;
}

APR_DECLARE(char *) apr_pvsprintf(apr_pool_t *p, const char *fmt, va_list ap)
{
    struct psprintf_data ps;
    void *ptr;

    ps.sms = (apr_sms_t*)p;
    ps.base = apr_sms_malloc(ps.sms, 512);
    if (ps.base == NULL) {
        fputs("[apr_pvsprintf] Ouch! Out of memory!\n", stderr);
        exit(1);
    }
    ps.vbuff.curpos = ps.base;
    ps.vbuff.endpos = ps.base + 511;
    apr_vformatter(psprintf_flush, &ps.vbuff, fmt, ap);
    *ps.vbuff.curpos++ = '\0';
    ptr = ps.base;
    ptr = apr_sms_realloc(ps.sms, ptr, (char*)ps.vbuff.curpos - (char*)ptr);
    if (ptr == NULL) {
        fputs("[apr_pvsprintf #2] Ouch! Out of memory!\n", stderr);
        exit(1);
    }
    return (char*)ptr;
}

APR_DECLARE_NONSTD(char*) apr_psprintf(apr_pool_t *p, const char *fmt, ...)
{
    va_list ap;
    char *res;

    va_start(ap,fmt);
    res = apr_pvsprintf(p, fmt, ap);
    va_end(ap);
    return res;
}

APR_DECLARE(void) apr_pool_note_subprocess(apr_pool_t *a, apr_proc_t *pid,
                                           enum kill_conditions how)
{
    struct process_chain *newpc = (struct process_chain*)
        apr_sms_malloc(a, sizeof(struct process_chain));

    newpc->pid       = pid;
    newpc->kill_how  = how;
    newpc->next      = a -> subprocesses;
    a->subprocesses  = newpc;
}

