## os2_libtool.m4 - Replacement for libtool.m4 on OS/2

AC_DEFUN(AC_PROG_LIBTOOL,
echo "using OS/2-specific aplibtool"

LIBTOOL="$srcdir/build/aplibtool"
AC_SUBST(LIBTOOL)dnl

gcc -o $CPPFLAGS $CFLAGS $LIBTOOL.exe $LIBTOOL.c
])dnl
