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

#include <stdio.h>      /* for sprintf */

#include "apr.h"
#include "apr_uuid.h"
#include "apr_errno.h"
#include "apr_lib.h"


void apr_format_uuid(char *buffer, const apr_uuid_t *uuid)
{
    const unsigned char *d = uuid->data;

    sprintf(buffer,
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
            d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
}

/* convert a pair of hex digits to an integer value [0,255] */
static unsigned char parse_hexpair(const char *s)
{
    int result;
    int temp;

    result = s[0] - '0';
    if (result > 48)
	result = (result - 39) << 4;
    else if (result > 16)
	result = (result - 7) << 4;
    else
	result = result << 4;

    temp = s[1] - '0';
    if (temp > 48)
	result |= temp - 39;
    else if (temp > 16)
	result |= temp - 7;
    else
	result |= temp;

    return (unsigned char)result;
}

apr_status_t apr_parse_uuid(apr_uuid_t *uuid, const char *uuid_str)
{
    int i;
    unsigned char *d = uuid->data;

    for (i = 0; i < 36; ++i) {
	char c = uuid_str[i];
	if (!apr_isxdigit(c) &&
	    !(c == '-' && (i == 8 || i == 13 || i == 18 || i == 23)))
            /* ### need a better value */
	    return APR_BADARG;
    }
    if (uuid_str[36] != '\0') {
        /* ### need a better value */
	return APR_BADARG;
    }

    d[0] = parse_hexpair(&uuid_str[0]);
    d[1] = parse_hexpair(&uuid_str[2]);
    d[2] = parse_hexpair(&uuid_str[4]);
    d[3] = parse_hexpair(&uuid_str[6]);

    d[4] = parse_hexpair(&uuid_str[9]);
    d[5] = parse_hexpair(&uuid_str[11]);

    d[6] = parse_hexpair(&uuid_str[14]);
    d[7] = parse_hexpair(&uuid_str[16]);

    d[8] = parse_hexpair(&uuid_str[19]);
    d[9] = parse_hexpair(&uuid_str[21]);

    for (i = 6; i--;)
	d[10 + i] = parse_hexpair(&uuid_str[i*2+24]);

    return APR_SUCCESS;
}
