/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
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
#include "apr_strings.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_hash.h"
#if APR_HAVE_STDIO_H
#include <stdio.h>
#endif
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif

static void dump_hash(apr_pool_t *p, apr_hash_t *h) 
{
    apr_hash_index_t *hi;
    char *val, *key;
    apr_ssize_t len;
    int i = 0;

    for (hi = apr_hash_first(p, h); hi; hi = apr_hash_next(hi)) {
        apr_hash_this(hi,(void*) &key, &len, (void*) &val);
        fprintf(stdout, "Key %s (%" APR_SSIZE_T_FMT ") Value %s\n", key, len, val);
        i++;
    }
    if (i != apr_hash_count(h)) 
        fprintf(stderr, "ERROR: #entries (%d) does not match count (%d)\n",
                i, apr_hash_count(h));
    else 
        fprintf(stdout, "#entries %d \n", i);
}

static void sum_hash(apr_pool_t *p, apr_hash_t *h, int *pcount, int *keySum, int *valSum) 
{
    apr_hash_index_t *hi;
    void *val, *key;
    int count = 0;

    *keySum = 0;
    *valSum = 0;
    *pcount = 0;
    for (hi = apr_hash_first(p, h); hi; hi = apr_hash_next(hi)) {
        apr_hash_this(hi, (void*)&key, NULL, &val);
        *valSum += *(int *)val;
        *keySum += *(int *)key;
        count++;
    }
    *pcount=count;
}

