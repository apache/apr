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

#ifndef APACHE_FILTER_H
#define APACHE_FILTER_H

#include "ap_config.h"

/* For ap_array_header_t */
#include "apr_lib.h"
#include "httpd.h"
#include "apr_buf.h"
#include "ap_hooks.h"  /* For the hooks ordering stuff */

typedef struct ap_filter_t {
    int current_filter;
} ap_filter_t;

typedef int HOOK_filter(request_rec *r, ap_filter_t *next, ap_bucket_brigade *buckets);

typedef struct _LINK_filter {
    HOOK_filter *pFunc; 
    const char *szName; 
    const char * const *aszPredecessors; 
    const char * const *aszSuccessors; 
    int nOrder; 
    ap_bucket_brigade *filter_data;
} LINK_filter;

#define AP_HOOK_FILTER          0      /* content-filter/munger/processor */
#define AP_HOOK_ENCODING       10      /* content-encoding */
#define AP_HOOK_PROCESSOR      20      /* digest/message processor */
#define AP_HOOK_TRANSPORT      30      /* transport-encoding */

/* This is usually the core filter.  This ensures that there is always a 
 * filter that can/will write out to the network.  If some other module 
 * wants to actually do the writing, they just insert themselves before 
 * this filter.  This is just like the default handler in 1.3.  If no other
 * handler took care of the request, then the default took it.  Same thing, if
 * no other Transport filter writes this to the network, then the default
 * (core) filter will be used.
 */
#define AP_HOOK_TRANSPORT_LAST 40 

/* If we go past the end of the filter stack, we have a big problem. */ 
#define AP_ENOBODY_WROTE       (-1)


API_EXPORT(ap_filter_t *) ap_init_filter(ap_pool_t *p);

API_EXPORT(void) ap_hook_filter(HOOK_filter *pf, request_rec *r,
                                const char * const *aszPre, 
                                const char * const *aszSucc, int nOrder); 

API_EXPORT(int) ap_pass_brigade(request_rec *r, ap_filter_t *next, 
                               ap_bucket_brigade *bucket);
API_EXPORT(void) ap_save_data_to_filter(request_rec *r, ap_filter_t *next,
                                        ap_bucket_brigade *data);

API_EXPORT(ap_bucket_brigade *) ap_get_saved_data(request_rec *r, 
                                                  ap_filter_t *next,
                                                  ap_bucket_brigade **data);

#endif /* ndef(AP_FILTER_H) */
