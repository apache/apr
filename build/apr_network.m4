dnl -----------------------------------------------------------------
dnl apr_network.m4: APR's autoconf macros for testing network support
dnl

dnl
dnl check for working getaddrinfo()
dnl
AC_DEFUN(APR_CHECK_WORKING_GETADDRINFO,[
  AC_CACHE_CHECK(for working getaddrinfo, ac_cv_working_getaddrinfo,[
  AC_TRY_RUN( [
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

void main(void) {
    struct addrinfo hints, *ai;
    int error;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    error = getaddrinfo("127.0.0.1", "8080", &hints, &ai);
    if (error) {
        exit(1);
    }
    else {
        exit(0);
    }
}
],[
  ac_cv_working_getaddrinfo="yes"
],[
  ac_cv_working_getaddrinfo="no"
],[
  ac_cv_working_getaddrinfo="yes"
])])
if test "$ac_cv_working_getaddrinfo" = "yes"; then
  AC_DEFINE(HAVE_GETADDRINFO, 1, [Define if getaddrinfo exists and works well enough for APR])
fi
])


dnl
dnl check for gethostbyname() which handles numeric address strings
dnl
AC_DEFUN(APR_CHECK_GETHOSTBYNAME_NAS,[
  AC_CACHE_CHECK(for gethostbyname() which handles numeric address strings, ac_cv_gethostbyname_nas,[
  AC_TRY_RUN( [
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
void main(void) {
    struct hostent *he = gethostbyname("127.0.0.1");
    if (he == NULL) {
        exit(1);
    }
    else {
        exit(0);
    }
}
],[
  ac_cv_gethostbyname_nas="yes"
],[
  ac_cv_gethostbyname_nas="no"
],[
  ac_cv_gethostbyname_nas="yes"
])])
if test "$ac_cv_gethostbyname_nas" = "yes"; then
  AC_DEFINE(GETHOSTBYNAME_HANDLES_NAS, 1, [Define if gethostbyname() handles nnn.nnn.nnn.nnn])
fi
])


dnl 
dnl check for socklen_t, fall back to unsigned int
dnl
AC_DEFUN(APR_CHECK_SOCKLEN_T,[
AC_CACHE_CHECK(for socklen_t, ac_cv_socklen_t,[
AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
],[
socklen_t foo = (socklen_t) 0;
],[
    ac_cv_socklen_t=yes
],[
    ac_cv_socklen_t=no
])
])

if test "$ac_cv_socklen_t" = "yes"; then
  AC_DEFINE(HAVE_SOCKLEN_T, 1, [Whether you have socklen_t])
fi
])


AC_DEFUN(APR_CHECK_INET_ADDR,[
AC_CACHE_CHECK(for inet_addr, ac_cv_func_inet_addr,[
AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
],[
inet_addr("127.0.0.1");
],[
    ac_cv_func_inet_addr=yes
],[
    ac_cv_func_inet_addr=no
])
])

if test "$ac_cv_func_inet_addr" = "yes"; then
  have_inet_addr=1
else
  have_inet_addr=0
fi
])


AC_DEFUN(APR_CHECK_INET_NETWORK,[
AC_CACHE_CHECK(for inet_network, ac_cv_func_inet_network,[
AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
],[
inet_network("127.0.0.1");
],[
    ac_cv_func_inet_network=yes
],[
    ac_cv_func_inet_network=no
])
])

if test "$ac_cv_func_inet_network" = "yes"; then
  have_inet_network=1
else
  have_inet_network=0
fi
])


AC_DEFUN(APR_CHECK_SOCKADDR_IN6,[
AC_CACHE_CHECK(for sockaddr_in6, ac_cv_define_sockaddr_in6,[
AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
],[
struct sockaddr_in6 sa;
],[
    ac_cv_define_sockaddr_in6=yes
],[
    ac_cv_define_sockaddr_in6=no
])
])

if test "$ac_cv_define_sockaddr_in6" = "yes"; then
  have_sockaddr_in6=1
