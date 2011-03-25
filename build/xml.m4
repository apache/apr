dnl -------------------------------------------------------- -*- autoconf -*-
dnl Licensed to the Apache Software Foundation (ASF) under one or more
dnl contributor license agreements.  See the NOTICE file distributed with
dnl this work for additional information regarding copyright ownership.
dnl The ASF licenses this file to You under the Apache License, Version 2.0
dnl (the "License"); you may not use this file except in compliance with
dnl the License.  You may obtain a copy of the License at
dnl
dnl     http://www.apache.org/licenses/LICENSE-2.0
dnl
dnl Unless required by applicable law or agreed to in writing, software
dnl distributed under the License is distributed on an "AS IS" BASIS,
dnl WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
dnl See the License for the specific language governing permissions and
dnl limitations under the License.


dnl
dnl APU_TRY_EXPAT_LINK(
dnl      test-message, cache-var-name, hdrs, libs,
dnl      [actions-on-success], [actions-on-failure])
dnl         
dnl Tests linking against expat with libraries 'libs' and includes
dnl 'hdrs', passing message + cache-var-name to AC_CACHE_CHECK.
dnl On success, sets $expat_libs to libs, sets $apu_has_expat to 1, 
dnl and runs actions-on-success; on failure runs actions-on-failure.
dnl
AC_DEFUN([APU_TRY_EXPAT_LINK], [
AC_CACHE_CHECK([$1], [$2], [
  apu_expat_LIBS=$LIBS
  apu_expat_CPPFLAGS=$CPPFLAGS
  LIBS="$LIBS $4"
  CPPFLAGS="$CPPFLAGS $INCLUDES"
  AC_TRY_LINK([#include <stdlib.h>
#include <$3>], [XML_ParserCreate(NULL);],
    [$2=yes], [$2=no])
  LIBS=$apu_expat_LIBS
  CPPFLAGS=$apu_expat_CPPFLAGS
])

if test $[$2] = yes; then
   AC_DEFINE([HAVE_]translit([$3], [a-z./], [A-Z__]), 1,
             [Define if $3 is available])
   apu_expat_libs="$4"
   apu_has_expat=1
   $5
   AC_SUBST(apu_has_expat)
else
   apu_has_expat=0
   $6
fi
])

dnl
dnl APU_SYSTEM_EXPAT: tests for a system expat installation
dnl If present, sets $apu_has_expat to 1 and adjusts LDFLAGS/CPPFLAGS
dnl appropriately.  This is mostly for compatibility with existing
dnl expat releases; all but the first APU_TRY_EXPAT_LINK call could
dnl be dropped later.
dnl
AC_DEFUN([APU_SYSTEM_EXPAT], [
 
  APU_TRY_EXPAT_LINK([Expat 1.95.x], apu_cv_expat_system, 
    [expat.h], [-lexpat])

  if test $apu_has_expat = 0; then
    APU_TRY_EXPAT_LINK([old Debian-packaged expat], apu_cv_expat_debian,
       [xmltok/xmlparse.h], [-lxmlparse -lxmltok])
  fi

  if test $apu_has_expat = 0; then
    APU_TRY_EXPAT_LINK([old FreeBSD-packaged expat], apu_cv_expat_freebsd,
       [xml/xmlparse.h], [-lexpat])
  fi

  if test $apu_has_expat = 0; then
    APU_TRY_EXPAT_LINK([Expat 1.0/1.1], apu_cv_expat_1011,
       [xmlparse/xmlparse.h], [-lexpat])
  fi

  if test $apu_has_expat = 0; then
    APR_ADDTO(LDFLAGS, [-L/usr/local/lib])
    APR_ADDTO(INCLUDES, [-I/usr/local/include])
 
    APU_TRY_EXPAT_LINK([Expat 1.95.x in /usr/local], 
       apu_cv_expat_usrlocal, [expat.h], [-lexpat], [], [
       APR_REMOVEFROM(LDFLAGS, [-L/usr/local/lib])
       APR_REMOVEFROM(INCLUDES, [-I/usr/local/include])
      ])
  fi
])


dnl
dnl APU_FIND_EXPAT: figure out where EXPAT is located (or use bundled)
dnl
AC_DEFUN([APU_FIND_EXPAT], [

save_cppflags="$CPPFLAGS"

apu_has_expat=0

AC_ARG_WITH([expat],
[  --with-expat=DIR        specify Expat location], [
  if test "$withval" = "yes"; then
    AC_MSG_ERROR([a directory must be specified for --with-expat])
  elif test "$withval" = "no"; then
    if test "$apu_has_libxml2" != "1"; then
      AC_MSG_ERROR([An XML parser is required!  If you disable expat, you must select --with-libxml2])
    fi
  else
    # Add given path to standard search paths if appropriate:
    if test "$apu_has_libxml2" = "1"; then
      AC_MSG_ERROR(Cannot build with both expat and libxml2 - please select one)
    fi
    if test "$withval" != "/usr"; then
      APR_ADDTO(INCLUDES, [-I$withval/include])
      APR_ADDTO(LDFLAGS, [-L$withval/lib])
    fi
  fi
])

if test "$apu_has_libxml2" != "1"; then
  APU_SYSTEM_EXPAT

  APR_ADDTO(APRUTIL_EXPORT_LIBS, [$apu_expat_libs])
  APR_ADDTO(LIBS, [$apu_expat_libs])

  APR_XML_DIR=$bundled_subdir
  AC_SUBST(APR_XML_DIR)
fi

CPPFLAGS=$save_cppflags
])


dnl
dnl APU_FIND_LIBXML2: figure out where LIBXML2 is located (or use bundled)
dnl
AC_DEFUN([APU_FIND_LIBXML2], [

save_cppflags="$CPPFLAGS"

apu_has_libxml2=0
apu_try_libxml2=0

AC_ARG_WITH([libxml2],
[  --with-libxml2=DIR      specify libxml2 location], [
  if test "$withval" = "yes"; then
    apu_try_libxml2=1
    APR_ADDTO(INCLUDES, [-I/usr/include/libxml2])
  elif test "$withval" != "no"; then
    apu_try_libxml2=1
    APR_ADDTO(INCLUDES, [-I$withval/include/libxml2])
    if test "$withval" != "/usr"; then
      APR_ADDTO(LDFLAGS, [-L$withval/lib])
    fi
  fi
  if test ${apu_try_libxml2} = "1" ; then

    #test for libxml2
    AC_CACHE_CHECK([libxml2], [apu_cv_libxml2], [
      apu_libxml2_CPPFLAGS=$CPPFLAGS
      apu_libxml2_LIBS="$LIBS -lxml2"
      CPPFLAGS="$CPPFLAGS $INCLUDES"
      LIBS="$LIBS -lxml2"
      AC_TRY_LINK(
        [#include <libxml/parser.h>],
        [xmlSAXHandler sax; xmlCreatePushParserCtxt(&sax, NULL, NULL, 0, NULL);],
        [apu_cv_libxml2=yes],
        [apu_cv_libxml2=no],
      )
      CPPFLAGS=$apu_libxml2_CPPFLAGS
      LIBS=$apu_libxml2_LIBS
    ])
    if test $apu_cv_libxml2 = yes ; then
      AC_DEFINE([HAVE_LIBXML2_H], 1, [libxml2 found])
      apu_has_libxml2=1
    else
      apu_has_libxml2=0
    fi
  fi
])
AC_SUBST(apu_has_libxml2)

if test ${apu_has_libxml2} = "1" ; then
  APR_ADDTO(APRUTIL_EXPORT_LIBS, [-lxml2])
  APR_ADDTO(LIBS, [-lxml2])
fi

CPPFLAGS=$save_cppflags
])

dnl
dnl APU_FIND_XML: Find an XML library
dnl
dnl Logic: we need exactly one but not both XML libraries
dnl        Make expat the default for back-compatibility.
dnl        Use libxml2 if a --with-libxml2 is specified (and works),
dnl        otherwise expat.
dnl
AC_DEFUN([APU_FIND_XML], [
APU_FIND_LIBXML2
APU_FIND_EXPAT

if test ${apu_has_expat} = "1" && test ${apu_has_libxml2} = "1" ; then
  AC_MSG_ERROR(Cannot build with both expat and libxml2 - please select one)
elif test ${apu_has_expat} != "1" && test ${apu_has_libxml2} != "1" ; then
  AC_MSG_ERROR(No XML parser found!  Please specify --with-expat or --with-libxml2)
fi
])
