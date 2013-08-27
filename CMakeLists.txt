PROJECT(APR C)

# Experimental cmake-based build support for APR on Windows
#
# General usage:
# 0. Read the todos down below and make sure this is good enough for your
#    purposes.
# 1. cd to a clean directory for building (i.e., don't build in your
#    source tree)
# 2. set CMAKE_LIBRARY_PATH=d:\path\to\prereq1\lib;d:\path\to\prereq2\lib;...
# 3. set CMAKE_INCLUDE_PATH=d:\path\to\prereq1\include;d:\path\to\prereq2\include;...
# 4. cmake -G "some backend, like 'NMake Makefiles'" \
#          -DCMAKE_INSTALL_PREFIX=d:/path/to/aprinst \
#          -DAPR-specific-flags \
#          d:/path/to/aprsource
#    Alternately, use cmake-gui and update settings in the GUI.
#
#    APR flags:
#        APU_USE_EXPAT          Use Expat as the underlying XML implementation
#                               Default: ON
#        APU_USE_LIBXML2        Use libxml2 as the underlying XML implementation
#                               Exactly one of APU_USE_EXPAT and APU_USE_LIBXML2
#                               must be specified.
#                               Default: OFF
#        APR_INSTALL_PRIVATE_H  Install extra .h files which are required by httpd
#                               but which aren't intended for use by applications.
#                               Default: OFF
#        APU_HAVE_ODBC          Build ODBC DBD driver
#                               Default: ON
#        APR_HAVE_IPV6          Enable IPv6 support
#                               Default: ON
#        APR_SHOW_SETTINGS      Display key build settings at the end of build
#                               generation
#                               Default: ON
#        APR_BUILD_TESTAPR      Build APR test suite
#                               Default: OFF
#
#    Other flags of interest:
#        CMAKE_C_FLAGS_RELEASE, _DEBUG, _RELWITHDEBINFO, _MINSIZEREL
#        CMAKE_BUILD_TYPE
#          For NMake Makefiles the choices are at least DEBUG, RELEASE,
#          RELWITHDEBINFO, and MINSIZEREL
#          Other backends make have other selections.
# 5. build using chosen backend (e.g., "nmake install")
#
# Todos to properly support Windows:
# . Fix problem where srcdir/include/apr.h (if it exists) is found before builddir/apr.h
#   (and similar for apu_want.h)
# . Build apr_app.c into libapr-2 properly (what about apr-2.lib?)
# . Options for remaining features, along with finding any necessary libraries
#   + APR_POOL_DEBUG
#   + DBM:
#     . APU_HAVE_GDBM
#     . APU_HAVE_NDBM
#     . APU_HAVE_DB
#   + DBD:
#     . APU_HAVE_PGSQL
#     . APU_HAVE_MYSQL
#     . APU_HAVE_SQLITE3
#     . APU_HAVE_SQLITE2
#     . APU_HAVE_ORACLE
#   + CRYPTO:
#     . APU_HAVE_NSS
#   + APU_HAVE_ICONV
#   + APU_USE_LIBXML2 (sketched in, but not working)
# . Static builds of APR modules
# . Alternate build of test programs to use libapr-2.dll
# . Support static *or* shared build of Expat
# . Some easier way to run the test suite (the stuff besides testall.exe)
# . Install CHANGES/LICENSE/NOTICE like Makefile.win
# . test/internal/testucs
# . aprapp-2.lib/libaprapp-2.lib
# . All the other stuff Jeff doesn't know about yet

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Either Expat or LibXml2 is required; the others are optional
FIND_PACKAGE(Expat)
FIND_PACKAGE(LibXml2)
FIND_PACKAGE(OpenSSL)

IF(NOT EXPAT_FOUND AND NOT LIBXML2_FOUND)
  MESSAGE(FATAL_ERROR "Either Expat or LibXml2 is required, but neither was found")
ENDIF()

IF(EXPAT_FOUND)
  OPTION(APU_USE_EXPAT      "Use Expat"                    ON)
  IF(LIBXML2_FOUND)
    OPTION(APU_USE_LIBXML2  "Use LibXml2"                  OFF)
  ENDIF()
ELSE()
  OPTION(APU_USE_LIBXML2    "Use LibXml2"                  ON)
ENDIF()

OPTION(APR_INSTALL_PRIVATE_H  "Install selected private .h files (for httpd)"  OFF)
OPTION(APU_HAVE_CRYPTO      "Crypto support"               OFF)
OPTION(APU_HAVE_ODBC        "Build ODBC DBD driver"        ON)
OPTION(APR_HAVE_IPV6        "IPv6 support"                 ON)
OPTION(APR_SHOW_SETTINGS    "Show the build configuration" ON)
OPTION(APR_BUILD_TESTAPR    "Build the test suite"         OFF)

