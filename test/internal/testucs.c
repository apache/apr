/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2002-2003 The Apache Software Foundation.  All rights
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
#include "arch/win32/apr_arch_utf8.h"
#include <wchar.h>
#include <string.h>

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
 *  Syntax: testucs [w|n]
 *
 *  If arg is not recognized, run both tests.
 */
int main(int argc, char **argv)
{
    struct testval s;
    memset (&s, 0, sizeof(s));

    if (argc < 2 || apr_tolower(*argv[1]) != 'w') {
        printf ("\n\nTesting Narrow Char Ranges\n");
        test_nrange(&s);
    }
    if (argc < 2 || apr_tolower(*argv[1]) != 'n') {
        printf ("\n\nTesting Wide Char Ranges\n");
        test_wrange(&s);
    }
    return 0;
}
