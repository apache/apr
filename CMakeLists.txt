# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Read README.cmake before using this.

# CMAKE_MINIMUM_REQUIRED should be the first directive in the file:
# https://cmake.org/cmake/help/latest/command/cmake_minimum_required.html
CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(APR C)

# Enable support for MSVC runtime library selection by abstraction
# if supported by CMake.
IF(POLICY CMP0091)
  CMAKE_POLICY(SET CMP0091 NEW)
ENDIF()

# Add simple support CMAKE_WARNING_AS_ERROR if CMake doesn't
# support it.
IF (CMAKE_VERSION VERSION_LESS 3.24)
  IF(CMAKE_WARNING_AS_ERROR AND MSVC)
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
  ENDIF()
ENDIF()

OPTION(APR_MINIMAL_BUILD    "Create minimal APR build"       OFF)
IF(NOT APR_MINIMAL_BUILD)
    FIND_PACKAGE(EXPAT)
    FIND_PACKAGE(LibXml2)
    FIND_PACKAGE(OpenSSL)
    FIND_PACKAGE(Iconv)
    FIND_PACKAGE(SQLite3)
    OPTION(APU_HAVE_ODBC     "Build ODBC DBD driver"         ON)
ELSE()
    OPTION(APU_HAVE_ODBC     "Build ODBC DBD driver"         OFF)
ENDIF()

SET(apr_name    apr-2)
SET(apr_libname libapr-2)

IF(NOT APU_USE_EXPAT AND NOT APU_USE_LIBXML2 AND NOT APU_USE_XMLLITE)
  OPTION(APU_USE_EXPAT      "Use Expat"                    OFF)
  OPTION(APU_USE_LIBXML2    "Use LibXml2"                  OFF)
  OPTION(APU_USE_XMLLITE    "Use XmlLite"                  ON)
ENDIF()

IF(APU_USE_EXPAT AND NOT EXPAT_FOUND)
  MESSAGE(FATAL_ERROR "Expat not found")
ENDIF()

IF(APU_USE_LIBXML2 AND NOT LIBXML2_FOUND)
  MESSAGE(FATAL_ERROR "LibXml2 not found")
ENDIF()

OPTION(APR_INSTALL_PRIVATE_H  "Install selected private .h files (for httpd)"  OFF)
OPTION(APU_HAVE_SQLITE3     "Build SQLite3 DBD driver"     OFF)
OPTION(APU_HAVE_CRYPTO      "Crypto support"               OFF)
OPTION(APU_HAVE_ICONV       "Xlate support"                OFF)
OPTION(APR_HAVE_IPV6        "IPv6 support"                 ON)
OPTION(INSTALL_PDB          "Install .pdb files (if generated)"  ON)
OPTION(APR_BUILD_TESTAPR    "Build the test suite"         ON)
OPTION(APR_BUILD_SHARED     "Build shared libraries"       ON)
OPTION(APR_BUILD_STATIC     "Build static libraries."      ON)
OPTION(TEST_STATIC_LIBS     "Test programs use APR static libraries instead of shared libraries?" OFF)

IF(NOT APR_BUILD_SHARED AND NOT APR_BUILD_STATIC)
  MESSAGE(FATAL_ERROR "Both APR_BUILD_SHARED and APR_BUILD_STATIC have been disabled")
ENDIF()

IF(NOT APR_BUILD_STATIC AND TEST_STATIC_LIBS)
  MESSAGE(FATAL_ERROR "APR_BUILD_STATIC have been disabled, but TEST_STATIC_LIBS is enabled")
ENDIF()

IF(NOT APU_USE_EXPAT AND NOT APU_USE_LIBXML2 AND NOT APU_USE_XMLLITE)
  MESSAGE(FATAL_ERROR "Either Expat or LibXml2 or XmlLite must be selected")
ENDIF()
IF(APU_USE_EXPAT AND APU_USE_LIBXML2)
  MESSAGE(FATAL_ERROR "Only one of Expat and LibXml2 can be selected")
ENDIF()

IF(APU_HAVE_CRYPTO)
IF(NOT OPENSSL_FOUND)
  MESSAGE(FATAL_ERROR "OpenSSL is the only supported crypto implementation, and it wasn't found!")
