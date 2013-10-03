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

PROJECT(APR C)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

OPTION(APR_INSTALL_PRIVATE_H  "Install selected private .h files (for httpd)"  OFF)
OPTION(APR_HAVE_IPV6        "IPv6 support"                 ON)
OPTION(APR_SHOW_SETTINGS    "Show the build configuration" ON)
OPTION(APR_BUILD_TESTAPR    "Build the test suite"         OFF)
SET(MIN_WINDOWS_VER             "Vista" 
    CACHE STRING "Minimum Windows version")

# create 1-or-0 representation of feature tests for apr.h

SET(apr_have_ipv6_10 0)

IF(APR_HAVE_IPV6)
  SET(apr_have_ipv6_10 1)
ENDIF()

IF("${MIN_WINDOWS_VER}" STREQUAL "")
  SET(win32_winnt_str "0x0600")
ELSEIF(${MIN_WINDOWS_VER} STREQUAL "Vista")
  SET(win32_winnt_str "0x0600")
ELSEIF(${MIN_WINDOWS_VER} STREQUAL "Windows7")
  SET(win32_winnt_str "0x0601")
ELSE()
  SET(win32_winnt_str ${MIN_WINDOWS_VER})
ENDIF()

CONFIGURE_FILE(include/apr.hwc
               ${PROJECT_BINARY_DIR}/apr.h)

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

INCLUDE_DIRECTORIES(${APR_INCLUDE_DIRECTORIES})

SET(APR_HEADERS ${PROJECT_BINARY_DIR}/apr.h)

# and misc/win32/apr_app.c

SET(APR_PUBLIC_HEADERS_STATIC
  include/apr_allocator.h
  include/apr_atomic.h
  include/apr_dso.h
  include/apr_env.h
  include/apr_errno.h
  include/apr_file_info.h
  include/apr_file_io.h
  include/apr_fnmatch.h
  include/apr_general.h
  include/apr_getopt.h
  include/apr_global_mutex.h
  include/apr_hash.h
  include/apr_inherit.h
  include/apr_lib.h
  include/apr_mmap.h
  include/apr_network_io.h
  include/apr_poll.h
  include/apr_pools.h
  include/apr_portable.h
  include/apr_proc_mutex.h
  include/apr_random.h
  include/apr_ring.h
  include/apr_shm.h
  include/apr_signal.h
  include/apr_skiplist.h
  include/apr_strings.h
  include/apr_support.h
  include/apr_tables.h
  include/apr_thread_cond.h
  include/apr_thread_mutex.h
  include/apr_thread_proc.h
  include/apr_thread_rwlock.h
  include/apr_time.h
  include/apr_user.h
  include/apr_version.h
  include/apr_want.h
)
SET(APR_PUBLIC_HEADERS_GENERATED
  ${PROJECT_BINARY_DIR}/apr.h
)

SET(APR_SOURCES
  atomic/win32/apr_atomic.c
  dso/win32/dso.c
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
  locks/win32/proc_mutex.c
  locks/win32/thread_cond.c
  locks/win32/thread_mutex.c
  locks/win32/thread_rwlock.c
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
  tables/apr_hash.c
  tables/apr_skiplist.c
  tables/apr_tables.c
  threadproc/win32/proc.c
  threadproc/win32/signals.c
  threadproc/win32/thread.c
  threadproc/win32/threadpriv.c
  time/win32/time.c
  time/win32/timestr.c
  user/win32/groupinfo.c
  user/win32/userinfo.c
)

SET(APR_TEST_SOURCES
  test/abts.c
  test/testargs.c
  test/testatomic.c
  test/testcond.c
  test/testdir.c
  test/testdso.c
  test/testdup.c
  test/testenv.c
  test/testfile.c
  test/testfilecopy.c
  test/testfileinfo.c
  test/testflock.c
  test/testfmt.c
  test/testfnmatch.c
  test/testglobalmutex.c
  test/testhash.c
  test/testipsub.c
  test/testlfs.c
  test/testlock.c
  test/testmmap.c
  test/testnames.c
  test/testoc.c
  test/testpath.c
  test/testpipe.c
  test/testpoll.c
  test/testpools.c
  test/testproc.c
  test/testprocmutex.c
  test/testrand.c
  test/testshm.c
  test/testsleep.c
  test/testsock.c
  test/testsockets.c
  test/testsockopt.c
  test/teststr.c
  test/teststrnatcmp.c
  test/testtable.c
  test/testtemp.c
  test/testthread.c
  test/testtime.c
  test/testud.c
  test/testuser.c
  test/testutil.c
  test/testvsn.c
)