int main(int argc, const char *const argv[])
{
    apr_pool_t *cntxt;
    apr_hash_t *h, *h2, *h3, *h4;

    int i, j, *val, *key;
    char *result;
    int sumKeys, sumVal, trySumKey, trySumVal;

    apr_initialize();
    atexit(apr_terminate);

    apr_pool_create(&cntxt, NULL);

    /* table defaults  */
    h = apr_hash_make(cntxt);
    if (h == NULL)  {
        fprintf(stderr, "ERROR: can not allocate HASH!\n");
        exit(-1);
    }

    apr_hash_set(h, "OVERWRITE", APR_HASH_KEY_STRING, "should not see this");
    apr_hash_set(h, "FOO3", APR_HASH_KEY_STRING, "bar3");
    apr_hash_set(h, "FOO3", APR_HASH_KEY_STRING, "bar3");
    apr_hash_set(h, "FOO1", APR_HASH_KEY_STRING, "bar1");
    apr_hash_set(h, "FOO2", APR_HASH_KEY_STRING, "bar2");
    apr_hash_set(h, "FOO4", APR_HASH_KEY_STRING, "bar4");
    apr_hash_set(h, "SAME1", APR_HASH_KEY_STRING, "same");
    apr_hash_set(h, "SAME2", APR_HASH_KEY_STRING, "same");
    apr_hash_set(h, "OVERWRITE", APR_HASH_KEY_STRING, "Overwrite key");

    result = apr_hash_get(h, "FOO2", APR_HASH_KEY_STRING);
    if (strcmp(result, "bar2"))
        fprintf(stderr, "ERROR:apr_hash_get FOO2 = %s (should be bar2)\n",
                result);

    result = apr_hash_get(h, "SAME2", APR_HASH_KEY_STRING);
    if (strcmp(result, "same"))
        fprintf(stderr, "ERROR:apr_hash_get SAME2 = %s (should be same)\n",
                result);

    result = apr_hash_get(h, "OVERWRITE", APR_HASH_KEY_STRING);
    if (strcmp(result, "Overwrite key"))
        fprintf(stderr, 
             "ERROR:apr_hash_get OVERWRITE = %s (should be 'Overwrite key')\n",
             result);

    result = apr_hash_get(h, "NOTTHERE", APR_HASH_KEY_STRING);
    if (result)
       fprintf(stderr, "ERROR:apr_hash_get NOTTHERE = %s (should be NULL)\n",
               result);
        
    result=apr_hash_get(h, "FOO3", APR_HASH_KEY_STRING);
    if (strcmp(result, "bar3"))
        fprintf(stderr, "ERROR:apr_hash_get FOO3 = %s (should be bar3)\n",
                result);
        
    dump_hash(cntxt, h);

    h2 =apr_hash_make(cntxt);
    if (h2 == NULL)  {
        fprintf(stderr, "ERROR: can not allocate HASH!\n");
        exit(-1);
    }
    sumKeys = 0;
    sumVal = 0;
    trySumKey = 0;
    trySumVal = 0;

    for (i = 0; i < 100; i++) {
        j = i * 10 + 1;
        sumKeys += j;
        sumVal += i;
        key = apr_palloc(cntxt, sizeof(int));
        *key = j;
        val = apr_palloc(cntxt, sizeof(int));
        *val = i;
        apr_hash_set(h2, key, sizeof(int), val);
    }

    sum_hash(cntxt, h2, &i, &trySumKey, &trySumVal);
    if (i==100) {
       fprintf(stdout, "All keys accounted for\n");
    } else {
       fprintf(stderr, "ERROR: Only got %d (out of 100)\n",i);
    }
    if (trySumVal != sumVal) {
       fprintf(stderr, "ERROR:Values don't add up Got %d expected %d\n",
               trySumVal, sumVal);
    }
    if (trySumKey != sumKeys) {
       fprintf(stderr, "ERROR:Keys don't add up Got %d expected %d\n",
               trySumKey, sumKeys);
    }

    j=891;
    apr_hash_set(h2, &j, sizeof(int), NULL);
    
    if (apr_hash_get(h2, &j, sizeof(int))) {
      fprintf(stderr, "ERROR: Delete not working\n");
    } else {
      fprintf(stdout, "Delete working\n");
    }
    sum_hash(cntxt, h2, &i, &trySumKey, &trySumVal);

    sumKeys -= 891;
    sumVal -= 89;

    if (i==99) {
       fprintf(stdout, "All keys accounted for.. Delete OK\n");
    } else {
       fprintf(stderr, "Only got %d (out of 99) Delete Not OK\n", i);
    }
    if (trySumVal != sumVal) {
       fprintf(stderr, "ERROR:Values don't add up Got %d expected %d\n",
               trySumVal, sumVal);
    }
    if (trySumKey != sumKeys) {
       fprintf(stderr, "ERROR:Keys don't add up Got %d expected %d\n",
               trySumKey, sumKeys);
    }

    /* test overlay */
    h3 = apr_hash_make(cntxt);
    /* test with blank hash tables */
    h4 = apr_hash_overlay(cntxt, h3, h);
    
    if (apr_hash_count(h4) != apr_hash_count(h)) {
        fprintf(stderr,
                "ERROR: overlay not working with blank overlay as overlay\n");
        dump_hash(cntxt, h4);
    }

    h4 = apr_hash_overlay(cntxt, h, h3);
    if (apr_hash_count(h4) != apr_hash_count(h)) {
        fprintf(stderr,
                "ERROR: overlay not working with blank overlay as base\n");
        dump_hash(cntxt, h4);
    }

    h4 = apr_hash_overlay(cntxt, h, h2);
    if (apr_hash_count(h4) != (apr_hash_count(h) + apr_hash_count(h2)))
        fprintf(stderr,
                "ERROR: overlay not working when overlaying 2 unique hashs\n");

    h4 = apr_hash_overlay(cntxt, h, h);
    if (apr_hash_count(h4) != apr_hash_count(h))  {
        fprintf(stderr,
                "ERROR: overlay not working when overlaying same hash\n");
        dump_hash(cntxt, h4);
    }
        
    result = apr_hash_get(h4, "FOO2", APR_HASH_KEY_STRING);
    if (strcmp(result, "bar2"))
        fprintf(stderr, "ERROR:apr_hash_get FOO2 = %s (should be bar2)\n",
                result);

    result = apr_hash_get(h4, "SAME2", APR_HASH_KEY_STRING);
    if (strcmp(result, "same"))
        fprintf(stderr, "ERROR:apr_hash_get SAME2 = %s (should be same)\n",
                result);
    
    result = apr_hash_get(h4, "OVERWRITE", APR_HASH_KEY_STRING);
    if (strcmp(result, "Overwrite key"))
        fprintf(stderr,
             "ERROR:apr_hash_get OVERWRITE = %s (should be 'Overwrite key')\n",
             result);

    result = apr_hash_get(h4, "NOTTHERE", APR_HASH_KEY_STRING);
    if (result)
        fprintf(stderr, "ERROR:apr_hash_get NOTTHERE = %s (should be NULL)\n",
                result);
        
    result = apr_hash_get(h4, "FOO3", APR_HASH_KEY_STRING);
    if (strcmp(result, "bar3"))
        fprintf(stderr, "ERROR:apr_hash_get FOO3 = %s (should be bar3)\n",
                result);

    apr_hash_set(h4, "FOO3", sizeof(int), NULL);              
    result = apr_hash_get(h4, "FOO3", APR_HASH_KEY_STRING);
    if (result)
        fprintf(stderr,
        "ERROR:apr_hash_get FOO3 = %s (should be NULL, we just deleted it!)\n",
        result);

    return 0;
}
