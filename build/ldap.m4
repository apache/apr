dnl -------------------------------------------------------- -*- autoconf -*-
dnl Copyright 2006 The Apache Software Foundation or its licensors, as
dnl applicable.
dnl
dnl Licensed under the Apache License, Version 2.0 (the "License");
dnl you may not use this file except in compliance with the License.
dnl You may obtain a copy of the License at
dnl
dnl     http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl Unless required by applicable law or agreed to in writing, software
dnl distributed under the License is distributed on an "AS IS" BASIS,
dnl WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl See the License for the specific language governing permissions and
dnl limitations under the License.

dnl
dnl LDAP module
dnl

# APU_SEARCH_FRAMEWORKS(FUNCTION, SEARCH-FRAMEWORKS,
#                       [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                       [OTHER-LIBRARIES])
# --------------------------------------------------------
# Search for a framework defining FUNC, if it's not already available.
AC_DEFUN([APU_SEARCH_FRAMEWORKS],
[AS_VAR_PUSHDEF([ac_Search], [ac_cv_search_$1])dnl
AC_CACHE_CHECK([for framework containing $1], [ac_Search],
[ac_func_search_save_LIBS=$LIBS
AC_LANG_CONFTEST([AC_LANG_CALL([], [$1])])
for ac_framework in '' $2
do
  if test -z "$ac_framework"; then
    ac_res="none required"
  else
    ac_res="-framework $ac_framework"
    LIBS="-framework $ac_framework $5 $ac_func_search_save_LIBS"
  fi
  AC_LINK_IFELSE([], [AS_VAR_SET([ac_Search], [$ac_res])])
  AS_VAR_SET_IF([ac_Search], [break])
done
AS_VAR_SET_IF([ac_Search], , [AS_VAR_SET([ac_Search], [no])])
rm conftest.$ac_ext
LIBS=$ac_func_search_save_LIBS])
AS_VAR_COPY([ac_res], [ac_Search])
AS_IF([test "$ac_res" != no],
  [test "$ac_res" = "none required" || LIBS="$ac_res $LIBS"
  $3],
      [$4])
AS_VAR_POPDEF([ac_Search])dnl
])

# APU_CHECK_FRAMEWORK(FRAMEWORK, FUNCTION,
#                     [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                     [OTHER-FRAMEWORKS])
# ------------------------------------------------------
#
AC_DEFUN([APU_CHECK_FRAMEWORK],
[m4_ifval([$3], , [AH_CHECK_LIB([$1])])dnl
AS_LITERAL_WORD_IF([$1],
	      [AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1_$2])],
	      [AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1""_$2])])dnl
AC_CACHE_CHECK([for $2 in -framework $1], [ac_Lib],
[apu_check_framework_save_LIBS=$LIBS
LIBS="-framework $1 $5 $LIBS"
AC_LINK_IFELSE([AC_LANG_CALL([], [$2])],
	       [AS_VAR_SET([ac_Lib], [yes])],
	       [AS_VAR_SET([ac_Lib], [no])])
LIBS=$apu_check_framework_save_LIBS])
AS_VAR_IF([ac_Lib], [yes],
      [m4_default([$3], [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_FRAMEWORK_$1))
  LIBS="-framework $1 $LIBS"
])],
      [$4])
AS_VAR_POPDEF([ac_Lib])dnl
])# APU_CHECK_FRAMEWORK



dnl 
dnl Find a particular LDAP library
dnl
AC_DEFUN([APU_FIND_LDAPLIB], [
  if test ${apu_have_ldap} != "1"; then
    ldaplib=$1
    extralib=$2
    # Clear the cache entry for subsequent APU_FIND_LDAPLIB invocations.
    changequote(,)
    ldaplib_cache_id="`echo $ldaplib | sed -e 's/[^a-zA-Z0-9_]/_/g'`"
    changequote([,])
    unset ac_cv_lib_${ldaplib_cache_id}_ldap_init
    unset ac_cv_lib_${ldaplib_cache_id}___ldap_init
    AC_CHECK_LIB(${ldaplib}, ldap_init, 
      [
        LDADD_ldap_found="-l${ldaplib} ${extralib}"
        AC_CHECK_LIB(${ldaplib}, ldap_sasl_interactive_bind, apu_have_ldap_sasl_interactive_bind="1", , ${extralib})
        AC_CHECK_LIB(${ldaplib}, ldap_connect, apu_have_ldap_connect="1", , ${extralib})
        apu_have_ldap="1";
      ], , ${extralib})
  fi
])


