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
  cmake version 3.1.3 or later is required to work with current OpenSSL
  releases.  (OpenSSL is an optional prerequisite of APR.)
* If using a command-line compiler: compiler and linker and related tools
  (Refer to the cmake documentation for more information.)

The following support libraries are mandatory:

* Either expat or libxml2

Additional support libraries allow optional features of APR to be enabled:

* OpenSSL
* many others potentially, though the build support isn't currently
  implemented

How to build
------------

1. cd to a clean directory for building (i.e., don't build in your
   source tree)

2. Some cmake backends may want your compile tools in PATH.  (Hint: "Visual
   Studio Command Prompt")

3. set CMAKE_LIBRARY_PATH=d:\path\to\prereq1\lib;d:\path\to\prereq2\lib;...

4. set CMAKE_INCLUDE_PATH=d:\path\to\prereq1\include;d:\path\to\prereq2\include;...

5. cmake -G "some backend, like 'NMake Makefiles'"
     -DCMAKE_INSTALL_PREFIX=d:/path/to/aprinst
     -DAPR-specific-flags
     d:/path/to/aprsource

   Alternately, use cmake-gui and update settings in the GUI.

   APR feature flags:

       Exactly one of APU_USE_EXPAT and APU_USE_LIBXML2 must be specified.

       APU_USE_EXPAT          Use Expat as the underlying XML implementation
                              Default: ON
       APU_USE_LIBXML2        Use libxml2 as the underlying XML
                              implementation
                              Default: OFF
       APR_INSTALL_PRIVATE_H  Install extra .h files which are required when
                              building httpd and Subversion but which aren't
                              intended for use by applications.
                              Default: OFF
       APU_HAVE_CRYPTO        Build crypt support (only the OpenSSL
                              implementation is currently supported)
                              Default: OFF
       APU_HAVE_ODBC          Build ODBC DBD driver
                              Default: ON
       APR_HAVE_IPV6          Enable IPv6 support
                              Default: ON
       APR_BUILD_TESTAPR      Build APR test suite
                              Default: OFF
       TEST_STATIC_LIBS       Build the test suite to test the APR static
                              library instead of the APR dynamic library.
                              Default: OFF
                              In order to build the test suite against both
                              static and dynamic libraries, separate builds
                              will be required, one with TEST_STATIC_LIBS
                              set to ON.
       MIN_WINDOWS_VER        Minimum Windows version supported by this build
                              (This controls the setting of _WIN32_WINNT.)
                              "Vista" or "Windows7" or a numeric value like
                              "0x0601"
                              Default: "Vista"
                              For desktop/server equivalence or other values,
                              refer to
                              http://msdn.microsoft.com/en-us/library/windows/
                              desktop/aa383745(v=vs.85).aspx
       INSTALL_PDB            Install .pdb files if generated.
                              Default: ON

   LIBXML2_ICONV_INCLUDE_DIR, LIBXML2_ICONV_LIBRARIES

       If using libxml2 for the XML implementation and the build of libxml2
       requires iconv, set these variables to allow iconv includes
       and libraries to be found.

   CMAKE_C_FLAGS_RELEASE, _DEBUG, _RELWITHDEBINFO, _MINSIZEREL

   CMAKE_BUILD_TYPE

       For NMake Makefiles the choices are at least DEBUG, RELEASE,
       RELWITHDEBINFO, and MINSIZEREL
       Other backends make have other selections.

6. build using chosen backend (e.g., "nmake install")

Tested generators
-----------------

1. NMake Makefiles

This has been tested successfully with the following:

* Visual Studio 2008 64-bit (requires KB948127 hot fix)
* Visual Studio 2012 64-bit

2. Visual Studio 11 (Visual Studio 2012 project files)

Known Bugs and Limitations
--------------------------

* If include/apr.h or other generated files have been created in the source
  directory by another build system, they will be used unexpectedly and
  cause the build to fail.
* Options should be provided for remaining features, along with finding any
  necessary libraries
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
* Static builds of APR modules are not supported.
* XML implementation (i.e., Expat or libxml2) could support static XML impl
  with apr-2.lib.

Generally:

* Many APR features have not been tested with this build.
* Developers need to examine the existing Windows build in great detail and see
  what is missing from the cmake-based build, whether a feature or some build
  nuance.
* Any feedback you can provide on your experiences with this build will be
  helpful.
