dnl ##
dnl ##
dnl ##
define(AC_USE_FUNCTION,[dnl
AC_CHECK_FUNCS($1)
if test ".$ac_func_$1" = .yes; then 
AC_DEFINE(USE_$2)
fi
])dnl
dnl ##
dnl ##
dnl ##
define(AC_CHECK_DEFINE,[dnl
AC_CACHE_CHECK(for $1 in $2, ac_cv_define_$1,
AC_EGREP_CPP([YES_IS_DEFINED], [
#include <$2>
#ifdef $1
YES_IS_DEFINED
#endif
], ac_cv_define_$1=yes; AC_DEFINE(HAVE_$1), ac_cv_define_$1=no)
)])dnl
dnl ##
dnl ##
dnl ##
define(AC_IFALLYES,[dnl
ac_rc=yes
for ac_spec in $1; do
    ac_type=`echo "$ac_spec" | sed -e 's/:.*$//'`
    ac_item=`echo "$ac_spec" | sed -e 's/^.*://'`
    case $ac_type in
        header )
            ac_item=`echo "$ac_item" | sed 'y%./+-%__p_%'`
            ac_var="ac_cv_header_$ac_item"
            ;;
        file )
            ac_item=`echo "$ac_item" | sed 'y%./+-%__p_%'`
            ac_var="ac_cv_file_$ac_item"
            ;;
        func )   ac_var="ac_cv_func_$ac_item"   ;;
        define ) ac_var="ac_cv_define_$ac_item" ;;
        custom ) ac_var="$ac_item" ;;
    esac
    eval "ac_val=\$$ac_var"
    if test ".$ac_val" != .yes; then
        ac_rc=no
        break
    fi
done
if test ".$ac_rc" = .yes; then
    :
    $2
else
    :
    $3
fi
])dnl
dnl ##
dnl ##
dnl ##
define(AC_BEGIN_DECISION,[dnl
ac_decision_item='$1'
ac_decision_msg='FAILED'
ac_decision=''
])dnl
define(AC_DECIDE,[dnl
ac_decision='$1'
ac_decision_msg='$2'
ac_decision_$1=yes
ac_decision_$1_msg='$2'
])dnl
define(AC_DECISION_OVERRIDE,[dnl
    ac_decision=''
    for ac_item in $1; do
         eval "ac_decision_this=\$ac_decision_${ac_item}"
         if test ".$ac_decision_this" = .yes; then
             ac_decision=$ac_item
             eval "ac_decision_msg=\$ac_decision_${ac_item}_msg"
         fi
    done
])dnl
define(AC_DECISION_FORCE,[dnl
ac_decision="$1"
eval "ac_decision_msg=\"\$ac_decision_${ac_decision}_msg\""
])dnl
define(AC_END_DECISION,[dnl
if test ".$ac_decision" = .; then
    echo "[$]0:Error: decision on $ac_decision_item failed" 1>&2
    exit 1
else
    if test ".$ac_decision_msg" = .; then
        ac_decision_msg="$ac_decision"
    fi
    AC_DEFINE_UNQUOTED(${ac_decision_item})
    AC_MSG_RESULT([decision on $ac_decision_item... $ac_decision_msg])
fi
])dnl

dnl ### AC_TRY_RUN had some problems actually using a programs return code,
dnl ### so I am re-working it here to be used in APR's configure script.
dnl MY_TRY_RUN(PROGRAM, [ACTION-IF-TRUE [, ACTION-IF-FALSE
dnl            [, ACTION-IF-CROSS-COMPILING]]])
AC_DEFUN(MY_TRY_RUN,
[if test "$cross_compiling" = yes; then
  ifelse([$4], ,
    [errprint(__file__:__line__: warning: [AC_TRY_RUN] called without default to allow cross compiling
)dnl
  AC_MSG_ERROR(can not run test program while cross compiling)],
  [$4])
else
  MY_TRY_RUN_NATIVE([$1], [$2], [$3])
fi
])

dnl Like AC_TRY_RUN but assumes a native-environment (non-cross) compiler.
dnl MY_TRY_RUN_NATIVE(PROGRAM, [ACTION-IF-TRUE [, ACTION-IF-FALSE]])
AC_DEFUN(MY_TRY_RUN_NATIVE,
[cat > conftest.$ac_ext <<EOF
[#]line __oline__ "configure"
#include "confdefs.h"
ifelse(AC_LANG, CPLUSPLUS, [#ifdef __cplusplus
extern "C" void exit(int);
#endif
])dnl
[$1]
EOF
if AC_TRY_EVAL(ac_link) && test -s conftest${ac_exeext} && (./conftest; exit) 2>/dev/null
then
dnl Don't remove the temporary files here, so they can be examined.
  ifelse([$2], , :, [$2])
else
ifelse([$3], , , [  $3 
  rm -fr conftest*
])dnl
fi
rm -fr conftest*])
dnl A variant of AC_CHECK_SIZEOF which allows the checking of
dnl sizes of non-builtin types
dnl AC_CHECK_SIZEOF_EXTENDED(INCLUDES, TYPE [, CROSS_SIZE])
AC_DEFUN(AC_CHECK_SIZEOF_EXTENDED,
[changequote(<<,>>)dnl
dnl The name to #define
define(<<AC_TYPE_NAME>>, translit(sizeof_$2, [a-z *], [A-Z_P]))dnl
dnl The cache variable
define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_$2, [ *],[<p>]))dnl
changequote([, ])dnl
AC_MSG_CHECKING(size of $2)
AC_CACHE_VAL(AC_CV_NAME,
[AC_TRY_RUN([#include <stdio.h>
$1
main()
{
  FILE *f=fopen("conftestval","w");
  if (!f) exit(1);
  fprintf(f, "%d\n", sizeof($2));
  exit(0);
}], AC_CV_NAME=`cat conftestval`, AC_CV_NAME=0, ifelse([$3],,,
AC_CV_NAME=$3))])dnl
AC_MSG_RESULT($AC_CV_NAME)
AC_DEFINE_UNQUOTED(AC_TYPE_NAME, $AC_CV_NAME)
undefine([AC_TYPE_NAME])dnl
undefine([AC_CV_NAME])dnl
])

dnl
dnl check for gethostbyname() which handles numeric address strings
dnl

AC_DEFUN(APR_CHECK_GETHOSTBYNAME_NAS,[
  AC_CACHE_CHECK(for gethostbyname() which handles numeric address strings, ac_cv_gethostbyname_nas,[
  AC_TRY_RUN( [
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#if HAVE_STDLIB_H
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

dnl Check for ranlib but, unlike the check provided with autoconf, set
dnl RANLIB to "true" if there is no ranlib instead of setting it to ":".
dnl OS/390 doesn't have ranlib and the make utility doesn't parse "RANLIB=:" 
dnl the way we might want it to.

AC_DEFUN(AC_PROG_RANLIB_NC,
[AC_CHECK_PROG(RANLIB, ranlib, ranlib, true)])

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
    AC_DEFINE(CHARSET_EBCDIC,, [Define if system uses EBCDIC])
  fi
])

sinclude(threads.m4)
sinclude(hints.m4)
