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
#include "apr_time.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "errno.h"
#include <stdio.h>
#ifdef BEOS
#include <unistd.h>
#endif

int main()
{
    ap_context_t *context;
    ap_time_t *time;
    ap_time_t *time2;
    ap_int32_t rv = 0;
    ap_int64_t t1, t2;

    fprintf(stdout, "Creating context.......");
    if (ap_create_context(&context, NULL) != APR_SUCCESS) {
        fprintf(stderr, "could not create context\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "Testing Time functions.\n");

    fprintf(stdout, "\tMaking new time variable.......");
    if (ap_make_time(&time, context) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate memory\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tGetting current time.......");
    if (ap_current_time(time) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't get the time\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");
    
    fprintf(stdout, "\tExploding Current time.......");
    if (ap_explode_time(time, APR_UTCTIME) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't explode the time\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    ap_make_time(&time2, context);
    fprintf(stdout, "\tGetting the number of seconds.......");
    if (ap_get_sec(time, &rv) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't get the seconds\n");
        exit(-1);
    }
    ap_set_sec(time2, rv);
    fprintf(stdout, "OK\n"); 

    fprintf(stdout, "\tGetting the number of minutes.......");
    if (ap_get_min(time, &rv) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't get the minutes\n");
        exit(-1);
    }
    ap_set_min(time2, rv);
    fprintf(stdout, "OK\n"); 

    fprintf(stdout, "\tGetting the number of hours.......");
    if (ap_get_hour(time, &rv) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't get the hours\n");
        exit(-1);
    }
    ap_set_hour(time2, rv);
    fprintf(stdout, "OK\n"); 

    fprintf(stdout, "\tGetting the number of days.......");
    if (ap_get_mday(time, &rv) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't get the days\n");
        exit(-1);
    }
    ap_set_mday(time2, rv);
    fprintf(stdout, "OK\n"); 

    fprintf(stdout, "\tGetting the month .......");
    if (ap_get_mon(time, &rv) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't get the month\n");
        exit(-1);
    }
    ap_set_mon(time2, rv);
    fprintf(stdout, "OK\n"); 

    fprintf(stdout, "\tGetting the year.......");
    if (ap_get_year(time, &rv) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't get the year\n");
        exit(-1);
    }
    ap_set_year(time2, rv);
    fprintf(stdout, "OK\n"); 

    fprintf(stdout, "\tGetting the weekday.......");
    if (ap_get_wday(time, &rv) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't get the week day\n");
        exit(-1);
    }
    ap_set_wday(time2, rv);
    fprintf(stdout, "OK\n"); 

    fprintf(stdout, "\tImploding the time.......");
    if (ap_implode_time(time2) != APR_SUCCESS ||
        ap_implode_time(time) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't implode time\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    fprintf(stdout, "\tComparing two time values.......");
    ap_get_curtime(time, &t1);
    ap_get_curtime(time2, &t2);
    if ((t1 == -1) || (t2 == -1) || (t1 != t2)) {
        fprintf(stderr, "Values don't match\n");
        exit(-1);
    }
    fprintf(stdout, "OK\n");

    return 1;
}    

