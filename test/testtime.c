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

#include "apr_time.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include <errno.h>
#include <stdio.h>
#ifdef BEOS
#include <unistd.h>
#endif

#define STR_SIZE 45

int main(void)
{
    apr_time_t now;
    apr_exploded_time_t xt, xt2;
    apr_time_t imp;
    apr_pool_t *p;
    char *str, *str2;
    apr_size_t sz;
    apr_int32_t hr_off = -5 * 3600; /* 5 hours in seconds */

    fprintf(stdout, "Testing Time functions.\n");

    if (apr_pool_create(&p, NULL) != APR_SUCCESS){
        printf("Failed to create a context!\n");
        exit (-1);
    }

    printf("\tapr_time_now....................................");
    now = apr_time_now();
    printf("OK\n");

    printf("\tapr_explode_gmt.................................");
    if (apr_explode_gmt(&xt, now) != APR_SUCCESS) {
        printf("Couldn't explode the time\n");
        exit(-1);
    }
    printf("OK\n");
    
    printf("\tapr_explode_localtime...........................");
    if (apr_explode_localtime(&xt2, now) != APR_SUCCESS) {
        printf("Couldn't explode the time\n");
        exit(-1);
    }
    printf("OK\n");

    printf("\tapr_implode_time................................");
    if (apr_implode_time(&imp, &xt) != APR_SUCCESS) {
        printf("Couldn't implode the time\n");
        exit(-1);
    }
    printf("OK\n");

    printf("\tchecking the explode/implode (GMT)..............");
    if (imp != now) {
	printf("mismatch\n"
                "\tapr_now()                %lld\n"
                "\tapr_implode() returned   %lld\n"
                "\terror delta was          %lld\n",
                now, imp, imp-now);
	exit(-1);
    }
    printf("OK\n");

    str = apr_pcalloc(p, sizeof(char) * STR_SIZE);
    str2 = apr_pcalloc(p, sizeof(char) * STR_SIZE);
    imp = 0;

    printf("\tapr_rfc822_date.................");
    if (apr_rfc822_date(str, now) != APR_SUCCESS){
        printf("Failed!\n");
        exit (-1);
    }
    printf("%s\n", str);

    printf("\tapr_ctime.......(local).........");
    if (apr_ctime(str, now) != APR_SUCCESS){
        printf("Failed!\n");
        exit(-1);
    }
    printf("%s\n", str);

    printf("\tapr_strftime....................");
    if (str == NULL){
        printf("Couldn't allocate memory.\n");
        exit (-1);
    } 
    if (apr_strftime(str, &sz, STR_SIZE, "%R %A %d %B %Y", &xt) !=
         APR_SUCCESS){
        printf("Failed!");
        exit (-1); 
    }
    printf("%s\n", str);

    printf("\tCurrent time (GMT)..............................");
    if (apr_strftime(str, &sz, STR_SIZE, "%T ", &xt) != APR_SUCCESS){
        printf("Failed!\n");
        exit (-1);
    }
    printf ("%s\n", str);    

    printf("\tTrying to explode time with 5 hour offset.......");
    if (apr_explode_time(&xt2, now, hr_off) != APR_SUCCESS){
        printf("Failed.\n");
        exit(-1);
    }
    printf("OK\n");

    printf("\tOffset Time Zone -5 hours.......................");
    if (apr_strftime(str2, &sz, STR_SIZE, "%T  (%z)", &xt2) != APR_SUCCESS){
        printf("Failed!\n");
        exit(-1);
    }
    printf("%s\n", str2);

    printf("\tComparing the created times.....................");
    if (! strcmp(str,str2)){
        printf("Failed\n");
    } else {
        printf("OK\n");
    }

    printf("\tChecking imploded time after offset.............");
    apr_implode_time(&imp, &xt2);
    hr_off *= APR_USEC_PER_SEC; /* microseconds */ 
    if (imp != now + hr_off){
        printf("Failed! :(\n");
        printf("Difference is %" APR_TIME_T_FMT " (should be %" 
               APR_TIME_T_FMT ")\n", imp - now, hr_off);
        exit(-1);
    }
    printf("OK\n");
    
    return 1;
}    

