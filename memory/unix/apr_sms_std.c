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

/* This code kindly donated to APR by 
 *    Elrond  <elrond@samba-tng.org>
 *    Luke Kenneth Casson Leighton <lkcl@samba-tng.org>
 *    Sander Striker <striker@samba-tng.org>
 *
 * May 2001
 */

#include "apr.h"
#include "apr_private.h"
#include "apr_sms.h"
#include <stdlib.h>

static const char *module_identity = "STANDARD";

/*
 * standard memory system
 */

static void *apr_sms_std_malloc(apr_sms_t *sms,
                                apr_size_t size)
{
    return malloc(size);
}

static void *apr_sms_std_calloc(apr_sms_t *sms,
                                apr_size_t size)
{
#if HAVE_CALLOC
    return calloc(1, size);
#else
    void *mem;
    mem = malloc(size);
    memset(mem, '\0', size);
    return mem;
#endif
}


static void *apr_sms_std_realloc(apr_sms_t *sms,
                                 void *mem, apr_size_t size)
{
    return realloc(mem, size);
}

static apr_status_t apr_sms_std_free(apr_sms_t *sms,
                                     void *mem)
{
    free(mem);
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_sms_std_create(apr_sms_t **sms)
{
    apr_sms_t *new_sms;
    apr_status_t rv;

    *sms = NULL;
    /* We don't have a parent so we allocate the memory
     * for the structure ourselves...
     */
    new_sms = apr_sms_std_calloc(NULL, sizeof(apr_sms_t));

    if (!new_sms)
        return APR_ENOMEM;

    if ((rv = apr_sms_init(new_sms, NULL)) != APR_SUCCESS)
        return rv;

    new_sms->malloc_fn  = apr_sms_std_malloc;
    new_sms->calloc_fn  = apr_sms_std_calloc;
    new_sms->realloc_fn = apr_sms_std_realloc;
    new_sms->free_fn    = apr_sms_std_free;
    new_sms->identity   = module_identity;

    /* as we're not a tracking memory module, i.e. we don't keep
     * track of our allocations, we don't have apr_sms_reset or
     * apr_sms_destroy functions.
     */
    
    apr_sms_assert(new_sms);

    *sms = new_sms;
    return APR_SUCCESS;
}