ENDIF()
ENDIF()
IF(APU_HAVE_ICONV)
IF(NOT Iconv_FOUND)
  MESSAGE(FATAL_ERROR "Libiconv wasn't found!")
ENDIF()
ENDIF()
IF(APU_HAVE_SQLITE3)
IF(NOT SQLite3_FOUND)
  MESSAGE(FATAL_ERROR "SQLite3 wasn't found!")
ENDIF()
ENDIF()

# create 1-or-0 representation of feature tests for apr.h

SET(apr_have_ipv6_10 0)
SET(apu_have_crypto_10 0)
SET(apu_use_libxml2_10 0)
SET(apu_use_expat_10 0)
SET(apu_use_xmllite_10 0)
SET(apu_have_iconv_10 0)
SET(apu_have_odbc_10 0)
SET(apu_have_sqlite3_10 0)

IF(APR_HAVE_IPV6)
  SET(apr_have_ipv6_10 1)
ENDIF()

IF(APU_HAVE_CRYPTO)
  SET(apu_have_crypto_10 1)
ENDIF()

IF(APU_USE_EXPAT)
  SET(apu_use_expat_10 1)
ELSEIF(APU_USE_LIBXML2)
  SET(apu_use_libxml2_10 1)
ELSE(APU_USE_XMLLITE)
  SET(apu_use_xmllite_10 1)
ENDIF()

IF(APU_HAVE_ICONV)
  SET(apu_have_iconv_10 1)
ENDIF()
IF(APU_HAVE_ODBC)
  SET(apu_have_odbc_10 1)
ENDIF()
IF(APU_HAVE_SQLITE3)
  SET(apu_have_sqlite3_10 1)
ENDIF()

CONFIGURE_FILE(include/apr.hwc
               ${PROJECT_BINARY_DIR}/apr.h)
# "COPYONLY" just because anything else isn't implemented ;)
CONFIGURE_FILE(include/private/apu_select_dbm.hw
               ${PROJECT_BINARY_DIR}/apu_select_dbm.h
               COPYONLY)
CONFIGURE_FILE(include/apu_want.hw
               ${PROJECT_BINARY_DIR}/apu_want.h
               COPYONLY)

ADD_EXECUTABLE(gen_test_char tools/gen_test_char.c)

ADD_CUSTOM_COMMAND(
  COMMENT "Generating character tables, apr_escape_test_char.h, for current locale"
  DEPENDS gen_test_char
  COMMAND $<TARGET_FILE:gen_test_char> > ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
  OUTPUT ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
)
ADD_CUSTOM_TARGET(
  test_char_header ALL
  DEPENDS ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
)

IF(APU_USE_EXPAT)
  SET(XMLLIB_INCLUDE_DIR ${EXPAT_INCLUDE_DIRS})
  SET(XMLLIB_LIBRARIES   ${EXPAT_LIBRARIES})
ELSEIF(APU_USE_LIBXML2)
  SET(XMLLIB_INCLUDE_DIR ${LIBXML2_INCLUDE_DIRS})
  SET(XMLLIB_LIBRARIES   ${LIBXML2_LIBRARIES})
ELSEIF(APU_USE_XMLLITE)
  SET(XMLLIB_INCLUDE_DIR "")
  SET(XMLLIB_LIBRARIES   "xmllite.lib")
ENDIF()

IF(APU_HAVE_ICONV)
  SET(XLATE_INCLUDE_DIR ${Iconv_INCLUDE_DIRS})
  SET(XLATE_LIBRARIES   ${Iconv_LIBRARIES})
ELSE()
  SET(XLATE_INCLUDE_DIR "")
  SET(XLATE_LIBRARIES   "")
ENDIF()
# Generated .h files are stored in PROJECT_BINARY_DIR, not the
# source tree.
#
# BROKEN: not searching PROJECT_BINARY_DIR first, so you have to
#         manually delete apr.h in PROJECT_SOURCE_DIR/include if
#         you've generated apr.h before using a different build