SET(install_targets)

# libapr-1 is shared, apr-1 is static
ADD_LIBRARY(libapr-1 SHARED ${APR_HEADERS} ${APR_SOURCES} ${PROJECT_BINARY_DIR}/apr.h)
SET(install_targets ${install_targets} libapr-1)
TARGET_LINK_LIBRARIES(libapr-1 ${APR_SYSTEM_LIBS})
SET_TARGET_PROPERTIES(libapr-1 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_EXPORT")

ADD_LIBRARY(apr-1 STATIC ${APR_HEADERS} ${APR_SOURCES} ${PROJECT_BINARY_DIR}/apr.h)
SET(install_targets ${install_targets} apr-1)
TARGET_LINK_LIBRARIES(apr-1 ${APR_SYSTEM_LIBS})
SET_TARGET_PROPERTIES(apr-1 PROPERTIES COMPILE_DEFINITIONS "APR_DECLARE_STATIC")

IF(APR_BUILD_TESTAPR)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/data)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/test/data/file_datafile.txt ${PROJECT_BINARY_DIR}/data/file_datafile.txt)
  EXECUTE_PROCESS(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/test/data/mmap_datafile.txt ${PROJECT_BINARY_DIR}/data/mmap_datafile.txt)

  ADD_EXECUTABLE(testall ${APR_TEST_SOURCES})
  TARGET_LINK_LIBRARIES(testall apr-1 ${APR_SYSTEM_LIBS})

  ADD_LIBRARY(mod_test MODULE test/mod_test.c)
  TARGET_LINK_LIBRARIES(mod_test apr-1 ${APR_SYSTEM_LIBS})
  SET_PROPERTY(TARGET mod_test APPEND PROPERTY LINK_FLAGS /export:print_hello)
  # nasty work-around for difficulties adding more than one additional flag
  # (they get joined in a bad way behind the scenes)
  GET_PROPERTY(link_flags TARGET mod_test PROPERTY LINK_FLAGS)
  SET(link_flags "${link_flags} /export:count_reps")
  SET_TARGET_PROPERTIES(mod_test PROPERTIES LINK_FLAGS ${link_flags})

  ADD_EXECUTABLE(occhild test/occhild.c)
  TARGET_LINK_LIBRARIES(occhild apr-1 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(globalmutexchild test/globalmutexchild.c)
  TARGET_LINK_LIBRARIES(globalmutexchild apr-1 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(proc_child test/proc_child.c)
  TARGET_LINK_LIBRARIES(proc_child apr-1 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(readchild test/readchild.c)
  TARGET_LINK_LIBRARIES(readchild apr-1 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(sockchild test/sockchild.c)
  TARGET_LINK_LIBRARIES(sockchild apr-1 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(testshmconsumer test/testshmconsumer.c)
  TARGET_LINK_LIBRARIES(testshmconsumer apr-1 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(testshmproducer test/testshmproducer.c)
  TARGET_LINK_LIBRARIES(testshmproducer apr-1 ${APR_SYSTEM_LIBS})

  ADD_EXECUTABLE(tryread test/tryread.c)
  TARGET_LINK_LIBRARIES(tryread apr-1 ${APR_SYSTEM_LIBS})

  # test programs are linked with static library
  SET_TARGET_PROPERTIES(testall mod_test occhild globalmutexchild proc_child readchild sockchild testshmconsumer testshmproducer tryread PROPERTIES COMPILE_FLAGS -DAPR_DECLARE_STATIC)

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
  INSTALL(FILES include/arch/apr_private_common.h DESTINATION include/arch)
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
  MESSAGE(STATUS "  Minimum Windows version ......... : ${MIN_WINDOWS_VER}")
  MESSAGE(STATUS "  Build test suite ................ : ${APR_BUILD_TESTAPR}")
  MESSAGE(STATUS "  Install private .h for httpd .... : ${APR_INSTALL_PRIVATE_H}")
ENDIF(APR_SHOW_SETTINGS)
