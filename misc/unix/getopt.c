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

APR_VAR_EXPORT int
    apr_opterr = 1,                      /* if error message should be printed */
    apr_optind = 1,                      /* index into parent argv vector */
    apr_optopt,                          /* character checked for validity */
    apr_optreset;                        /* reset getopt */
APR_VAR_EXPORT char *apr_optarg = "";    /* argument associated with option */

#define EMSG    ""

APR_EXPORT(apr_status_t) apr_getopt(apr_int32_t nargc, char *const *nargv, 
                                    const char *ostr, apr_int32_t *rv, 
                                    apr_pool_t *cont)
{
    char *p;
    static char *place = EMSG;   /* option letter processing */
    char *oli;                   /* option letter list index */

    if (apr_optreset || !*place) {   /* update scanning pointer */
        apr_optreset = 0;
        if (apr_optind >= nargc || *(place = nargv[apr_optind]) != '-') {
            place = EMSG;
            *rv = apr_optopt;
            return (APR_EOF);
        }
        if (place[1] && *++place == '-') {        /* found "--" */
            ++apr_optind;
            place = EMSG;
            *rv = apr_optopt;
            return (APR_EOF);
        }
    }                                /* option letter okay? */
    if ((apr_optopt = (int) *place++) == (int) ':' ||
        !(oli = strchr(ostr, apr_optopt))) {
        /*
         * if the user didn't specify '-' as an option,
         * assume it means -1.
         */
        if (apr_optopt == (int) '-') {
            *rv = apr_optopt;
            return (APR_EOF);
        }
        if (!*place)
            ++apr_optind;
        if (apr_opterr && *ostr != ':') {
            if (!(p = strrchr(*nargv, '/')))
                p = *nargv;
            else
                ++p;
            (void) fprintf(stderr,
                           "%s: illegal option -- %c\n", p, apr_optopt);
        }
        *rv = apr_optopt;
        return APR_BADCH;
    }
    if (*++oli != ':') {        /* don't need argument */
        apr_optarg = NULL;
        if (!*place)
            ++apr_optind;
    }
    else {                        /* need an argument */
        if (*place)                /* no white space */
            apr_optarg = place;
        else if (nargc <= ++apr_optind) {        /* no arg */
            place = EMSG;
            if (*ostr == ':') {
                *rv = apr_optopt;
                return (APR_BADARG);
            }
            if (apr_opterr) {
                if (!(p = strrchr(*nargv, '/')))
                    p = *nargv;
                else
                    ++p;
                (void) fprintf(stderr,
                               "%s: option requires an argument -- %c\n",
                               p, apr_optopt);
            }
            *rv = apr_optopt;
            return (APR_BADCH);
        }
        else                        /* white space */
            apr_optarg = nargv[apr_optind];
        place = EMSG;
        ++apr_optind;
    }
    *rv = apr_optopt;
    return APR_SUCCESS;
}


