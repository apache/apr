/* -*- mode: c; c-file-style: "k&r" -*-

  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
  Copyright (C) 2000 by Martin Pool <mbp@humbug.org.au>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "apr.h"
#include "apr_lib.h"

#ifndef APR_STRINGS_H
#define APR_STRINGS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*

=head1 int ap_strnatcmp(char const *a, char const *b)

B<Do a natural order comparison of two strings.>

    arg 1) The first string to compare
    arg 2) The second string to compare
    return) Either <0, 0, or >0.  If the first string is less than the second
            this returns <0, if they are equivalent it returns 0, and if the
            first string is greater than second string it retuns >0.

=cut
 */
int ap_strnatcmp(char const *a, char const *b);

/*

=head1 int ap_strnatcmp(char const *a, char const *b)

B<Do a natural order comparison of two strings ignoring the case of the strings.>

    arg 1) The first string to compare
    arg 2) The second string to compare
    return) Either <0, 0, or >0.  If the first string is less than the second
            this returns <0, if they are equivalent it returns 0, and if the
            first string is greater than second string it retuns >0.

=cut
 */
int ap_strnatcasecmp(char const *a, char const *b);

/*

=head1 char *ap_pstrdup(ap_pool_t *c, const char *s)

B<duplicate a string into memory allocated out of a pool>

    arg 1) The pool to allocate out of
    arg 2) The string to allocate
    return) The new string

=cut
 */
APR_EXPORT(char *) ap_pstrdup(ap_pool_t *p, const char *s);

/*

=head1 char *ap_pstrndup(ap_pool_t *c, const char *s, ap_size_t n)

B<duplicate the first n characters ofa string into memory allocated out of a poo
l>

    arg 1) The pool to allocate out of
    arg 2) The string to allocate
    arg 3) The number of characters to duplicate
    return) The new string

=cut
 */
APR_EXPORT(char *) ap_pstrndup(ap_pool_t *p, const char *s, ap_size_t n);

/*
=head1 char *ap_pstrcat(ap_pool_t *c, ...)

B<Concatenate multiple strings, allocating memory out a pool>

    arg 1) The pool to allocate out of
    ...) The strings to concatenate.  The final string must be NULL
    return) The new string

=cut
 */
APR_EXPORT_NONSTD(char *) ap_pstrcat(ap_pool_t *p, ...);

/*

=head1 char *ap_pvsprintf(ap_pool_t *c, const char *fmt, va_list ap)
B<printf-style style printing routine.  The data is output to a string allocated
 from a pool>

    arg 1) The pool to allocate out of
    arg 2) The format of the string
    arg 3) The arguments to use while printing the data
    return) The new string

=cut
 */
APR_EXPORT(char *) ap_pvsprintf(ap_pool_t *p, const char *fmt, va_list ap);

/*

=head1 char *ap_psprintf(ap_pool_t *c, const char *fmt, ...)

B<printf-style style printing routine.  The data is output to a string allocated from a pool>

    arg 1) The pool to allocate out of
    arg 2) The format of the string
    ...) The arguments to use while printing the data
    return) The new string

=cut
 */
APR_EXPORT_NONSTD(char *) ap_psprintf(ap_pool_t *p, const char *fmt, ...);

/*

=head1 char *ap_cpystrn(char *dst, const char *src, size_t dst_size)

B<copy n characters from src to dest>

    arg 1) The destination string
    arg 2) The source string
    arg 3) The number of characters to copy

B<NOTE>:  We re-implement this function to implement these specific changes:
        1) strncpy() doesn't always null terminate and we want it to.
        2) strncpy() null fills, which is bogus, esp. when copy 8byte strings
           into 8k blocks.
        3) Instead of returning the pointer to the beginning of the
           destination string, we return a pointer to the terminating '\0'
           to allow us to check for truncation.

=cut
 */
APR_EXPORT(char *) ap_cpystrn(char *dst, const char *src, size_t dst_size);

#ifdef __cplusplus
}
#endif

#endif  /* !APR_STRINGS_H */
