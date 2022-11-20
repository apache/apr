/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apr.h"
#include "apr_private.h"
#include "apr_errno.h"
#include "apr_arch_utf8.h"

/* Implementation of RFC 3629, "UTF-8, a transformation format of ISO 10646"
 * with particular attention to canonical translation forms (see section 10
 * "Security Considerations" of the RFC for more info).
 *
 * Since several architectures including Windows support unicode, with utf-16
 * used as the actual storage conventions by that archicture, these functions
 * exist to transform or validate utf-16 strings into APR's 'char' type
 * convention.  It is left up to the operating system to determine the
 * validitity of the string, e.g. normative forms, in the context of
 * its native language support.  Other file systems which support filename
 * characters of 0x80-0xff but have no explicit requirement for Unicode
 * will find this function useful only for validating the character sequences
 * and rejecting poorly encoded utf-8 sequences.
 *
 * len  utf-4 range (hex)  utf-8 octet sequence (binary)
 * 1:2  00000000-0000007F  0xxxxxxx
 * 2:2  00000080-000007FF  110XXXXx 10xxxxxx
 * 3:2  00000800-0000FFFF  1110XXXX 10Xxxxxx 10xxxxxx
 * 4:4  00010000-001FFFFF  11110XXX 10XXxxxx 10xxxxxx 10xxxxxx
 *      00200000-03FFFFFF  111110XX 10XXXxxx 10xxxxxx 10xxxxxx 10xxxxxx
 *      04000000-7FFFFFFF  1111110X 10XXXXxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 *
 * One of the X bits must be 1 to avoid overlong representation in utf-8.
 *
 * For conversion into utf-16, the 4th form is limited in range to 0010 FFFF,
 * and the final two forms are used only by full utf-32, per RFC 3629;
 *
 *   "Pairs of UCS-2 values between D800 and DFFF (surrogate pairs in
 *   Unicode parlance), being actually UCS-4 characters transformed
 *   through UTF-16, need special treatment: the UTF-16 transformation
 *   must be undone, yielding a UCS-4 character that is then transformed
 *   as above."
 *
 * From RFC2781 UTF-16: the compressed ISO 10646 encoding bitmask
 *
 *   U' = U - 0x10000
 *   U' = 00000000 0000yyyy yyyyyyxx xxxxxxxx
 *                     W1 = 110110yy yyyyyyyy
 *                     W2 = 110111xx xxxxxxxx
 *   Max U' = 0000 00001111 11111111 11111111
 *   Max U  = 0000 00010000 11111111 11111111
 *
 * Also note ISO/IEC 10646:2014 Clause 9.4: "Because surrogate code points
 * are not UCS scalar values, UTF-32 code units in the range
 * 0000 D800-0000 DFFF are ill-formed" for future reference in adding any
 * utf-32 accessor functions.
 *
 * Len is the table above is a mapping of bytes used for utf-8:utf-16 values,
 * which results in these conclusions of maximum allocations;
 *
 * apr_conv_utf8_to_utf16 out bytes:sizeof(in) * 1 <= Req <= sizeof(in) * 2
 * apr_conv_utf16_to_utf8 out words:sizeof(in) / 2 <= Req <= sizeof(in) * 3 / 2
 */

