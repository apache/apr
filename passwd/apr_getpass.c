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

/* apr_getpass.c: abstraction to provide for obtaining a password from the
 * command line in whatever way the OS supports.  In the best case, it's a
 * wrapper for the system library's getpass() routine; otherwise, we
 * use one we define ourselves.
 */
#include "apr_private.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_errno.h"
#include <sys/types.h>
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_CONIO_H
#include <conio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if defined(HAVE_TERMIOS_H) && !defined(HAVE_GETPASS)
#include <termios.h>
#endif

#if !APR_CHARSET_EBCDIC
#define LF 10
#define CR 13
#else /* APR_CHARSET_EBCDIC */
#define LF '\n'
#define CR '\r'
#endif /* APR_CHARSET_EBCDIC */

#define MAX_STRING_LEN 256

#define ERR_OVERFLOW 5

#ifndef HAVE_GETPASS

/* MPE, Win32 and BeOS all lack a native getpass() */

#if !defined(HAVE_TERMIOS_H) && !defined(WIN32)
/*
 * MPE lacks getpass() and a way to suppress stdin echo.  So for now, just
 * issue the prompt and read the results with echo.  (Ugh).
 */

static char *getpass(const char *prompt)
{
    static char password[MAX_STRING_LEN];

    fputs(prompt, stderr);
    gets((char *) &password);

    if (strlen((char *) &password) > (MAX_STRING_LEN - 1)) {
	password[MAX_STRING_LEN - 1] = '\0';
    }

    return (char *) &password;
}

#elif defined (HAVE_TERMIOS_H)
#include <stdio.h>

static char *getpass(const char *prompt)
{
    struct termios attr;
    static char password[MAX_STRING_LEN];
    int n=0;
    fputs(prompt, stderr);
    fflush(stderr);
	
    if (tcgetattr(STDIN_FILENO, &attr) != 0)
        return NULL;
	attr.c_lflag &= ~(ECHO);
    
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &attr) != 0)
		return NULL;
    while ((password[n] = getchar()) != '\n') {
        if (password[n] >= ' ' && password[n] <= '~') {
            n++;
        } else {
            fprintf(stderr,"\n");
            fputs(prompt, stderr);
            fflush(stderr);
            n = 0;
        }
    }
 
    password[n] = '\0';
    printf("\n");
    if (n > (MAX_STRING_LEN - 1)) {
        password[MAX_STRING_LEN - 1] = '\0';
    }

    attr.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &attr);
    return (char*) &password;
}

#else

/*
 * Windows lacks getpass().  So we'll re-implement it here.
 */

static char *getpass(const char *prompt)
{
    static char password[MAX_STRING_LEN];
    int n = 0;

    fputs(prompt, stderr);
    
    while ((password[n] = _getch()) != '\r') {
        if (password[n] >= ' ' && password[n] <= '~') {
            n++;
            printf("*");
        }
	else {
            printf("\n");
            fputs(prompt, stderr);
            n = 0;
        }
    }
 
    password[n] = '\0';
    printf("\n");

    if (n > (MAX_STRING_LEN - 1)) {
        password[MAX_STRING_LEN - 1] = '\0';
    }

    return (char *) &password;
}

#endif /* no getchar or _getch */

#endif /* no getpass */

/*
 * Use the OS getpass() routine (or our own) to obtain a password from
 * the input stream.
 *
 * Exit values:
 *  0: Success
 *  5: Partial success; entered text truncated to the size of the
 *     destination buffer
 *
 * Restrictions: Truncation also occurs according to the host system's
 * getpass() semantics, or at position 255 if our own version is used,
 * but the caller is *not* made aware of it.
 */

APR_DECLARE(apr_status_t) apr_getpass(const char *prompt, char *pwbuf, size_t *bufsiz)
{
    char *pw_got = NULL;
    int result = 0;

    pw_got = getpass(prompt);
    if (strlen(pw_got) > (*bufsiz - 1)) {
	*bufsiz = ERR_OVERFLOW;
        return APR_ENAMETOOLONG;
    }
    apr_cpystrn(pwbuf, pw_got, *bufsiz);
    *bufsiz = result;
    return APR_SUCCESS; 
}
