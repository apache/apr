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
#include <stdio.h>
#include <string.h>
#include "apr_mmap.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"

/* hmmm, what is a truly portable define for the max path
 * length on a platform?
 */
#define PATH_LEN 255

int main()
{
    ap_context_t *context;
    ap_mmap_t *themmap = NULL;
    ap_status_t status = 0;
    char *file;

    fprintf (stdout,"APR MMAP Test\n*************\n\n");
    
    fprintf(stdout,"Creating context....................");    
    if (ap_create_context(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Failed.\n");
        exit(-1);
    }
    fprintf(stdout,"OK\n");
    
    file = (char*) malloc(sizeof(char) * PATH_LEN);
    getcwd(file, PATH_LEN);
    strncat(file,"/testmmap.c",11);  
    
    fprintf(stdout,"Trying to mmap file.................");
    if (ap_mmap_create(&themmap, file, context) != APR_SUCCESS) {
        fprintf(stderr,"Failed.\n");
        exit (-1);
    }
    fprintf(stdout,"OK\n");

    fprintf(stdout,"Trying to delete the mmap file......");
    if (ap_mmap_delete(themmap) != APR_SUCCESS) {
        fprintf(stderr,"Failed!\n");
        exit (-1);
    }
    fprintf(stdout,"OK\n");

    fprintf (stdout,"\nTest Complete\n");
    
    return 1;
}
