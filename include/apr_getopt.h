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

API_VAR_IMPORT int
    ap_opterr,                          /* if error message should be printed */
    ap_optind,                          /* index into parent argv vector */
    ap_optopt,                          /* character checked for validity */
    ap_optreset;                        /* reset getopt */
API_VAR_IMPORT char *
    ap_optarg;                          /* argument associated with option */

/*

=head1 ap_status_t ap_getopt(ap_int32_t nargc, char *const *nargv, const char *ostr, ap_int32_t *rv, ap_pool_t *cont)

B<Parse the command line options passed to the program.>

    arg 1) The number of arguments passed to ap_getopt to parse
    arg 2) The array of command line options to parse
    arg 3) A string of characters that are acceptable options to the program.
           characters followed by ":" are required to have an option 
           associated 
    arg 4) The next option found.  There are four potential values for 
           this variable on exit. They are:
               APR_EOF    --  No more options to parse
               APR_BADCH  --  Found a bad option character
               APR_BADARG --  Missing parameter for the found option
               Other      --  The next option found.
    arg 5) The pool to operate on.

B<NOTE>:  Arguments 2 and 3 are most commonly argc and argv from 
          main(argc, argv)

=cut
 */
ap_status_t ap_getopt(ap_int32_t nargc, char *const *nargv, const char *ostr,
                      ap_int32_t *rv, ap_pool_t *cont);

#endif  /* ! APR_GETOPT_H */