APR_DECLARE(apr_status_t) apr_conv_utf8_to_utf16(const char *in,
                                                 apr_size_t *inbytes,
                                                 apr_wchar_t *out,
                                                 apr_size_t *outwords)
{
    apr_int64_t newch, mask;
    apr_size_t expect, eating;
    int ch;

    while (*inbytes && *outwords)
    {
        ch = (unsigned char)(*in++);
        if (!(ch & 0200)) {
            /* US-ASCII-7 plain text
             */
            --*inbytes;
            --*outwords;
            *(out++) = ch;
        }
        else
        {
            if ((ch & 0300) != 0300) {
                /* Multibyte Continuation is out of place
                 */
                return APR_EINVAL;
            }
            else
            {
                /* Multibyte Sequence Lead Character
                 *
                 * Compute the expected bytes while adjusting
                 * or lead byte and leading zeros mask.
                 */
                mask = 0340;
                expect = 1;
                while ((ch & mask) == mask) {
                    mask |= mask >> 1;
                    if (++expect > 3) /* (or 5 for a utf-32 code point) */
                        return APR_EINVAL;
                }
                newch = ch & ~mask;
                eating = expect + 1;
                if (*inbytes <= expect)
                    return APR_INCOMPLETE;
                /* Reject values of excessive leading 0 bits
                 * utf-8 _demands_ the shortest possible byte length
                 */
                if (expect == 1) {
                    if (!(newch & 0036))
                        return APR_EINVAL;
                }
                else {
                    /* Reject values of excessive leading 0 bits
                     */
                    if (!newch && !((unsigned char)*in & 0077 & (mask << 1)))
                        return APR_EINVAL;
                    if (expect == 2) {
                        /* Reject values D800-DFFF when not utf-16 encoded
                         */
                        if (newch == 0015 && ((unsigned char)*in & 0040))
                            return APR_EINVAL;
                    }
                    else if (expect == 3) {
                        /* Short circuit values > 110000
                         */
                        if (newch > 4)
                            return APR_EINVAL;
                        if (newch == 4 && ((unsigned char)*in & 0060))
                            return APR_EINVAL;
                    }
                }
                /* Where the boolean (expect > 2) is true, we will need
                 * an extra word for the output.
                 */
                if (*outwords < (apr_size_t)(expect > 2) + 1)
                    break; /* buffer full */
                while (expect--)
                {
                    /* Multibyte Continuation must be legal */
                    if (((ch = (unsigned char)*(in++)) & 0300) != 0200)
                        return APR_EINVAL;
                    newch <<= 6;
                    newch |= (ch & 0077);
                }
                *inbytes -= eating;
                /* newch is now a true utf-32 character
                 *
                 * now we need to fold to utf-16
                 */
                if (newch < 0x10000)
                {
                    --*outwords;
                    *(out++) = (apr_wchar_t) newch;
                }
                else
                {
                    *outwords -= 2;
                    newch -= 0x10000;
                    *(out++) = (apr_wchar_t) (0xD800 | (newch >> 10));
                    *(out++) = (apr_wchar_t) (0xDC00 | (newch & 0x03FF));
                }
            }
        }
    }
    /* Buffer full 'errors' aren't errors, the client must inspect both
     * the inbytes and outwords values
     */
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) apr_conv_utf16_to_utf8(const apr_wchar_t *in,
                                                 apr_size_t *inwords,
                                                 char *out,
                                                 apr_size_t *outbytes)
{
    apr_int64_t newch, require;
    apr_size_t need;
    char *invout;
    int ch;

    while (*inwords && *outbytes)
    {
        ch = (unsigned short)(*in++);
        if (ch < 0x80)
        {
            --*inwords;
            --*outbytes;
            *(out++) = (unsigned char) ch;
        }
        else
        {
            if ((ch & 0xFC00) == 0xDC00) {
                /* Invalid Leading utf-16 Multiword Continuation Character
                 */
                return APR_EINVAL;
            }
            if ((ch & 0xFC00) == 0xD800) {
                /* Leading utf-16 Multiword Character
                 */
                if (*inwords < 2) {
                    /* Missing utf-16 Multiword Continuation Character
                     */
                    return APR_INCOMPLETE;
                }
                if (((unsigned short)(*in) & 0xFC00) != 0xDC00) {
                    /* Invalid utf-16 Multiword Continuation Character
                     */
                    return APR_EINVAL;
                }
                newch = (ch & 0x03FF) << 10 | ((unsigned short)(*in++) & 0x03FF);
                newch += 0x10000;
            }
            else {
                /* utf-16 Single Word Character
                 */
                newch = ch;
            }
            /* Determine the absolute minimum utf-8 bytes required
             */
            require = newch >> 11;
            need = 1;
            while (require)
                require >>= 5, ++need;
            if (need >= *outbytes)
                break; /* Insufficient buffer */
            *inwords -= (need > 2) + 1;
            *outbytes -= need + 1;
            /* Compute the utf-8 characters in last to first order,
             * calculating the lead character length bits along the way.
             */
            ch = 0200;
            out += need + 1;
            invout = out;
            while (need--) {
                ch |= ch >> 1;
                *(--invout) = (unsigned char)(0200 | (newch & 0077));
                newch >>= 6;
            }
            /* Compute the lead utf-8 character and move the dest offset
             */
            *(--invout) = (unsigned char)(ch | newch);
        }
    }
    /* Buffer full 'errors' aren't errors, the client must inspect both
     * the inwords and outbytes values
     */
    return APR_SUCCESS;
}
