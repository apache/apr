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

#include "fileio.h"
#include "apr_file_io.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <string.h>

ap_status_t ap_create_pipe(struct file_t **in, struct file_t **out, ap_context_t *cont)
{
    ULONG filedes[2];
    ULONG rc;

    rc = DosCreatePipe(filedes, filedes+1, 4096);
        
    if (rc) {
        return os2errno(rc);
    }
    
    (*in) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));
    (*in)->cntxt = cont;
    (*in)->filedes = filedes[0];
    (*in)->fname = ap_pstrdup(cont, "PIPE");
    (*in)->isopen = TRUE;
    (*in)->buffered = FALSE;
    (*in)->flags = 0;
    ap_register_cleanup(cont, *in, apr_file_cleanup, ap_null_cleanup);

    (*out) = (struct file_t *)ap_palloc(cont, sizeof(struct file_t));
    (*out)->cntxt = cont;
    (*out)->filedes = filedes[1];
    (*out)->fname = ap_pstrdup(cont, "PIPE");
    (*out)->isopen = TRUE;
    (*out)->buffered = FALSE;
    (*out)->flags = 0;
    ap_register_cleanup(cont, *out, apr_file_cleanup, ap_null_cleanup);

    return APR_SUCCESS;
}



ap_status_t ap_create_namedpipe(char **new, char *dirpath, ap_fileperms_t perm, ap_context_t *cont)
{
    /* Not yet implemented, interface not suitable */
    return -1;
} 

 
