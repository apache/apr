/*
 * Copyright (c) 1987, 1993
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
 */

#include "apr_private.h"
#include "apr_file_io.h" /* prototype of apr_mkstemp() */
#include "apr_strings.h" /* prototype of apr_mkstemp() */
#include "fileio.h" /* prototype of apr_mkstemp() */

#ifndef HAVE_MKSTEMP

#ifndef __warn_references
#define __warn_references(a,b) 
#endif
#if defined(SVR4) || defined(WIN32)
#ifdef SVR4
#include <inttypes.h>
#endif
#define arc4random() rand()
#define seedrandom(a) srand(a)
#else
#ifdef APR_HAS_STDINT_H
#include <stdint.h>
#endif
#define arc4random() random()
#define seedrandom(a) srandom(a)
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static const unsigned char padchar[] =
"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
static apr_uint32_t randseed=0;

static int gettemp(char *path, apr_file_t *doopen, int domkdir, int slen,
                   apr_pool_t *p)
{
	register char *start, *trv, *suffp;
	char *pad;
	apr_finfo_t sbuf;
    apr_status_t rv;
	apr_uint32_t randnum;

	if (doopen && domkdir) {
		errno = EINVAL;
		return(0);
	}
	if (randseed==0) {
		randseed = time(NULL);
		seedrandom(randseed);
	}

	for (trv = path; *trv; ++trv)
		;
	trv -= slen;
	suffp = trv;
	--trv;
	if (trv < path) {
		errno = EINVAL;
		return (0);
	}

	/* Fill space with random characters */
	while (*trv == 'X') {
		randnum = arc4random() % (sizeof(padchar) - 1);
		*trv-- = padchar[randnum];
	}
	start = trv + 1;

	/*
	 * check the target directory.
	 */
	if (doopen || domkdir) {
		for (;; --trv) {
			if (trv <= path)
				break;
			if (*trv == '/') {
				*trv = '\0';
				rv = apr_stat(&sbuf, path, APR_FINFO_TYPE, p);
				*trv = '/';
				if (rv != APR_SUCCESS)
					return(0);
				if (sbuf.filetype != APR_DIR) {
					errno = ENOTDIR;
					return(0);
				}
				break;
			}
		}
	}

	for (;;) {
		errno = 0;
		if (doopen) {
			if ((rv = apr_file_open(&doopen, path, APR_CREATE|APR_EXCL|APR_READ|APR_WRITE, 
                                    APR_UREAD | APR_UWRITE, p)) == APR_SUCCESS)
				return(1);
			if (errno != EEXIST)
				return(0);
		} else if (domkdir) {
			if (apr_dir_make(path, 
                                APR_UREAD | APR_UWRITE | APR_UEXECUTE, p) == 0)
				return(1);
			if (errno != EEXIST)
				return(0);
		} else if ((rv = apr_lstat(&sbuf, path, APR_FINFO_TYPE, p)) != APR_SUCCESS)
			return(rv == ENOENT ? 1 : 0);

		/* If we have a collision, cycle through the space of filenames */
		for (trv = start;;) {
			if (*trv == '\0' || trv == suffp)
				return(0);
			pad = strchr((char *)padchar, *trv);
			if (pad == NULL || !*++pad)
				*trv++ = padchar[0];
			else {
				*trv++ = *pad;
				break;
			}
		}
	}
	/*NOTREACHED*/
}

#else

#if APR_HAVE_STDLIB_H
#include <stdlib.h> /* for mkstemp() - Single Unix */
#endif
#if APR_HAVE_UNISTD_H
#include <unistd.h> /* for mkstemp() - FreeBSD */
#endif
#endif /* !defined(HAVE_MKSTEMP) */

APR_DECLARE(apr_status_t) apr_file_mktemp(apr_file_t **fp, char *template, apr_pool_t *p)
{
#ifndef HAVE_MKSTEMP
    int rv;
#else
    int fd;
#endif

#ifndef HAVE_MKSTEMP
    rv = gettemp(template, (*fp), 0, 0, p);
    if (rv == 0) {
        return errno;
    }
#else
    (*fp) = apr_pcalloc(p, sizeof(**fp));
    (*fp)->cntxt = p;
    (*fp)->timeout = -1;
    (*fp)->blocking = BLK_ON;
    (*fp)->flags = APR_READ | APR_WRITE | APR_EXCL | APR_DELONCLOSE;

    fd = mkstemp(template);
    if (fd == -1) {
        return errno;
    }
    (*fp)->fname = apr_pstrdup(p, template);
    (*fp)->filedes = fd;

#endif
    apr_file_remove((*fp)->fname, p);
#ifdef WIN32
    apr_pool_cleanup_register((*fp)->cntxt, (void *)(*fp),
                              file_cleanup, file_cleanup);
#elif defined(OS2)
    apr_pool_cleanup_register((*fp)->cntxt, (void *)(*fp),
                              apr_file_cleanup, apr_file_cleanup);
#else
    apr_pool_cleanup_register((*fp)->cntxt, (void *)(*fp),
                              apr_unix_file_cleanup, apr_unix_file_cleanup);
#endif
    return APR_SUCCESS;
}

