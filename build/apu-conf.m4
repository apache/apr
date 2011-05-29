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
dnl custom autoconf rules for APRUTIL
dnl

dnl
dnl APU_FIND_APR: figure out where APR is located
dnl
AC_DEFUN([APU_FIND_APR], [

  dnl use the find_apr.m4 script to locate APR. sets apr_found and apr_config
  APR_FIND_APR(,,,[2])
  if test "$apr_found" = "no"; then
    AC_MSG_ERROR(APR could not be located. Please use the --with-apr option.)
  fi

  APR_BUILD_DIR="`$apr_config --installbuilddir`"

  dnl make APR_BUILD_DIR an absolute directory (we'll need it in the
  dnl sub-projects in some cases)
  APR_BUILD_DIR="`cd $APR_BUILD_DIR && pwd`"

  APR_INCLUDES="`$apr_config --includes`"
  APR_LIBS="`$apr_config --link-libtool --libs`"
  APR_SO_EXT="`$apr_config --apr-so-ext`"
  APR_LIB_TARGET="`$apr_config --apr-lib-target`"

  AC_SUBST(APR_INCLUDES)
  AC_SUBST(APR_LIBS)
  AC_SUBST(APR_BUILD_DIR)
])


dnl
dnl APU_CHECK_CRYPT_R_STYLE
dnl
dnl  Decide which of a couple of flavors of crypt_r() is necessary for
dnl  this platform.
dnl
AC_DEFUN([APU_CHECK_CRYPT_R_STYLE], [

AC_CACHE_CHECK([style of crypt_r], apr_cv_crypt_r_style, 
[AC_TRY_COMPILE([#include <crypt.h>],
 [CRYPTD buffer;
  crypt_r("passwd", "hash", &buffer);], 
 [apr_cv_crypt_r_style=cryptd],
 [AC_TRY_COMPILE([#include <crypt.h>],
  [struct crypt_data buffer;
   crypt_r("passwd", "hash", &buffer);], 
  [apr_cv_crypt_r_style=struct_crypt_data],
  [apr_cv_crypt_r_style=none])])])

if test "$apr_cv_crypt_r_style" = "cryptd"; then
   AC_DEFINE(CRYPT_R_CRYPTD, 1, [Define if crypt_r has uses CRYPTD])
elif test "$apr_cv_crypt_r_style" = "struct_crypt_data"; then
   AC_DEFINE(CRYPT_R_STRUCT_CRYPT_DATA, 1, [Define if crypt_r uses struct crypt_data])
fi
])
