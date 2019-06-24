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
dnl Crypto module
dnl

dnl
dnl APU_CHECK_CRYPTO: look for crypto libraries and headers
dnl
AC_DEFUN([APU_CHECK_CRYPTO], [
  apu_have_crypto=0
  apu_have_crypto_prng=0
  apu_have_openssl=0
  apu_have_nss=0
  apu_have_commoncrypto=0

  old_libs="$LIBS"
  old_cppflags="$CPPFLAGS"
  old_ldflags="$LDFLAGS"

  AC_ARG_WITH([crypto], [APR_HELP_STRING([--with-crypto], [enable crypto support])],
  [
    cryptolibs="openssl nss commoncrypto"

    if test "$withval" = "yes"; then

      crypto_library_enabled=0
      for cryptolib in $cryptolibs; do
        eval v=\$with_$cryptolib
        if test "$v" != "" -a "$v" != "no"; then
          crypto_library_enabled=1
        fi
      done

      if test "$crypto_library_enabled" = "0"; then
        for cryptolib in $cryptolibs; do
          eval v=\$with_$cryptolib
          if test "$v" != "no"; then
            eval with_$cryptolib=yes
            crypto_library_enabled=1
          fi
        done
	if test "$crypto_library_enabled" = "1"; then
          AC_MSG_NOTICE([Crypto was requested but no crypto library was found; autodetecting possible libraries])
        else
          AC_ERROR([Crypto was requested but all possible crypto libraries were disabled.])
	fi
      fi

      APU_CHECK_CRYPTO_OPENSSL
      APU_CHECK_CRYPTO_NSS
      APU_CHECK_CRYPTO_COMMONCRYPTO
      dnl add checks for other varieties of ssl here
      if test "$apu_have_crypto" = "0"; then
        AC_ERROR([Crypto was requested but no crypto library could be enabled; specify the location of a crypto library using --with-openssl, --with-nss, and/or --with-commoncrypto.])
      elif test "$apu_have_openssl" = "1"; then
        dnl PRNG only implemented with openssl for now
        apu_have_crypto_prng=1
      fi
    fi
  ], [
      apu_have_crypto=0
      apu_have_crypto_prng=0
  ])

  AC_SUBST(apu_have_crypto)
  AC_SUBST(apu_have_crypto_prng)

])
dnl