IF(NOT APU_USE_EXPAT AND NOT APU_USE_LIBXML2)
  MESSAGE(FATAL_ERROR "Either Expat or LibXml2 must be selected")
ENDIF()
IF(APU_USE_EXPAT AND APU_USE_LIBXML2)
  MESSAGE(FATAL_ERROR "Only one of Expat and LibXml2 can be selected")
ENDIF()

IF(APU_HAVE_CRYPTO)
IF(NOT OPENSSL_FOUND)
  MESSAGE(FATAL_ERROR "OpenSSL is the only supported crypto implementation, and it wasn't found!")
ENDIF()
ENDIF()

# create 1-or-0 representation of feature tests for apr.h

SET(apr_have_ipv6_10 0)
SET(apu_have_crypto_10 0)
SET(apu_use_libxml2_10 0)
SET(apu_use_expat_10 0)

IF(APR_HAVE_IPV6)
  SET(apr_have_ipv6_10 1)
ENDIF()

IF(APU_HAVE_CRYPTO)
  SET(apu_have_crypto_10 1)
ENDIF()

IF(APU_USE_EXPAT)
  SET(apu_use_expat_10 1)
ELSE()
  SET(apu_use_libxml2_10 1)
ENDIF()

CONFIGURE_FILE(include/apr.hwc
               ${PROJECT_BINARY_DIR}/apr.h)
# "COPYONLY" just because anything else isn't implemented ;)
CONFIGURE_FILE(include/apu_want.hw
               ${PROJECT_BINARY_DIR}/apu_want.h
               COPYONLY)

ADD_EXECUTABLE(gen_test_char tools/gen_test_char.c)
GET_TARGET_PROPERTY(GEN_TEST_CHAR_EXE gen_test_char LOCATION)
ADD_CUSTOM_COMMAND(
  COMMENT "Generating character tables, apr_escape_test_char.h, for current locale"
  DEPENDS gen_test_char
  COMMAND ${GEN_TEST_CHAR_EXE} > ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
  OUTPUT ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
)
ADD_CUSTOM_TARGET(
  test_char_header ALL
  DEPENDS ${PROJECT_BINARY_DIR}/apr_escape_test_char.h
)

IF(APU_USE_EXPAT)
  SET(XMLLIB_INCLUDE_DIR ${EXPAT_INCLUDE_DIRS})
  SET(XMLLIB_LIBRARIES   ${EXPAT_LIBRARIES})
ELSE()
  SET(XMLLIB_INCLUDE_DIR ${LIBXML2_INCLUDE_DIR})
  SET(XMLLIB_LIBRARIES   ${LIBXML2_LIBRARIES})
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
  mswsock
  rpcrt4
)

INCLUDE_DIRECTORIES(${APR_INCLUDE_DIRECTORIES} ${XMLLIB_INCLUDE_DIR})

SET(APR_HEADERS ${PROJECT_BINARY_DIR}/apr.h)

# and misc/win32/apr_app.c

SET(APR_PUBLIC_HEADERS_STATIC
  include/apr_allocator.h
  include/apr_anylock.h
  include/apr_atomic.h
  include/apr_base64.h
  include/apr_buckets.h
  include/apr_crypto.h
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
  include/apr_reslist.h
  include/apr_ring.h
  include/apr_rmm.h
  include/apr_sdbm.h
  include/apr_sha1.h
  include/apr_shm.h
  include/apr_signal.h
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
  crypto/apr_md4.c
  crypto/apr_md5.c
  crypto/apr_passwd.c
  crypto/apr_sha1.c
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
  shmem/win32/shm.c
  strings/apr_cpystrn.c
  strings/apr_fnmatch.c
  strings/apr_snprintf.c
  strings/apr_strings.c
  strings/apr_strnatcmp.c
  strings/apr_strtok.c
  strmatch/apr_strmatch.c
  tables/apr_hash.c
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
  util-misc/apr_queue.c
  util-misc/apr_reslist.c
  util-misc/apr_rmm.c
  util-misc/apr_thread_pool.c
  util-misc/apu_dso.c
  xlate/xlate.c
  xml/apr_xml.c
  xml/apr_xml_expat.c
  xml/apr_xml_libxml2.c
)

