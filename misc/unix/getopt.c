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
#include "apr_strings.h"

#define EMSG    ""

/* Regardless of what we're invoked as, just print out the last part
 * of the path */
static const char *pretty_path (const char *name) 
{
    const char *p;
    if (!(p = strrchr(name, '/')))
        return p;
    else
        return ++p;
}

APR_DECLARE(apr_status_t) apr_initopt(apr_getopt_t **os, apr_pool_t *cont,
                                     int argc, char *const *argv)
{
    *os = apr_palloc(cont, sizeof(apr_getopt_t));
    (*os)->cont = cont;
    (*os)->err = 1;
    (*os)->ind = 1;
    (*os)->place = EMSG;
    (*os)->argc = argc;
    (*os)->argv = argv;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_getopt(apr_getopt_t *os, const char *opts, 
                                    char *optch, const char **optarg)
{
    const char *oli;  /* option letter list index */

    if (os->reset || !*os->place) {   /* update scanning pointer */
        os->reset = 0;
        if (os->ind >= os->argc || *(os->place = os->argv[os->ind]) != '-') {
            os->place = EMSG;
            *optch = os->opt;
            return (APR_EOF);
        }
        if (os->place[1] && *++os->place == '-') {        /* found "--" */
            ++os->ind;
            os->place = EMSG;
            *optch = os->opt;
            return (APR_EOF);
        }
    }                                /* option letter okay? */
    if ((os->opt = (int) *os->place++) == (int) ':' ||
        !(oli = strchr(opts, os->opt))) {
        /*
         * if the user didn't specify '-' as an option,
         * assume it means -1.
         */
        if (os->opt == (int) '-') {
            *optch = os->opt;
            return (APR_EOF);
        }
        if (!*os->place)
            ++os->ind;
        if (os->err && *opts != ':') {
            (void) fprintf(stderr,
                           "%s: illegal option -- %c\n",
                           pretty_path(*os->argv), os->opt);
        }
        *optch = os->opt;
        return (APR_BADCH);
    }
    if (*++oli != ':') {        /* don't need argument */
        *optarg = NULL;
        if (!*os->place)
            ++os->ind;
    }
    else {                        /* need an argument */
        if (*os->place)                /* no white space */
            *optarg = os->place;
        else if (os->argc <= ++os->ind) {        /* no arg */
            os->place = EMSG;
            if (*opts == ':') {
                *optch = os->opt;
                return (APR_BADARG);
            }
            if (os->err) {
                (void) fprintf(stderr,
                               "%s: option requires an argument -- %c\n",
                               pretty_path(*os->argv), os->opt);
            }
            *optch = os->opt;
            return (APR_BADCH);
        }
        else                        /* white space */
            *optarg = os->argv[os->ind];
        os->place = EMSG;
        ++os->ind;
    }
    *optch = os->opt;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_getopt_long(apr_getopt_t *os, 
                                          const char *opts, 
                                          const apr_getopt_long_t *long_opts,
                                          int *optval, 
                                          const char **optarg)
     
{
    const apr_getopt_long_t *ptr;
    const char *opt = os->argv[os->ind];
    const char *arg = os->argv[os->ind +1];
    int arg_index_incr = 1;
  
    /* Finished processing opts */
    if (os->ind >= os->argc)
        return APR_EOF;

    /* End of options processing if we encounter "--" */
    if (strcmp(opt, "--") == 0)
        return APR_EOF;

    /* 
     * End of options processing if we encounter something that
     * doesn't start with "-" or "--" (it's not an option if we hit it
     * here, it's an argument) 
     */
    if (*opt != '-')
        return APR_EOF;

    if ((os->ind + 1) >= os->argc) 
        arg = NULL;

    /* Handle --foo=bar style opts */
    if (strchr(opt, '=')) {
        const char *index = strchr(opt, '=') + 1;
        opt = apr_pstrndup(os->cont, opt, ((index - opt) - 1));
        if (*index != '\0') /* account for "--foo=" */
            arg = apr_pstrdup(os->cont, index);
        arg_index_incr = 0;
    }                       

    /* If it's a longopt */
    if (opt[1] == '-') {
        /* see if it's in our array of long opts */
        for (ptr = long_opts; ptr->name; ptr++) {
            if (strcmp((opt + 2), ptr->name) == 0) { /* it's in the array */
                if (ptr->has_arg) { 
                    if (((os->ind + 1) >= os->argc) 
                        && (arg == NULL)) {
                        fprintf(stderr,
                                "%s: option requires an argument: %s\n",
                                pretty_path(*os->argv), opt);
                        return APR_BADARG;
                    }
                                
                    /* If we make it here, then we should be ok. */
                    *optarg = arg;
                    os->ind += arg_index_incr;
                }
                else { /* has no arg */ 
                    *optarg = NULL;
                }
                *optval = ptr->val;
                ++os->ind;
                return APR_SUCCESS;
            } 
        }

        /* If we get here, then we don't have the longopt in our
         * longopts array 
         */
        fprintf(stderr, "%s: illegal option: %s\n", 
                pretty_path(*os->argv), opt);
        return APR_BADCH;
    }

    {   /* otherwise, apr_getopt gets it. */
        char optch;
        apr_status_t status;
        status = apr_getopt (os, opts, &optch, optarg);
        *optval = optch;
        return status;
    }
}




