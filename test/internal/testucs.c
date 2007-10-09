/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr.h"
#include "arch/win32/apr_arch_utf8.h"
#include <wchar.h>
#include <string.h>
#include <assert.h>

struct testval {
    unsigned char n[8];
    wchar_t w[4];
    int nl;
    int wl;
};

void displaynw(struct testval *f, struct testval *l)
{
    char x[80], *t = x;
    int i;
    for (i = 0; i < f->nl; ++i)
        t += sprintf(t, "%02X ", f->n[i]);
    *(t++) = '-'; 
    for (i = 0; i < l->nl; ++i)
        t += sprintf(t, " %02X", l->n[i]);
    *(t++) = ' ';
    *(t++) = '=';
    *(t++) = ' ';
    for (i = 0; i < f->wl; ++i)
        t += sprintf(t, "%04X ", f->w[i]);
    *(t++) = '-';
    for (i = 0; i < l->wl; ++i)
        t += sprintf(t, " %04X", l->w[i]);
    *t = '\0';
    puts(x);
}

/*
 *  Test every possible byte value. 
 *  If the test passes or fails at this byte value we are done.
 *  Otherwise iterate test_nrange again, appending another byte.
 */
void test_nrange(struct testval *p)
{
    struct testval f, l, s;
    apr_status_t rc;
    int success = 0;
    
    memcpy (&s, p, sizeof(s));
    ++s.nl;    
    
    do {
        apr_size_t nl = s.nl, wl = sizeof(s.w) / 2;
        rc = apr_conv_utf8_to_ucs2(s.n, &nl, s.w, &wl);
        s.wl = (sizeof(s.w) / 2) - wl;
        if (!nl && rc == APR_SUCCESS) {
            if (!success) {
                memcpy(&f, &s, sizeof(s));
                success = -1;
            }
            else {
                if (s.wl != l.wl 
                 || memcmp(s.w, l.w, (s.wl - 1) * 2) != 0
                 || s.w[s.wl - 1] != l.w[l.wl - 1] + 1) {
                    displaynw(&f, &l);
                    memcpy(&f, &s, sizeof(s));
                }
            }            
            memcpy(&l, &s, sizeof(s));
        }
        else {
            if (success) {
                displaynw(&f, &l);
                success = 0;
            }
            if (rc == APR_INCOMPLETE) {
                test_nrange(&s);
            }
        }
    } while (++s.n[s.nl - 1]);

    if (success) {
        displaynw(&f, &l);
        success = 0;
    }
}

/* 
 *  Test every possible word value. 
 *  Once we are finished, retest every possible word value.
 *  if the test fails on the following null word, iterate test_nrange 
 *  again, appending another word.
 *  This assures the output order of the two tests are in sync.
 */
void test_wrange(struct testval *p)
{
    struct testval f, l, s;
    apr_status_t rc;
    int success = 0;
    
    memcpy (&s, p, sizeof(s));
    ++s.wl;    
    
    do {
        apr_size_t nl = sizeof(s.n), wl = s.wl;        
        rc = apr_conv_ucs2_to_utf8(s.w, &wl, s.n, &nl);
        s.nl = sizeof(s.n) - nl;
        if (!wl && rc == APR_SUCCESS) {
            if (!success) {
                memcpy(&f, &s, sizeof(s));
                success = -1;
            }
            else {
                if (s.nl != l.nl 
                 || memcmp(s.n, l.n, s.nl - 1) != 0
                 || s.n[s.nl - 1] != l.n[l.nl - 1] + 1) {
                    displaynw(&f, &l);
                    memcpy(&f, &s, sizeof(s));
                }
            }            
            memcpy(&l, &s, sizeof(s));
        }
        else {
            if (success) {
                displaynw(&f, &l);
                success = 0;
            }
        }
    } while (++s.w[s.wl - 1]);

    if (success) {
        displaynw(&f, &l);
        success = 0;
    }

    do {
        int wl = s.wl, nl = sizeof(s.n);
        rc = apr_conv_ucs2_to_utf8(s.w, &wl, s.n, &nl);
        s.nl = sizeof(s.n) - s.nl;
        if (rc == APR_INCOMPLETE) {
            test_wrange(&s);
        }
    } while (++s.w[s.wl - 1]);
}

