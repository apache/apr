dnl APR_TRY_COMPILE_NO_WARNING(INCLUDES, FUNCTION-BODY,
dnl             [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl Tries a compile test with warnings activated so that the result
dnl is false if the code doesn't compile cleanly.
dnl
AC_DEFUN(APR_TRY_COMPILE_NO_WARNING,
[if test "x$CFLAGS_WARN" = "x"; then
  apr_tcnw_flags=""
else
  apr_tcnw_flags=$CFLAGS_WARN
fi
if test "$GCC" = "yes"; then 
  apr_tcnw_flags="$apr_tcnw_flags -Werror"
fi
changequote(', ')
cat > conftest.$ac_ext <<EOTEST
#include "confdefs.h"
'$1'
int main(int argc, const char * const argv[]) {
'$2'
; return 0; }
EOTEST
changequote([, ])
if ${CC-cc} -c $CFLAGS $CPPFLAGS $apr_tcnw_flags conftest.$ac_ext 1>&AC_FD_CC ; then
  ifelse([$3], , :, [rm -rf conftest*
  $3])
else
  echo "configure: failed or warning program:" >&AC_FD_CC
  cat conftest.$ac_ext >&AC_FD_CC
  ifelse([$4], , , [rm -rf conftest*
  $4])
fi
rm -f conftest*])

dnl
dnl RUN_SUBDIR_CONFIG_NOW(dir [, sub-package-cmdline-args])
dnl
AC_DEFUN(RUN_SUBDIR_CONFIG_NOW, [
  # save our work to this point; this allows the sub-package to use it
  AC_CACHE_SAVE

  echo "configuring package in $1 now"
  ac_popdir=`pwd`
  ac_abs_srcdir=`(cd $srcdir/$1 && pwd)`
  apr_config_subdirs="$1"
  test -d $1 || $MKDIR $1
  cd $1

changequote(, )dnl
      # A "../" for each directory in /$config_subdirs.
      ac_dots=`echo $apr_config_subdirs|sed -e 's%^\./%%' -e 's%[^/]$%&/%' -e 's%[^/]*/%../%g'`
changequote([, ])dnl

  # Make the cache file name correct relative to the subdirectory.
  case "$cache_file" in
  /*) ac_sub_cache_file=$cache_file ;;
  *) # Relative path.
    ac_sub_cache_file="$ac_dots$cache_file" ;;
  esac

  # The eval makes quoting arguments work.
  if eval $ac_abs_srcdir/configure $ac_configure_args --cache-file=$ac_sub_cache_file --srcdir=$ac_abs_srcdir $2
  then :
    echo "$1 configured properly"
  else
    echo "configure failed for $1"
    exit 1
  fi

  cd $ac_popdir

  # grab any updates from the sub-package
  AC_CACHE_LOAD
])
dnl
dnl REENTRANCY_FLAGS
dnl
dnl Set some magic defines
dnl
AC_DEFUN(REENTRANCY_FLAGS,[
  if test -z "$host_alias"; then
    host_alias=`$ac_config_guess`
  fi
  case "$host_alias" in
  *solaris*)
    PTHREAD_FLAGS="-D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT";;
  *freebsd*)
    PTHREAD_FLAGS="-D_REENTRANT -D_THREAD_SAFE";;
  *openbsd*)
    PTHREAD_FLAGS="-D_POSIX_THREADS";;
  *linux*)
    PTHREAD_FLAGS="-D_REENTRANT";;
  *aix*)
    PTHREAD_FLAGS="-D_THREAD_SAFE";;
  *irix*)
    PTHREAD_FLAGS="-D_POSIX_THREAD_SAFE_FUNCTIONS";;
  *hpux*)
    PTHREAD_FLAGS="-D_REENTRANT";;
  *sco*)
    PTHREAD_FLAGS="-D_REENTRANT";;
dnl Solves sigwait() problem, creates problems with u_long etc.
dnl    PTHREAD_FLAGS="-D_REENTRANT -D_XOPEN_SOURCE=500 -D_POSIX_C_SOURCE=199506 -D_XOPEN_SOURCE_EXTENDED=1";;
  esac

  if test -n "$PTHREAD_FLAGS"; then
    CPPFLAGS="$CPPFLAGS $PTHREAD_FLAGS"
    THREAD_CPPFLAGS="$THREAD_CPPFLAGS $PTHREAD_FLAGS"
  fi
])dnl

dnl gcc issues warnings when parsing AIX 4.3.3's pthread.h
dnl which causes autoconf to incorrectly conclude that
dnl pthreads is not available.
dnl Turn off warnings if we're using gcc.
AC_DEFUN(CHECK_PTHREADS_H, [
  if test "$GCC" = "yes"; then
    SAVE_FL="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -w"
    AC_CHECK_HEADERS(pthread.h, [ $1 ] , [ $2 ] )
    CPPFLAGS="$SAVE_FL"
  else
    AC_CHECK_HEADERS(pthread.h, [ $1 ] , [ $2 ] )
  fi
])dnl

AC_DEFUN(APR_CHECK_PTHREAD_GETSPECIFIC_TWO_ARGS, [
AC_CACHE_CHECK(whether pthread_getspecific takes two arguments, ac_cv_pthread_getspecific_two_args,[
AC_TRY_COMPILE([
#include <pthread.h>
],[
pthread_key_t key;
void *tmp;
pthread_getspecific(key,&tmp);
],[
    ac_cv_pthread_getspecific_two_args=yes
],[
    ac_cv_pthread_getspecific_two_args=no
])
])

if test "$ac_cv_pthread_getspecific_two_args" = "yes"; then
  AC_DEFINE(PTHREAD_GETSPECIFIC_TAKES_TWO_ARGS, 1, [Define if pthread_getspecific() has two args])
fi

])dnl

AC_DEFUN(APR_CHECK_PTHREAD_ATTR_GETDETACHSTATE_ONE_ARG, [
AC_CACHE_CHECK(whether pthread_attr_getdetachstate takes one argument, ac_cv_pthread_attr_getdetachstate_one_arg,[
AC_TRY_COMPILE([
#include <pthread.h>
],[
pthread_attr_t *attr;
pthread_attr_getdetachstate(attr);
],[
    ac_cv_pthread_attr_getdetachstate_one_arg=yes
],[
    ac_cv_pthread_attr_getdetachstate_one_arg=no
])
])

if test "$ac_cv_pthread_attr_getdetachstate_one_arg" = "yes"; then
  AC_DEFINE(PTHREAD_ATTR_GETDETACHSTATE_TAKES_ONE_ARG, 1, [Define if pthread_attr_getdetachstate() has one arg])
fi

])dnl
dnl
dnl PTHREADS_CHECK_COMPILE
dnl
dnl Check whether the current setup can use POSIX threads calls
dnl
AC_DEFUN(PTHREADS_CHECK_COMPILE, [
AC_TRY_RUN( [
#include <pthread.h>
#include <stddef.h>

void *thread_routine(void *data) {
    return data;
}

int main() {
    pthread_t thd;
    pthread_mutexattr_t mattr;
    int data = 1;
    pthread_mutexattr_init(&mattr);
    return pthread_create(&thd, NULL, thread_routine, &data);
} ], [ 
  pthreads_working="yes"
  ], [
  pthreads_working="no"
  ], pthreads_working="no" ) ] )dnl
dnl
dnl PTHREADS_CHECK()
dnl
dnl Try to find a way to enable POSIX threads
dnl
AC_DEFUN(PTHREADS_CHECK,[
if test -n "$ac_cv_pthreads_lib"; then
  LIBS="$LIBS -l$ac_cv_pthreads_lib"
fi

if test -n "$ac_cv_pthreads_cflags"; then
  CFLAGS="$CFLAGS $ac_cv_pthreads_cflags"
  THREAD_CFLAGS="$THREAD_CFLAGS $ac_cv_pthreads_cflags"
fi

PTHREADS_CHECK_COMPILE

AC_CACHE_CHECK(for pthreads_cflags,ac_cv_pthreads_cflags,[
ac_cv_pthreads_cflags=""
if test "$pthreads_working" != "yes"; then
  for flag in -kthread -pthread -pthreads -mthreads -Kthread -threads -mt; do 
    ac_save="$CFLAGS"
    CFLAGS="$CFLAGS $flag"
    PTHREADS_CHECK_COMPILE
    if test "$pthreads_working" = "yes"; then
      ac_cv_pthreads_cflags="$flag"
      dnl this was already added to CFLAGS; add to THREAD_CFLAGS, too
      THREAD_CFLAGS="$THREAD_CFLAGS $ac_cv_pthreads_cflags"
      break
    fi
    CFLAGS="$ac_save"
  done
fi
])

AC_CACHE_CHECK(for pthreads_lib, ac_cv_pthreads_lib,[
ac_cv_pthreads_lib=""
if test "$pthreads_working" != "yes"; then
  for lib in pthread pthreads c_r; do
    ac_save="$LIBS"
    LIBS="$LIBS -l$lib"
    PTHREADS_CHECK_COMPILE
    if test "$pthreads_working" = "yes"; then
      ac_cv_pthreads_lib="$lib"
      break
    fi
    LIBS="$ac_save"
  done
fi
])

if test "$pthreads_working" = "yes"; then
  threads_result="POSIX Threads found"
else
  threads_result="POSIX Threads not found"
fi
])dnl

dnl
dnl Apache and APR "hints" file
dnl  We preload various configure settings depending
dnl  on previously obtained platform knowledge.
dnl  We allow all settings to be overridden from
dnl  the command-line.
dnl
dnl  We maintain the "format" that we've used
dnl  under 1.3.x, so we don't exactly follow
dnl  what is "recommended" by autoconf.

dnl
dnl APR_DOEXTRA
dnl
dnl  Handle the use of EXTRA_* variables.
dnl  Basically, EXTRA_* vars are added to the
dnl  current settings of their "parents". We
dnl  can expand as needed for additional
dnl  EXTRA_* variables by adding them to the
dnl  "for i in..." line.
dnl
dnl  To handle recursive configures, the 1st time
dnl  through we need to null out EXTRA_* and then
dnl  export the lot of them, since we want/need
dnl  them to exist in the sub-configures' environment.
dnl  The nasty eval's allow us to use the 'for'
dnl  construct and save some lines of code.
dnl
AC_DEFUN(APR_DOEXTRA, [
  for i in CFLAGS LDFLAGS LIBS
  do
    eval APR_TMP=\$EXTRA_$i
    if test -n "$APR_TMP"; then
      eval $i=\"\$$i $APR_TMP\"
      eval export $i
      eval unset EXTRA_${i}
      eval export EXTRA_${i}
    fi
  done
])

dnl
dnl APR_SETIFNULL(variable, value)
dnl
dnl  Set variable iff it's currently null
dnl
AC_DEFUN(APR_SETIFNULL,[
  if test -z "$$1"; then
    echo "  Setting $1 to \"$2\""
    $1="$2"; export $1
  fi
])

dnl
dnl APR_SETVAR(variable, value)
dnl
dnl  Set variable no matter what
dnl
AC_DEFUN(APR_SETVAR,[
  echo "  Forcing $1 to \"$2\""
  $1="$2"; export $1
])

dnl
dnl APR_ADDTO(variable, value)
dnl
dnl  Add value to variable
dnl
AC_DEFUN(APR_ADDTO,[
  echo "  Adding \"$2\" to $1"
  $1="$$1 $2"; export $1
])

dnl
dnl APR_CHECK_ICONV_INBUF
dnl
dnl  Decide whether or not the inbuf parameter to iconv() is const.
dnl
dnl  We try to compile something without const.  If it fails to 
dnl  compile, we assume that the system's iconv() has const.  
dnl  Unfortunately, we won't realize when there was a compile
dnl  warning, so we allow a variable -- apr_iconv_inbuf_const -- to
dnl  be set in hints.m4 to specify whether or not iconv() has const
dnl  on this parameter.
dnl
AC_DEFUN(APR_CHECK_ICONV_INBUF,[
AC_MSG_CHECKING(for type of inbuf parameter to iconv)
if test "x$apr_iconv_inbuf_const" = "x"; then
    APR_TRY_COMPILE_NO_WARNING([
    #include <stddef.h>
    #include <iconv.h>
    ],[
    iconv(0,(char **)0,(size_t *)0,(char **)0,(size_t *)0);
    ], apr_iconv_inbuf_const="0", apr_iconv_inbuf_const="1")
fi
if test "$apr_iconv_inbuf_const" = "1"; then
    AC_DEFINE(APR_ICONV_INBUF_CONST, 1, [Define if the inbuf parm to iconv() is const char **])
    msg="const char **"
else
    msg="char **"
fi
AC_MSG_RESULT([$msg])
])
