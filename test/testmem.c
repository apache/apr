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

#include "apr_sms.h"
#include "apr_sms_tracking.h"
#include "apr_sms_trivial.h"
#include "apr_sms_blocks.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_time.h"
#include "test_apr.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "test_apr.h"

#define LUMPS       10
#define LUMP_SIZE   1024
#define TIMED_RUNS  10
#define TIMED_LOOPS 50
char *ptrs[1000];

typedef struct _test_ {
    void * (*malloc_fn)     (void *memsys, apr_size_t size);
    void * (*calloc_fn)     (void *memsys, apr_size_t size);
    void * (*free_fn)       (void *memsys, void *memptr);
    void * (*reset_fn)      (void *memsys);
    void * memory;
    char * title;
    int large_tests;
    apr_time_t howlong;
} _test_;

#define T_QTY 5 /* how many tests do we have?? */
static _test_ t[T_QTY];

static void its_an_sms(apr_sms_t *ams, _test_ *t, char *name, int lt)
{
    t->malloc_fn = (void*)apr_sms_malloc;
    t->calloc_fn = (void*)apr_sms_calloc;
    t->free_fn =   (void*)apr_sms_free;
    t->reset_fn =  (void*)apr_sms_reset;
    t->memory = ams;
    t->title = name;
    t->large_tests = lt;
    t->howlong = 0;
}

static void its_a_pool(apr_pool_t *pool, _test_ *t, char *name, int lt)
{
    t->malloc_fn = (void*)apr_palloc;
    t->calloc_fn = (void*)apr_pcalloc;
    t->free_fn = NULL;
    t->reset_fn = (void*)apr_pool_clear;
    t->memory = pool;
    t->title = name;
    t->large_tests = lt;
    t->howlong = 0;
}

static int malloc_test(_test_ *t, apr_size_t size, int howmany, int verbose)
{
    int cntr;

    if (verbose)
        printf("    Malloc'ing %d lumps of memory, each of %" APR_SIZE_T_FMT " bytes  ",
               howmany, size);
    for (cntr = 0;cntr < howmany;cntr ++){
        ptrs[cntr] = t->malloc_fn(t->memory, size);
        if (!ptrs[cntr]){
            printf("Failed\n");
            fprintf(stderr,"Failed @ lump %d of %d\n", cntr + 1, howmany);
            return 1;
        }
    }
    if (verbose)
        printf ("OK\n");

    return 0;
}

static int calloc_test(_test_ *t, apr_size_t size, int howmany, int verbose)
{
    int cntr, cntr2;
    
    if (verbose)
        printf("    Calloc'ing %d lumps of memory, each %" APR_SIZE_T_FMT " bytes          ",
               howmany, size);
    for (cntr = 0;cntr < howmany;cntr ++){
        ptrs[cntr] = t->calloc_fn(t->memory, size);
        if (!ptrs[cntr]){
            printf("Failed\n");
            fprintf(stderr, "Failed @ lump %d of %d\n", cntr + 1, howmany);
            return 1;
        }
    }
    if (verbose) {
        printf ("OK\n");
        printf("     (checking that memory is zeroed                      ");
    }
    for (cntr = 0;cntr < howmany;cntr++){
        for (cntr2 = 0;cntr2 < size; cntr2 ++){
            if (*(ptrs[cntr] + cntr2) != 0){
                printf("Failed\n");
                fprintf(stderr, "Failed!\nGot %d instead of 0 at byte %d of chunk %d [%p]\n",
                       *(ptrs[cntr] + cntr2), cntr2 + 1, cntr + 1, ptrs[cntr] + cntr2);
                return 1;
            }
        }
    } 
    if (verbose)
        printf("OK)\n");

    return 0;
}

