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
#ifdef  SVR4
#include <inttypes.h>
#define arc4random() rand()
#define seedrandom(a) srand(a)
#else
#include <stdint.h>
#define arc4random() random()
#define seedrandom(a) srandom(a)
#endif
#define _open(a,b,c) open(a,b,c)


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
static uint32_t randseed=0;

static int gettemp(char *path, register int *doopen, int domkdir, int slen)
{
	register char *start, *trv, *suffp;
	char *pad;
	struct stat sbuf;
	int rval;
	uint32_t randnum;

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
				rval = stat(path, &sbuf);
				*trv = '/';
				if (rval != 0)
					return(0);
				if (!S_ISDIR(sbuf.st_mode)) {
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
			if ((*doopen =
			    _open(path, O_CREAT|O_EXCL|O_RDWR, 0600)) >= 0)
				return(1);
			if (errno != EEXIST)
				return(0);
		} else if (domkdir) {
			if (mkdir(path, 0700) == 0)
				return(1);
			if (errno != EEXIST)
				return(0);
		} else if (lstat(path, &sbuf))
			return(errno == ENOENT ? 1 : 0);

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

apr_status_t apr_file_mktemp(apr_file_t **fp, char *template, apr_pool_t *p)
{
    int fd;
#ifndef HAVE_MKSTEMP
    int rv;
#endif

    (*fp) = apr_pcalloc(p, sizeof(**fp));
    (*fp)->cntxt = p;
    (*fp)->timeout = -1;
    (*fp)->blocking = BLK_ON;
    (*fp)->flags = APR_READ | APR_WRITE | APR_EXCL | APR_DELONCLOSE;

#ifndef HAVE_MKSTEMP
    rv = gettemp(path, &fd, 0, 0);
    if (rv == 0) {
        return errno;
    }
#else
    fd = mkstemp(template);
    if (fd == -1) {
        return errno;
    }
#endif
    (*fp)->fname = apr_pstrdup(p, template);
    (*fp)->filedes = fd;
    unlink((*fp)->fname);
    apr_pool_cleanup_register((*fp)->cntxt, (void *)(*fp),
                              apr_unix_file_cleanup, apr_unix_file_cleanup);
    return APR_SUCCESS;
}

#endif /* !defined(HAVE_MKSTEMP) */