dnl
dnl APU_FIND_LDAP: figure out where LDAP is located
dnl
AC_DEFUN([APU_FIND_LDAP],  [

echo $ac_n "${nl}checking for ldap support..."

apu_have_ldap_sasl_interactive_bind="0"
apu_have_ldap_connect="0"
apu_have_ldap="0";
apu_have_ldap_openldap="0"
apu_have_ldap_microsoft="0"
apu_have_ldap_tivoli="0"
apu_have_ldap_zos="0"
apu_have_ldap_other="0"
LDADD_ldap_found=""

AC_ARG_WITH(lber,[  --with-lber=library     lber library to use],
  [
    if test "$withval" = "yes"; then
      apu_liblber_name="lber"
    else
      apu_liblber_name="$withval"
    fi
  ],
  [
    apu_liblber_name="lber"
  ])

AC_ARG_WITH(ldap-include,[  --with-ldap-include=path  path to ldap include files with trailing slash])
AC_ARG_WITH(ldap-lib,[  --with-ldap-lib=path    path to ldap lib file])
AC_ARG_WITH(ldap,[  --with-ldap             enable ldap],
  [
    if test "$with_ldap" = "yes"; then

      save_cppflags="$CPPFLAGS"
      save_ldflags="$LDFLAGS"
      save_libs="$LIBS"

      if test -n "$with_ldap_include"; then
        ldap_CPPFLAGS="-I$with_ldap_include"
        APR_ADDTO(CPPFLAGS, [$ldap_CPPFLAGS])
      fi
      if test -n "$with_ldap_lib"; then
        ldap_LDFLAGS="-L$with_ldap_lib"
        APR_ADDTO(LDFLAGS, [$ldap_LDFLAGS])
      fi

      if test -z "$with_ldap_include" && test -z "$with_ldap_lib"; then
        APU_CHECK_FRAMEWORK(LDAP, ldap_init,
        [
          LDADD_ldap_found="-framework LDAP"
          apu_have_ldap="1";
        ])
      fi

      APU_FIND_LDAPLIB("ldap", "-llber")

      if test ${apu_have_ldap} != "1"; then
        AC_MSG_ERROR(could not find an LDAP library)
      else
        APR_ADDTO(LDADD_ldap, [$LDADD_ldap_found $ldap_LDFLAGS])
      fi

      AC_CHECK_LIB($apu_liblber_name, ber_init,
        [APR_ADDTO(LDADD_ldap, [-l${apu_liblber_name}])])

      AC_CHECK_HEADERS(lber.h, lber_h=["#include <lber.h>"])
      AC_CHECK_HEADERS(ldap.h, ldap_h=["#include <ldap.h>"])

      if test -n "$ldap_h"; then
        apr_cv_hdr_ldap_h=yes
      fi

      if test "$apr_cv_hdr_ldap_h" = "yes"; then
        AC_CACHE_CHECK([for LDAP toolkit],
                       [apr_cv_ldap_toolkit], [
          if test "x$apr_cv_ldap_toolkit" = "x"; then
            AC_EGREP_CPP([OpenLDAP], [$lber_h
                         $ldap_h 
                         LDAP_VENDOR_NAME], [apu_have_ldap_openldap="1"
                                             apr_cv_ldap_toolkit="OpenLDAP"])
          fi
          if test "x$apr_cv_ldap_toolkit" = "x"; then
            AC_EGREP_CPP([Sun Microsystems Inc.], [$lber_h
                         $ldap_h
                         LDAP_VENDOR_NAME], [apu_have_ldap_solaris="1"
                                             apr_cv_ldap_toolkit="Solaris"])
          fi
          if test "x$apr_cv_ldap_toolkit" = "x"; then
            AC_EGREP_CPP([Microsoft Corporation.], [$lber_h
                         $ldap_h
                         LDAP_VENDOR_NAME], [apu_have_ldap_microsoft="1"
                                             apr_cv_ldap_toolkit="Microsoft"])
          fi
          if test "x$apr_cv_ldap_toolkit" = "x"; then
            AC_EGREP_CPP([International Business Machines], [$lber_h
                         $ldap_h
                         LDAP_VENDOR_NAME], [apu_have_ldap_tivoli="1"
                                             apr_cv_ldap_toolkit="Tivoli"])
          fi
          if test "x$apr_cv_ldap_toolkit" = "x"; then
            case "$host" in
            *-ibm-os390)
              AC_EGREP_CPP([IBM], [$lber_h
                                   $ldap_h], [apu_have_ldap_zos="1"
                                              apr_cv_ldap_toolkit="z/OS"])
              ;;
            esac
          fi
          if test "x$apr_cv_ldap_toolkit" = "x"; then
            apu_have_ldap_other="1"
            apr_cv_ldap_toolkit="unknown"
          fi
        ])
      fi

      CPPFLAGS=$save_cppflags
      LDFLAGS=$save_ldflags
      LIBS=$save_libs
    fi
  ])

AC_CHECK_HEADERS([sasl.h sasl/sasl.h])

AC_SUBST(ldap_h)
AC_SUBST(lber_h)
AC_SUBST(apu_have_ldap_sasl_interactive_bind)
AC_SUBST(apu_have_ldap_connect)
AC_SUBST(apu_have_ldap)
AC_SUBST(apu_have_ldap_openldap)
AC_SUBST(apu_have_ldap_solaris)
AC_SUBST(apu_have_ldap_microsoft)
AC_SUBST(apu_have_ldap_tivoli)
AC_SUBST(apu_have_ldap_zos)
AC_SUBST(apu_have_ldap_other)
AC_SUBST(LDADD_ldap)

])