static int write_test(apr_size_t size, int howmany, int verbose)
{
    int cntr,cntr2;
    int val;
    
    if (verbose)
        printf("%-60s", "    Writing to the lumps of memory");

    for (cntr = 0;cntr < howmany;cntr ++){
        if (size == 64) {
            /* we go past 256 in our tests, so use a different value :) */
            val = 99;
        } else {
            val = cntr;
        }
        if (memset(ptrs[cntr], val, size) != ptrs[cntr]){
            printf("Failed\n");
            fprintf(stderr,"Failed to write into lump %d\n", cntr + 1);
            return 1;
        }
    }

    if (verbose) {
        printf("OK\n");    

        printf("%-60s", "    Check what we wrote");
    }

    for (cntr = 0;cntr < howmany;cntr++){
        if (size == 64) {
            val = 99;
        } else {
            val = cntr;
        }
        for (cntr2 = 0;cntr2 < size; cntr2 ++){
            if (*(ptrs[cntr] + cntr2) != val){
                printf("Failed\n");
                fprintf(stderr,"Got %d instead of %d at byte %d\n",
                       *(ptrs[cntr] + cntr2), val, cntr2 + 1);
                return 1;
            }
        }
    } 
    if (verbose)
        printf("OK\n");   
    return 0;
}

static int free_memory(_test_ *t, int qty, int verbose)
{
    int cntr;
    
    if (verbose)
        printf("    Freeing the memory we created                           ");
    /* pools don't really do free... */
    if (t->free_fn) {
        for (cntr = 0;cntr < qty;cntr ++){
            if (t->free_fn(t->memory, ptrs[cntr]) != APR_SUCCESS){
                printf("Failed\n");
                fprintf(stderr,"Failed to free block %d\n", cntr + 1);
                return 1;
            }
        }
    }
    
    if (verbose)
        printf("OK\n");
    return 0;
}

static int reset_memory(_test_ *t, int loops, int verbose)
{
    if (verbose)
        printf("    Resetting the memory we created                           ");
    if (!t->reset_fn) {
        free_memory(t, loops, verbose);
    } else {
        t->reset_fn(t->memory);
    }
    
    if (verbose)
        printf("OK\n");
    return 0;
}

static int simple_test(_test_ *t, int verbose)
{
    char msg[60];
    if (t->large_tests == 0)
        return 0;
        
    sprintf(msg, "    Big allocation test for %s", t->title);
    printf("%-60s", msg);
    if (malloc_test(t, 4096, 100, verbose))
        return 1;
    if (write_test(4096, 100, verbose))
        return 1;
    if (free_memory(t, 100, verbose))
        return 1;
    if (calloc_test(t, 4096, 100, verbose))
        return 1;
    if (write_test(4096, 100, verbose))
        return 1;
    if (free_memory(t, 100, verbose))
        return 1;
    printf("OK\n");
    return 0;
}

static int small_test(_test_ *t, int verbose)
{
    char msg[60];
    sprintf(msg, "    Small allocation test for %s", t->title);
    printf("%-60s", msg);
    if (malloc_test(t, 64, 100,  verbose))
        return 1;
    if (write_test(64, 100, verbose))
        return 1;
    if (free_memory(t, 100, verbose))
        return 1;
    printf("OK\n");
    return 0;
}
    
static int timed_test(_test_ *t, int verbose)
{
    int iloop, oloop, ooloop, rv;
    apr_time_t t1=0, t2=0, t3=0, t4 = 0, tmp = 0, total = 0;
    char msg[60];
    apr_size_t sz[5] = {1024, 4096, 256, 8 * 1024, 1024};
        
    if (t->large_tests == 0)
        return 0;
 
    sprintf(msg, "    Timed alloc test (%d - %d) for %s", LUMPS, LUMPS + ( 3 * LUMPS),
            t->title);
    if (verbose) {
        printf("%s\n", msg);
        printf("    alloc      <--------        timings (usecs)        -------->\n");
        printf("    size           malloc /     calloc /      reset /      total\n");
    } else {
        printf("%-60s", msg);
    }
         
    for (ooloop = 0; ooloop < 5; ooloop ++) {
        for (oloop = 0; oloop < TIMED_LOOPS;oloop ++) {
            for (iloop = 0; iloop < TIMED_RUNS; iloop ++) {
                TIME_FUNCTION(tmp, (rv = malloc_test(t, sz[ooloop], 100, 0)))
                t1 += tmp;
                if (rv)
                    return 1;
                TIME_FUNCTION(tmp, (rv = write_test(sz[ooloop], 100, 0)))
                if (rv)
                    return 1;
                TIME_FUNCTION(tmp, (rv = reset_memory(t, 100, 0)))
                t2 += tmp;
                if (rv)
                    return 1;
            }
            for (iloop = 0; iloop < TIMED_RUNS; iloop++) {
                TIME_FUNCTION(tmp, (rv = calloc_test(t, sz[ooloop], 100, 0)))
                t3 += tmp;
                if (rv)
                    return 1;
                TIME_FUNCTION(tmp, (rv = write_test(sz[ooloop], 100, 0)))
                if (rv)
                    return 1;
                TIME_FUNCTION(tmp, (rv = reset_memory(t, 100, 0)))
                t4 += tmp;
                if (rv)
                    return 1;
            }
        }
        if (verbose)
            printf("    %4" APR_SIZE_T_FMT "       %10lld / %10lld / %10lld / %10lld\n",
                   sz[ooloop], t1, t3, t2 + t4, t1 + t2 + t3 + t4);
        total += (t1 + t2 + t3 + t4);
        t1=0;t2=0;t3=0;t4=0; 
    }
    if (verbose) {        
        printf("          average = %lld\n", 
               (total / TIMED_LOOPS));
    } else {
        printf("OK\n");
    }
    t->howlong = (total / TIMED_LOOPS);
    return 0;
}

