dnl
dnl REENTRANCY_FLAGS
dnl
dnl Set some magic defines
dnl
AC_DEFUN(REENTRANCY_FLAGS,[
  if test -z "$host_alias"; then
    host_alias=`$ac_config_guess`
dnl AC_MSG_ERROR(host_alias is not set. Make sure to run config.guess)
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
fi

PTHREADS_CHECK_COMPILE

AC_CACHE_CHECK(for pthreads_cflags,ac_cv_pthreads_cflags,[
ac_cv_pthreads_cflags=""
if test "$pthreads_working" != "yes"; then
  for flag in -pthreads -pthread -mthreads -Kthread -threads; do 
    ac_save="$CFLAGS"
    CFLAGS="$CFLAGS $flag"
    PTHREADS_CHECK_COMPILE
    if test "$pthreads_working" = "yes"; then
      ac_cv_pthreads_cflags="$flag"
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