else
  have_sockaddr_in6=0
fi
])


dnl
dnl Check to see if this platform includes sa_len in it's
dnl struct sockaddr.  If it does it changes the length of sa_family
dnl which could cause us problems
dnl
AC_DEFUN(APR_CHECK_SOCKADDR_SA_LEN,[
AC_CACHE_CHECK(for sockaddr sa_len, ac_cv_define_sockaddr_sa_len,[
AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
],[
struct sockaddr_in sai;
int i = sai.sin_len;
],[
  ac_cv_define_sockaddr_sa_len=yes
],[
  ac_cv_define_sockaddr_sa_len=no
])
])

if test "$ac_cv_define_sockaddr_sa_len" = "yes"; then
  AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1 ,[Define if we have length field in sockaddr_in])
fi
])


dnl
dnl APR_INADDR_NONE
dnl
dnl checks for missing INADDR_NONE macro
dnl
AC_DEFUN(APR_INADDR_NONE,[
  AC_CACHE_CHECK(whether system defines INADDR_NONE, ac_cv_inaddr_none,[
  AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
],[
unsigned long foo = INADDR_NONE;
],[
    ac_cv_inaddr_none=yes
],[
    ac_cv_inaddr_none=no
])])
  if test "$ac_cv_inaddr_none" = "no"; then
    apr_inaddr_none="((unsigned int) 0xffffffff)"
  else
    apr_inaddr_none="INADDR_NONE"
  fi
])


dnl
dnl APR_H_ERRNO_COMPILE_CHECK
dnl
AC_DEFUN(APR_H_ERRNO_COMPILE_CHECK,[
  if test x$1 != x; then
    CFLAGS="-D$1 $CFLAGS"
  fi
  AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
],[
int h_e = h_errno;
],[
  if test x$1 != x; then
    ac_cv_h_errno_cflags="$1"
  else
    ac_cv_h_errno_cflags=yes
  fi
],[
  ac_cv_h_errno_cflags=no
])])


dnl
dnl APR_CHECK_H_ERRNO_FLAG
dnl
dnl checks which flags are necessary for <netdb.h> to define h_errno
dnl
AC_DEFUN(APR_CHECK_H_ERRNO_FLAG,[
  AC_MSG_CHECKING([for h_errno in netdb.h])
  AC_CACHE_VAL(ac_cv_h_errno_cflags,[
    APR_H_ERRNO_COMPILE_CHECK
    if test "$ac_cv_h_errno_cflags" = "no"; then
      ac_save="$CFLAGS"
      for flag in _XOPEN_SOURCE_EXTENDED; do
        APR_H_ERRNO_COMPILE_CHECK($flag)
        if test "$ac_cv_h_errno_cflags" != "no"; then
          break
        fi
      done
      CFLAGS="$ac_save"
    fi
  ])
  if test "$ac_cv_h_errno_cflags" != "no"; then
    if test "$ac_cv_h_errno_cflags" != "yes"; then
      CFLAGS="-D$ac_cv_h_errno_cflags $CFLAGS"
      AC_MSG_RESULT([yes, with -D$ac_cv_h_errno_cflags])
    else
      AC_MSG_RESULT([$ac_cv_h_errno_cflags])
    fi
  else
    AC_MSG_RESULT([$ac_cv_h_errno_cflags])
  fi
])


AC_DEFUN(APR_EBCDIC,[
  AC_CACHE_CHECK([whether system uses EBCDIC],ac_cv_ebcdic,[
  AC_TRY_RUN( [
int main(void) { 
  return (unsigned char)'A' != (unsigned char)0xC1; 
} 
],[
  ac_cv_ebcdic="yes"
],[
  ac_cv_ebcdic="no"
],[
  ac_cv_ebcdic="no"
])])
  if test "$ac_cv_ebcdic" = "yes"; then
    apr_charset_ebcdic=1
  else
    apr_charset_ebcdic=0
  fi
  AC_SUBST(apr_charset_ebcdic)
])

