Experimental cmake-based build support for APR on Microsoft Windows

Status
------

This build support is currently intended only for Microsoft Windows.

This build support is experimental.  Specifically,

* It does not support all features of APR.
* Some components may not be built correctly and/or in a manner
  compatible with the previous Windows build support.
* Build interfaces, such as the mechanisms which are used to enable
  optional functionality or specify prerequisites, may change from
  release to release as feedback is received from users and bugs and
  limitations are resolved.

Important: Refer to the "Known Bugs and Limitations" section for further
           information.

           It is beyond the scope of this document to document or explain
           how to utilize the various cmake features, such as different
           build backends or provisions for finding support libraries.

           Please refer to the cmake documentation for additional information
           that applies to building any project with cmake.

Prerequisites
-------------

The following tools must be in PATH:

* cmake, version 2.8 or later
* If using a command-line compiler: compiler and linker and related tools
  (Refer to the cmake documentation for more information.)

How to build
------------

1. cd to a clean directory for building (i.e., don't build in your
   source tree)

2. Some cmake backends may want your compile tools in PATH.  (Hint: "Visual
   Studio Command Prompt")

3. cmake -G "some backend, like 'NMake Makefiles'"
     -DCMAKE_INSTALL_PREFIX=d:/path/to/aprinst
     -DAPR-specific-flags
     d:/path/to/aprsource

   Alternately, use cmake-gui and update settings in the GUI.

   APR feature flags:

       APR_INSTALL_PRIVATE_H  Install extra .h files which are required by
                              httpd but which aren't intended for use by
                              applications.
                              Default: OFF
       APR_HAVE_IPV6          Enable IPv6 support
                              Default: ON
       APR_SHOW_SETTINGS      Display key build settings at the end of build
                              generation
                              Default: ON
       APR_BUILD_TESTAPR      Build APR test suite
                              Default: OFF

   CMAKE_C_FLAGS_RELEASE, _DEBUG, _RELWITHDEBINFO, _MINSIZEREL

   CMAKE_BUILD_TYPE

       For NMake Makefiles the choices are at least DEBUG, RELEASE,
       RELWITHDEBINFO, and MINSIZEREL
       Other backends make have other selections.

4. build using chosen backend (e.g., "nmake install")

Known Bugs and Limitations
--------------------------

* If include/apr.h or other generated files have been created in the source
  directory by another build system, they will be used unexpectedly and
  cause the build to fail.
* apr_app.c, aprapp-1.lib, and libaprapp-1.lib are not handled properly.
* Options should be provided for remaining features:
  + APR_POOL_DEBUG
  + DBM:
    . APU_HAVE_GDBM
    . APU_HAVE_NDBM
    . APU_HAVE_DB
  + DBD:
    . APU_HAVE_PGSQL
    . APU_HAVE_MYSQL
    . APU_HAVE_SQLITE3
    . APU_HAVE_SQLITE2
    . APU_HAVE_ORACLE
  + CRYPTO:
    . APU_HAVE_NSS
  + APU_HAVE_ICONV
* No test program build to use libapr-1.dll is created.
* No script or other mechanism is provided to run the test suite.
* CHANGES/LICENSE/NOTICE is not installed, unlike Makefile.win.
* test/internal/testucs is not built.
