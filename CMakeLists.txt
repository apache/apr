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
cmake_minimum_required(VERSION 3.5)

project(APR C)

include(CMakeDependentOption)

# Enable support for MSVC runtime library selection by abstraction
# if supported by CMake.
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

# Add simple support CMAKE_WARNING_AS_ERROR if CMake doesn't
# support it.
if(CMAKE_VERSION VERSION_LESS 3.24)
  if(CMAKE_WARNING_AS_ERROR AND MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /WX")
  endif()
endif()

option(APR_MINIMAL_BUILD    "Create minimal APR build"       OFF)
if(NOT APR_MINIMAL_BUILD)
    option(APU_HAVE_ODBC     "Build ODBC DBD driver"         ON)
else()
    option(APU_HAVE_ODBC     "Build ODBC DBD driver"         OFF)
endif()

set(apr_libname libapr-2)

set(APR_XML_BACKEND "xmllite" CACHE STRING "XML library to use (expat|libxml|xmllite)")
set_property(CACHE APR_XML_BACKEND PROPERTY STRINGS "expat;libxml2;xmllite")

option(APR_INSTALL_PRIVATE_H  "Install selected private .h files (for httpd)"  OFF)
option(APU_HAVE_SQLITE3     "Build SQLite3 DBD driver"     OFF)
option(APU_HAVE_PGSQL       "Build PostgreSQL DBD driver"  OFF)
option(APU_HAVE_CRYPTO      "Crypto support"               OFF)
option(APU_HAVE_ICONV       "Xlate support"                OFF)
option(APR_HAS_LDAP         "LDAP support"                 ON)
option(APR_HAVE_IPV6        "IPv6 support"                 ON)
option(INSTALL_PDB          "Install .pdb files (if generated)"  ON)
option(APR_BUILD_TESTAPR    "Build the test suite"         ON)
option(BUILD_SHARED_LIBS    "Build using shared libraries" ON)
cmake_dependent_option(APR_MODULAR_DSO "Use DSO build of modular components" ON "BUILD_SHARED_LIBS" OFF)
option(APR_POOL_DEBUG       "Turn on pools debugging"      OFF)

set(APR_INSTALL_BIN_DIR      "bin"                       CACHE STRING "Install subdirectory for binary files")
set(APR_INSTALL_LIB_DIR      "lib"                       CACHE STRING "Install subdirectory for library files")
set(APR_INSTALL_INCLUDE_DIR  "include/apr-2"             CACHE STRING "Install subdirectory for include files")

if(APU_USE_EXPAT)
  message(FATAL_ERROR "APU_USE_EXPAT option is not supported. Use APR_XML_BACKEND.")
endif()

if(APU_USE_LIBXML2)
  message(FATAL_ERROR "APU_USE_LIBXML2 option is not supported. Use APR_XML_BACKEND.")
endif()

if(APU_USE_XMLLITE)
  message(FATAL_ERROR "APU_USE_XMLLITE option is not supported. Use APR_XML_BACKEND.")
endif()

if(APR_MODULAR_DSO AND NOT BUILD_SHARED_LIBS)
  message(FATAL_ERROR "APR_MODULAR_DSO requires shared libraries build option enabled.")
endif()

if(APU_HAVE_CRYPTO)
  find_package(OpenSSL REQUIRED)
  set(APU_HAVE_OPENSSL ON)
  set(APU_HAVE_CRYPTO_PRNG ON)
endif()

if(APR_XML_BACKEND STREQUAL "expat")
  find_package(expat CONFIG REQUIRED)
  set(XMLLIB_LIBRARIES   expat::expat)
  set(APU_USE_EXPAT ON)
elseif(APR_XML_BACKEND STREQUAL "libxml2")
  find_package(LibXml2 REQUIRED)
  set(XMLLIB_LIBRARIES   LibXml2::LibXml2)
  set(APU_USE_LIBXML2 ON)
elseif(APR_XML_BACKEND STREQUAL "xmllite")
  set(XMLLIB_LIBRARIES   "xmllite.lib")
  set(APU_USE_XMLLITE ON)
else()
  message(FATAL_ERROR "Unsupported APR_XML_BACKEND value (${APR_XML_BACKEND}).")
endif()

if(APU_HAVE_ICONV)
  find_package(Iconv REQUIRED)
endif()

if(APU_HAVE_SQLITE3)
  find_package(SQLite3 REQUIRED)
endif()

if(APU_HAVE_PGSQL)
  find_package(PostgreSQL REQUIRED)
endif()

set(LDAP_LIBRARIES)
if(APR_HAS_LDAP)
  set(LDAP_LIBRARIES wldap32)
endif()

if(APR_POOL_DEBUG)
  add_compile_definitions(APR_POOL_DEBUG=1)
endif()

if(APR_MODULAR_DSO)
  add_compile_definitions(APR_HAVE_MODULAR_DSO=1)
else()
  add_compile_definitions(APR_HAVE_MODULAR_DSO=0)
endif()

# Read current version.
file(STRINGS include/apr_version.h VERSION_STRINGS REGEX "#define (APR_MAJOR_VERSION|APR_MINOR_VERSION|APR_PATCH_VERSION)")

string(REGEX REPLACE ".*#define APR_MAJOR_VERSION[ \t]+([0-9]+).*" "\\1" APR_MAJOR_VERSION ${VERSION_STRINGS})
string(REGEX REPLACE ".*#define APR_MINOR_VERSION[ \t]+([0-9]+).*" "\\1" APR_MINOR_VERSION ${VERSION_STRINGS})
string(REGEX REPLACE ".*#define APR_PATCH_VERSION[ \t]+([0-9]+).*" "\\1" APR_PATCH_VERSION ${VERSION_STRINGS})

configure_file(include/apr.hwc
               ${PROJECT_BINARY_DIR}/apr.h)
# "COPYONLY" just because anything else isn't implemented ;)
configure_file(include/private/apu_ldap_internal.hw
               ${PROJECT_BINARY_DIR}/apu_ldap_internal.h
               COPYONLY)
configure_file(include/private/apu_select_dbm.hw
               ${PROJECT_BINARY_DIR}/apu_select_dbm.h
               COPYONLY)
configure_file(include/apu_want.hw
               ${PROJECT_BINARY_DIR}/apu_want.h
               COPYONLY)

add_executable(gen_test_char tools/gen_test_char.c)

add_custom_command(
  COMMENT "Generating character tables, apr_escape_test_char.h, for current locale"
  DEPENDS gen_test_char
  COMMAND $<TARGET_FILE:gen_test_char> > ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
  OUTPUT ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
)
add_custom_target(
  test_char_header ALL
  DEPENDS ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
)

if(APU_HAVE_ICONV)
  set(XLATE_LIBRARIES    Iconv::Iconv)
else()
  set(XLATE_LIBRARIES   "")
endif()
# Generated .h files are stored in PROJECT_BINARY_DIR, not the
# source tree.
#
# BROKEN: not searching PROJECT_BINARY_DIR first, so you have to
#         manually delete apr.h in PROJECT_SOURCE_DIR/include if
#         you've generated apr.h before using a different build

set(APR_INCLUDE_DIRECTORIES
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/arch/win32>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/arch/unix>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/private>
)

set(APR_SYSTEM_LIBS
  ws2_32
  rpcrt4
  bcrypt
)

set(APR_PUBLIC_HEADERS_STATIC
  include/apr_allocator.h
  include/apr_anylock.h
  include/apr_atomic.h
  include/apr_base64.h
  include/apr_buckets.h
  include/apr_buffer.h
  include/apr_crypto.h
  include/apr_cstr.h
  include/apr_date.h
  include/apr_dbd.h
  include/apr_dbm.h
  include/apr_dso.h
  include/apr_encode.h
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
  include/apr_jose.h
  include/apr_json.h
  include/apr_ldap.h
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
set(APR_PUBLIC_HEADERS_GENERATED
  ${PROJECT_BINARY_DIR}/apr.h
  ${PROJECT_BINARY_DIR}/apu_want.h
)

set(APR_SOURCES
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
  buffer/apr_buffer.c
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

if(APR_HAS_LDAP)
  set(APR_SOURCES ${APR_SOURCES} ldap/apr_ldap_stub.c ldap/apr_ldap_url.c)
endif()

# Sources and libraries for APR builtin drivers. Used when APR_MODULAR_DSO=OFF.
set(APR_EXTRA_SOURCES)
set(APR_EXTRA_LIBRARIES)

set(APR_TEST_SUITES
  testargs
  testatomic
  testbase64
  testbuckets
  testbuffer
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
  testldap
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

set(install_targets)
set(install_modules)
set(install_bin_pdb)
set(dbd_drivers)

if (MSVC)
  # Ignore Microsoft's interpretation of secure development
  # and the POSIX string handling API
  add_compile_definitions(_CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_WARNINGS)
endif()

macro(ADD_APR_MODULE name dllname sources libraries)
  if(APR_MODULAR_DSO)
    add_library(${name} MODULE ${sources})
    list(APPEND install_modules ${name})
    list(APPEND install_bin_pdb $<TARGET_PDB_FILE:${name}>)

    target_sources(${name} PRIVATE libapr.rc)
    target_compile_definitions(${name}
                               PRIVATE DLL_NAME=${dllname})

    target_link_libraries(${name}
                          PRIVATE ${apr_libname} ${APR_SYSTEM_LIBS})
    target_link_libraries(${name}
                          PRIVATE ${libraries})
  else()
    list(APPEND APR_EXTRA_SOURCES ${sources})
    list(APPEND APR_EXTRA_LIBRARIES ${libraries})
  endif()
endmacro()

if(APU_HAVE_CRYPTO)
  if(NOT OPENSSL_FOUND)
    message(FATAL_ERROR "Only OpenSSL-based crypto is currently implemented in the cmake build")
  endif()

  ADD_APR_MODULE(
    apr_crypto_openssl-2 "apr_crypto_openssl"
    "crypto/apr_crypto_openssl.c"
    "OpenSSL::Crypto")
endif()

IF(APR_HAS_LDAP)
  ADD_APR_MODULE(
    apr_ldap-2 "apr_ldap"
    "ldap/apr_ldap.c"
    ${LDAP_LIBRARIES}
  )
ENDIF()

if(APU_HAVE_ODBC)
  list(APPEND dbd_drivers odbc)

  ADD_APR_MODULE(
    apr_dbd_odbc-2 "apr_dbd_odbc"
    "dbd/apr_dbd_odbc.c"
    "odbc32;odbccp32"
  )
endif()

if(APU_HAVE_SQLITE3)
  list(APPEND dbd_drivers sqlite3)

  ADD_APR_MODULE(
    apr_dbd_sqlite3-2 "apr_dbd_sqlite3-1"
    "dbd/apr_dbd_sqlite3.c"
    "SQLite::SQLite3"
  )
endif()

if(APU_HAVE_PGSQL)
  list(APPEND dbd_drivers pgsql)

  ADD_APR_MODULE(apr_dbd_pgsql-2 "apr_dbd_pgsql"
                 "dbd/apr_dbd_pgsql.c"
                 "PostgreSQL::PostgreSQL")

  add_compile_definitions(HAVE_LIBPQ_FE_H)
endif()

if (BUILD_SHARED_LIBS)
  add_library(${apr_libname} SHARED ${APR_SOURCES} ${APR_EXTRA_SOURCES} ${APR_PUBLIC_HEADERS_GENERATED} libapr.rc)
  list(APPEND install_targets ${apr_libname})
  list(APPEND install_bin_pdb $<TARGET_PDB_FILE:${apr_libname}>)
  target_link_libraries(${apr_libname}
                        PRIVATE ${XMLLIB_LIBRARIES} ${XLATE_LIBRARIES} ${APR_SYSTEM_LIBS} ${APR_EXTRA_LIBRARIES})
  target_compile_definitions(${apr_libname}
                             PRIVATE "APR_DECLARE_EXPORT"
                             INTERFACE "APR_DECLARE_IMPORT")
  target_include_directories(${apr_libname}
    PRIVATE ${APR_INCLUDE_DIRECTORIES}
    INTERFACE $<INSTALL_INTERFACE:${APR_INSTALL_INCLUDE_DIR}> ${APR_INCLUDE_DIRECTORIES}
  )
  add_dependencies(${apr_libname} test_char_header)
else()
  add_library(${apr_libname} STATIC ${APR_SOURCES} ${APR_EXTRA_SOURCES} ${APR_PUBLIC_HEADERS_GENERATED})
  list(APPEND install_targets ${apr_libname})
  # no .pdb file generated for static libraries
  target_link_libraries(${apr_libname}
                        PRIVATE ${XMLLIB_LIBRARIES} ${XLATE_LIBRARIES} ${APR_SYSTEM_LIBS} ${APR_EXTRA_LIBRARIES})
  target_compile_definitions(${apr_libname}
                             PRIVATE "APR_DECLARE_STATIC"
                             INTERFACE "APR_DECLARE_STATIC")
  target_include_directories(${apr_libname}
    PRIVATE ${APR_INCLUDE_DIRECTORIES}
    INTERFACE $<INSTALL_INTERFACE:${APR_INSTALL_INCLUDE_DIR}> ${APR_INCLUDE_DIRECTORIES}
  )
  add_dependencies(${apr_libname} test_char_header)
endif()

if (BUILD_SHARED_LIBS)
  add_library(libaprapp-2 STATIC misc/win32/apr_app.c misc/win32/internal.c ${APR_PUBLIC_HEADERS_GENERATED})
  list(APPEND install_targets libaprapp-2)
  set_target_properties(libaprapp-2 PROPERTIES COMPILE_DEFINITIONS APR_APP)
  target_include_directories(libaprapp-2
    PRIVATE ${APR_INCLUDE_DIRECTORIES}
  )
else()
  add_library(libaprapp-2 STATIC misc/win32/apr_app.c misc/win32/internal.c ${APR_PUBLIC_HEADERS_GENERATED})
  list(APPEND install_targets libaprapp-2)
  set_target_properties(libaprapp-2 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_STATIC;APR_APP")
  target_include_directories(libaprapp-2
    PRIVATE ${APR_INCLUDE_DIRECTORIES}
  )
endif()

if(APR_BUILD_TESTAPR)
  enable_testing()
  # Create a "check" target that displays test program output to the console.
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

  # copy data files to build directory so that we can run programs from there
  execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory 
                  ${PROJECT_BINARY_DIR}/data)
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                  ${PROJECT_SOURCE_DIR}/test/data/billion-laughs.xml
                  ${PROJECT_BINARY_DIR}/data/billion-laughs.xml)
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different 
                  ${PROJECT_SOURCE_DIR}/test/data/file_datafile.txt
                  ${PROJECT_BINARY_DIR}/data/file_datafile.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${PROJECT_SOURCE_DIR}/test/data/mmap_datafile.txt
                  ${PROJECT_BINARY_DIR}/data/mmap_datafile.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${PROJECT_SOURCE_DIR}/test/data/mmap_large_datafile.txt
                  ${PROJECT_BINARY_DIR}/data/mmap_large_datafile.txt)

  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  ${PROJECT_SOURCE_DIR}/test/echoargs.bat
                  ${PROJECT_BINARY_DIR}/echoargs.bat)

  add_executable(testapp test/testapp.c)
  target_link_libraries(testapp ${apr_libname} libaprapp-2)
  set_target_properties(testapp PROPERTIES LINK_FLAGS /entry:wmainCRTStartup)
  add_test(NAME testapp COMMAND testapp)

  set (APR_TEST_SOURCES
    test/abts.c
    test/testlfsabi32.c
    test/testlfsabi64.c
    test/testlfsabi_include.c
    test/testutil.c
    )

  foreach(testsuite ${APR_TEST_SUITES})
    list(APPEND APR_TEST_SOURCES test/${testsuite}.c)
  endforeach()

  add_executable(testall ${APR_TEST_SOURCES})
  target_link_libraries(testall ${apr_libname} ws2_32.lib)
  set_target_properties(testall PROPERTIES COMPILE_DEFINITIONS "BINPATH=$<TARGET_FILE_DIR:testall>")

  foreach(test ${APR_TEST_SUITES})
    add_test(NAME ${test} COMMAND testall -v ${test})
  endforeach()

  add_library(mod_test MODULE test/mod_test.c)
  target_link_libraries(mod_test ${apr_libname})
  set_property(TARGET mod_test APPEND PROPERTY LINK_FLAGS /export:print_hello)
  # nasty work-around for difficulties adding more than one additional flag
  # (they get joined in a bad way behind the scenes)
  get_property(link_flags TARGET mod_test PROPERTY LINK_FLAGS)
  set(link_flags "${link_flags} /export:count_reps")
  set_target_properties(mod_test PROPERTIES LINK_FLAGS ${link_flags})

  # Build all the single-source executable files with no special build
  # requirements.
  set(single_source_programs
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

  foreach(sourcefile ${single_source_programs})
    string(REGEX REPLACE ".*/([^\\]+)\\.c" "\\1" proggie ${sourcefile})
    add_executable(${proggie} ${sourcefile})
    target_link_libraries(${proggie} ${apr_libname})
    set_target_properties(${proggie} PROPERTIES COMPILE_DEFINITIONS "BINPATH=$<TARGET_FILE_DIR:${proggie}>")

    add_dependencies(testall ${proggie})
  endforeach()

  # Add tests for programs that run by themselves with no arguments.
  set(simple_tests
    testmutexscope
    testutf
  )

  foreach(simple ${simple_tests})
    add_test(NAME ${simple} COMMAND ${simple})
  endforeach()

  # testlockperf takes forever on Windows with default counter limit
  add_test(NAME testlockperf COMMAND testlockperf -c 50000)

  # dbd and sendfile are run multiple times with different parameters.
  foreach(somedbd ${dbd_drivers})
    add_test(NAME dbd-${somedbd} COMMAND dbd ${somedbd})
  endforeach()

  foreach(sendfile_mode blocking nonblocking timeout)
    add_test(NAME sendfile-${sendfile_mode} COMMAND sendfile client ${sendfile_mode} startserver)
  endforeach()

  # No test is added for echod+sockperf.  Those will have to be run manually.

endif (APR_BUILD_TESTAPR)

# Installation
include(CMakePackageConfigHelpers)

string(TOLOWER "${PROJECT_NAME}" project_name_lower)

write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/${project_name_lower}-config-version.cmake"
  VERSION ${APR_MAJOR_VERSION}.${APR_MINOR_VERSION}.${APR_PATCH_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(TARGETS ${install_targets} EXPORT apr-config
        RUNTIME DESTINATION ${APR_INSTALL_BIN_DIR}
        LIBRARY DESTINATION ${APR_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${APR_INSTALL_LIB_DIR}
)

install(EXPORT apr-config
        NAMESPACE apr::
        DESTINATION "lib/cmake/${project_name_lower}"
        FILE "${project_name_lower}-config.cmake"
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${project_name_lower}-config-version.cmake"
        DESTINATION "lib/cmake/${project_name_lower}"
)

# For DSO modules, install only binaries
install(TARGETS ${install_modules}
        DESTINATION ${APR_INSTALL_BIN_DIR})

if(INSTALL_PDB)
  install(FILES ${install_bin_pdb}
          DESTINATION ${APR_INSTALL_BIN_DIR}
          CONFIGURATIONS RelWithDebInfo Debug)
endif()

install(FILES ${APR_PUBLIC_HEADERS_STATIC} ${APR_PUBLIC_HEADERS_GENERATED} DESTINATION ${APR_INSTALL_INCLUDE_DIR})
if(APR_INSTALL_PRIVATE_H)
  # Kludges for unexpected dependencies of httpd 2.x, not installed by default
  set(APR_PRIVATE_H_FOR_HTTPD
    include/arch/win32/apr_arch_file_io.h
    include/arch/win32/apr_arch_misc.h
    include/arch/win32/apr_arch_utf8.h
    include/arch/win32/apr_private.h
    )
  install(FILES ${APR_PRIVATE_H_FOR_HTTPD} DESTINATION ${APR_INSTALL_INCLUDE_DIR}/arch/win32)
endif()

STRING(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)
message(STATUS "")
message(STATUS "")
message(STATUS "APR configuration summary:")
message(STATUS "")

message(STATUS "  Build type ...................... : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install .pdb (if available)...... : ${INSTALL_PDB}")
message(STATUS "  Install prefix .................. : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "    Directory for binary files .... : PREFIX/${APR_INSTALL_BIN_DIR}")
message(STATUS "    Directory for library files ... : PREFIX/${APR_INSTALL_LIB_DIR}")
message(STATUS "    Directory for include files ... : PREFIX/${APR_INSTALL_INCLUDE_DIR}")
message(STATUS "  C compiler ...................... : ${CMAKE_C_COMPILER}")
message(STATUS "  IPv6 ............................ : ${APR_HAVE_IPV6}")
message(STATUS "  DBD ODBC driver ................. : ${APU_HAVE_ODBC}")
message(STATUS "  DBD SQLite3 driver .............. : ${APU_HAVE_SQLITE3}")
message(STATUS "  DBD PostgreSQL .................. : ${APU_HAVE_PGSQL}")
message(STATUS "  LDAP driver ..................... : ${APR_HAS_LDAP}")
message(STATUS "  XML backend ..................... : ${APR_XML_BACKEND}")
message(STATUS "  Have Crypto ..................... : ${APU_HAVE_CRYPTO}")
message(STATUS "  Have Iconv ...................... : ${APU_HAVE_ICONV}")
message(STATUS "  Library files for XML ........... : ${XMLLIB_LIBRARIES}")
message(STATUS "  DSO build of modular components.. : ${APR_MODULAR_DSO}")
message(STATUS "  Turn on pools debugging ..........: ${APR_POOL_DEBUG}")
message(STATUS "  Build shared libraries .......... : ${BUILD_SHARED_LIBS}")
message(STATUS "  Build test suite ................ : ${APR_BUILD_TESTAPR}")
message(STATUS "  Install private .h for httpd .... : ${APR_INSTALL_PRIVATE_H}")