/*
 *  Test every possible byte value. 
 *  If the test passes or fails at this byte value we are done.
 *  Otherwise iterate test_nrange again, appending another byte.
 */
void test_ranges()
{
    struct testval ntest, wtest;
    apr_status_t nrc, wrc;
    apr_size_t inlen;
    unsigned long matches = 0;

    memset(&ntest, 0, sizeof(ntest));
    ++ntest.nl;

    memset(&wtest, 0, sizeof(wtest));
    ++wtest.wl;

    do {
        do {
            inlen = ntest.nl;
            ntest.wl = sizeof(ntest.w) / 2;
            nrc = apr_conv_utf8_to_ucs2(ntest.n, &inlen, ntest.w, &ntest.wl);
            if (nrc == APR_SUCCESS) {
                ntest.wl = (sizeof(ntest.w) / 2) - ntest.wl;
                break;
            }
            if (nrc == APR_INCOMPLETE) {
                ++ntest.nl;
                if (ntest.nl > 6) {
                    printf ("\n\nUnexpected utf8 sequence of >6 bytes;\n");
                    exit(255);
                }
                continue;
            }
            else {
                while (!(++ntest.n[ntest.nl - 1])) {
                    if (!(--ntest.nl))
                        break;
                }
            }
        } while (ntest.nl);

        do {
            inlen = wtest.wl;
            wtest.nl = sizeof(wtest.n);
            wrc = apr_conv_ucs2_to_utf8(wtest.w, &inlen, wtest.n, &wtest.nl);
            if (wrc == APR_SUCCESS) {
                wtest.nl = sizeof(wtest.n) - wtest.nl;
                break;
            }
            else {
                if (!(++wtest.w[wtest.wl - 1])) {
                    if (wtest.wl == 1)
                        ++wtest.wl;
                    else
                        ++wtest.w[0];

                    /* On the second pass, ensure lead word is incomplete */
                    do {
                        inlen = 1;
                        wtest.nl = sizeof(wtest.n);
                        if (apr_conv_ucs2_to_utf8(wtest.w, &inlen, wtest.n, &wtest.nl)
                                == APR_INCOMPLETE)
                            break;
                        if (!(++wtest.w[0])) {
                            wtest.wl = 0;
                            break;
                        }
                    } while (1);
                }
            }
        } while (wtest.wl);

        if (!ntest.nl && !wtest.wl)
            break;

        /* Identical? */
        if ((wtest.nl != ntest.nl)
         || (memcmp(wtest.n, ntest.n, ntest.nl) != 0)
         || (wtest.wl != ntest.wl)
         || (memcmp(ntest.w, wtest.w, wtest.wl * 2) != 0)) {
            printf ("\n\nMismatch of w/n conversion at;\n");
            displaynw(&ntest, &wtest);
            exit(255);
        }
        ++matches;

        while (!(++ntest.n[ntest.nl - 1])) {
            if (!(--ntest.nl))
                break;
        }

        if (!(++wtest.w[wtest.wl - 1])) {
            if (wtest.wl == 1)
                ++wtest.wl;
            else
                ++wtest.w[0];

            /* On the second pass, ensure lead word is incomplete */
            do {
                inlen = 1;
                wtest.nl = sizeof(wtest.n);
                if (apr_conv_ucs2_to_utf8(wtest.w, &inlen, wtest.n, &wtest.nl)
                        == APR_INCOMPLETE)
                    break;
                if (!(++wtest.w[0])) {
                    wtest.wl = 0;
                    break;
                }
            } while (1);
        }
    } while (wtest.wl || ntest.nl);

    printf ("\n\nutf8 and ucs2 sequences of %lu transformations matched OK.\n",
            matches);
}

/*
 *  Syntax: testucs [w|n]
 *
 *  If no arg or arg is not recognized, run equality sequence test.
 */
int main(int argc, char **argv)
{
    struct testval s;
    memset (&s, 0, sizeof(s));

    if (argc >= 2 && apr_tolower(*argv[1]) != 'w') {
        printf ("\n\nTesting Narrow Char Ranges\n");
        test_nrange(&s);
    }
    else if (argc >= 2 && apr_tolower(*argv[1]) != 'n') {
        printf ("\n\nTesting Wide Char Ranges\n");
        test_wrange(&s);
    }
    else {
        test_ranges();
    }
    return 0;
}
