/*
 * Copyright (c) 1987, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"

int opterr = 1,                      /* if error message should be printed */
    optind = 1,                      /* index into parent argv vector */
    optopt,                          /* character checked for validity */
    optreset;                        /* reset getopt */
char *optarg;                        /* argument associated with option */

#define EMSG    ""

/* ***APRDOC********************************************************
 * ap_status_t ap_getopt(ap_context_t *, ap_int32_t, char *const *, 
 *                       const char *, ap_int32_t)
 *    Parse the command line options passed to the program.
 * arg 1) The context to operate on.
 * arg 2) The number of arguments passed to ap_getopt to parse
 * arg 3) The array of command line options to parse
 * arg 4) A string of characters that are acceptable options to the program.
 *        characters followed by ":" are required to have an option 
 *        associated 
 * arg 5) The next option found.  There are four potential values for 
 *        this variable on exit. They are:
 *            APR_EOF    --  No more options to parse
 *            APR_BADCH  --  Found a bad option character
 *            APR_BADARG --  Missing parameter for the found option
 *            Other      --  The next option found.
 * NOTE:  Arguments 2 and 3 are most commonly argc and argv from 
 *        main(argc, argv)
 */
ap_status_t ap_getopt(struct context_t *cont, ap_int32_t nargc, 
                      char *const *nargv, const char *ostr, ap_int32_t *rv)
{
    char *p;
    static char *place = EMSG;   /* option letter processing */
    char *oli;                   /* option letter list index */

    if (optreset || !*place) {   /* update scanning pointer */
        optreset = 0;
        if (optind >= nargc || *(place = nargv[optind]) != '-') {
            place = EMSG;
            *rv = optopt;
            return (APR_EOF);
        }
        if (place[1] && *++place == '-') {        /* found "--" */
            ++optind;
            place = EMSG;
            *rv = optopt;
            return (APR_EOF);
        }
    }                                /* option letter okay? */
    if ((optopt = (int) *place++) == (int) ':' ||
        !(oli = strchr(ostr, optopt))) {
        /*
         * if the user didn't specify '-' as an option,
         * assume it means -1.
         */
        if (optopt == (int) '-')
            *rv = optopt;
            return (APR_EOF);
        if (!*place)
            ++optind;
        if (opterr && *ostr != ':') {
            if (!(p = strrchr(*nargv, '/')))
                p = *nargv;
            else
                ++p;
            (void) fprintf(stderr,
                           "%s: illegal option -- %c\n", p, optopt);
        }
        *rv = optopt;
        return APR_BADCH;
    }
    if (*++oli != ':') {        /* don't need argument */
        optarg = NULL;
        if (!*place)
            ++optind;
    }
    else {                        /* need an argument */
        if (*place)                /* no white space */
            optarg = place;
        else if (nargc <= ++optind) {        /* no arg */
            place = EMSG;
            if (*ostr == ':')
                *rv = optopt;
                return (APR_BADARG);
            if (opterr) {
                if (!(p = strrchr(*nargv, '/')))
                    p = *nargv;
                else
                    ++p;
                (void) fprintf(stderr,
                               "%s: option requires an argument -- %c\n",
                               p, optopt);
            }
            *rv = optopt;
            return (APR_BADCH);
        }
        else                        /* white space */
            optarg = nargv[optind];
        place = EMSG;
        ++optind;
    }
    *rv = optopt;
    return APR_SUCCESS;
}


