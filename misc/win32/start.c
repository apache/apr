/* ====================================================================
 * Copyright (c) 1999 The Apache Group.  All rights reserved.
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
 * individuals on behalf of the Apache Group.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

#include "apr_win.h"
#include "misc.h"
#include "apr_general.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_lib.h"
#include <string.h>
#include <process.h>

ap_status_t clean_cont(void *data)
{
    WSACleanup();
    return APR_SUCCESS;
}
    

ap_status_t ap_create_context(ap_context_t *cont, void *data, ap_context_t **newcont)
{
    int iVersionRequested;
    WSADATA wsaData;
    int err;
    ap_context_t *new;
    ap_pool_t *pool;

    if (cont) {
        pool = ap_make_sub_pool(cont->pool);
    }
    else {
        pool = ap_init_alloc();;
    }
        
    if (pool == NULL) {
        return APR_ENOPOOL;
    }
    
	if (cont) {
		new = (ap_context_t *)ap_palloc(cont, sizeof(ap_context_t));
	}
	else {
		new = (ap_context_t *)malloc(sizeof(ap_context_t));
	}
    new->pool = pool;
    if (data == NULL && cont) {
        new->prog_data = cont->prog_data;
    }
    else {
        new->prog_data = data;
    }
    if (cont) { 
        new->signal_safe = cont->signal_safe;
        new->cancel_safe = cont->cancel_safe;
    }
    else {
        new->signal_safe = 0;
        new->cancel_safe = 0;
    }

    iVersionRequested = MAKEWORD(WSAHighByte, WSALowByte);
    err = WSAStartup((WORD) iVersionRequested, &wsaData);
    if (err) {
        return APR_EEXIST;
    }
    if (LOBYTE(wsaData.wVersion) != WSAHighByte ||
        HIBYTE(wsaData.wVersion) != WSALowByte) {
	    WSACleanup();
	    return APR_EEXIST;
    }

    ap_register_cleanup(new, NULL, clean_cont, NULL);

    *newcont = new;
    return APR_SUCCESS;
}

ap_status_t ap_set_signal_safe(ap_context_t *cont, ap_int16_t safe)
{
    if (cont) { 
        cont->signal_safe = safe;
        return APR_SUCCESS;
    }
    return APR_ENOCONT;
}

ap_status_t ap_set_cancel_safe(ap_context_t *cont, ap_int16_t safe)
{
    if (cont) {
        cont->cancel_safe = safe;
        return APR_SUCCESS;
    }
    return APR_ENOCONT;
}

ap_status_t ap_destroy_context(ap_context_t *cont)
{
    ap_destroy_pool(cont);
    return APR_SUCCESS;
}

ap_status_t ap_get_oslevel(ap_context_t *cont, ap_oslevel_e *level)
{
	static OSVERSIONINFO oslev;
	static BOOL first = TRUE;

	if (first) {
		first = FALSE;
		GetVersionEx(&oslev);
	}
	if (oslev.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		(*level) = APR_WIN_NT;
		return APR_SUCCESS;
	}
	else if (oslev.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
		if (oslev.dwMinorVersion == 0) {
			(*level) = APR_WIN_95;
			return APR_SUCCESS;
		}
		else if (oslev.dwMinorVersion > 0) {
			(*level) = APR_WIN_98;
			return APR_SUCCESS;
		}
	}
	return APR_EEXIST;
}

ap_status_t ap_set_userdata(struct context_t *cont, void *data)
{
    if (cont) { 
        cont->prog_data = data;
        return APR_SUCCESS;
    }
    return APR_ENOCONT;
}

ap_status_t ap_get_userdata(struct context_t *cont, void **data)
{
    if (cont) { 
        (*data) = cont->prog_data;
        return APR_SUCCESS;
    }
    return APR_ENOCONT;
}

/* This puts one thread in a Listen for signals mode */
ap_status_t ap_initialize(void)
{
    unsigned tid;

    if (_beginthreadex(NULL, 0, SignalHandling, NULL, 0, &tid) == 0) {
        return APR_EEXIST;
    }

    while (thread_ready() != 1) {
        sleep(1);
    }

    return APR_SUCCESS;
}
