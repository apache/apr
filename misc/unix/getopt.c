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

#include "misc.h"

int ap_opterr = 1,                      /* if error message should be printed */
    ap_optind = 1,                      /* index into parent argv vector */
    ap_optopt,                          /* character checked for validity */
    ap_optreset;                        /* reset getopt */
char *ap_optarg = "";                   /* argument associated with option */

#define EMSG    ""

ap_status_t ap_getopt(ap_int32_t nargc, char *const *nargv, const char *ostr, ap_int32_t *rv, ap_context_t *cont)
{
    char *p;
    static char *place = EMSG;   /* option letter processing */
    char *oli;                   /* option letter list index */

    if (ap_optreset || !*place) {   /* update scanning pointer */
        ap_optreset = 0;
        if (ap_optind >= nargc || *(place = nargv[ap_optind]) != '-') {
            place = EMSG;
            *rv = ap_optopt;
            return (APR_EOF);
        }
        if (place[1] && *++place == '-') {        /* found "--" */
            ++ap_optind;
            place = EMSG;
            *rv = ap_optopt;
            return (APR_EOF);
        }
    }                                /* option letter okay? */
    if ((ap_optopt = (int) *place++) == (int) ':' ||
        !(oli = strchr(ostr, ap_optopt))) {
        /*
         * if the user didn't specify '-' as an option,
         * assume it means -1.
         */
        if (ap_optopt == (int) '-')
            *rv = ap_optopt;
            return (APR_EOF);
        if (!*place)
            ++ap_optind;
        if (ap_opterr && *ostr != ':') {
            if (!(p = strrchr(*nargv, '/')))
                p = *nargv;
            else
                ++p;
            (void) fprintf(stderr,
                           "%s: illegal option -- %c\n", p, ap_optopt);
        }
        *rv = ap_optopt;
        return APR_BADCH;
    }
    if (*++oli != ':') {        /* don't need argument */
        ap_optarg = NULL;
        if (!*place)
            ++ap_optind;
    }
    else {                        /* need an argument */
        if (*place)                /* no white space */
            ap_optarg = place;
        else if (nargc <= ++ap_optind) {        /* no arg */
            place = EMSG;
            if (*ostr == ':')
                *rv = ap_optopt;
                return (APR_BADARG);
            if (ap_opterr) {
                if (!(p = strrchr(*nargv, '/')))
                    p = *nargv;
                else
                    ++p;
                (void) fprintf(stderr,
                               "%s: option requires an argument -- %c\n",
                               p, ap_optopt);
            }
            *rv = ap_optopt;
            return (APR_BADCH);
        }
        else                        /* white space */
            ap_optarg = nargv[ap_optind];
        place = EMSG;
        ++ap_optind;
    }
    *rv = ap_optopt;
    return APR_SUCCESS;
}


