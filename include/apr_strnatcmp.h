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

#ifndef APR_STRNATCMP_H
#define APR_STRNATCMP_H

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

#ifdef __cplusplus
}
#endif

#endif  /* !APR_STRNATCMP_H */
