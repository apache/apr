dnl -----------------------------------------------------------------
dnl apr_threads.m4: APR's autoconf macros for testing thread support
dnl

dnl
dnl APR_CHECK_PTHREADS_H([ ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl
dnl gcc issues warnings when parsing AIX 4.3.3's pthread.h
dnl which causes autoconf to incorrectly conclude that
dnl pthreads is not available.
dnl Turn off warnings if we're using gcc.
dnl
AC_DEFUN(APR_CHECK_PTHREADS_H, [
  if test "$GCC" = "yes"; then
    SAVE_FL="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -w"
    AC_CHECK_HEADERS(pthread.h, [ $1 ] , [ $2 ] )
    CPPFLAGS="$SAVE_FL"
  else
    AC_CHECK_HEADERS(pthread.h, [ $1 ] , [ $2 ] )
  fi
])dnl


dnl
dnl APR_CHECK_PTHREAD_GETSPECIFIC_TWO_ARGS
dnl
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


dnl
dnl APR_CHECK_PTHREAD_ATTR_GETDETACHSTATE_ONE_ARG
dnl
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
dnl APR_PTHREADS_CHECK_COMPILE
dnl
dnl Check whether the current setup can use POSIX threads calls
dnl
AC_DEFUN(APR_PTHREADS_CHECK_COMPILE, [
AC_TRY_RUN( [
#include <pthread.h>
#include <stddef.h>

void *thread_routine(void *data) {
    return data;
}

int main() {
    pthread_t thd;
    pthread_mutexattr_t mattr;
    pthread_once_t once_init = PTHREAD_ONCE_INIT;
    int data = 1;
    pthread_mutexattr_init(&mattr);
    return pthread_create(&thd, NULL, thread_routine, &data);
} ], [ 
  pthreads_working="yes"
  ], [
  pthreads_working="no"
  ], pthreads_working="no" )
])dnl


dnl
dnl APR_PTHREADS_CHECK()
dnl
dnl Try to find a way to enable POSIX threads
dnl
AC_DEFUN(APR_PTHREADS_CHECK,[
if test -n "$ac_cv_pthreads_lib"; then
  LIBS="$LIBS -l$ac_cv_pthreads_lib"
fi

if test -n "$ac_cv_pthreads_cflags"; then
  CFLAGS="$CFLAGS $ac_cv_pthreads_cflags"
fi

APR_PTHREADS_CHECK_COMPILE

AC_CACHE_CHECK(for pthreads_cflags,ac_cv_pthreads_cflags,[
ac_cv_pthreads_cflags=""
if test "$pthreads_working" != "yes"; then
  for flag in -kthread -pthread -pthreads -mthreads -Kthread -threads; do 
    ac_save="$CFLAGS"
    CFLAGS="$CFLAGS $flag"
    APR_PTHREADS_CHECK_COMPILE
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
    APR_PTHREADS_CHECK_COMPILE
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
dnl APR_PTHREADS_CHECK_SAVE
dnl APR_PTHREADS_CHECK_RESTORE
dnl
dnl Save the global environment variables that might be modified during
dnl the checks for threading support so that they can restored if the
dnl result is not what the caller wanted.
dnl
AC_DEFUN(APR_PTHREADS_CHECK_SAVE, [
  apr_pthsv_CFLAGS="$CFLAGS"
  apr_pthsv_LIBS="$LIBS"
])dnl

AC_DEFUN(APR_PTHREADS_CHECK_RESTORE, [
  CFLAGS="$apr_pthsv_CFLAGS"
  LIBS="$apr_pthsv_LIBS"
])dnl

dnl
dnl APR_CHECK_SIGWAIT_ONE_ARG
dnl
AC_DEFUN(APR_CHECK_SIGWAIT_ONE_ARG,[
  AC_CACHE_CHECK(whether sigwait takes one argument,ac_cv_sigwait_one_arg,[
  AC_TRY_COMPILE([
#if defined(__NETBSD__) || defined(DARWIN)
    /* When using the unproven-pthreads package, we need to pull in this
     * header to get a prototype for sigwait().  Else things will fail later
     * on.  XXX Should probably be fixed in the unproven-pthreads package.
     * Darwin is declaring sigwait() in the wrong place as well.
     */
#include <pthread.h>
#endif
#include <signal.h>
],[
  sigset_t set;
 
  sigwait(&set);
],[
  ac_cv_sigwait_one_arg=yes
],[
  ac_cv_sigwait_one_arg=no
])])
  if test "$ac_cv_sigwait_one_arg" = "yes"; then
    AC_DEFINE(SIGWAIT_TAKES_ONE_ARG,1,[ ])
  fi
])
