/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
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
#include "apr_private.h"
#include "apr_general.h"
#include "apr_portable.h"
#include "apr_arch_misc.h"
#include <wincrypt.h>


APR_DECLARE(apr_status_t) apr_generate_random_bytes(unsigned char * buf,
#ifdef APR_ENABLE_FOR_1_0
                                                    apr_size_t length)
#else
                                                    int length)
#endif
{
    HCRYPTPROV hProv;
    apr_status_t res = APR_SUCCESS;

    /* 0x40 bit = CRYPT_SILENT, only introduced in more recent PSDKs 
     * and will only work for Win2K and later.
     */
    DWORD flags = CRYPT_VERIFYCONTEXT
                | ((apr_os_level >= APR_WIN_2000) ? 0x40 : 0);

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, flags)) {
	return apr_get_os_error();
    }
    if (!CryptGenRandom(hProv, length, buf)) {
    	res = apr_get_os_error();
    }
    CryptReleaseContext(hProv, 0);
    return res;
}


APR_DECLARE(apr_status_t) apr_os_uuid_get(unsigned char *uuid_data)
{
    /* Note: this call doesn't actually require CoInitialize() first 
     *
     * XXX: we should scramble the bytes or some such to eliminate the
     * possible misuse/abuse since uuid is based on the NIC address, and
     * is therefore not only a uniqifier, but an identity (which might not
     * be appropriate in all cases.
     *
     * Note that Win2000, XP and later no longer suffer from this problem,
     * a scrambling fix is only needed for (apr_os_level < APR_WIN_2000)
     */
    if (FAILED(UuidCreate((UUID *)uuid_data))) {
        return APR_EGENERAL;
    }
    return APR_SUCCESS;
}
