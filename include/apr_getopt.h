/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000 The Apache Software Foundation.  All rights
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

#ifndef APR_GETOPT_H
#define APR_GETOPT_H

/**
 * @package APR command arguments
 */ 

typedef struct apr_getopt_t apr_getopt_t;
/**
 * Structure to store command line argument information.
 */ 
struct apr_getopt_t {
    /** context for processing */
    apr_pool_t *cont;
    /** if error message should be printed */
    int err;
    /** index into parent argv vector */
    int ind;
    /** character checked for validity */
    int opt;
    /** reset getopt */
    int reset;
    /** count of arguments */
    int argc;
    /** array of pointers to arguments */
    char **argv;
    /** argument associated with option */
    char const* place;
    /** set to nonzero to support interleaving */
    int interleave;
    /** range of non-option arguments skipped for interleaving */
    int skip_start;
    int skip_end;
};

typedef struct apr_longopt_t apr_longopt_t;

struct apr_longopt_t {
    /** long option name, or NULL if option has no long name */
    const char *name;
    /** option letter, or a value greater than 255 if option has no letter */
    int optch;
    /** nonzero if option takes an argument */
    int has_arg;
};

/**
 * Initialize the arguments for parsing by apr_getopt().
 * @param cont The pool to operate on
 * @param os   The options structure created for apr_getopt()
 * @param argc The number of arguments to parse
 * @param argv The array of arguments to parse
 * @tip Arguments 2 and 3 are most commonly argc and argv from main(argc, argv)
 * @deffunc apr_status_t apr_initopt(apr_getopt_t **os, apr_pool_t *cont,int argc, char *const *argv)
 */
APR_DECLARE(apr_status_t) apr_initopt(apr_getopt_t **os, apr_pool_t *cont,
                                      int argc, char **argv);

/**
 * Parse the options initialized by apr_initopt().
 * @param os     The apr_opt_t structure returned by apr_initopt()
 * @param opts   A string of characters that are acceptable options to the 
 *               program.  Characters followed by ":" are required to have an 
 *               option associated
 * @param optch  The next option character parsed
 * @param optarg The argument following the option character:
 * @tip There are four potential status values on exit. They are:
 * <PRE>
 *             APR_EOF      --  No more options to parse
 *             APR_BADCH    --  Found a bad option character
 *             APR_BADARG   --  No argument followed @parameter:
 *             APR_SUCCESS  --  The next option was found.
 * </PRE>
 * @deffunc apr_status_t apr_getopt(apr_getopt_t *os, const char *opts, char *optch, const char **optarg)
 */
APR_DECLARE(apr_status_t) apr_getopt(apr_getopt_t *os, const char *opts, 
                                     char *optch, const char **optarg);

/**
 * Parse the options initialized by apr_initopt(), accepting long
 * options beginning with "--" in addition to single-character
 * options beginning with "-".
 * @param os     The apr_getopt_t structure created by apr_initopt()
 * @param opts   A pointer to a list of apr_longopt_t structures, which can
 *               be initialized with { "name", optch, has_args }.  has_args
 *               is nonzero if the option requires an argument.  A structure
 *               with an optch value of 0 terminates the list.
 * @param optch  Receives the value of "optch" from the apr_longopt_t structure
 *               corresponding to the next option matched.
 * @param optarg Receives the argument following the option, if any.
 * @tip There are four potential status values on exit.   They are:
 * <PRE>
 *             APR_EOF      --  No more options to parse
 *             APR_BADCH    --  Found a bad option character
 *             APR_BADARG   --  No argument followed @parameter:
 *             APR_SUCCESS  --  The next option was found.
 * </PRE>
 * When APR_SUCCESS is returned, os->ind gives the index of the first
 * non-option argument.  On error, a message will be printed to stdout unless
 * os->err is set to 0.  If os->interleave is set to nonzero, options can come
 * after arguments, and os->argv will be permuted to leave non-option arguments
 * at the end.
 * @deffunc apr_status_t apr_getopt_long(apr_getopt_t *os, const apr_longopt_t *opts, int *optch, const char **optarg)
 */
APR_DECLARE(apr_status_t) apr_getopt_long(apr_getopt_t *os,
					  const apr_longopt_t *opts,
					  int *optch, const char **optarg);
#endif  /* ! APR_GETOPT_H */
