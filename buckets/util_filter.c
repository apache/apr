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
 *
 * Portions of this software are based upon public domain software
 * originally written at the National Center for Supercomputing Applications,
 * University of Illinois, Urbana-Champaign.
 */

#include "httpd.h"
#include "util_filter.h"

API_EXPORT(ap_filter_t *) ap_init_filter(ap_pool_t *p)
{
    ap_filter_t *f = ap_pcalloc(p, sizeof(f));
    return f;
}

API_EXPORT(int) ap_pass_brigade(request_rec *r, ap_filter_t *next, 
                               ap_bucket_brigade *buf)
{
    int rv;
    LINK_filter *hook;

    if (next->current_filter > r->filters->nelts) {
        return AP_ENOBODY_WROTE;
    }

    hook = (LINK_filter *)r->filters->elts;
    rv = hook[next->current_filter++].pFunc(r, next, buf);
    next->current_filter--;
    return rv;
}
    
API_EXPORT(void) ap_save_data_to_filter(request_rec *r, ap_filter_t *next,
                                        ap_bucket_brigade *data)
{
    LINK_filter *hook;

    hook = ((LINK_filter *)r->filters->elts);

    if (hook->filter_data) {
        ap_bucket_brigade_catenate(hook->filter_data, data);  
    }
    else {
        hook->filter_data = data;
    }
}

API_EXPORT(ap_bucket_brigade *) ap_get_saved_data(request_rec *r, 
                                       ap_filter_t *next,
                                       ap_bucket_brigade **data)
{
    LINK_filter *hook;

    hook = ((LINK_filter *)r->filters->elts);

    if (hook->filter_data) {
        ap_bucket_brigade_catenate(hook->filter_data, *data);
        *data = hook->filter_data;
    }
    hook->filter_data = NULL;
    return *data;
}

API_EXPORT(void) ap_hook_filter(HOOK_filter *pf, request_rec *r,
                                const char * const *aszPre,
                                const char * const *aszSucc, int nOrder)
{
    LINK_filter *hook;

    if(!r->filters) {
        r->filters=ap_make_array(ap_global_hook_pool,1,sizeof(LINK_filter));
        ap_hook_sort_register("filter",&r->filters);
    }

    hook = (LINK_filter *)r->filters->elts;

    hook=ap_push_array(r->filters);
    hook->pFunc=pf;
    hook->aszPredecessors=aszPre;
    hook->aszSuccessors=aszSucc;
    hook->nOrder=nOrder;
    hook->szName=ap_debug_module_name;
    hook->filter_data = ap_bucket_brigade_create(r->pool);
    if(ap_debug_module_hooks) { 
        ap_show_hook("filter",aszPre,aszSucc);
    }
}


