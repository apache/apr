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
#include "test_apr.h"

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
    apr_int64_t hr_off_64;

    printf("APR Time Functions\n==================\n\n");

    STD_TEST_NEQ("Creating a pool to use", apr_pool_create(&p, NULL))

    printf("%-60s", "    apr_time_now()");
    now = apr_time_now();
    printf("OK\n");

    STD_TEST_NEQ("    apr_explode_gmt", apr_explode_gmt(&xt, now))
    
    STD_TEST_NEQ("    apr_explode_localtime", apr_explode_localtime(&xt2, now))

    STD_TEST_NEQ("    apr_implode_time (GMT)", apr_implode_time(&imp, &xt))

    printf("%-60s", "    checking GMT explode == implode");
    if (imp != now) {
	printf("mismatch\n"
                "\t\tapr_now()                %" APR_INT64_T_FMT "\n"
                "\t\tapr_implode() returned   %" APR_INT64_T_FMT "\n"
                "\t\terror delta was          %" APR_TIME_T_FMT "\n"
                "\t\tshould have been         0\n",
                now, imp, imp-now);
	exit(-1);
    }
    printf("OK\n");

    STD_TEST_NEQ("    apr_implode_gmt (GMT)",
                 apr_implode_gmt(&imp, &xt))

    printf("%-60s", "    checking GMT explode == GMT implode");
    if (imp != now) {
        printf("mismatch\n"
                "\t\tapr_now()                %" APR_INT64_T_FMT "\n"
                "\t\tapr_implode() returned   %" APR_INT64_T_FMT "\n"
                "\t\terror delta was          %" APR_TIME_T_FMT "\n"
                "\t\tshould have been         0\n",
                now, imp, imp-now);
        exit(-1);
    }
    printf("OK\n");

    STD_TEST_NEQ("    apr_implode_gmt (localtime)",
                 apr_implode_gmt(&imp, &xt2))

    printf("%-60s", "    checking localtime explode == GMT implode");
    if (imp != now) {
	printf("mismatch\n"
                "\t\tapr_now()                %" APR_INT64_T_FMT "\n"
                "\t\tapr_implode() returned   %" APR_INT64_T_FMT "\n"
                "\t\terror delta was          %" APR_TIME_T_FMT "\n"
                "\t\tshould have been         0\n",
                now, imp, imp-now);
	exit(-1);
    }
    printf("OK\n");

    str = apr_pcalloc(p, sizeof(char) * STR_SIZE);
    str2 = apr_pcalloc(p, sizeof(char) * STR_SIZE);
    imp = 0;

    if (!str || !str2) {
        printf("Failure!\n");
        fprintf(stderr,"Failed to allocate memory!\n");
        exit(-1);
    }

    STD_TEST_NEQ("    apr_rfc822_date", apr_rfc822_date(str, now))
    printf("        ( %s )\n", str);

    STD_TEST_NEQ("    apr_ctime (local time)", apr_ctime(str, now))
    printf("        ( %s )\n", str);

    STD_TEST_NEQ("    apr_strftime (GMT) (24H day date month year)", 
                 apr_strftime(str, &sz, STR_SIZE, "%R %A %d %B %Y", &xt))
    printf("        ( %s )\n", str);

    STD_TEST_NEQ("    apr_strftime (GMT) (HH:MM:SS)",
                 apr_strftime(str, &sz, STR_SIZE, "%T", &xt))
    printf ("        ( %s )\n", str);    

    STD_TEST_NEQ("    apr_explode_time (GMT -5 hours)",
                 apr_explode_time(&xt2, now, hr_off))

    STD_TEST_NEQ("    apr_strftime (offset) (HH:MM:SS)",
                 apr_strftime(str2, &sz, STR_SIZE, "%T", &xt2))
    printf("        ( %s )\n", str2);

    TEST_EQ("    Comparing the GMT and offset time strings",
             strcmp(str, str2), 0, "OK", "Failed")
    printf("        ( %s != %s )\n", str, str2);

    STD_TEST_NEQ("    apr_implode_time (offset)",
                 apr_implode_time(&imp, &xt2))
   
    hr_off_64 = (apr_int64_t) hr_off * APR_USEC_PER_SEC; /* microseconds */ 
    printf("%-60s","    Checking offset is correct");
    if (imp != now + hr_off_64){
        printf("Failed! :(\n");
        printf("Difference is %" APR_INT64_T_FMT " (should be %"
               APR_INT64_T_FMT")\n", imp - now, hr_off_64);
        exit(-1);
    }
    printf("OK\n");
    printf("        ( %lld - %lld = %lld )\n", imp, now, imp - now);
 
    printf("\nTest Complete.\n");
    return 1;
}    