AC_DEFUN([APU_CHECK_CRYPTO_OPENSSL], [
  openssl_have_headers=0
  openssl_have_libs=0

  old_libs="$LIBS"
  old_cppflags="$CPPFLAGS"
  old_ldflags="$LDFLAGS"

  AC_ARG_WITH([openssl], 
  [APR_HELP_STRING([--with-openssl=DIR], [specify location of OpenSSL])],
  [
    if test "$withval" = "yes"; then
      AC_CHECK_HEADERS(openssl/x509.h, [openssl_have_headers=1])
      AC_CHECK_LIB(crypto, EVP_CIPHER_CTX_new, openssl_have_libs=1)
      if test "$openssl_have_headers" != "0" && test "$openssl_have_libs" != "0"; then
        apu_have_openssl=1
      fi
    elif test "$withval" = "no"; then
      apu_have_openssl=0
    else

      openssl_CPPFLAGS="-I$withval/include"
      openssl_LDFLAGS="-L$withval/lib "

      APR_ADDTO(CPPFLAGS, [$openssl_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$openssl_LDFLAGS])

      AC_MSG_NOTICE(checking for openssl in $withval)
      AC_CHECK_HEADERS(openssl/x509.h, [openssl_have_headers=1])
      AC_CHECK_LIB(crypto, EVP_CIPHER_CTX_new, openssl_have_libs=1)
      if test "$openssl_have_headers" != "0" && test "$openssl_have_libs" != "0"; then
        apu_have_openssl=1
        APR_ADDTO(LDFLAGS, [-L$withval/lib])
        APR_ADDTO(INCLUDES, [-I$withval/include])
      fi

      AC_CHECK_DECLS([EVP_PKEY_CTX_new, OPENSSL_init_crypto], [], [],
                     [#include <openssl/evp.h>])

    fi
  ], [
    apu_have_openssl=0
  ])

  AC_SUBST(apu_have_openssl)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_openssl" = "1"; then
    APR_ADDTO(LDADD_crypto_openssl, [$openssl_LDFLAGS -lcrypto])
    apu_have_crypto=1

    AC_MSG_CHECKING([for const input buffers in OpenSSL])
    AC_TRY_COMPILE([#include <openssl/rsa.h>],
        [ const unsigned char * buf;
          unsigned char * outbuf;
          RSA rsa;

                RSA_private_decrypt(1,
                                                        buf,
                                                        outbuf,
                                                        &rsa,
                                                        RSA_PKCS1_PADDING);

        ],
        [AC_MSG_RESULT([yes])]
        [AC_DEFINE([CRYPTO_OPENSSL_CONST_BUFFERS], 1, [Define that OpenSSL uses const buffers])],
        [AC_MSG_RESULT([no])])

  fi  
  AC_SUBST(LDADD_crypto_openssl)
  AC_SUBST(apu_have_crypto)

  LIBS="$old_libs"
  CPPFLAGS="$old_cppflags"
  LDFLAGS="$old_ldflags"
])

AC_DEFUN([APU_CHECK_CRYPTO_NSS], [
  nss_have_libs=0

  old_libs="$LIBS"
  old_cppflags="$CPPFLAGS"
  old_ldflags="$LDFLAGS"

  AC_ARG_WITH([nss], 
  [APR_HELP_STRING([--with-nss=DIR], [specify location of NSS])],
  [
    if test "$withval" = "yes"; then
      AC_PATH_TOOL([PKG_CONFIG], [pkg-config])
      if test -n "$PKG_CONFIG"; then
        nss_CPPFLAGS=`$PKG_CONFIG --cflags-only-I nss`
        nss_LDFLAGS=`$PKG_CONFIG --libs nss`
        APR_ADDTO(CPPFLAGS, [$nss_CPPFLAGS])
        APR_ADDTO(LDFLAGS, [$nss_LDFLAGS])
      fi
      nss_have_prerrorh=0
      nss_have_nssh=0
      nss_have_pk11pubh=0
      AC_CHECK_HEADERS(prerror.h, [nss_have_prerrorh=1])
      AC_CHECK_HEADERS(nss/nss.h nss.h, [nss_have_nssh=1])
      AC_CHECK_HEADERS(nss/pk11pub.h pk11pub.h, [nss_have_pk11pubh=1])
      nss_have_headers=${nss_have_prerrorh}${nss_have_nssh}${nss_have_pk11pubh}
      AC_CHECK_LIB(nspr4, PR_Initialize, AC_CHECK_LIB(nss3, PK11_CreatePBEV2AlgorithmID, [nss_have_libs=1],,-lnspr4))
      if test "$nss_have_headers" = "111" && test "$nss_have_libs" != "0"; then
        apu_have_nss=1
      fi
    elif test "$withval" = "no"; then
      apu_have_nss=0
    elif test "x$withval" != "x"; then

      nss_CPPFLAGS="-I$withval/include/nss -I$withval/include/nss3 -I$withval/include/nspr -I$withval/include/nspr4 -I$withval/include -I$withval/../public"
      nss_LDFLAGS="-L$withval/lib -L$withval/lib/nss -L$withval/lib/nspr "

      APR_ADDTO(CPPFLAGS, [$nss_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$nss_LDFLAGS])

      AC_MSG_NOTICE(checking for nss in $withval)
      nss_have_prerrorh=0
      nss_have_nssh=0
      nss_have_pk11pubh=0
      AC_CHECK_HEADERS(prerror.h, [nss_have_prerrorh=1])
      AC_CHECK_HEADERS(nss/nss.h nss.h, [nss_have_nssh=1])
      AC_CHECK_HEADERS(nss/pk11pub.h pk11pub.h, [nss_have_pk11pubh=1])
      nss_have_headers=${nss_have_prerrorh}${nss_have_nssh}${nss_have_pk11pubh}
      AC_CHECK_LIB(nspr4, PR_Initialize, AC_CHECK_LIB(nss3, PK11_CreatePBEV2AlgorithmID, [nss_have_libs=1],,-lnspr4))
      if test "$nss_have_headers" = "111" && test "$nss_have_libs" != "0"; then
        apu_have_nss=1
      fi

    fi
    if test "$apu_have_nss" != "0"; then
      APR_ADDTO(INCLUDES, [$nss_CPPFLAGS])
    fi
  ], [
    apu_have_nss=0
  ])

  AC_SUBST(apu_have_nss)

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_nss" = "1"; then
    APR_ADDTO(LDADD_crypto_nss, [$nss_LDFLAGS -lnspr4 -lnss3])
    apu_have_crypto=1
  fi
  AC_SUBST(LDADD_crypto_nss)
  AC_SUBST(apu_have_crypto)

  LIBS="$old_libs"
  CPPFLAGS="$old_cppflags"
  LDFLAGS="$old_ldflags"
])

AC_DEFUN([APU_CHECK_CRYPTO_COMMONCRYPTO], [
  apu_have_commoncrypto=0
  commoncrypto_have_headers=0
  commoncrypto_have_libs=0

  old_libs="$LIBS"
  old_cppflags="$CPPFLAGS"
  old_ldflags="$LDFLAGS"

  AC_ARG_WITH([commoncrypto], 
  [APR_HELP_STRING([--with-commoncrypto=DIR], [specify location of CommonCrypto])],
  [
    if test "$withval" = "yes"; then
      AC_CHECK_HEADERS(CommonCrypto/CommonKeyDerivation.h, [commoncrypto_have_headers=1])
      AC_CHECK_LIB(System, CCKeyDerivationPBKDF, AC_CHECK_LIB(System, CCCryptorCreate, [commoncrypto_have_libs=1],,-lcrypto))
      if test "$commoncrypto_have_headers" != "0" && test "$commoncrypto_have_libs" != "0"; then
        apu_have_commoncrypto=1
      fi
    elif test "$withval" = "no"; then
      apu_have_commoncrypto=0
    else

      commoncrypto_CPPFLAGS="-I$withval/include"
      commoncrypto_LDFLAGS="-L$withval/lib "

      APR_ADDTO(CPPFLAGS, [$commoncrypto_CPPFLAGS])
      APR_ADDTO(LDFLAGS, [$commoncrypto_LDFLAGS])

      AC_MSG_NOTICE(checking for commoncrypto in $withval)
      AC_CHECK_HEADERS(CommonCrypto/CommonKeyDerivation.h, [commoncrypto_have_headers=1])
      AC_CHECK_LIB(System, CCKeyDerivationPBKDF, AC_CHECK_LIB(System, CCCryptorCreate, [commoncrypto_have_libs=1],,-lcrypto))
      if test "$commoncrypto_have_headers" != "0" && test "$commoncrypto_have_libs" != "0"; then
        apu_have_commoncrypto=1
        APR_ADDTO(LDFLAGS, [-L$withval/lib])
        APR_ADDTO(INCLUDES, [-I$withval/include])
      fi

    fi
  ], [
    apu_have_commoncrypto=0
  ])

  dnl Since we have already done the AC_CHECK_LIB tests, if we have it, 
  dnl we know the library is there.
  if test "$apu_have_commoncrypto" = "1"; then
    apu_have_crypto=1
  fi
  AC_SUBST(apu_have_commoncrypto)
  AC_SUBST(LDADD_crypto_commoncrypto)
  AC_SUBST(apu_have_crypto)

  LIBS="$old_libs"
  CPPFLAGS="$old_cppflags"
  LDFLAGS="$old_ldflags"
])

dnl