SET(APR_TEST_SOURCES
  test/abts.c
  test/testargs.c
  test/testatomic.c
  test/testbase64.c
  test/testbuckets.c
  test/testcond.c
  test/testcrypto.c
  test/testdate.c
  test/testdbd.c
  test/testdbm.c
  test/testdir.c
  test/testdso.c
  test/testdup.c
  test/testenv.c
  test/testescape.c
  test/testfile.c
  test/testfilecopy.c
  test/testfileinfo.c
  test/testflock.c
  test/testfmt.c
  test/testfnmatch.c
  test/testglobalmutex.c
  test/testhash.c
  test/testhooks.c
  test/testipsub.c
  test/testlfs.c
  test/testlfsabi.c
  test/testlfsabi32.c
  test/testlfsabi64.c
  test/testlfsabi_include.c
  test/testlock.c
  test/testmd4.c
  test/testmd5.c
  test/testmemcache.c
  test/testmmap.c
  test/testnames.c
  test/testoc.c
  test/testpass.c
  test/testpath.c
  test/testpipe.c
  test/testpoll.c
  test/testpools.c
  test/testproc.c
  test/testprocmutex.c
  test/testqueue.c
  test/testrand.c
  test/testreslist.c
  test/testrmm.c
  test/testshm.c
  test/testsleep.c
  test/testsock.c
  test/testsockets.c
  test/testsockopt.c
  test/teststr.c
  test/teststrmatch.c
  test/teststrnatcmp.c
  test/testtable.c
  test/testtemp.c
  test/testthread.c
  test/testtime.c
  test/testud.c
  test/testuri.c
  test/testuser.c
  test/testutil.c
  test/testuuid.c
  test/testvsn.c
  test/testxlate.c
  test/testxml.c
)

SET(install_targets)

