/* ====================================================================
 * Copyright (c) 2000 The Apache Software Foundation.  All rights reserved.
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
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Software Foundation" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Software Foundation.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Software Foundation
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE Apache Software Foundation ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE Apache Software Foundation OR
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
 * individuals on behalf of the Apache Software Foundation.
 * For more information on the Apache Software Foundation and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

/* common .c
 * This file has any function that is truly common and platform
 * neutral.  Or at least that's the theory.
 * 
 * The header files are a problem so there are a few #ifdef's to take
 * care of those.
 *
 */

#ifdef BEOS
#include "../beos/mmap_h.h"
#else
#include "mmap_h.h"
#endif

#if HAVE_MMAP

/* ***APRDOC********************************************************
 * ap_status_t ap_mmap_offset(void **addr, ap_mmap_t *mmap, ap_offset_t offset)
 *    Move the pointer into the mmap'ed file to the specified offset.
 * arg 1) The pointer to the offset specified.
 * arg 2) The mmap'ed file.
 * arg 3) The offset to move to.
 */
ap_status_t ap_mmap_offset(void **addr, ap_mmap_t *mmap, ap_off_t offset)
{  
    if (offset < 0 || offset > mmap->size)
        return APR_EINVAL;
    
    (*addr) = (char *) mmap->mm + offset;
    return APR_SUCCESS;
}

#endif
