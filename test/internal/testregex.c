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


#include "apr_strings.h"
#include "apr_pools.h"
#include "apr_general.h"
#include "apr_hash.h"
#include "apr_lib.h"
#include "apr_time.h"
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

int main( int argc, char** argv) {
    apr_pool_t *context;
    regex_t regex;
    int rc;
    int i;
    int iters;
    apr_time_t now;
    apr_time_t end;
    apr_hash_t *h;
    

    if (argc !=4 ) {
            fprintf(stderr, "Usage %s match string #iterations\n",argv[0]);
            return -1;
    }
    iters = atoi( argv[3]);
    
    apr_initialize() ;
    atexit(apr_terminate);
    if (apr_pool_create(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Something went wrong\n");
        exit(-1);
    }
    rc = regcomp( &regex, argv[1], REG_EXTENDED|REG_NOSUB);


    if (rc) {
        char errbuf[2000];
        regerror(rc, &regex,errbuf,2000);
        fprintf(stderr,"Couldn't compile regex ;(\n%s\n ",errbuf);
        return -1;
    }
    if ( regexec( &regex, argv[2], 0, NULL,0) == 0 ) {
        fprintf(stderr,"Match\n");
    }
    else {
        fprintf(stderr,"No Match\n");
    }
    now = apr_time_now();
    for (i=0;i<iters;i++) {
        regexec( &regex, argv[2], 0, NULL,0) ;
    }
    end=apr_time_now();
    puts(apr_psprintf( context, "Time to run %d regex's          %8lld\n",iters,end-now));
    h = apr_hash_make( context);
    for (i=0;i<70;i++) {
            apr_hash_set(h,apr_psprintf(context, "%dkey",i),APR_HASH_KEY_STRING,"1");
    }
    now = apr_time_now();
    for (i=0;i<iters;i++) {
        apr_hash_get( h, argv[2], APR_HASH_KEY_STRING);
    }
    end=apr_time_now();
    puts(apr_psprintf( context, "Time to run %d hash (no find)'s %8lld\n",iters,end-now));
    apr_hash_set(h, argv[2],APR_HASH_KEY_STRING,"1");
    now = apr_time_now();
    for (i=0;i<iters;i++) {
        apr_hash_get( h, argv[2], APR_HASH_KEY_STRING);
    }
    end=apr_time_now();
    puts(apr_psprintf( context, "Time to run %d hash (find)'s    %8lld\n",iters,end-now));
 
    return 0;
}
