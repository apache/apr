/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
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
 *
 *	@(#)fnmatch.h	8.1 (Berkeley) 6/2/93
 */

/* This file has been modified by the Apache Software Foundation. */
#ifndef	_APR_FNMATCH_H_
#define	_APR_FNMATCH_H_

#include "apr_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

#define	FNM_NOMATCH	1	/* Match failed. */

#define	FNM_NOESCAPE	0x01	/* Disable backslash escaping. */
#define	FNM_PATHNAME	0x02	/* Slash must be matched by slash. */
#define	FNM_PERIOD	0x04	/* Period must be matched by period. */
/* This flag is an Apache addition */
#define FNM_CASE_BLIND  0x08    /* Compare characters case-insensitively. */

/*

=head1 ap_status_t ap_fnmatch(const char *pattern, const char *strings, int flags)

B<Try to match the string to the given pattern.>

    arg 1) The pattern to match to
    arg 2) The string we are trying to match
    arg 3) flags to use in the match.  Bitwise OR of:
                FNM_NOESCAPE   --  Disable backslash escaping
                FNM_PATHNAME   --  Slash must be matched by slash
                FNM_PERIOD     --  Period must be matched by period
                FNM_CASE_BLIND --  Compare characters case-insensitively.

=cut
 */

APR_EXPORT(ap_status_t) ap_fnmatch(const char *pattern, const char *strings,
			    int flags);

/*

=head1 ap_status_t ap_is_fnmatch(const char *pattern)

B<Determine if the given pattern is a regular expression.>

    arg 1) The pattern to search for glob characters.

=cut
 */
APR_EXPORT(int) ap_is_fnmatch(const char *pattern);

#ifdef __cplusplus
}
#endif

#endif /* !_FNMATCH_H_ */