SET(APR_INCLUDE_DIRECTORIES
  ${PROJECT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include/arch/win32
  ${CMAKE_CURRENT_SOURCE_DIR}/include/arch/unix
  ${CMAKE_CURRENT_SOURCE_DIR}/include/private
)

SET(APR_SYSTEM_LIBS
  ws2_32
  rpcrt4
  bcrypt
)

INCLUDE_DIRECTORIES(${APR_INCLUDE_DIRECTORIES} ${XMLLIB_INCLUDE_DIR} ${XLATE_INCLUDE_DIR})

SET(APR_PUBLIC_HEADERS_STATIC
  include/apr_allocator.h
  include/apr_anylock.h
  include/apr_atomic.h
  include/apr_base64.h
  include/apr_buckets.h
  include/apr_crypto.h
  include/apr_cstr.h
  include/apr_date.h
  include/apr_dbd.h
  include/apr_dbm.h
  include/apr_dso.h
  include/apr_env.h
  include/apr_errno.h
  include/apr_escape.h
  include/apr_file_info.h
  include/apr_file_io.h
  include/apr_fnmatch.h
  include/apr_general.h
  include/apr_getopt.h
  include/apr_global_mutex.h
  include/apr_hash.h
  include/apr_hooks.h
  include/apr_inherit.h
  include/apr_lib.h
  include/apr_md4.h
  include/apr_md5.h
  include/apr_memcache.h
  include/apr_mmap.h
  include/apr_network_io.h
  include/apr_optional.h
  include/apr_optional_hooks.h
  include/apr_perms_set.h
  include/apr_poll.h
  include/apr_pools.h
  include/apr_portable.h
  include/apr_proc_mutex.h
  include/apr_queue.h
  include/apr_random.h
  include/apr_redis.h
  include/apr_reslist.h
  include/apr_ring.h
  include/apr_rmm.h
  include/apr_sdbm.h
  include/apr_sha1.h
  include/apr_shm.h
  include/apr_signal.h
  include/apr_siphash.h
  include/apr_skiplist.h
  include/apr_strings.h
  include/apr_strmatch.h
  include/apr_tables.h
  include/apr_thread_cond.h
  include/apr_thread_mutex.h
  include/apr_thread_pool.h
  include/apr_thread_proc.h
  include/apr_thread_rwlock.h
  include/apr_time.h
  include/apr_uri.h
  include/apr_user.h
  include/apr_uuid.h
  include/apr_version.h
  include/apr_want.h
  include/apr_xlate.h
  include/apr_xml.h
  include/apu.h
  include/apu_errno.h
  include/apu_version.h
)
SET(APR_PUBLIC_HEADERS_GENERATED
  ${PROJECT_BINARY_DIR}/apr.h
  ${PROJECT_BINARY_DIR}/apu_want.h
)

SET(APR_SOURCES
  atomic/win32/apr_atomic.c
  atomic/win32/apr_atomic64.c
  buckets/apr_brigade.c
  buckets/apr_buckets.c
  buckets/apr_buckets_alloc.c
  buckets/apr_buckets_eos.c
  buckets/apr_buckets_file.c
  buckets/apr_buckets_flush.c
  buckets/apr_buckets_heap.c
  buckets/apr_buckets_mmap.c
  buckets/apr_buckets_pipe.c
  buckets/apr_buckets_pool.c
  buckets/apr_buckets_refcount.c
  buckets/apr_buckets_simple.c
  buckets/apr_buckets_socket.c
  crypto/apr_crypto.c
  crypto/apr_crypto_prng.c
  crypto/apr_md4.c
  crypto/apr_md5.c
  crypto/apr_passwd.c
  crypto/apr_sha1.c
  crypto/apr_siphash.c
  crypto/crypt_blowfish.c
  crypto/getuuid.c
  crypto/uuid.c
  dbd/apr_dbd.c
  dbm/apr_dbm.c
  dbm/apr_dbm_sdbm.c
  dbm/sdbm/sdbm.c
  dbm/sdbm/sdbm_hash.c
  dbm/sdbm/sdbm_lock.c
  dbm/sdbm/sdbm_pair.c
  dso/win32/dso.c
  encoding/apr_base64.c
  encoding/apr_encode.c
  encoding/apr_escape.c
  file_io/unix/copy.c
  file_io/unix/fileacc.c
  file_io/unix/filepath_util.c
  file_io/unix/fullrw.c
  file_io/unix/mktemp.c
  file_io/unix/tempdir.c
  file_io/win32/buffer.c
  file_io/win32/dir.c
  file_io/win32/filedup.c
  file_io/win32/filepath.c
  file_io/win32/filestat.c
  file_io/win32/filesys.c
  file_io/win32/flock.c
  file_io/win32/open.c
  file_io/win32/pipe.c
  file_io/win32/readwrite.c
  file_io/win32/seek.c
  jose/apr_jose.c
  jose/apr_jose_decode.c
  jose/apr_jose_encode.c
  json/apr_json.c
  json/apr_json_decode.c
  json/apr_json_encode.c
  hooks/apr_hooks.c
  locks/win32/proc_mutex.c
  locks/win32/thread_cond.c
  locks/win32/thread_mutex.c
  locks/win32/thread_rwlock.c
  memcache/apr_memcache.c
  memory/unix/apr_pools.c
  misc/unix/errorcodes.c
  misc/unix/getopt.c
  misc/unix/otherchild.c
  misc/unix/version.c
  misc/win32/charset.c
  misc/win32/env.c
  misc/win32/internal.c
  misc/win32/misc.c
  misc/win32/rand.c
  misc/win32/start.c
  misc/win32/utf8.c
  mmap/unix/common.c
  mmap/win32/mmap.c
  network_io/unix/inet_ntop.c
  network_io/unix/inet_pton.c
  network_io/unix/multicast.c
  network_io/unix/sockaddr.c
  network_io/unix/socket_util.c
  network_io/win32/sendrecv.c
  network_io/win32/socket_pipe.c
  network_io/win32/sockets.c
  network_io/win32/sockopt.c
  passwd/apr_getpass.c
  poll/unix/poll.c
  poll/unix/pollcb.c
  poll/unix/pollset.c
  poll/unix/select.c
  poll/unix/wakeup.c
  random/unix/apr_random.c
  random/unix/sha2.c
  random/unix/sha2_glue.c
  redis/apr_redis.c
  shmem/win32/shm.c
  strings/apr_cpystrn.c
  strings/apr_cstr.c
  strings/apr_fnmatch.c
  strings/apr_snprintf.c
  strings/apr_strings.c
  strings/apr_strnatcmp.c
  strings/apr_strtok.c
  strmatch/apr_strmatch.c
  tables/apr_hash.c
  tables/apr_skiplist.c
  tables/apr_tables.c
  threadproc/win32/proc.c
  threadproc/win32/signals.c
  threadproc/win32/thread.c
  threadproc/win32/threadpriv.c
  time/win32/time.c
  time/win32/timestr.c
  uri/apr_uri.c
  user/win32/groupinfo.c
  user/win32/userinfo.c
  util-misc/apr_date.c
  util-misc/apr_error.c
  util-misc/apr_queue.c
  util-misc/apr_reslist.c
  util-misc/apr_rmm.c
  util-misc/apr_thread_pool.c
  util-misc/apu_dso.c
  xlate/xlate.c
  xml/apr_xml.c
  xml/apr_xml_expat.c
  xml/apr_xml_libxml2.c
  xml/apr_xml_xmllite.c
)

SET(APR_TEST_SUITES
  testargs
  testatomic
  testbase64
  testbuckets
  testcond
  testcrypto
  testdate
  testdbd
  testdbm
  testdir
  testdso
  testdup
  testenv
  testencode
  testescape
  testfile
  testfilecopy
  testfileinfo
  testflock
  testfmt
  testfnmatch
  testglobalmutex
  testhash
  testhooks
  testjson
  testjose
  testipsub
  testlfs
  testlfsabi
  testlock
  testmd4
  testmd5
  testmemcache
  testmmap
  testnames
  testoc
  testpass
  testpath
  testpipe
  testpoll
  testpools
  testproc
  testprocmutex
  testqueue
  testrand
  testredis
  testreslist
  testrmm
  testshm
  testsiphash
  testskiplist
  testsleep
  testsock
  testsockets
  testsockopt
  teststr
  teststrmatch
  teststrnatcmp
  testtable
  testtemp
  testthread
  testtime
  testud
  testuri
  testuser
  testuuid
  testvsn
  testxlate
  testxml
)

SET(install_targets)
SET(install_modules)
SET(install_bin_pdb)
SET(dbd_drivers)

IF (MSVC)
  # Ignore Microsoft's interpretation of secure development
  # and the POSIX string handling API
  ADD_COMPILE_DEFINITIONS(_CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_WARNINGS)
ENDIF()

# libapr-2 is shared, apr-2 is static
IF (APR_BUILD_SHARED)
  ADD_LIBRARY(${apr_libname} SHARED ${APR_SOURCES} ${APR_PUBLIC_HEADERS_GENERATED} libapr.rc)
  LIST(APPEND install_targets ${apr_libname})
  LIST(APPEND install_bin_pdb ${PROJECT_BINARY_DIR}/${apr_libname}.pdb)
  TARGET_LINK_LIBRARIES(${apr_libname} ${XMLLIB_LIBRARIES} ${XLATE_LIBRARIES} ${APR_SYSTEM_LIBS})
  SET_TARGET_PROPERTIES(${apr_libname} PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_EXPORT;APR_HAVE_MODULAR_DSO=1")
  ADD_DEPENDENCIES(${apr_libname} test_char_header)
ENDIF()

IF (APR_BUILD_STATIC)
  ADD_LIBRARY(${apr_name} STATIC ${APR_SOURCES} ${APR_PUBLIC_HEADERS_GENERATED})
  LIST(APPEND install_targets ${apr_name})
  # no .pdb file generated for static libraries
  TARGET_LINK_LIBRARIES(${apr_name} ${XMLLIB_LIBRARIES} ${XLATE_LIBRARIES} ${APR_SYSTEM_LIBS})
  SET_TARGET_PROPERTIES(${apr_name} PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_STATIC;APR_HAVE_MODULAR_DSO=1")
  ADD_DEPENDENCIES(${apr_name} test_char_header)
ENDIF()

# libaprapp-2 and aprapp-2 are static
IF (APR_BUILD_SHARED)
  ADD_LIBRARY(libaprapp-2 STATIC misc/win32/apr_app.c misc/win32/internal.c ${APR_PUBLIC_HEADERS_GENERATED})
  LIST(APPEND install_targets libaprapp-2)
  SET_TARGET_PROPERTIES(libaprapp-2 PROPERTIES COMPILE_DEFINITIONS APR_APP)
ENDIF()

IF (APR_BUILD_STATIC)
  ADD_LIBRARY(aprapp-2 STATIC misc/win32/apr_app.c misc/win32/internal.c ${APR_PUBLIC_HEADERS_GENERATED})
  LIST(APPEND install_targets aprapp-2)
  SET_TARGET_PROPERTIES(aprapp-2 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_STATIC;APR_APP")
ENDIF()

IF(APU_HAVE_CRYPTO)
  IF(NOT OPENSSL_FOUND)
    MESSAGE(FATAL_ERROR "Only OpenSSL-based crypto is currently implemented in the cmake build")
  ENDIF()
  ADD_LIBRARY(apr_crypto_openssl-2 SHARED crypto/apr_crypto_openssl.c libapr.rc)
  LIST(APPEND install_modules ${PROJECT_BINARY_DIR}/apr_crypto_openssl-2.dll)
  LIST(APPEND install_bin_pdb ${PROJECT_BINARY_DIR}/apr_crypto_openssl-2.pdb)
  TARGET_LINK_LIBRARIES(apr_crypto_openssl-2 ${apr_libname} ${APR_SYSTEM_LIBS} ${OPENSSL_LIBRARIES})
  SET_TARGET_PROPERTIES(apr_crypto_openssl-2 PROPERTIES INCLUDE_DIRECTORIES "${APR_INCLUDE_DIRECTORIES};${OPENSSL_INCLUDE_DIR}")
  SET_TARGET_PROPERTIES(apr_crypto_openssl-2 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_EXPORT;APR_HAVE_MODULAR_DSO=1")
  SET_TARGET_PROPERTIES(apr_crypto_openssl-2 PROPERTIES COMPILE_FLAGS "-DDLL_NAME=apr_crypto_openssl")
ENDIF()

IF(APU_HAVE_ODBC)
  ADD_LIBRARY(apr_dbd_odbc-2 SHARED dbd/apr_dbd_odbc.c libapr.rc)
  LIST(APPEND install_modules ${PROJECT_BINARY_DIR}/apr_dbd_odbc-2.dll)
  LIST(APPEND install_bin_pdb ${PROJECT_BINARY_DIR}/apr_dbd_odbc-2.pdb)
  LIST(APPEND dbd_drivers odbc)
  TARGET_LINK_LIBRARIES(apr_dbd_odbc-2 ${apr_libname} ${APR_SYSTEM_LIBS} odbc32 odbccp32)
  SET_PROPERTY(TARGET apr_dbd_odbc-2 APPEND PROPERTY LINK_FLAGS /export:apr_dbd_odbc_driver)
  SET_TARGET_PROPERTIES(apr_dbd_odbc-2 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_EXPORT;APR_HAVE_MODULAR_DSO=1")
  SET_TARGET_PROPERTIES(apr_dbd_odbc-2 PROPERTIES COMPILE_FLAGS "-DDLL_NAME=apr_dbd_odbc")
ENDIF()

IF(APU_HAVE_SQLITE3)
  ADD_LIBRARY(apr_dbd_sqlite3-2 SHARED dbd/apr_dbd_sqlite3.c libapr.rc)
  LIST(APPEND install_modules ${PROJECT_BINARY_DIR}/apr_dbd_sqlite3-2.dll)
  LIST(APPEND install_bin_pdb ${PROJECT_BINARY_DIR}/apr_dbd_sqlite3-2.pdb)
  LIST(APPEND dbd_drivers sqlite3)
  TARGET_LINK_LIBRARIES(apr_dbd_sqlite3-2 ${apr_libname} ${APR_SYSTEM_LIBS} ${SQLite3_LIBRARIES})
  SET_PROPERTY(TARGET apr_dbd_sqlite3-2 APPEND PROPERTY LINK_FLAGS /export:apr_dbd_sqlite3_driver)
  SET_TARGET_PROPERTIES(apr_dbd_sqlite3-2 PROPERTIES INCLUDE_DIRECTORIES "${APR_INCLUDE_DIRECTORIES};${SQLite3_INCLUDE_DIRS}")
  SET_TARGET_PROPERTIES(apr_dbd_sqlite3-2 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_EXPORT;APR_HAVE_MODULAR_DSO=1")
  SET_TARGET_PROPERTIES(apr_dbd_sqlite3-2 PROPERTIES COMPILE_FLAGS "-DDLL_NAME=apr_dbd_sqlite3")
ENDIF()

IF(APR_BUILD_TESTAPR)
  ENABLE_TESTING()
  # Create a "check" target that displays test program output to the console.
  ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

  # copy data files to build directory so that we can run programs from there
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory 
                  ${PROJECT_BINARY_DIR}/data)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                  ${PROJECT_SOURCE_DIR}/test/data/billion-laughs.xml
                  ${PROJECT_BINARY_DIR}/data/billion-laughs.xml)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                  ${PROJECT_SOURCE_DIR}/test/data/file_datafile.txt
                  ${PROJECT_BINARY_DIR}/data/file_datafile.txt)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${PROJECT_SOURCE_DIR}/test/data/mmap_datafile.txt
                  ${PROJECT_BINARY_DIR}/data/mmap_datafile.txt)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${PROJECT_SOURCE_DIR}/test/data/mmap_large_datafile.txt
                  ${PROJECT_BINARY_DIR}/data/mmap_large_datafile.txt)

  IF(TEST_STATIC_LIBS)
    SET(whichapr    ${apr_name})
    SET(whichaprapp aprapp-2)
    SET(apiflag     -DAPR_DECLARE_STATIC)
  ELSE()
    SET(whichapr    ${apr_libname})
    SET(whichaprapp libaprapp-2)
    SET(apiflag)
  ENDIF()

  ADD_EXECUTABLE(testapp test/testapp.c)
  TARGET_LINK_LIBRARIES(testapp ${whichapr} ${whichaprapp} ${XMLLIB_LIBRARIES} ${XLATE_LIBRARIES} ${APR_SYSTEM_LIBS})
  SET_TARGET_PROPERTIES(testapp PROPERTIES LINK_FLAGS /entry:wmainCRTStartup)
  IF(apiflag)
    SET_TARGET_PROPERTIES(testapp PROPERTIES COMPILE_FLAGS ${apiflag})
  ENDIF()
  ADD_TEST(NAME testapp COMMAND testapp)

  SET (APR_TEST_SOURCES
    test/abts.c
    test/testlfsabi32.c
    test/testlfsabi64.c
    test/testlfsabi_include.c
    test/testutil.c
    )

  FOREACH(testsuite ${APR_TEST_SUITES})
    LIST(APPEND APR_TEST_SOURCES test/${testsuite}.c)
  ENDFOREACH()

  ADD_EXECUTABLE(testall ${APR_TEST_SOURCES})
  TARGET_LINK_LIBRARIES(testall ${whichapr} ${XMLLIB_LIBRARIES} ${XLATE_LIBRARIES} ${APR_SYSTEM_LIBS})
  SET_TARGET_PROPERTIES(testall PROPERTIES COMPILE_DEFINITIONS "BINPATH=$<TARGET_FILE_DIR:testall>")
  IF(apiflag)
    SET_TARGET_PROPERTIES(testall PROPERTIES COMPILE_FLAGS ${apiflag})
  ENDIF()

  FOREACH(test ${APR_TEST_SUITES})
    ADD_TEST(NAME ${test} COMMAND testall -v ${test})
  ENDFOREACH()

  ADD_LIBRARY(mod_test MODULE test/mod_test.c)
  TARGET_LINK_LIBRARIES(mod_test ${whichapr} ${APR_SYSTEM_LIBS})
  SET_PROPERTY(TARGET mod_test APPEND PROPERTY LINK_FLAGS /export:print_hello)
  # nasty work-around for difficulties adding more than one additional flag
  # (they get joined in a bad way behind the scenes)
  GET_PROPERTY(link_flags TARGET mod_test PROPERTY LINK_FLAGS)
  SET(link_flags "${link_flags} /export:count_reps")
  SET_TARGET_PROPERTIES(mod_test PROPERTIES LINK_FLAGS ${link_flags})
  IF(apiflag)
    SET_TARGET_PROPERTIES(mod_test PROPERTIES COMPILE_FLAGS ${apiflag})
  ENDIF()

  # Build all the single-source executable files with no special build
  # requirements.
  SET(single_source_programs
    test/dbd.c
    test/echoargs.c
    test/echod.c
    test/sendfile.c
    test/sockperf.c
    test/testlockperf.c
    test/testmutexscope.c
    test/globalmutexchild.c
    test/occhild.c
    test/proc_child.c
    test/readchild.c
    test/sockchild.c
    test/memcachedmock.c
    test/testshmproducer.c
    test/testshmconsumer.c
    test/tryread.c
    test/internal/testutf.c
  )

  FOREACH(sourcefile ${single_source_programs})
    STRING(REGEX REPLACE ".*/([^\\]+)\\.c" "\\1" proggie ${sourcefile})
    ADD_EXECUTABLE(${proggie} ${sourcefile})
    TARGET_LINK_LIBRARIES(${proggie} ${whichapr} ${XMLLIB_LIBRARIES} ${XLATE_LIBRARIES} ${APR_SYSTEM_LIBS})
    SET_TARGET_PROPERTIES(${proggie} PROPERTIES COMPILE_DEFINITIONS "BINPATH=$<TARGET_FILE_DIR:${proggie}>")
    IF(apiflag)
      SET_TARGET_PROPERTIES(${proggie} PROPERTIES COMPILE_FLAGS ${apiflag})
    ENDIF()
  ENDFOREACH()

  # Add tests for programs that run by themselves with no arguments.
  SET(simple_tests
    testmutexscope
    testutf
  )

  FOREACH(simple ${simple_tests})
    ADD_TEST(NAME ${simple} COMMAND ${simple})
  ENDFOREACH()

  # testlockperf takes forever on Windows with default counter limit
  ADD_TEST(NAME testlockperf COMMAND testlockperf -c 50000)

  # dbd and sendfile are run multiple times with different parameters.
  FOREACH(somedbd ${dbd_drivers})
    ADD_TEST(NAME dbd-${somedbd} COMMAND dbd ${somedbd})
  ENDFOREACH()

  FOREACH(sendfile_mode blocking nonblocking timeout)
    ADD_TEST(NAME sendfile-${sendfile_mode} COMMAND sendfile client ${sendfile_mode} startserver)
  ENDFOREACH()

  # No test is added for echod+sockperf.  Those will have to be run manually.

ENDIF (APR_BUILD_TESTAPR)

# Installation

INSTALL(TARGETS ${install_targets}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
       )

# For DSO modules, install only binaries
INSTALL(FILES ${install_modules}
        DESTINATION bin)

IF(INSTALL_PDB)
  INSTALL(FILES ${install_bin_pdb}
          DESTINATION bin
          CONFIGURATIONS RelWithDebInfo Debug)
ENDIF()

INSTALL(FILES ${APR_PUBLIC_HEADERS_STATIC} ${APR_PUBLIC_HEADERS_GENERATED} DESTINATION include/apr-2)
IF(APR_INSTALL_PRIVATE_H)
  # Kludges for unexpected dependencies of httpd 2.x, not installed by default
  SET(APR_PRIVATE_H_FOR_HTTPD
    include/arch/win32/apr_arch_file_io.h
    include/arch/win32/apr_arch_misc.h
    include/arch/win32/apr_arch_utf8.h
    include/arch/win32/apr_private.h
    )
  INSTALL(FILES ${APR_PRIVATE_H_FOR_HTTPD} DESTINATION include/apr-2/arch/win32)
ENDIF()

STRING(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)
MESSAGE(STATUS "")
MESSAGE(STATUS "")
MESSAGE(STATUS "APR configuration summary:")
MESSAGE(STATUS "")

MESSAGE(STATUS "  Build type ...................... : ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "  Install .pdb (if available)...... : ${INSTALL_PDB}")
MESSAGE(STATUS "  Install prefix .................. : ${CMAKE_INSTALL_PREFIX}")
MESSAGE(STATUS "  C compiler ...................... : ${CMAKE_C_COMPILER}")
MESSAGE(STATUS "  IPv6 ............................ : ${APR_HAVE_IPV6}")
MESSAGE(STATUS "  DBD ODBC driver ................. : ${APU_HAVE_ODBC}")
MESSAGE(STATUS "  DBD SQLite3 driver .............. : ${APU_HAVE_SQLITE3}")
MESSAGE(STATUS "  Use Expat ....................... : ${APU_USE_EXPAT}")
MESSAGE(STATUS "  Use LibXml2 ..................... : ${APU_USE_LIBXML2}")
MESSAGE(STATUS "  Use XmlLite ..................... : ${APU_USE_XMLLITE}")
MESSAGE(STATUS "  Have Crypto ..................... : ${APU_HAVE_CRYPTO}")
MESSAGE(STATUS "  Have Iconv ...................... : ${APU_HAVE_ICONV}")
MESSAGE(STATUS "  Library files for XML ........... : ${XMLLIB_LIBRARIES}")
MESSAGE(STATUS "  Build shared libs ............... : ${APR_BUILD_SHARED}")
MESSAGE(STATUS "  Build static libs ............... : ${APR_BUILD_STATIC}")
MESSAGE(STATUS "  Build test suite ................ : ${APR_BUILD_TESTAPR}")
IF(TEST_STATIC_LIBS)
MESSAGE(STATUS "    (testing static libraries)")
ELSE()
MESSAGE(STATUS "    (testing dynamic libraries)")
ENDIF()
MESSAGE(STATUS "  Install private .h for httpd .... : ${APR_INSTALL_PRIVATE_H}")
