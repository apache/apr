dnl
dnl REENTRANCY_FLAGS
dnl
dnl Set some magic defines
dnl
AC_DEFUN(REENTRANCY_FLAGS,[
  case "`uname -sr`" in
  "SunOS 5"*)
    CFLAGS="$CFLAGS -D_POSIX_PTHREAD_SEMANTICS";;
  esac
  
dnl This works for now..
  CFLAGS="$CFLAGS -D_REENTRANT"
])dnl
dnl
dnl PTHREADS_CHECK_COMPILE
dnl
dnl Check whether the current setup can use POSIX threads calls
dnl
AC_DEFUN(PTHREADS_CHECK_COMPILE, [
AC_TRY_RUN( [
#include <pthread.h>

void *thread_routine(void *data) {
    return data;
}

int main() {
    pthread_t thd;
    int data = 1;
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
  for flag in -pthreads -pthread -mthreads; do 
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
