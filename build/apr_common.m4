dnl -----------------------------------------------------------------
dnl apr_common.m4: APR's general-purpose autoconf macros
dnl

dnl APR_CONFIG_NICE(filename)
dnl
dnl Saves a snapshot of the configure command-line for later reuse
dnl
AC_DEFUN(APR_CONFIG_NICE,[
  rm -f $1
  cat >$1<<EOF
#! /bin/sh
#
# Created by configure

EOF
  if test -n "$CFLAGS"; then
    echo "CFLAGS=\"$CFLAGS\"; export CFLAGS" >> $1
  fi
  if test -n "$CPPFLAGS"; then
    echo "CPPFLAGS=\"$CPPFLAGS\"; export CPPFLAGS" >> $1
  fi
  if test -n "$LDFLAGS"; then
    echo "LDFLAGS=\"$LDFLAGS\"; export LDFLAGS" >> $1
  fi
  if test -n "$LIBS"; then
    echo "LIBS=\"$LIBS\"; export LIBS" >> $1
  fi
  if test -n "$INCLUDES"; then
    echo "INCLUDES=\"$INCLUDES\"; export INCLUDES" >> $1
  fi
  if test -n "$NOTEST_CFLAGS"; then
    echo "NOTEST_CFLAGS=\"$NOTEST_CFLAGS\"; export NOTEST_CFLAGS" >> $1
  fi
  if test -n "$NOTEST_CPPFLAGS"; then
    echo "NOTEST_CPPFLAGS=\"$NOTEST_CPPFLAGS\"; export NOTEST_CPPFLAGS" >> $1
  fi
  if test -n "$NOTEST_LDFLAGS"; then
    echo "NOTEST_LDFLAGS=\"$NOTEST_LDFLAGS\"; export NOTEST_LDFLAGS" >> $1
  fi
  if test -n "$NOTEST_LIBS"; then
    echo "NOTEST_LIBS=\"$NOTEST_LIBS\"; export NOTEST_LIBS" >> $1
  fi

  for arg in [$]0 "[$]@"; do
    echo "\"[$]arg\" \\" >> $1
  done
  echo '"[$]@"' >> $1
  chmod +x $1
])dnl

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

  # Make the cache file pathname absolute for the subdirs
  # required to correctly handle subdirs that might actually
  # be symlinks
  case "$cache_file" in
  /*) # already absolute
    ac_sub_cache_file=$cache_file ;;
  *)  # Was relative path.
    ac_sub_cache_file="$ac_popdir/$cache_file" ;;
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
dnl APR_SAVE_THE_ENVIRONMENT(variable_name)
dnl
dnl Stores the variable (usually a Makefile macro) for later restoration
dnl
AC_DEFUN(APR_SAVE_THE_ENVIRONMENT,[
  apr_ste_save_$1="$$1"
])dnl

dnl
dnl APR_RESTORE_THE_ENVIRONMENT(variable_name, prefix_)
dnl
dnl Uses the previously saved variable content to figure out what configure
dnl has added to the variable, moving the new bits to prefix_variable_name
dnl and restoring the original variable contents.  This makes it possible
dnl for a user to override configure when it does something stupid.
dnl
AC_DEFUN(APR_RESTORE_THE_ENVIRONMENT,[
if test "x$apr_ste_save_$1" = "x"; then
  $2$1="$$1"
  $1=
else
  if test "x$apr_ste_save_$1" = "x$$1"; then
    $2$1=
  else
    $2$1=`echo $$1 | sed -e "s%${apr_ste_save_$1}%%"`
    $1="$apr_ste_save_$1"
  fi
fi
echo "  restoring $1 to \"$$1\""
echo "  setting $2$1 to \"$$2$1\""
AC_SUBST($2$1)
])dnl

dnl
dnl APR_SETIFNULL(variable, value)
dnl
dnl  Set variable iff it's currently null
dnl
AC_DEFUN(APR_SETIFNULL,[
  if test -z "$$1"; then
    echo "  setting $1 to \"$2\""
    $1="$2"
  fi
])dnl

dnl
dnl APR_SETVAR(variable, value)
dnl
dnl  Set variable no matter what
dnl
AC_DEFUN(APR_SETVAR,[
  echo "  forcing $1 to \"$2\""
  $1="$2"
])dnl

dnl
dnl APR_ADDTO(variable, value)
dnl
dnl  Add value to variable
dnl
AC_DEFUN(APR_ADDTO,[
  if test "x$$1" = "x"; then
    echo "  setting $1 to \"$2\""
    $1="$2"
  else
    apr_addto_bugger="$2"
    for i in $apr_addto_bugger; do
      apr_addto_duplicate="0"
      for j in $$1; do
        if test "x$i" = "x$j"; then
          apr_addto_duplicate="1"
          break
        fi
      done
      if test $apr_addto_duplicate = "0"; then
        echo "  adding \"$i\" to $1"
        $1="$$1 $i"
      fi
    done
  fi
])dnl

dnl
dnl APR_REMOVEFROM(variable, value)
dnl
dnl Remove a value from a variable
dnl
AC_DEFUN(APR_REMOVEFROM,[
  if test "x$$1" = "x$2"; then
    echo "  nulling $1"
    $1=""
  else
    apr_new_bugger=""
    apr_removed=0
    for i in $$1; do
      if test "x$i" != "x$2"; then
        apr_new_bugger="$apr_new_bugger $i"
      else
        apr_removed=1
      fi
    done
    if test $apr_removed = "1"; then
      echo "  removed \"$2\" from $1"
      $1=$apr_new_bugger
    fi
  fi
]) dnl

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

dnl
dnl APR_CHECK_APR_DEFINE( symbol, path_to_apr )
dnl
AC_DEFUN(APR_CHECK_APR_DEFINE,[
    AC_EGREP_CPP(YES_IS_DEFINED, [
    #include "$2/include/apr.h"
    #if $1
    YES_IS_DEFINED
    #endif
    ], ac_cv_define_$1=yes, ac_cv_define_$1=no)
])

define(APR_CHECK_FILE,[
ac_safe=`echo "$1" | sed 'y%./+-%__p_%'`
AC_MSG_CHECKING([for $1])
AC_CACHE_VAL(ac_cv_file_$ac_safe, [
  if test -r $1; then
    eval "ac_cv_file_$ac_safe=yes"
  else
    eval "ac_cv_file_$ac_safe=no"
  fi
])dnl
if eval "test \"`echo '$ac_cv_file_'$ac_safe`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
ifelse([$3], , , [$3])
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
        struct ) ac_var="ac_cv_struct_$ac_item" ;;
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
dnl APR_CHECK_STRERROR_R_RC
dnl
dnl  Decide which style of retcode is used by this system's 
dnl  strerror_r().  It either returns int (0 for success, -1
dnl  for failure), or it returns a pointer to the error 
dnl  string.
dnl
dnl
AC_DEFUN(APR_CHECK_STRERROR_R_RC,[
AC_MSG_CHECKING(for type of return code from strerror_r)
AC_TRY_RUN([
#include <errno.h>
#include <stdio.h>
main()
{
  char buf[1024];
  if (strerror_r(ERANGE, buf, sizeof buf) < 1) {
    exit(0);
  }
  else {
    exit(1);
  }
}], [
    ac_cv_strerror_r_rc_int=yes ], [
    ac_cv_strerror_r_rc_int=no ], [
    ac_cv_strerror_r_rc_int=no ] )
if test "x$ac_cv_strerror_r_rc_int" = xyes; then
  AC_DEFINE(STRERROR_R_RC_INT)
  msg="int"
else
  msg="pointer"
fi
AC_MSG_RESULT([$msg])
] )
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
dnl  all "." and "-" chars. If the 3rd parameter is "yes" then instead of
dnl  setting to 1 or 0, we set FLAG-TO-SET to yes or no.
dnl  
AC_DEFUN(APR_FLAG_HEADERS,[
AC_CHECK_HEADERS($1)
for aprt_i in $1
do
    ac_safe=`echo "$aprt_i" | sed 'y%./+-%__p_%'`
    aprt_2=`echo "$aprt_i" | sed -e 's%/%_%g' -e 's/\.//g' -e 's/-//g'`
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

dnl Iteratively interpolate the contents of the second argument
dnl until interpolation offers no new result. Then assign the
dnl final result to $1.
dnl
dnl Example:
dnl
dnl foo=1
dnl bar='${foo}/2'
dnl baz='${bar}/3'
dnl APR_EXPAND_VAR(fraz, $baz)
dnl   $fraz is now "1/2/3"
dnl 
AC_DEFUN(APR_EXPAND_VAR,[
ap_last=
ap_cur="$2"
while test "x${ap_cur}" != "x${ap_last}";
do
  ap_last="${ap_cur}"
  ap_cur=`eval "echo ${ap_cur}"`
done
$1="${ap_cur}"
])

dnl
dnl Removes the value of $3 from the string in $2, strips of any leading
dnl slashes, and returns the value in $1.
dnl
dnl Example:
dnl orig_path="${prefix}/bar"
dnl APR_PATH_RELATIVE(final_path, $orig_path, $prefix)
dnl    $final_path now contains "bar"
AC_DEFUN(APR_PATH_RELATIVE,[
ap_stripped=`echo $2 | sed -e "s#^$3##"`
# check if the stripping was successful
if test "x$2" != "x${ap_stripped}"; then
    # it was, so strip of any leading slashes
    $1="`echo ${ap_stripped} | sed -e 's#^/*##'`"
else
    # it wasn't so return the original
    $1="$2"
fi
])