static int timed_test_64byte(_test_ *t, int small, int verbose)
{
    apr_size_t sz[4] = {100,300,600,1000};
    int iloop, oloop, ooloop, rv;
    apr_time_t t1=0, t2=0, t3=0, tmp = 0, total = 0;
    apr_time_t tt1=0, tt2=0, tt3=0;
    char msg[80];
    
    if (small) {
        sz[0] = 100;
        sz[1] = 100;
        sz[2] = 100;
        sz[3] = 100;
    }
            
    sprintf(msg, "    64 byte alloc test (%" APR_SIZE_T_FMT " - %" APR_SIZE_T_FMT " loops) %s",
            sz[0], sz[3], t->title);
    if (verbose) {
        printf("%s\n", msg);
        printf("                    <------    timings (usecs)    ------>\n");
        printf("    allocations          alloc /      reset /      total\n");
    } else {
        printf("%-60s", msg);
    }
    
    for (ooloop = 0; ooloop < 4; ooloop ++) {
        t1=0;t2=0;t3=0;
        for (oloop = 0; oloop < TIMED_LOOPS * 2;oloop ++) {
            for (iloop = 0; iloop < TIMED_RUNS; iloop ++) {
                TIME_FUNCTION(tmp, (rv = malloc_test(t, 64, sz[ooloop], 0)))
                t1 += tmp;
                if (rv)
                    return 1;
                tmp = apr_time_now();
                if (write_test(64, sz[ooloop], 0))
                    return 1;
                t2 += (apr_time_now() - tmp);
                tmp = apr_time_now();
                if (reset_memory(t, sz[ooloop], 0))
                    return 1;
                t3 += (apr_time_now() - tmp);
            }
        }
        if (verbose) {
            printf("    %4" APR_SIZE_T_FMT "            %10lld / %10lld / %10lld\n",
                   sz[ooloop], t1, t2, t1 + t2);
        }
        tt1 += t1;tt2 += t2;tt3 += t3;
        total += (t1 + t2 + t3);
        t1 = 0; t2 = 0;
    }
    if (verbose)
        printf("        average over 4 runs = %lld\n\n", total / 4);      
    else
        printf("OK\n");
        
    t->howlong = total / 4;
    
    return 0;
}

static void print_timed_results(void)
{
    int i;
    printf("    Percentage Results  averages  %% of pools  %% of std sms\n");
    for (i=0;i < T_QTY; i++) {
        float pa = (float)t[i].howlong / (float)t[0].howlong;
        float pb = (float)t[i].howlong / (float)t[1].howlong;       
        printf("    %-20s %-8lld  %7.02f %%     %7.02f %%\n", t[i].title, t[i].howlong,
               pa * 100, pb * 100);
    } 
    printf("\n");  
    for (i=0;i<T_QTY;i++)
        t[i].howlong = 0;
}

