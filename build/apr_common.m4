dnl -----------------------------------------------------------------
dnl apr_common.m4: APR's general-purpose autoconf macros
dnl

dnl
dnl APR_SUBDIR_CONFIG(dir [, sub-package-cmdline-args])
dnl
AC_DEFUN(APR_SUBDIR_CONFIG, [
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
])dnl


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
dnl APR_CHECK_DEFINE_FILES( symbol, header_file [header_file ...] )
dnl
AC_DEFUN(APR_CHECK_DEFINE_FILES,[
  AC_CACHE_CHECK([for $1 in $2],ac_cv_define_$1,[
    ac_cv_define_$1=no
    for curhdr in $2
    do
      AC_EGREP_CPP(YES_IS_DEFINED, [
      #include <$curhdr>
      #ifdef $1
      YES_IS_DEFINED
      #endif
      ], ac_cv_define_$1=yes)
    done
  ])
  if test "$ac_cv_define_$1" = "yes"; then
    AC_DEFINE(HAVE_$1)
  fi
])


dnl
dnl APR_CHECK_DEFINE( symbol, header_file )
dnl
AC_DEFUN(APR_CHECK_DEFINE,[
  AC_CACHE_CHECK([for $1 in $2],ac_cv_define_$1,[
    AC_EGREP_CPP(YES_IS_DEFINED, [
    #include <$2>
    #ifdef $1
    YES_IS_DEFINED
    #endif
    ], ac_cv_define_$1=yes, ac_cv_define_$1=no)
  ])
  if test "$ac_cv_define_$1" = "yes"; then
    AC_DEFINE(HAVE_$1)
  fi
])


define(APR_IFALLYES,[dnl
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
])


define(APR_BEGIN_DECISION,[dnl
ac_decision_item='$1'
ac_decision_msg='FAILED'
ac_decision=''
])


define(APR_DECIDE,[dnl
ac_decision='$1'
ac_decision_msg='$2'
ac_decision_$1=yes
ac_decision_$1_msg='$2'
])


define(APR_DECISION_OVERRIDE,[dnl
    ac_decision=''
    for ac_item in $1; do
         eval "ac_decision_this=\$ac_decision_${ac_item}"
         if test ".$ac_decision_this" = .yes; then
             ac_decision=$ac_item
             eval "ac_decision_msg=\$ac_decision_${ac_item}_msg"
         fi
    done
])


define(APR_DECISION_FORCE,[dnl
ac_decision="$1"
eval "ac_decision_msg=\"\$ac_decision_${ac_decision}_msg\""
])


define(APR_END_DECISION,[dnl
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
])


dnl
dnl APR_CHECK_SIZEOF_EXTENDED(INCLUDES, TYPE [, CROSS_SIZE])
dnl
dnl A variant of AC_CHECK_SIZEOF which allows the checking of
dnl sizes of non-builtin types
dnl
AC_DEFUN(APR_CHECK_SIZEOF_EXTENDED,
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
if ${CC-cc} -c $CFLAGS $CPPFLAGS $apr_tcnw_flags conftest.$ac_ext 2>&AC_FD_CC ; then
  ifelse([$3], , :, [rm -rf conftest*
  $3])
else
  echo "configure: failed or warning program:" >&AC_FD_CC
  cat conftest.$ac_ext >&AC_FD_CC
  ifelse([$4], , , [rm -rf conftest*
  $4])
fi
rm -f conftest*
])dnl


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
])dnl


dnl the following is a newline, a space, a tab, and a backslash (the
dnl backslash is used by the shell to skip newlines, but m4 sees it;
dnl treat it like whitespace).
dnl WARNING: don't reindent these lines, or the space/tab will be lost!
define([apr_whitespace],[
 	\])

dnl
dnl APR_COMMA_ARGS(ARG1 ...)
dnl  convert the whitespace-separated arguments into comman-separated
dnl  arguments.
dnl
dnl APR_FOREACH(CODE-BLOCK, ARG1, ARG2, ...)
dnl  subsitute CODE-BLOCK for each ARG[i]. "eachval" will be set to ARG[i]
dnl  within each iteration.
dnl
changequote({,})
define({APR_COMMA_ARGS},{patsubst([$}{1],[[}apr_whitespace{]+],[,])})
define({APR_FOREACH},
  {ifelse($}{2,,,
          [define([eachval],
                  $}{2)$}{1[]APR_FOREACH([$}{1],
                                         builtin([shift],
                                                 builtin([shift], $}{@)))])})
changequote([,])

dnl APR_FLAG_HEADERS(HEADER-FILE ... [, FLAG-TO-SET ] [, "yes" ])
dnl  we set FLAG-TO-SET to 1 if we find HEADER-FILE, otherwise we set to 0
dnl  if FLAG-TO-SET is null, we automagically determine it's name
dnl  by changing all "/" to "_" in the HEADER-FILE and dropping
dnl  all "." chars. If the 3rd parameter is "yes" then instead of
dnl  setting to 1 or 0, we set FLAG-TO-SET to yes or no.
dnl  
AC_DEFUN(APR_FLAG_HEADERS,[
AC_CHECK_HEADERS($1)
for aprt_i in $1
do
    ac_safe=`echo "$aprt_i" | sed 'y%./+-%__p_%'`
    aprt_2=`echo "$aprt_i" | sed -e 's%/%_%g' -e 's/\.//g'`
    if eval "test \"`echo '$ac_cv_header_'$ac_safe`\" = yes"; then
       eval "ifelse($2,,$aprt_2,$2)=ifelse($3,yes,yes,1)"
    else
       eval "ifelse($2,,$aprt_2,$2)=ifelse($3,yes,no,0)"
    fi
done
])

dnl APR_FLAG_FUNCS(FUNC ... [, FLAG-TO-SET] [, "yes" ])
dnl  if FLAG-TO-SET is null, we automagically determine it's name
dnl  prepending "have_" to the function name in FUNC, otherwise
dnl  we use what's provided as FLAG-TO-SET. If the 3rd parameter
dnl  is "yes" then instead of setting to 1 or 0, we set FLAG-TO-SET
dnl  to yes or no.
dnl
dnl  DON'T USE YET !!
dnl
AC_DEFUN(APR_FLAG_FUNCS,[
AC_CHECK_FUNCS($1)
for aprt_j in $1
do
    aprt_3="have_$aprt_j"
    if eval "test \"`echo '$ac_cv_func_'$aprt_j`\" = yes"; then
       eval "ifelse($2,,$aprt_3,$2)=ifelse($3,yes,yes,1)"
    else
       eval "ifelse($2,,$aprt_3,$2)=ifelse($3,yes,no,0)"
    fi
done
])