# libapr-2 is shared, apr-2 is static
ADD_LIBRARY(libapr-2 SHARED ${APR_HEADERS} ${APR_SOURCES} ${PROJECT_BINARY_DIR}/apr.h)
SET(install_targets ${install_targets} libapr-2)
TARGET_LINK_LIBRARIES(libapr-2 ${XMLLIB_LIBRARIES} ${APR_SYSTEM_LIBS})
SET_TARGET_PROPERTIES(libapr-2 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_EXPORT;APR_HAVE_MODULAR_DSO")
ADD_DEPENDENCIES(libapr-2 test_char_header)

ADD_LIBRARY(apr-2 STATIC ${APR_HEADERS} ${APR_SOURCES} ${PROJECT_BINARY_DIR}/apr.h)
SET(install_targets ${install_targets} apr-2)
TARGET_LINK_LIBRARIES(apr-2 ${XMLLIB_LIBRARIES} ${APR_SYSTEM_LIBS})
SET_TARGET_PROPERTIES(apr-2 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_STATIC;APR_HAVE_MODULAR_DSO")
ADD_DEPENDENCIES(apr-2 test_char_header)

IF(APU_HAVE_CRYPTO)
  IF(NOT OPENSSL_FOUND)
    MESSAGE(FATAL_ERROR "Only OpenSSL-based crypto is currently implemented in the cmake build")
  ENDIF()
  ADD_LIBRARY(apr_crypto_openssl-2 SHARED crypto/apr_crypto_openssl.c)
  SET(install_targets ${install_targets} apr_crypto_openssl-2)
  SET_TARGET_PROPERTIES(apr_crypto_openssl-2 PROPERTIES INCLUDE_DIRECTORIES "${APR_INCLUDE_DIRECTORIES};${OPENSSL_INCLUDE_DIR}")
  TARGET_LINK_LIBRARIES(apr_crypto_openssl-2 libapr-2 ${APR_SYSTEM_LIBS} ${OPENSSL_LIBRARIES})
ENDIF()

IF(APU_HAVE_ODBC)
  ADD_LIBRARY(apr_dbd_odbc-2 SHARED dbd/apr_dbd_odbc.c)
  SET(install_targets ${install_targets} apr_dbd_odbc-2)
  TARGET_LINK_LIBRARIES(apr_dbd_odbc-2 libapr-2 ${APR_SYSTEM_LIBS} odbc32 odbccp32)
  SET_PROPERTY(TARGET apr_dbd_odbc-2 APPEND PROPERTY LINK_FLAGS /export:apr_dbd_odbc_driver)
  SET_TARGET_PROPERTIES(apr_dbd_odbc-2 PROPERTIES COMPILE_DEFINITIONS "APU_HAVE_ODBC;HAVE_SQL_H;APR_DECLARE_EXPORT;APU_DSO_MODULE_BUILD")
ENDIF()

IF(APR_BUILD_TESTAPR)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/data)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/test/data/billion-laughs.xml ${PROJECT_BINARY_DIR}/data/billion-laughs.xml)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/test/data/file_datafile.txt ${PROJECT_BINARY_DIR}/data/file_datafile.txt)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/test/data/mmap_datafile.txt ${PROJECT_BINARY_DIR}/data/mmap_datafile.txt)

  ADD_EXECUTABLE(testall ${APR_TEST_SOURCES})
  TARGET_LINK_LIBRARIES(testall apr-2 ${XMLLIB_LIBRARIES} ${APR_SYSTEM_LIBS})

  ADD_LIBRARY(mod_test MODULE test/mod_test.c)
  TARGET_LINK_LIBRARIES(mod_test apr-2 ${APR_SYSTEM_LIBS})
  SET_PROPERTY(TARGET mod_test APPEND PROPERTY LINK_FLAGS /export:print_hello)
  # nasty work-around for difficulties adding more than one additional flag
  # (they get joined in a bad way behind the scenes)
  GET_PROPERTY(link_flags TARGET mod_test PROPERTY LINK_FLAGS)
  SET(link_flags "${link_flags} /export:count_reps")
  SET_TARGET_PROPERTIES(mod_test PROPERTIES LINK_FLAGS ${link_flags})

  ADD_EXECUTABLE(dbd test/dbd.c)
  TARGET_LINK_LIBRARIES(dbd apr-2 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(occhild test/occhild.c)
  TARGET_LINK_LIBRARIES(occhild apr-2 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(globalmutexchild test/globalmutexchild.c)
  TARGET_LINK_LIBRARIES(globalmutexchild apr-2 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(proc_child test/proc_child.c)
  TARGET_LINK_LIBRARIES(proc_child apr-2 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(readchild test/readchild.c)
  TARGET_LINK_LIBRARIES(readchild apr-2 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(sockchild test/sockchild.c)
  TARGET_LINK_LIBRARIES(sockchild apr-2 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(testshmconsumer test/testshmconsumer.c)
  TARGET_LINK_LIBRARIES(testshmconsumer apr-2 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(testshmproducer test/testshmproducer.c)
  TARGET_LINK_LIBRARIES(testshmproducer apr-2 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(tryread test/tryread.c)
  TARGET_LINK_LIBRARIES(tryread apr-2 ${APR_SYSTEM_LIBS})

  # test programs are linked with static library
  SET_TARGET_PROPERTIES(testall dbd mod_test occhild globalmutexchild proc_child readchild sockchild testshmconsumer testshmproducer tryread PROPERTIES COMPILE_FLAGS -DAPR_DECLARE_STATIC)

ENDIF (APR_BUILD_TESTAPR)

# Installation

INSTALL(TARGETS ${install_targets}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
       )

INSTALL(FILES ${APR_PUBLIC_HEADERS_STATIC} ${APR_PUBLIC_HEADERS_GENERATED} DESTINATION include)
IF(APR_INSTALL_PRIVATE_H)
  # Kludges for unexpected dependencies of httpd 2.x, not installed by default
  SET(APR_PRIVATE_H_FOR_HTTPD
    include/arch/win32/apr_arch_file_io.h
    include/arch/win32/apr_arch_misc.h
    include/arch/win32/apr_arch_utf8.h
    include/arch/win32/apr_private.h
    )
  INSTALL(FILES ${APR_PRIVATE_H_FOR_HTTPD} DESTINATION include/arch/win32)
ENDIF()

IF(APR_SHOW_SETTINGS)
  STRING(TOUPPER "${CMAKE_BUILD_TYPE}" buildtype)
  MESSAGE(STATUS "")
  MESSAGE(STATUS "")
  MESSAGE(STATUS "APR configuration summary:")
  MESSAGE(STATUS "")
  MESSAGE(STATUS "  Install prefix .................. : ${CMAKE_INSTALL_PREFIX}")
  MESSAGE(STATUS "  C compiler ...................... : ${CMAKE_C_COMPILER}")
  MESSAGE(STATUS "  IPv6 ............................ : ${APR_HAVE_IPV6}")
  MESSAGE(STATUS "  DBD ODBC driver ................. : ${APU_HAVE_ODBC}")
  MESSAGE(STATUS "  Use Expat ....................... : ${APU_USE_EXPAT}")
  MESSAGE(STATUS "  Use LibXml2 ..................... : ${APU_USE_LIBXML2}")
  MESSAGE(STATUS "  Library files for XML ........... : ${XMLLIB_LIBRARIES}")
  MESSAGE(STATUS "  Build test suite ................ : ${APR_BUILD_TESTAPR}")
  MESSAGE(STATUS "  Install private .h for httpd .... : ${APR_INSTALL_PRIVATE_H}")
ENDIF(APR_SHOW_SETTINGS)
