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

/*
 * This attempts to generate V1 UUIDs according to the Internet Draft
 * located at http://www.webdav.org/specs/draft-leach-uuids-guids-01.txt
 */

#include <unistd.h>     /* for getpid, gethostname */
#include <stdlib.h>     /* for rand, srand */
#include <sys/time.h>   /* for gettimeofday */

#include "apr.h"
#include "apr_private.h"
#include "apr_uuid.h"
#include "apr_md5.h"
#include "apr_general.h"
#if APR_HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#define NODE_LENGTH 6

static int uuid_state_seqnum;
static unsigned char uuid_state_node[NODE_LENGTH] = { 0 };


static void get_random_info(unsigned char node[NODE_LENGTH])
{
#if APR_HAS_RANDOM

    (void) apr_generate_random_bytes(node, NODE_LENGTH);

#else

    unsigned char seed[MD5_DIGESTSIZE];
    apr_md5_ctx_t c;

    /* ### probably should revise some of this to be a bit more portable */

    /* Leach & Salz use Linux-specific struct sysinfo;
     * replace with pid/tid for portability (in the spirit of mod_unique_id) */
    struct {
	/* Add thread id here, if applicable, when we get to pthread or apr */
	pid_t pid;
        struct timeval t;
        char hostname[257];

    } r;

    apr_MD5Init(&c);
    r.pid = getpid();
    gettimeofday(&r.t, (struct timezone *)0);
    gethostname(r.hostname, 256);
    apr_MD5Update(&c, (const unsigned char *)&r, sizeof(r));
    apr_MD5Final(seed, &c);

    memcpy(node, seed, NODE_LENGTH);    /* use a subset of the seed bytes */
#endif
}

/* This implementation generates a random node ID instead of a
   system-dependent call to get IEEE node ID. This is also more secure:
   we aren't passing out our MAC address.
*/
static void get_pseudo_node_identifier(unsigned char *node)
{
    get_random_info(node);
    node[0] |= 0x80;                    /* this designates a random node ID */
}

static void get_system_time(apr_uint64_t *uuid_time)
{
    struct timeval tp;

    /* ### fix this call to be more portable? */
    gettimeofday(&tp, (struct timezone *)0);

    /* Offset between UUID formatted times and Unix formatted times.
       UUID UTC base time is October 15, 1582.
       Unix base time is January 1, 1970.      */
    *uuid_time = (tp.tv_sec * 10000000) + (tp.tv_usec * 10) +
        0x01B21DD213814000LL;
}

/* true_random -- generate a crypto-quality random number. */
static int true_random(void)
{
    apr_uint64_t time_now;

#if APR_HAS_RANDOM
    unsigned char buf[2];

    if (apr_generate_random_bytes(buf, 2) == APR_SUCCESS) {
        return (buf[0] << 8) | buf[1];
    }
#endif

    /* crap. this isn't crypto quality, but it will be Good Enough */

    get_system_time(&time_now);
    srand((unsigned int)(((time_now >> 32) ^ time_now) & 0xffffffff));

    return rand() & 0x0FFFF;
}

static void init_state(void)
{
    uuid_state_seqnum = true_random();
    get_pseudo_node_identifier(uuid_state_node);
}

static void get_current_time(apr_uint64_t *timestamp)
{
    /* ### this needs to be made thread-safe! */

    apr_uint64_t time_now;
    static apr_uint64_t time_last = 0;
    static int fudge = 0;

    get_system_time(&time_now);
        
    /* if clock reading changed since last UUID generated... */
    if (time_last != time_now) {
        /* The clock reading has changed since the last UUID was generated.
           Reset the fudge factor. if we are generating them too fast, then
           the fudge may need to be reset to something greater than zero. */
        if (time_last + fudge > time_now)
            fudge = time_last + fudge - time_now + 1;
        else
            fudge = 0;
        time_last = time_now;
    }
    else {
        /* We generated two really fast. Bump the fudge factor. */
        ++fudge;
    }

    *timestamp = time_now + fudge;
}

void apr_get_uuid(apr_uuid_t *uuid)
{
    apr_uint64_t timestamp;
    unsigned char *d = uuid->data;

    if (!uuid_state_node[0])
        init_state();

    get_current_time(&timestamp);

    d[0] = (unsigned char)timestamp;
    d[1] = (unsigned char)(timestamp >> 8);
    d[2] = (unsigned char)(timestamp >> 16);
    d[3] = (unsigned char)(timestamp >> 24);
    d[4] = (unsigned char)(timestamp >> 32);
    d[5] = (unsigned char)(timestamp >> 40);
    d[6] = (unsigned char)(timestamp >> 48);
    d[7] = (unsigned char)(((timestamp >> 56) & 0x0F) | 0x10);

    d[8] = (unsigned char)(((uuid_state_seqnum >> 8) & 0x3F) | 0x80);
    d[9] = (unsigned char)uuid_state_seqnum;

    memcpy(&d[10], uuid_state_node, NODE_LENGTH);
}