int main(int argc, char **argv)
{
    apr_sms_t *ams, *bms, *dms, *tms;
    apr_pool_t *pool;
    int i;
    apr_sms_t *lsms[10];
 
    apr_initialize();
        
    printf("APR Memory Test\n");
    printf("===============\n\n");

    printf("Creating the memory systems...\n");
    STD_TEST_NEQ("    Creating a pool", 
                 apr_pool_create(&pool, NULL))
    STD_TEST_NEQ("    Creating the standard memory system",
                 apr_sms_std_create(&ams))   
    STD_TEST_NEQ("    Creating the tracking memory system",
                 apr_sms_tracking_create(&bms, ams))
    STD_TEST_NEQ("    Creating a 64 byte block system",
                 apr_sms_blocks_create(&dms, ams, 64))
    STD_TEST_NEQ("    Creating a trivial system",
                 apr_sms_trivial_create(&tms, ams))

/* if we're using tag's then add them :) */
#if APR_DEBUG_TAG_SMS
    apr_sms_tag("top-level", ams);
    apr_sms_tag("tracking", bms);
    apr_sms_tag("blocks", dms);
    apr_sms_tag("trivial", tms);
#endif

    its_a_pool(pool, &t[0], "Pool code",     1);
    its_an_sms(ams,  &t[1], "Standard sms",  1);
    t[1].reset_fn = NULL;
    its_an_sms(bms,  &t[2], "Tracking sms",  1);
    its_an_sms(dms,  &t[3], "Blocks sms",    0);
    its_an_sms(tms,  &t[4], "Trivial sms",   1);
        
    printf("Checking sms identities...\n");
    TEST_NEQ("    Checking identity of standard memory system",
             strcmp(apr_sms_identity(ams), "STANDARD"), 0,
             "OK","Not STANDARD")
    TEST_NEQ("    Checking the identity of tracking memory system",
             strcmp(apr_sms_identity(bms), "TRACKING"), 0,
             "OK", "Not TRACKING")
    TEST_NEQ("    Checking the identity of blocks memory system",
             strcmp(apr_sms_identity(dms), "BLOCKS"), 0,
             "OK", "Not BLOCKS")
    TEST_NEQ("    Checking the identity of trivial memory system",
             strcmp(apr_sms_identity(tms), "TRIVIAL"), 0,
             "OK", "Not TRIVIAL")

    printf("Big allocation test...\n");
    for (i = 0; i < T_QTY; i++) {
        if (simple_test(&t[i], 0)) {
            exit (-1);
        }
    }

    printf("64 byte allocation test...\n");
    for (i = 0; i< T_QTY; i++) {
        if (small_test(&t[i], 0))
            exit(-1);
    }

    printf("Timed test #1\n");
    for (i = 0; i< T_QTY; i++) {
        if (timed_test_64byte(&t[i], 1, 0)) {
            exit (-1);
        }
    }
    print_timed_results();
    
    printf("Timed test #2\n");
    for (i = 0; i< T_QTY; i++) {
        if (timed_test_64byte(&t[i], 0, 0)) {
            exit (-1);
        }
    }
    print_timed_results();

    printf("Timed test #3\n");
    for (i = 0; i< T_QTY; i++) {
        if (timed_test(&t[i], 1))
            exit(-1);
    }
    print_timed_results();
   
    printf("Destroying the memory...\n");

    STD_TEST_NEQ("Trying to destroy the trivial memory system",
                 apr_sms_destroy(tms))
    STD_TEST_NEQ("Trying to destroy the tracking memory system",
                 apr_sms_destroy(bms))
    STD_TEST_NEQ("Trying to destroy the block memory system",
                 apr_sms_destroy(dms))                        

    printf("Testing layering...\n");
    apr_sms_tracking_create(&lsms[0], ams); 
    for (i=1;i<5;i++) {
        apr_sms_tracking_create(&lsms[i], lsms[i-1]);
    }
    for (i=5;i<10;i++) {
        apr_sms_tracking_create(&lsms[i], lsms[4]);
    }
    STD_TEST_NEQ("Trying to destroy the standard memory system",
                 apr_sms_destroy(ams))

    printf("Memory test passed.\n");
    return (0);          
}
